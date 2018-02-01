
#include "UseCurl.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define LOG_ERROR printf
#define LOG_DEBUG printf

using namespace std;


CurlMultiHandler::CurlMultiHandler()
{
	m_curlMulti = NULL;
	m_curlMulti = curl_multi_init();
	if (NULL == m_curlMulti)
	{
		LOG_ERROR("初始化curl失败");
	}
}

CurlMultiHandler::~CurlMultiHandler()
{
	if (NULL != m_curlMulti)
	{
		curl_multi_cleanup(m_curlMulti);
	}
}

CURLMcode CurlMultiHandler::MultiPerform(CURLM *multi_handle, int *running_handles)                            
{
	CURLMcode res = CURLM_CALL_MULTI_PERFORM;
	while(CURLM_CALL_MULTI_PERFORM == res)
	{
		res = curl_multi_perform(multi_handle, running_handles);
	}
	return res;
}


void CurlMultiHandler::Handle()                            
{
	int running_handles = 0; 
	CURLMcode res = MultiPerform(m_curlMulti, &running_handles);
	if (CURLM_OK != res)
	{
		LOG_ERROR("curl_multi_perform失败, error = %s\n", curl_multi_strerror(res));
		return;
	}
	if (0 == running_handles)//no task to handle， release cpu.
	{
		usleep(1000*30);
		return;
	}
	while (running_handles > 0)//running_handles ==0  occupy cpu
	{
		if (!curl_multi_select(m_curlMulti))  
		{  
			LOG_ERROR("curl_multi_select error\n");
			break;  
		}

		// When  an  application  has  found out there’s data available for the multi_handle or a timeout has elapsed, the application should call this function to read/write
		//whatever there is to read or write right now etc.
		//This function does not  require  that  there
		//actually is any data available for reading or that data can be written, it can be called just in case.
		res = MultiPerform(m_curlMulti, &running_handles);

		if (CURLM_OK != res)
		{
			LOG_ERROR("curl_multi_perform失败, error = %s\n", curl_multi_strerror(res));
			break;
		}
	}
	int msgs_left;  

	while(CURLMsg* msg = curl_multi_info_read(m_curlMulti, &msgs_left))  
	{  
		if (CURLMSG_DONE == msg->msg)   // 已经完成事件的handle
		{  
			CURL* curl = msg->easy_handle;
			DelCurlHandler(curl);
		}
		else
		{
			LOG_ERROR("curl_multi_info_read失败, error = %s\n", curl_easy_strerror(msg->data.result));
		}
	}
}


void CurlMultiHandler::AddCurlHandler(CURL* curl)
{
	curl_multi_add_handle(m_curlMulti, curl);
}


void CurlMultiHandler::DelCurlHandler(CURL* curl)
{
	curl_multi_remove_handle(m_curlMulti, curl);   

	CurlStr *curl_str = NULL;
	curl_easy_getinfo(curl, CURLINFO_PRIVATE, &curl_str);
	if (NULL == curl_str)
	{
		LOG_ERROR("logic error");
	}
	else
	{
		FreeStr(curl_str);
	}

	curl_easy_cleanup(curl);
}

bool CurlMultiHandler::curl_multi_select(CURLM * curl_m)
{
	struct timeval timeout_tv;  
	fd_set  fd_read;  
	fd_set  fd_write;  
	fd_set  fd_except;  
	int    max_fd = -1;    

	 // 注意这里一定要清空fdset,curl_multi_fdset不会执行fdset的清空操作  //
	FD_ZERO(&fd_read);  
	FD_ZERO(&fd_write);  
	FD_ZERO(&fd_except);  

	timeout_tv.tv_sec = 0;  
	timeout_tv.tv_usec = 1000*500;  

	// 获取multi curl需要监听的文件描述符集合 fd_set //   
	curl_multi_fdset(curl_m, &fd_read, &fd_write, &fd_except, &max_fd);  

  /**
     * When max_fd returns with -1,
     * you need to wait a while and then proceed and call curl_multi_perform anyway.
     * How long to wait? I would suggest 100 milliseconds at least,
     * but you may want to test it out in your own particular conditions to find a suitable value.
     */
	if (-1 == max_fd)
	{
		usleep(1000*100);
		return false;
	}

	int ret_code = ::select(max_fd + 1, &fd_read, &fd_write, NULL, &timeout_tv);  
	if(-1 == ret_code)  
	{  
		//LOG_ERROR("select error, error = [%s]", strerror(errno))
		return  false;  
	}  

	//  timeout or readable/writable sockets
	return true;
}

bool CurlMultiHandler::SendHttpGet( const char* url_str, RevCallBack cb, void* user_para, bool bssl, bool debug/*=false*/, curl_slist* header /*= NULL*/ )
{
	CURL* curl = curl_easy_init();
	if (!curl)
	{
		LOG_ERROR("初始化curl失败");
		return false;
	}
	CurlStr *pCurlStr = MallocStr();
	if (NULL == pCurlStr)
	{
		LOG_ERROR("malloc fail!");
		return false;
	}
	pCurlStr->url.assign(url_str); //避免用外部传进来的不安全指针，生存期内部不可控

	curl_easy_setopt(curl, CURLOPT_PRIVATE, pCurlStr);

	curl_easy_setopt(curl, CURLOPT_URL, pCurlStr->url.c_str());
	
	CURLcode res = CURLE_OK;
	if (NULL == header)
	{//NULL的情况，保持旧代码逻辑
		res = curl_easy_setopt(curl, CURLOPT_HEADER, false);
		if (CURLE_OK != res)
		{
			LOG_ERROR("CURLOPT_HEADER失败, error = %s", curl_easy_strerror(res));
			return false;
		}
	}
	else
	{
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
	}

	if (bssl)
	{
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
	}

	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);

	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt( curl , CURLOPT_TIMEOUT, 5);

	if (cb)
	{
		res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
		if (CURLE_OK != res)
		{
			LOG_ERROR("CURLOPT_WRITEFUNCTION失败, error = %s", curl_easy_strerror(res));
			return false;
		}
	}

	if (user_para)
	{
		res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, user_para);
		if (CURLE_OK != res)
		{
			LOG_ERROR("CURLOPT_WRITEDATA失败, error = %s", curl_easy_strerror(res));
			return false;
		}
	}


	if (debug)
	{
		res = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		if (CURLE_OK != res)
		{
			LOG_ERROR("CURLOPT_VERBOSE失败, error = %s", curl_easy_strerror(res));
			return false;
		}
	}

	AddCurlHandler(curl);  
	return true;
}

