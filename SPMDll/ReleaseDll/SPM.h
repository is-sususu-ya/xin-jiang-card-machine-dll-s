 
#ifndef _SPM_HEADER_H
#define _SPM_HEADER_H

#ifdef linux
#include <winttype.h>
#else
#include <winsock2.h>
#include <windows.h>
#define EXPAPI		__declspec(dllexport)
#define CALLTYPE	__stdcall
#endif


#define SPM_EVT_ONLINE		1
#define SPM_EVT_OFFLINE		2
#define SPM_EVT_QRCODE		3

#ifdef __cplusplus
extern "C" {
#endif

typedef void(CALLTYPE *SPM_CallBack)(HANDLE h, int code);

EXPAPI HANDLE CALLTYPE SPM_Create();
EXPAPI BOOL CALLTYPE SPM_Destory(HANDLE h);
EXPAPI BOOL CALLTYPE SPM_IsOnline(HANDLE h);
EXPAPI BOOL CALLTYPE SPM_Open(HANDLE h, const char *devName); 
EXPAPI BOOL CALLTYPE SPM_Close(HANDLE h);
#ifdef _WIN32 || defined _WIN64
EXPAPI BOOL CALLTYPE SPM_SetWinMsg(HANDLE h, HWND wnd, int nMsgNo);
#endif
EXPAPI BOOL CALLTYPE SPM_SetCallBack(HANDLE h, SPM_CallBack);
EXPAPI BOOL CALLTYPE SPM_Led_ClearAll(HANDLE h);
EXPAPI BOOL CALLTYPE SPM_Led_SendLineText(HANDLE h, int nLineNumber, int color, int alignType, const char *text, int len );
EXPAPI BOOL CALLTYPE SPM_Voice_SendText(HANDLE h, int vol, const char *text, int len);
EXPAPI BOOL CALLTYPE SPM_Lcd_ChangeContext(HANDLE h, int index, const char *text, int len);
EXPAPI BOOL CALLTYPE SPM_SyncTime(HANDLE h);
EXPAPI BOOL CALLTYPE SPM_Reboot(HANDLE h);
EXPAPI BOOL CALLTYPE SPM_EnableQrCode(HANDLE h, BOOL en);
EXPAPI BOOL CALLTYPE SPM_GetQrCode(HANDLE h, char *buf);
 

#ifdef __cplusplus
}
#endif

#endif