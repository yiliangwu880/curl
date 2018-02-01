
#include "UseCurl.h"
#include <stdlib.h>

using namespace std;

size_t RspCallBack(void* buffer, size_t size, size_t cnt, void* para)
{
	printf("cb size=%ld\n", size*cnt);
	//printf("%s\n", (char *)buffer);
	//printf("para %ld\n-------------------------\n", (reinterpret_cast<int*>(para)));
	return size*cnt;
}

char url1[]="https://www.baidu.com/";
char url2[]="http://service.sj.91.com/usercenter/AP.aspx";

void RandomReq(CurlMultiHandler &obj)
{
	int r = rand();
	//if (r > 0X7000FFFF)
	{
		char url1[]="https://www.baidu.com/";
		char url2[]="http://service.sj.91.com/usercenter/AP.aspx";
		if (r%2==0)
		{
			obj.SendHttpGet(url1, RspCallBack, (void *)11, true, false);
		}
		else
		{
			obj.SendHttpGet(url2, RspCallBack, (void *)11, true, false);
		}
	}
	
}
int testCurl()
{
	CurlMultiHandler obj;

	obj.SendHttpGet(url2, RspCallBack, (void *)11, true, false);

	while (true)
	{
		obj.Handle();
		RandomReq(obj);
	}
	
	return 0;
}

