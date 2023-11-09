/**  这是缴费机动态库测试demo **/
/* date: 2023-05-08 **/
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
typedef int DWORD;
typedef const char *LPCTSTR;
typedef void *HANDLE;

#define SPM_EVT_ONLINE 1
#define SPM_EVT_OFFLINE 2
#define SPM_EVT_QRCODE 3
#define SPM_EVT_IOCHANGE 4
#define SPM_EVT_HTTP_RESPONSE 5

typedef void (*SPM_CallBack)(HANDLE h, int code);
typedef HANDLE (*_SPM_Create)();
typedef BOOL (*_SPM_Destory)(HANDLE h);
typedef BOOL (*_SPM_IsOnline)(HANDLE h);
typedef BOOL (*_SPM_Open)(HANDLE h, const char *devName);
typedef BOOL (*_SPM_Close)(HANDLE h);
typedef BOOL (*_SPM_SetCallBack)(HANDLE h, SPM_CallBack cv);
typedef BOOL (*_SPM_Led_ClearAll)(HANDLE h);
typedef BOOL (*_SPM_Led_SendLineText)(HANDLE h, int nLineNumber, int color, int alignType, const char *text, int len);
typedef BOOL (*_SPM_Voice_SendText)(HANDLE h, int vol, const char *text, int len);
typedef BOOL (*_SPM_Lcd_ChangeContext)(HANDLE h, int index, const char *text, int len);
typedef BOOL (*_SPM_SyncTime)(HANDLE h);
typedef BOOL (*_SPM_GetQrCode)(HANDLE h, char *buf);
typedef BOOL (*_SPM_GpioOutPut)(HANDLE h, int pin, int val);
typedef BOOL (*_SPM_AnswerPhone)(HANDLE h, char *phoneId, int reply, int timeout);
typedef BOOL (*_SPM_CallPhone)(HANDLE h, int index, char *phoneId, int timeout);
typedef BOOL (*_SPM_InitPhone)(HANDLE h, char *server, int port, char *phoneId, char *password);

#define LoadFuction(name)                                              \
    _##name name = NULL;                                               \
    name = (_##name)dlsym(lib, #name);                                 \
    if (!name)                                                         \
    {                                                                  \
        fprintf(stderr, "resolve error 【%s】%s\n", #name, dlerror()); \
        exit(0);                                                       \
    }                                                                  \
    else                                                               \
    {                                                                  \
        fprintf(stdout, "resolve 【%s】 success \r\n", #name);         \
    }

static int has_code = 0;

void onSPMEvent(HANDLE h, int code)
{
    char QRcode[64] = {0};
    printf("Dvice Event:%d\r\n", code);
    switch (code)
    {
    case SPM_EVT_QRCODE:
        has_code = 1;
        break;
    default:
        break;
    }
}

int main(int argc, char const *argv[])
{
    /* 加载并解析所有接口 */
    char buf[32];
    char QRcode[32] = {0};
    HANDLE hSPM = NULL;
    const char *dev = "/dev/ttyUSB0";
    int bd = 9600;
    HANDLE lib = dlopen("./libSPM.so", RTLD_LAZY);
    if (!lib)
    {
        printf("load libSPM.so failed: %s \n", dlerror());
        return 0;
    }
    LoadFuction(SPM_Create);
    LoadFuction(SPM_Destory);
    LoadFuction(SPM_IsOnline);
    LoadFuction(SPM_Open);
    LoadFuction(SPM_Close);
    LoadFuction(SPM_SetCallBack);
    LoadFuction(SPM_Led_ClearAll);
    LoadFuction(SPM_Led_SendLineText);
    LoadFuction(SPM_Voice_SendText);
    LoadFuction(SPM_Lcd_ChangeContext);
    LoadFuction(SPM_SyncTime);
    LoadFuction(SPM_GetQrCode);
    LoadFuction(SPM_AnswerPhone);
    LoadFuction(SPM_CallPhone);
    LoadFuction(SPM_InitPhone); 
    if (argc >= 2)
        dev = strdup(argv[1]); 
    hSPM = SPM_Create();
    if (SPM_Open(hSPM, dev))
    {
        printf("open device success!\n");
        SPM_SetCallBack(hSPM, onSPMEvent);
        int bQuit = 0;
        int chr;
        char phone[64] = {0};
        while (!bQuit)
        {
            chr = getc(stdin);
            switch (chr)
            {
            case 'q':
                bQuit = 1;
                break;
            case '1':
                SPM_InitPhone(hSPM, "192.168.1.123", 12312, "123456", "543211");
                break;
            case '2':
                SPM_CallPhone(hSPM, 0, "12331", 23);
                break;
            case '3':
                SPM_AnswerPhone(hSPM, phone, 1, 0);
                break;
            case '4':
                SPM_SyncTime(hSPM);
                break;
            default:
                break;
            }
        }
    }
    else
    {
        printf("open device failed!\n");
    }
    return 0;
}
