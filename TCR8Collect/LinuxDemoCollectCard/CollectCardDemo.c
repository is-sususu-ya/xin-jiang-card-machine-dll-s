/**  ���������տ����Ķ�̬�����demo **/
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
    if (!name) { fprintf(stderr, "resolve error ��%s��%s\n", #name, dlerror()); exit(0); }else{fprintf(stdout,"resolve ��%s�� success \r\n", #name );}
 
static void onACCEvet(int code, int param )
{
    // printf("��Ϣ����,������Ϣ����� ������Ϊ����ֵ���տ�����̬��ʹ��˵��V1.0.pfd �ļ� ��1ҳ �����¼����¼����� ���֣�\r\n");
    printf("�տ����¼��ţ�%d - ������%d \r\n", code, param );
    unsigned int status;
    switch( code )
    {
        case 1: printf("���İ帴λ�ϵ�\r\n"); break;
        case 7: 
        ACC_GetChannelState(param, &status ); 
        if( status & 0x01 )
        {
            // ���ߴ��п�����
        }else if(status & 0x2 )
        {
            // �����п�
        }
        // other

        break;
        // ����ҵ����Ҫ�������¼�������ͬ�Ĵ���
        default:
        break;
    }
}
 
int main(int argc, char const *argv[])
{
    /* ���ز��������нӿ� */
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

    /** ���Դ��豸 **/
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
        printf("�����տ���ʧ�ܣ�\r\n");
        return 0;
    }
    ACC_SetEventCallBack(onACCEvet);
    while( 1 )
    {
        //* �ӿڵ���ʾ��
        fgets( buf, sizeof(buf), stdin );
        switch( buf[0] )
        {
            case '1':
            printf("���չ���ͨ������λ�õĿ�\r\n");
            ACC_RecycleCard();
            break;
            case '2':
            printf("�˿�");
            ACC_ReturnCard();
            break;
            case '3':
            printf("�л����߰嵽��λ��1\r\n");
            ACC_SwitchAntenna( 1 );
            break;
            default:
            break;
        }
    }
    ACC_CloseDevice();
    return 0;
}
