/**
use excample:
MultCurlMgr实现参考用，没有curl\UseCurl\src目录下的好用。
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
    //获取CURL，用来设置参数
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

    //加处理对象， 返回对象给用户设置任务
    //注意，返回值会在任务完成后变无效，只提供用户作临时用
    CurlHanlder *AddHandler(const char *str_curl, CURL_REV_FUN fun, void *cb_para, unsigned int time_out_ms = 1000*10);

    bool AddHandler(const char* str_curl, const char* url_para_str, CURL_REV_FUN fun, void* cb_para, curl_slist* header = NULL, bool bssl = false, bool bdebug = false);

    //轮询里面调用，处理收发任务
    void Perform();

private:
    static size_t CallBack(void *buffer, size_t size, size_t count, void * para);
    int CurlMultiSelect(CURLM * curl_m);
    CurlHanlder *Find(CURL *id);
private:
     CURLM *m_curlm;
     Id2Handler m_ls_handler;  //对象池, 考虑用池提高分配效率
};

