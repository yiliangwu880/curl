#include "mult_curl.h"
#include "curl.h"
#include <iostream>
#include <utility>

using namespace std;

size_t MultCurlMgr::CallBack(void *buffer, size_t size, size_t count, void * para)
{
    CURL *id = static_cast<CURL *>(para);
    const CurlHanlder *pHandler = MultCurlMgr::Instance().Find(id);
    if (NULL == pHandler)
    {
        return 0;
    }
    pHandler->Receive((const char *)buffer, size * count);
    return size * count;
};




MultCurlMgr::MultCurlMgr()
    :m_curlm(NULL)
{
}

bool MultCurlMgr::Init()
{
    if (NULL != m_curlm)
    {
        return false;
    }
    m_curlm = curl_multi_init();
    if (NULL == m_curlm)
    {
        return false;
    }
    return true;
}

MultCurlMgr::~MultCurlMgr()
{
    if (NULL != m_curlm)
    {
        curl_multi_cleanup(m_curlm);
        m_curlm = NULL;
    }
}


CurlHanlder * MultCurlMgr::AddHandler( const char *url, CURL_REV_FUN fun, void *cb_para, unsigned int time_out_ms /*= 0*/ )
{
    if (NULL == m_curlm)
    {
        return NULL;
    }
    CURL * curl = curl_easy_init();
    if (NULL == curl)
    {
        return NULL;
    }

    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    if (time_out_ms > 0)
    {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, time_out_ms);
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CallBack);

    pair<Id2Handler::iterator, bool> ret = m_ls_handler.insert(make_pair(curl, CurlHanlder(fun, curl, cb_para)));
    if (!ret.second)
    {
        curl_easy_cleanup(curl);
        return NULL;
    }

    CurlHanlder *pHandler = &((ret.first)->second);
    pHandler->SetUrl(url);
    curl_easy_setopt(curl, CURLOPT_URL, pHandler->GetUrl().c_str());//必须引用管理对象的字符串地址，避免执行时被用户代码修改
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)curl);

    curl_multi_add_handle(m_curlm, curl);
    return pHandler;
}

bool MultCurlMgr::AddHandler( const char* str_curl, const char* url_para_str, CURL_REV_FUN fun, void* cb_para, curl_slist* header/* = NULL*/, bool bssl /*= false*/, bool bdebug /*= false*/ )
{
    CurlHanlder *pHandler = AddHandler(str_curl, fun, cb_para);
    if (NULL == pHandler)
    {
        return false;
    }

    CURL* curl = pHandler->GetCurl();
    if (NULL == curl)
    {
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

    if (bssl)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    }
    curl_easy_setopt(curl, CURLOPT_POST, 1);

    CURLcode res = CURLE_OK;
    if (url_para_str != NULL)
    {
        string &url_para = pHandler->UrlPara();
        url_para = url_para_str;
        res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, url_para.c_str());
        if (CURLE_OK != res)
        {
            return false;
        }
    }

    if (bdebug)
    {
        res = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        if (CURLE_OK != res)
        {
            return false;
        }
    }
    return true;
}


/**
 * 使用select函数监听multi curl文件描述符的状态
 * 监听成功返回0，监听失败返回-1
 */
int MultCurlMgr::CurlMultiSelect(CURLM * curl_m)
{
    int ret = 0;

    struct timeval timeout_tv;
    fd_set  fd_read;
    fd_set  fd_write;
    fd_set  fd_except;
    int     max_fd = -1;

    // 注意这里一定要清空fdset,curl_multi_fdset不会执行fdset的清空操作  //
    FD_ZERO(&fd_read);
    FD_ZERO(&fd_write);
    FD_ZERO(&fd_except);

    // 设置select超时时间  //
    timeout_tv.tv_sec = 1;
    timeout_tv.tv_usec = 0;

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
        return -1;
    }

    /**
     * 执行监听，当文件描述符状态发生改变的时候返回
     * 返回0，程序调用curl_multi_perform通知curl执行相应操作
     * 返回-1，表示select错误
     * 注意：即使select超时也需要返回0，具体可以去官网看文档说明
     */
    int ret_code = ::select(max_fd + 1, &fd_read, &fd_write, &fd_except, &timeout_tv);
    switch(ret_code)
    {
    case -1:
        /* select error */
        ret = -1;
        break;
    case 0:
        /* select timeout */
    default:
        /* one or more of curl's file descriptors say there's data to read or write*/
        ret = 0;
        break;
    }

    return ret;
}

void MultCurlMgr::Perform()
{
    if (NULL == m_curlm)
    {
        return;
    }
    /*
     * 调用curl_multi_perform函数执行curl请求
     * url_multi_perform返回CURLM_CALL_MULTI_PERFORM时，表示需要继续调用该函数直到返回值不是CURLM_CALL_MULTI_PERFORM为止
     * running_handles变量返回正在处理的easy curl数量，running_handles为0表示当前没有正在执行的curl请求
     */
    int running_handles = 0;
    while (CURLM_CALL_MULTI_PERFORM == curl_multi_perform(m_curlm, &running_handles))
    {
    }

    while (running_handles)
    {
        if (-1 == CurlMultiSelect(m_curlm))
        {
            //select 没准备好
            break;
        } else {
            // select监听到事件，调用curl_multi_perform通知curl执行相应的操作 //
            while (CURLM_CALL_MULTI_PERFORM == curl_multi_perform(m_curlm, &running_handles))
            {
            }
        }
    }

    // 输出执行结果 //
    int         msgs_left = 0;
    CURLMsg *   msg;
    while( msg = curl_multi_info_read(m_curlm, &msgs_left) )
    {
        CURL *id = msg->easy_handle;
        if (CURLMSG_DONE == msg->msg)
        {
            curl_multi_remove_handle(m_curlm, msg->easy_handle);
            curl_easy_cleanup(msg->easy_handle);
            msg = NULL;
        }
        else
        {
            printf("curl_multi_info_read失败, error = %s", curl_easy_strerror(msg->data.result));
        }
        if(!m_ls_handler.erase(id))
        {
            printf("error curl = %lld\n", id);
        }
        else
        {
            printf("erase curl = %lld\n", id);
        }
    }
}

CurlHanlder * MultCurlMgr::Find( CURL *id )
{
    Id2Handler::iterator it = m_ls_handler.find(id);
    if (it == m_ls_handler.end())
    {
        return NULL;
    }
    return &(it->second);
}

void CurlHanlder::Receive( const char *buf, size_t size ) const
{
    if (NULL != m_rev_fun)
    {
        (*m_rev_fun)(buf, size, m_cb_para);
    }
}

void CurlHanlder::SetUrl( const char *url )
{
    m_url = url;
}

const std::string & CurlHanlder::GetUrl() const
{
    return m_url;
}
