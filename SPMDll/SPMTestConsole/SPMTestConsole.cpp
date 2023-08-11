// SPMTestConsole.cpp : 定义控制台应用程序的入口点。
//

#define _CRT_SECURE_NO_WARNINGS
#include "stdafx.h"
#include "../SPMAPI/SPM.h"
 
int main()
{
	char buf[20];
	HANDLE h = SPM_Create();
	SPM_Open(h, "COM2");
	if (!h)
	{
		printf("open failed!");
		return 0;
	}
	while (1)
	{
		fgets(buf, sizeof(buf), stdin);
		printf("iuput>");
		switch (buf[0])
		{
		case 'a':
			SPM_Led_ClearAll(h);
			break;
		case 'b':
			SPM_Led_SendLineText(h, 1, 0xaa, 1, "Hello", 5);
			SPM_Led_SendLineText(h, 2, 0xaa, 1, "Word", 5);
			break;
		case 'c':
			SPM_Voice_SendText(h, 2, "Hello Word", 10 );
			break;
		case '1':
			SPM_SetHttpProxyAddress(h, 0, "http://www.baidu.com", strlen("http://www.baidu.com"));
			break;
		case '2':
			SPM_SetHttpPostProxyRequest(h, 0, 1, 1, "a=123", 5 );
			break;
		case 'd':
			printf("on line:%d\r\n", SPM_IsOnline(h));
			break;
		default:
			break;
		} 
	}
    return 0;
}
 