


#ifndef TFI_HEADER_INCLUDED
#define TFI_HEADER_INCLUDED

#ifdef linux
typedef int DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef signed long long INT64;
typedef int BOOL;
typedef void * HANDLE;
typedef int SOCKET;
typedef void *PVOID;
typedef const char *LPCTSTR;
typedef char * LPSTR;
#define OUT 
#define IN 
#define TRUE 1
#define FALSE 0 
typedef int bool;
#define true 1
#define false 0
#endif

 
#ifdef linux
#define _API_EXPORT
#define CALLING_CONV
typedef unsigned long COLORREF;
#else
#define _API_EXPORT 	__declspec(dllexport)
#define CALLING_CONV 	__stdcall
#endif
 
#ifdef __cplusplus
extern "C" {
#endif

typedef short SHORT;
  
// typedef void (CALLING_CONV *TFI_CallBack)( HANDLE h, int evt );
typedef void (*TFI_CallBack)(HANDLE h, int evt);

_API_EXPORT HANDLE CALLING_CONV TFI_Create();
_API_EXPORT BOOL CALLING_CONV TFI_Destroy(HANDLE h);
#ifdef linux
_API_EXPORT BOOL CALLING_CONV TFI_OpenCom(HANDLE h, const char *tty_device);
#else
_API_EXPORT BOOL CALLING_CONV TFI_OpenCom(HANDLE h, int iComID);
#endif
_API_EXPORT BOOL CALLING_CONV TFI_OpenNet(HANDLE h, const char *chIP);
_API_EXPORT BOOL CALLING_CONV TFI_Close(HANDLE h) ;
_API_EXPORT BOOL CALLING_CONV TFI_ClearAll(HANDLE h);
_API_EXPORT BOOL CALLING_CONV TFI_GetDeviceInfo(HANDLE h, char *pInfo );
_API_EXPORT BOOL CALLING_CONV TFI_SendLine(HANDLE h,int nLineNo,COLORREF nColor,int nRollType,int nRolltime,const char *szText);
_API_EXPORT BOOL CALLING_CONV TFI_SetMode(HANDLE h,BYTE mode,char *pRowType);
_API_EXPORT BOOL CALLING_CONV TFI_AlarmSet(HANDLE h,BYTE bAlarm,BYTE nTime);
_API_EXPORT BOOL CALLING_CONV TFI_SendVoice(HANDLE h, int vul,  const char* stVoice);
_API_EXPORT BOOL CALLING_CONV TFI_SetPassLight(HANDLE h,int x,int y,BYTE bPass);
_API_EXPORT BOOL CALLING_CONV TFI_ClearPipMode(HANDLE h,BYTE nLineNo);
_API_EXPORT BOOL CALLING_CONV TFI_DisplayPipMode(HANDLE h,BYTE nLineNo,SHORT x,SHORT y,BYTE lineH,COLORREF nColor,const char *szText);
_API_EXPORT BOOL CALLING_CONV TFI_EnableLog(HANDLE h, BOOL bEnable);
_API_EXPORT BOOL CALLING_CONV TFI_SetLogPath(HANDLE h, const char* Path);
_API_EXPORT BOOL CALLING_CONV TFI_PlayDu(HANDLE h, int vul);
_API_EXPORT BOOL CALLING_CONV TFI_SetPicture(HANDLE h,COLORREF nColor, int x,int bopen);
_API_EXPORT BOOL CALLING_CONV TFI_SetCallBack(HANDLE h,TFI_CallBack cb);
#if defined _WIN32 || defined _WIN64
_API_EXPORT BOOL CALLING_CONV TFI_SetWinMsg(HANDLE h, HWND hwnd, int msgno);
#endif
_API_EXPORT int CALLING_CONV TFI_GetQrCode(HANDLE h, char *qrcode );

// 回调或者windows消息事件码
// 上线消息
#define TFI_EVT_ONLINE		1
// 离线消息
#define TFI_EVT_OFFLINE		2
// 二维码数据到来
#define TFI_EVT_QRCODE		3
// 费显主板两个按键消息
#define TFI_EVT_BTN_NO1		4
#define TFI_EVT_BTN_NO2		5

#define TFI_EVT_MIGRATE_QRCODE	0x503

#ifdef __cplusplus
}
#endif

#endif


