#ifndef __ALBDLL_H__
#define __ALBDLL_H__

#ifdef linux
#define _API_EXPORT
#define CALLTYPE
#else		// _WIN32 or _WIN64
#define _API_EXPORT //__declspec(dllexport)
#define CALLTYPE __stdcall
#endif

#ifdef linux
#include "..\common\wintype.h"
#else
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C"{
#endif


typedef void (* DEVEventCallBack)( void *h, int nEventId, int nParam );

#if defined _WIN32 || defined _WIN64
_API_EXPORT void   CALLTYPE DEV_EnableEventMessage( HWND hWnd, UINT MsgID );	/* obsoleted API, use DEV_EnableEventMessageEx instead */
_API_EXPORT BOOL CALLTYPE DEV_EnableEventMessageEx( HANDLE h, HWND hWnd, UINT MsgID );
#else
// linux user need callback mechanism to handle event
_API_EXPORT BOOL   CALLTYPE DEV_SetEventHandle( HANDLE h, DEVEventCallBack pCallBack );
#endif

/* 1. 使用网线和栏杆机控制器通信时 strIP 为栏杆机控制器的IP地址，格式为"192.168.1.35"
*  2. 使用串口和栏杆机控制器通信时 strIP 为连接栏杆机控制器的串口号，格式为"COM3"
*/
_API_EXPORT HANDLE CALLTYPE DEV_Open( const char *strIP );
_API_EXPORT BOOL   CALLTYPE DEV_Close( HANDLE h );
_API_EXPORT BOOL   CALLTYPE DEV_ALB_Ctrl( HANDLE h, BOOL bOpen );
_API_EXPORT BOOL   CALLTYPE DEV_GetStatus( HANDLE h, DWORD *dwStatus );

// 消息编号
#define EVID_BALUSTRADE	1
#define EVID_FCOIL		2
#define EVID_BCOIL		3
#define EVID_ACCEPT		96
#define EVID_REFUSE		97
#define EVID_ONLINE		98
#define EVID_OFFLINE	99

#ifdef __cplusplus
}
#endif

#endif //
