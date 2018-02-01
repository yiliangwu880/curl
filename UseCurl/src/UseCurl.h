/*
	封装CURL多任务处理接口
*/

#pragma once

#include <curl/curl.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <list>

using namespace std;

typedef size_t (*RevCallBack)(void* ptr, size_t size, size_t nmemb, void* user_pare);


struct CurlStr
{
	std::string post_fields; //CURLOPT_POSTFIELDS
	std::string url;
};

class CurlMultiHandler
{
public:
    CurlMultiHandler();
    ~CurlMultiHandler();

	//处理任务，需要循环调用。
    void Handle();      

public:
    /////////////////发送接口的封装////////////////////////

    //安全接口，url_str指向内存生存期没要求。 要是讲究效率，不能用这种接口
	bool SendHttpGet(const char* url_str, RevCallBack cb, void* user_para, bool bssl, bool debug=false, curl_slist* header = NULL);

	//直接用curl_easy_perform 发送http post 字符串请求
	/** header：协议头，默认为null，
		parameterStr：要post的表单数据
		callBack：接收的回调函数
		recvParam：接收函数的参数
		bssl：是否需要认证证书
		bdebug：是否打开调试信息
	**/
	bool SendHttpPost(const char* urlStr, const char* para_str, RevCallBack cb, void* user_para, curl_slist* header, bool bssl, bool debug=false);

    //直接用curl_easy_perform 发送http post 一块内存数据请求
    //注意字符串指针指向的内存，生存期必须继续到 网络任务处理完。
	/** header：协议头，默认为null，
		mem：要post的表单内存
        memLen：要发送的内存的size
		callBack：接收的回调函数
		recvParam：接收函数的参数
		bssl：是否需要认证证书
		bdebug：是否打开调试信息
	**/
    bool sendRecvHttpPostMemory(const char* urlStr, curl_slist* header, void* mem, int memLen, RevCallBack callBack, void* recvParam, bool bssl, bool bdebug=false);

private:
    void AddCurlHandler(CURL* curl);
    void DelCurlHandler(CURL* curl);
    // 用::select 监听curl_m 事件, true表示成功
    bool curl_multi_select(CURLM * curl_m) ;
	// 封装库的curl_multi_perform怪调用方式
	CURLMcode MultiPerform(CURLM *multi_handle, int *running_handles);


	CurlStr *MallocStr();
	void FreeStr(CurlStr *pCurlStr);
private:
	CURLM *m_curlMulti;
};

