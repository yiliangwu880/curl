
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

char url1[] = "https://www.baidu.com/";
char url2[] = "http://service.sj.91.com/usercenter/AP.aspx";

void RandomReq(CurlMultiHandler &obj)
{
	int r = rand();
	//if (r > 0X7000FFFF)
	{
		if (r%2==0)
		{
			printf("send 1\n");
			obj.SendHttpGet(url1, RspCallBack, (void *)11, true, false);
		}
		else
		{
			printf("send 2\n");
			obj.SendHttpGet(url2, RspCallBack, (void *)11, true, false);
		}
	}
	
}
int testCurl()
{
	CurlMultiHandler obj;
	printf("send 2\n");
	obj.SendHttpGet(url2, RspCallBack, (void *)11, true, false);

	while (true)
	{
		obj.Handle();
		RandomReq(obj);
	}
	
	return 0;
}

int testErrorNetName()
{
	CurlMultiHandler obj;
	obj.SendHttpGet("slldld", RspCallBack, (void *)11, true, false); //BUG第一个请求失败，不会释放资源，待查
	int cnt = 0;
	while (true)
	{
		obj.Handle();
		cnt++;
		//obj.SendHttpGet("slldld", RspCallBack, (void *)11, true, false);
		
		
		
		if (cnt >=5)
		{
			break;
		}
		
	}

	return 0;
}
