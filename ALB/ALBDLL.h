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

/* 1. ʹ�����ߺ����˻�������ͨ��ʱ strIP Ϊ���˻���������IP��ַ����ʽΪ"192.168.1.35"
*  2. ʹ�ô��ں����˻�������ͨ��ʱ strIP Ϊ�������˻��������Ĵ��ںţ���ʽΪ"COM3"
*/
_API_EXPORT HANDLE CALLTYPE DEV_Open( const char *strIP );
_API_EXPORT BOOL   CALLTYPE DEV_Close( HANDLE h );
_API_EXPORT BOOL   CALLTYPE DEV_ALB_Ctrl( HANDLE h, BOOL bOpen );
_API_EXPORT BOOL   CALLTYPE DEV_GetStatus( HANDLE h, DWORD *dwStatus );

// ��Ϣ���
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
