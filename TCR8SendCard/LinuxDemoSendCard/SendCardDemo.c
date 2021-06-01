/**  这是自助发卡机的动态库调用demo **/
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
 
typedef void   (*ACMEventHandle)(int a, int b); 
typedef  BOOL  (*_ACM_SetEventCallBackFunc)(ACMEventHandle cb);
typedef  BOOL  (*_ACM_OpenDevice)(const char *devName, int nBaudRate);
typedef  BOOL  (*_ACM_CloseDevice)(void); 
typedef  BOOL  (*_ACM_Reject)(void);
typedef BOOL   (*_ACM_IssueCard)(void);
typedef  BOOL  (*_ACM_RecycleCard)(void); 
typedef  BOOL  (*_ACM_SwitchAntenna)(int nPosition);
typedef  BOOL  (*_ACM_IsOnline)(void); 
typedef  BOOL  (*_ACM_ForceEject)(int nChannel);
typedef  BOOL  (*_ACM_ForceRecycle)(int nChannel);
typedef  BOOL  (*_ACM_GetFirmwareVer)(DWORD *ver);
typedef  BOOL  (*_ACM_SetBoxSN)(int nChannel, DWORD dwSN);
typedef  BOOL  (*_ACM_GetBoxSN)(int nChannel, DWORD *dwSN);
typedef  BOOL  (*_ACM_SetCardCounter)(int nChannel, int nCount);
typedef  BOOL  (*_ACM_GetCardCounter)(int nChannel, int *nCount);
typedef  BOOL  (*_ACM_GetChannelState)(int nChannel, int *nState);
typedef  BOOL  (*_ACM_IsBoxLoad)(int nChannel);    
typedef int	  (*_ACM_GetTxData)(char *buf, int size);
typedef int	  (*_ACM_GetRxData)(char *buf, int size);
typedef int   (*_ACM_GetKernelLog)(char *buf, int size);


#define LoadFuction( name ) \
    _##name name = NULL;\
    name = (_##name)dlsym(lib, #name );\
    if (!name) { fprintf(stderr, "resolve error 【%s】%s\n", #name, dlerror()); exit(0); }else{fprintf(stdout,"resolve 【%s】 success \r\n", #name );}

static void onACMEvet(int code, int param )
{
    // printf("消息处理,具体消息处理见 SU12-朗为无人值守发卡机动态库使用说明V1.5A.pdf 文件 第1页 卡机事件和事件参数 表部分！\r\n");
    printf("收卡机事件号：%d - 参数：%d \r\n", code, param );
    switch( code )
    {
        case 1: printf("核心板复位上电\r\n"); break;
    }
}
 
int main(int argc, char const *argv[])
{
    /* 加载并解析所有接口 */
    char buf[32];
    HANDLE hCM = NULL;
    HANDLE lib = dlopen("libTCR8ACM.so", RTLD_LAZY );
    LoadFuction(ACM_OpenDevice);
    LoadFuction(ACM_SetEventCallBackFunc);
    LoadFuction(ACM_CloseDevice); 
    LoadFuction(ACM_Reject);
    LoadFuction(ACM_IssueCard);
    LoadFuction(ACM_RecycleCard); 
    LoadFuction(ACM_SwitchAntenna);
    LoadFuction(ACM_IsOnline); 
    LoadFuction(ACM_ForceEject);
    LoadFuction(ACM_ForceRecycle);
    LoadFuction(ACM_GetFirmwareVer);
    LoadFuction(ACM_SetBoxSN);
    LoadFuction(ACM_GetBoxSN);
    LoadFuction(ACM_SetCardCounter);
    LoadFuction(ACM_GetCardCounter); 
    LoadFuction(ACM_GetChannelState);
    LoadFuction(ACM_IsBoxLoad);     
    LoadFuction(ACM_GetTxData);
    LoadFuction(ACM_GetRxData);
    LoadFuction(ACM_GetKernelLog); 

    /** 尝试打开设备 **/
	const char *dev = "/dev/ttyAMA2";
	int bd = 9600;
	if( argc >= 2 )
		dev = strdup( argv[1] );
	if( argc >= 3 )
		bd = atoi( argv[2] );
    hCM = ACM_OpenDevice(dev, bd );
    if( !hCM )
    {
        printf("连接收卡机失败！\r\n");
        return 0;
    }
    //  设置消息处理函数
    ACM_SetEventCallBackFunc(onACMEvet);
    // 
    while( 1 )
    {
        //* 接口调用示例
        fgets( buf, sizeof(buf), stdin );
        switch( buf[0] )
        {
            case '1':
            printf("将工作通道上天线位置的卡推出！\r\n");
            ACM_IssueCard();
            break;
            case '2':
            printf("回收工作通道天线位置的卡"); 
            ACM_RecycleCard();
            break;
            case '3':
            printf("拒绝发卡");
            ACM_Reject();
            break;
            case '4':
            printf("切换天线板到工位：1\r\n");
            ACM_SwitchAntenna( 1 );
            break;
            default:
            break;
        }
    }
    ACM_CloseDevice();
    return 0;
}
