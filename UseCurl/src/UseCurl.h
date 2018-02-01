/*
	��װCURL��������ӿ�
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

	//����������Ҫѭ�����á�
    void Handle();      

public:
    /////////////////���ͽӿڵķ�װ////////////////////////

    //��ȫ�ӿڣ�url_strָ���ڴ�������ûҪ�� Ҫ�ǽ���Ч�ʣ����������ֽӿ�
	bool SendHttpGet(const char* url_str, RevCallBack cb, void* user_para, bool bssl, bool debug=false, curl_slist* header = NULL);

	//ֱ����curl_easy_perform ����http post �ַ�������
	/** header��Э��ͷ��Ĭ��Ϊnull��
		parameterStr��Ҫpost�ı�����
		callBack�����յĻص�����
		recvParam�����պ����Ĳ���
		bssl���Ƿ���Ҫ��֤֤��
		bdebug���Ƿ�򿪵�����Ϣ
	**/
	bool SendHttpPost(const char* urlStr, const char* para_str, RevCallBack cb, void* user_para, curl_slist* header, bool bssl, bool debug=false);

    //ֱ����curl_easy_perform ����http post һ���ڴ���������
    //ע���ַ���ָ��ָ����ڴ棬�����ڱ�������� �����������ꡣ
	/** header��Э��ͷ��Ĭ��Ϊnull��
		mem��Ҫpost�ı��ڴ�
        memLen��Ҫ���͵��ڴ��size
		callBack�����յĻص�����
		recvParam�����պ����Ĳ���
		bssl���Ƿ���Ҫ��֤֤��
		bdebug���Ƿ�򿪵�����Ϣ
	**/
    bool sendRecvHttpPostMemory(const char* urlStr, curl_slist* header, void* mem, int memLen, RevCallBack callBack, void* recvParam, bool bssl, bool bdebug=false);

private:
    void AddCurlHandler(CURL* curl);
    void DelCurlHandler(CURL* curl);
    // ��::select ����curl_m �¼�, true��ʾ�ɹ�
    bool curl_multi_select(CURLM * curl_m) ;
	// ��װ���curl_multi_perform�ֵ��÷�ʽ
	CURLMcode MultiPerform(CURLM *multi_handle, int *running_handles);


	CurlStr *MallocStr();
	void FreeStr(CurlStr *pCurlStr);
private:
	CURLM *m_curlMulti;
};