bool CurlMultiHandler::SendHttpPost( const char* url_str, const char* para_str, RevCallBack cb, void* user_para, curl_slist* header, bool bssl, bool debug/*=false*/ )
{
	CURL* curl = curl_easy_init();
	if (!curl)
	{
		LOG_ERROR("初始化curl失败");
		return false;
	}

	CurlStr *pCurlStr = MallocStr();
	if (NULL == pCurlStr)
	{
		LOG_ERROR("malloc fail!");
		return false;
	}
	pCurlStr->url.assign(url_str); //避免用外部传进来的不安全指针，生存期内部不可控
	if (NULL != para_str)
	{
		pCurlStr->post_fields.assign(para_str);
	}
	

	curl_easy_setopt(curl, CURLOPT_PRIVATE, pCurlStr);

	if (NULL != header)
	{
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
	}
	else
	{
		curl_easy_setopt(curl, CURLOPT_HEADER, 0);
	}

	CURLcode res = curl_easy_setopt(curl, CURLOPT_URL, pCurlStr->url.c_str());
	if (CURLE_OK != res)
	{
		LOG_ERROR("CURLOPT_READFUNCTION失败, error = %s", curl_easy_strerror(res));
		return false;
	}
	if (bssl)
	{
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
	}
	curl_easy_setopt(curl, CURLOPT_POST, 1);

	// 不重用
	//  curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt( curl , CURLOPT_TIMEOUT, 3);

	res = CURLE_OK;

	if (NULL != para_str)
	{
		res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, pCurlStr->post_fields.c_str());
		if (CURLE_OK != res)
		{
			LOG_ERROR("CURLOPT_READFUNCTION失败, error = %s", curl_easy_strerror(res));
			return false;
		}
	}

	if (NULL != cb)
	{
		res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
		if (CURLE_OK != res)
		{
			LOG_ERROR("CURLOPT_WRITEFUNCTION失败, error = %s", curl_easy_strerror(res));
			return false;
		}
	}

	if (NULL != user_para)
	{
		res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, user_para);
		if (CURLE_OK != res)
		{
			LOG_ERROR("CURLOPT_WRITEDATA失败, error = %s", curl_easy_strerror(res));
			return false;
		}
	}

	if (debug)
	{
		res = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		if (CURLE_OK != res)
		{
			LOG_ERROR("CURLOPT_VERBOSE失败, error = %s", curl_easy_strerror(res));
			return false;
		}
	}

	AddCurlHandler(curl);  

	return true;
}

bool CurlMultiHandler::sendRecvHttpPostMemory(const char* urlStr, curl_slist* header, void* mem, int memLen, RevCallBack callBack, void* recvParam, bool bssl, bool bdebug)
{
	CURL* curl = curl_easy_init();
	if (!curl)
	{
		LOG_ERROR("初始化curl失败");
		return false;
	}

	if (NULL != header)
	{
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
	}
	else
	{
		curl_easy_setopt(curl, CURLOPT_HEADER, 0);
	}

	CURLcode res = curl_easy_setopt(curl, CURLOPT_URL, urlStr);
	if (CURLE_OK != res)
	{
		LOG_ERROR("CURLOPT_READFUNCTION失败, error = %s", curl_easy_strerror(res));
		return false;
	}
	if (bssl)
	{
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
	}

	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt( curl , CURLOPT_TIMEOUT, 5);

	curl_easy_setopt(curl, CURLOPT_POST, 1);

	res = CURLE_OK;
	res = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, memLen);
	if (CURLE_OK != res)
	{
		LOG_ERROR("CURLOPT_POSTFIELDSIZE失败, error = %s", curl_easy_strerror(res));
		return false;
	}

	res = curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, mem);
	if (CURLE_OK != res)
	{
		LOG_ERROR("CURLOPT_COPYPOSTFIELDS失败, error = %s", curl_easy_strerror(res));
		return false;
	}

	if (NULL != callBack)
	{
		res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callBack);
		if (CURLE_OK != res)
		{
			LOG_ERROR("CURLOPT_WRITEFUNCTION失败, error = %s", curl_easy_strerror(res));
			return false;
		}
	}

	if (NULL != recvParam)
	{
		res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, recvParam);
		if (CURLE_OK != res)
		{
			LOG_ERROR("CURLOPT_WRITEDATA失败, error = %s", curl_easy_strerror(res));
			return false;
		}
	}

	if (bdebug)
	{
		res = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		if (CURLE_OK != res)
		{
			LOG_ERROR("CURLOPT_VERBOSE失败, error = %s", curl_easy_strerror(res));
			return false;
		}
	}

	AddCurlHandler(curl);   

	return true;
}

CurlStr * CurlMultiHandler::MallocStr()
{ //看需要，可以优化内存分配方式
	return new CurlStr();
}

void CurlMultiHandler::FreeStr( CurlStr *pCurlStr )
{
	if (NULL != pCurlStr)
	{
		delete pCurlStr;
	}
}






