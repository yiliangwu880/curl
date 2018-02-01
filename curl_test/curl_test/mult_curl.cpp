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
    curl_easy_setopt(curl, CURLOPT_URL, pHandler->GetUrl().c_str());//�������ù��������ַ�����ַ������ִ��ʱ���û������޸�
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
 * ʹ��select��������multi curl�ļ���������״̬
 * �����ɹ�����0������ʧ�ܷ���-1
 */
int MultCurlMgr::CurlMultiSelect(CURLM * curl_m)
{
    int ret = 0;

    struct timeval timeout_tv;
    fd_set  fd_read;
    fd_set  fd_write;
    fd_set  fd_except;
    int     max_fd = -1;

    // ע������һ��Ҫ���fdset,curl_multi_fdset����ִ��fdset����ղ���  //
    FD_ZERO(&fd_read);
    FD_ZERO(&fd_write);
    FD_ZERO(&fd_except);

    // ����select��ʱʱ��  //
    timeout_tv.tv_sec = 1;
    timeout_tv.tv_usec = 0;

    // ��ȡmulti curl��Ҫ�������ļ����������� fd_set //
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
     * ִ�м��������ļ�������״̬�����ı��ʱ�򷵻�
     * ����0���������curl_multi_perform֪ͨcurlִ����Ӧ����
     * ����-1����ʾselect����
     * ע�⣺��ʹselect��ʱҲ��Ҫ����0���������ȥ�������ĵ�˵��
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
     * ����curl_multi_perform����ִ��curl����
     * url_multi_perform����CURLM_CALL_MULTI_PERFORMʱ����ʾ��Ҫ�������øú���ֱ������ֵ����CURLM_CALL_MULTI_PERFORMΪֹ
     * running_handles�����������ڴ����easy curl������running_handlesΪ0��ʾ��ǰû������ִ�е�curl����
     */
    int running_handles = 0;
    while (CURLM_CALL_MULTI_PERFORM == curl_multi_perform(m_curlm, &running_handles))
    {
    }

    while (running_handles)
    {
        if (-1 == CurlMultiSelect(m_curlm))
        {
            //select û׼����
            break;
        } else {
            // select�������¼�������curl_multi_perform֪ͨcurlִ����Ӧ�Ĳ��� //
            while (CURLM_CALL_MULTI_PERFORM == curl_multi_perform(m_curlm, &running_handles))
            {
            }
        }
    }

    // ���ִ�н�� //
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
            printf("curl_multi_info_readʧ��, error = %s", curl_easy_strerror(msg->data.result));
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
