/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2018-05-09 07:15:38 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:   (), 
 *        Company:  
 *
 * =====================================================================================
 */
#include "TFI.h"
#include <stdio.h>

static void getQr(const char *var )
{
	printf("Get Qr:%s \r\n", var );
}


int main(int argc, char *argv[])
{
	int mode = 0;
	int i = 0;
	HANDLE handle;
	handle = TFI_Create();
	if( handle == NULL )
	{
		printf("create error..\r\n");
		return;
	}
#if 1
	TFI_OpenCom( handle,  argv[1] );
	TFI_SetCallBack( getQr );
	while( 1 )
	{
		TFI_SendLine( handle, 1, 0xffffffff, 1, 1, "第一行测试文本..\r\n" );
		TFI_PlayDu( handle, 3  );
	 	TFI_SendVoice( handle , 2,   "你好中国！" );
		sleep( 3 );
	}
#else
	TFI_OpenNet(handle,  argv[2] );
	TFI_SetCallBack( getQr );
	while( 1 )
	{
		TFI_SendLine( handle, 1, 0xffffffff, 1, 1, "第一行测试文本..\r\n" );
		TFI_PlayDu( handle, 3  );
	 	TFI_SendVoice( handle , 2,   "你好中国！" );
		sleep( 3 );
	}

#endif
	TFI_Close( handle );
	return 0;
}

