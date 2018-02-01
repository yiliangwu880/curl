

#include "mult_curl.h"
#include "stdio.h"


 void TestRev(const char *buf, size_t size, void *cb_para)
 {
     printf("rev size=%d, cb_para=%d\n", size, cb_para);
 }

 namespace
 {
     void test1()
     {

         MultCurlMgr &obj = MultCurlMgr::Instance();
         if(!obj.Init())
         {
             printf("error");
             return;
         }

         CurlHanlder *pHandler = obj.AddHandler("www.baidu.com", TestRev, (void *)1);
         if (NULL == pHandler)
         {
             printf("error");
             return;
         }
         obj.AddHandler("news.163.com", TestRev, (void *)2);
         int cnt=0;
         while (1)
         {
             obj.Perform();
             cnt++;
             // if (cnt%1 == 0)
             {
                 obj.AddHandler("http://news.163.com/", TestRev, (void *)3);
             }
         }
     }
     
     void test2()
     {

         MultCurlMgr &obj = MultCurlMgr::Instance();
         if(!obj.Init())
         {
             printf("error");
             return;
         }

         obj.AddHandler("http://mis.migc.xiaomi.com/api/biz/service/verifySession.do"
             ,"no_use"
             ,TestRev
             ,(void *)22
             ,NULL, true, true
             );
        // bool AddHandler(const char* str_curl, const char* url_para_str, CURL_REV_FUN fun, void* cb_para, curl_slist* header = NULL, bool bssl = false, bool bdebug = false);

         int cnt=0;
         while (1)
         {
             obj.Perform();
             cnt++;

         }
     }
 }

void mult_test()
{
    test2();


}




