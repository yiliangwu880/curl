/**
use excample:
MultCurlMgrʵ�ֲο��ã�û��curl\UseCurl\srcĿ¼�µĺ��á�
*/

#pragma once

#include "curl.h"
#include <map>

typedef void (*CURL_REV_FUN)(const char *buf, size_t size, void *cb_para);
class MultCurlMgr;

class CurlHanlder
{
    friend class MultCurlMgr;
public:
    CurlHanlder(CURL_REV_FUN fun, CURL *curl, void *cb_pare)
        :m_rev_fun(fun)
        ,m_curl(curl)
        ,m_cb_para(cb_pare)
    {
    }
    //��ȡCURL���������ò���
    CURL *GetCurl(){return m_curl;};
    const std::string &GetUrl() const;
    std::string &UrlPara(){return m_url_para;};
private:
    void Receive(const char *buf, size_t size) const;
    void SetUrl(const char *url);

private:
    CURL_REV_FUN m_rev_fun;
    CURL *m_curl;
    void *m_cb_para;
    std::string m_url;
    std::string m_url_para;
};


class MultCurlMgr
{
    typedef std::map<CURL *, CurlHanlder> Id2Handler;
private:
    MultCurlMgr();
    ~MultCurlMgr();

public:
    static MultCurlMgr &Instance()
    {
        static MultCurlMgr d;
        return d;
    }

    bool Init();

    //�Ӵ������ ���ض�����û���������
    //ע�⣬����ֵ����������ɺ����Ч��ֻ�ṩ�û�����ʱ��
    CurlHanlder *AddHandler(const char *str_curl, CURL_REV_FUN fun, void *cb_para, unsigned int time_out_ms = 1000*10);

    bool AddHandler(const char* str_curl, const char* url_para_str, CURL_REV_FUN fun, void* cb_para, curl_slist* header = NULL, bool bssl = false, bool bdebug = false);

    //��ѯ������ã������շ�����
    void Perform();

private:
    static size_t CallBack(void *buffer, size_t size, size_t count, void * para);
    int CurlMultiSelect(CURLM * curl_m);
    CurlHanlder *Find(CURL *id);
private:
     CURLM *m_curlm;
     Id2Handler m_ls_handler;  //�����, �����ó���߷���Ч��
};

