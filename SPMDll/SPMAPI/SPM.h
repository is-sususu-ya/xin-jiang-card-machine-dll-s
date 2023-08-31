 
#ifndef _SPM_HEADER_H
#define _SPM_HEADER_H

#ifdef linux 
#include "wintype.h"
#define CALLTYPE
#define WINAPI
#define EXPAPI
#else
#include <winsock2.h>
#include <windows.h>
#define EXPAPI		__declspec(dllexport)
#define CALLTYPE	__stdcall
#endif


#define SPM_EVT_ONLINE		1
#define SPM_EVT_OFFLINE		2
#define SPM_EVT_QRCODE		3
#define SPM_EVT_IOCHANGE	4
#define SPM_EVT_HTTP_RESPONSE	5
#define SPM_EVT_CALL		200

#ifdef __cplusplus
extern "C" {
#endif

typedef void(CALLTYPE *SPM_CallBack)(HANDLE h, int code);
// typedef void(CALLTYPE *SPM_CallBack)(HANDLE h, int code);

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
EXPAPI BOOL CALLTYPE SPM_GetQrCodeEx(HANDLE h, int *index, char *buf);
EXPAPI BOOL CALLTYPE SPM_GetQrCode(HANDLE h, char *buf); 

<<<<<<< HEAD
=======
//EXPAPI BOOL CALLTYPE SPM_InitPhone(HANDLE h,  char* server, char*  clientId);
>>>>>>> feece9679f41cfe91447ca26b9467578960e885d
EXPAPI BOOL CALLTYPE SPM_InitPhone(HANDLE h, char* server, int port, char* phoneId1, char* password1, char* phoneId2, char* password2);
EXPAPI BOOL CALLTYPE SPM_CallPhone(HANDLE h, int index, char *phoneId, int timeout);
EXPAPI BOOL CALLTYPE SPM_AnswerPhone(HANDLE h, char *phoneId, int reply, int timeout);

// GPIO 输出
EXPAPI BOOL CALLTYPE SPM_GpioOutPut(HANDLE h, int pin, int val);
// GPIO 脉冲输出
EXPAPI BOOL CALLTYPE SPM_GpioPulse(HANDLE h, int pin, int tout);
// GPIO 反向脉冲输出
EXPAPI BOOL CALLTYPE SPM_GpioPulseNegative(HANDLE h, int pin, int tout);
// 读取DI值
EXPAPI BOOL CALLTYPE SPM_ReadDi(HANDLE h, int *parma);

/* 
	设置请求地址，每个设备只需要设置一次，设置后，系统会写入缴费机配置
*/
EXPAPI BOOL CALLTYPE SPM_SetHttpProxyAddress(HANDLE h, int index, const char *url, int size);
/* 设置连接超时参数 */
EXPAPI BOOL CALLTYPE SPM_SetHttpProxyConfig(HANDLE h, int con_tout, int pro_tou);
/*发起Post请求*/ 
EXPAPI BOOL CALLTYPE SPM_SetHttpPostProxyRequest(HANDLE h, int index, int id, int type, const char *conten, int size);
/*发起GEt请求*/
EXPAPI BOOL CALLTYPE SPM_SetHttpGetProxyRequest(HANDLE h, int id, const char *url, int size);
/*获取响应信息*/
EXPAPI BOOL CALLTYPE SPM_GetHttpProxyResponse(HANDLE h, int *id, int *ret, char *content, int max_size);

#ifdef __cplusplus
}
#endif

#endif