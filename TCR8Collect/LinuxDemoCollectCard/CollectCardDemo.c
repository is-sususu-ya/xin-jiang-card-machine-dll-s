/**  这是自助收卡机的动态库调用demo **/
/* date: 2021-05-31 **/
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

typedef int BOOL;
typedef int	DWORD;
typedef const char*  LPCTSTR;
typedef void * HANDLE;

 
typedef void   (*ACCEventHandle)(int a, int b); 
typedef  BOOL  (*_ACC_SetEventCallBack)(ACCEventHandle cb);
typedef  BOOL  (*_ACC_OpenDevice)(const char *devName, int nBaudRate);
typedef  BOOL  (*_ACC_CloseDevice)(void);
typedef  BOOL  (*_ACC_CollectCard)(void);
typedef  BOOL  (*_ACC_RecycleCard)(void);
typedef  BOOL  (*_ACC_ReturnCard)(void);
typedef  BOOL  (*_ACC_SwitchAntenna)(int nPosition);
typedef  BOOL  (*_ACC_IsOnline)(void);
typedef  BOOL  (*_ACC_ForceReturn)(int nChannel);
typedef  BOOL  (*_ACC_ForceRecycle)(int nChannel);
typedef  BOOL  (*_ACC_GetFirmwareVer)(DWORD *ver);
typedef  BOOL  (*_ACC_SetBoxSN)(int nChannel, DWORD dwSN);
typedef  BOOL  (*_ACC_GetBoxSN)(int nChannel, DWORD *dwSN);
typedef  BOOL  (*_ACC_SetCardCounter)(int nChannel, int nCount);
typedef  BOOL  (*_ACC_GetCardCounter)(int nChannel, int *nCount);
typedef  BOOL  (*_ACC_GetChannelState)(int nChannel, int *nState);
typedef  BOOL  (*_ACC_IsBoxLoad)(int nChannel);    
typedef int	  (*_ACC_GetTxData)(char *buf, int size);
typedef int	  (*_ACC_GetRxData)(char *buf, int size);
typedef int   (*_ACC_GetKernelLog)(char *buf, int size);


#define LoadFuction( name ) \
    _##name name = NULL;\
    name = (_##name)dlsym(lib, #name );\
    if (!name) { fprintf(stderr, "resolve error 【%s】%s\n", #name, dlerror()); exit(0); }else{fprintf(stdout,"resolve 【%s】 success \r\n", #name );}
 
static void onACCEvet(int code, int param )
{
    // printf("消息处理,具体消息处理见 苏州朗为无人值守收卡机动态库使用说明V1.0.pfd 文件 第1页 卡机事件和事件参数 表部分！\r\n");
    printf("收卡机事件号：%d - 参数：%d \r\n", code, param );
    unsigned int status;
    switch( code )
    {
        case 1: printf("核心板复位上电\r\n"); break;
        case 7: 
        ACC_GetChannelState(param, &status ); 
        if( status & 0x01 )
        {
            // 天线处有卡。。
        }else if(status & 0x2 )
        {
            // 卡口有卡
        }
        // other

        break;
        // 根据业务需要，依据事件号做不同的处理
        default:
        break;
    }
}
 
int main(int argc, char const *argv[])
{
    /* 加载并解析所有接口 */
    char buf[32];
    HANDLE lib = dlopen("libTCR8ACC.so", RTLD_LAZY );
    LoadFuction(ACC_OpenDevice);
    LoadFuction(ACC_SetEventCallBack);
    LoadFuction(ACC_CloseDevice);
    LoadFuction(ACC_CollectCard);
    LoadFuction(ACC_RecycleCard);
    LoadFuction(ACC_ReturnCard);
    LoadFuction(ACC_SwitchAntenna);
    LoadFuction(ACC_IsOnline);
    LoadFuction(ACC_ForceReturn);
    LoadFuction(ACC_ForceRecycle);
    LoadFuction(ACC_GetFirmwareVer);
    LoadFuction(ACC_SetBoxSN);
    LoadFuction(ACC_GetBoxSN);
    LoadFuction(ACC_SetCardCounter);
    LoadFuction(ACC_GetCardCounter); 
    LoadFuction(ACC_GetChannelState);
    LoadFuction(ACC_IsBoxLoad);     
    LoadFuction(ACC_GetTxData);
    LoadFuction(ACC_GetRxData);
    LoadFuction(ACC_GetKernelLog); 

    /** 尝试打开设备 **/
    HANDLE hCM = NULL; // 
	const char *dev = "/dev/ttyAMA2";
	int bd = 9600;
	if( argc >= 2 )
		dev = strdup( argv[1] );
	if( argc >= 3 )
		bd = atoi( argv[2] );
    hCM = ACC_OpenDevice(dev, bd );
    if( !hCM )
    {
        printf("连接收卡机失败！\r\n");
        return 0;
    }
    ACC_SetEventCallBack(onACCEvet);
    while( 1 )
    {
        //* 接口调用示例
        fgets( buf, sizeof(buf), stdin );
        switch( buf[0] )
        {
            case '1':
            printf("回收工作通道天线位置的卡\r\n");
            ACC_RecycleCard();
            break;
            case '2':
            printf("退卡");
            ACC_ReturnCard();
            break;
            case '3':
            printf("切换天线板到工位：1\r\n");
            ACC_SwitchAntenna( 1 );
            break;
            default:
            break;
        }
    }
    ACC_CloseDevice();
    return 0;
}
