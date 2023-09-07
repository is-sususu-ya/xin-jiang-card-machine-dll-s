

#ifndef AUTO_CARD_MACHINE_SEND
#define AUTO_CARD_MACHINE_SEND

#ifndef linux 
#define DLLAPI		__declspec(dllexport)
#define CALLTYPE	__stdcall  
#include <windows.h>
#else  // [linux]
#include "wintype.h" 
#define DLLAPI
#define CALLTYPE
#define	IN
#define OUT 
#define _MAX_FNAME 256
#define _MAX_EXT 64 
#endif

#ifdef __cplusplus
extern "C" {
#endif

	typedef void(CALLTYPE *ACM_EventCallBack)(int, int);
#ifndef linux	
	DLLAPI BOOL CALLTYPE ACM_OpenDevice(int nCOM, int nBaudRate);
	DLLAPI void CALLTYPE ACM_SetEventMessage(HWND hWnd, UINT MsgNum);
#else
	DLLAPI BOOL CALLTYPE ACM_OpenDevice(const char *ttyDev, int nBaudRate);
#endif
	DLLAPI void CALLTYPE ACM_SetEventCallBackFunc(ACM_EventCallBack cb); 
	DLLAPI BOOL CALLTYPE ACM_CloseDevice(void);
	DLLAPI BOOL CALLTYPE ACM_IssueCard(void);
	DLLAPI BOOL CALLTYPE ACM_RecycleCard(void);
	DLLAPI BOOL CALLTYPE ACM_Reject(void);
	DLLAPI BOOL CALLTYPE ACM_SwitchAntenna(int nPosition);
	DLLAPI BOOL CALLTYPE ACM_IsOnline(void);
	DLLAPI BOOL CALLTYPE ACM_SwitchChannel(int nChannel);
	DLLAPI BOOL CALLTYPE ACM_ForceEject(int nChannel);
	DLLAPI BOOL CALLTYPE ACM_ForceRecycle(int nChannel);
	DLLAPI BOOL CALLTYPE ACM_GetFirmwareVer(DWORD *ver);
	DLLAPI BOOL CALLTYPE ACM_SetBoxSN(int nChannel, DWORD dwSN);
	DLLAPI BOOL CALLTYPE ACM_GetBoxSN(int nChannel, DWORD *dwSN);
	DLLAPI BOOL CALLTYPE ACM_SetCardCounter(int nChannel, int nCount);
	DLLAPI BOOL CALLTYPE ACM_GetCardCounter(int nChannel, int *nCount);
	DLLAPI BOOL CALLTYPE ACM_GetChannelState(int nChannel, int *nState);
	DLLAPI BOOL CALLTYPE ACM_GetActiveChannel(int nPosition, int *nActiveChannel);
	DLLAPI BOOL CALLTYPE ACM_IsBoxLoad(int nChannel);
	DLLAPI LPCTSTR CALLTYPE ACM_GetEventText(int nEventId, int nParam);
	DLLAPI LPCTSTR CALLTYPE ACM_GetTransStateText(int st);
	DLLAPI LPCTSTR CALLTYPE ACM_GetButtonIgnoreText(int code);
	DLLAPI int	CALLTYPE ACM_GetTxData(char *buf, int size);
	DLLAPI int	CALLTYPE ACM_GetRxData(char *buf, int size);
	DLLAPI int CALLTYPE ACM_GetKernelLog(char *buf, int size);
	DLLAPI BOOL CALLTYPE ACM_PlayAudio(int nIndex);
	DLLAPI BOOL CALLTYPE ACM_TriggerButton(int nChannel);
	DLLAPI BOOL CALLTYPE ACM_PullBackToAnt(int nChannel);

	/**
	 * @brief 面板控制指令
	 * 
	 * @param deck  工位 1： 上工位 2： 下工位
	 * @param ctrl   伸缩控制： 1： 伸出    0： 缩回
	 * @return DLLAPI 
	 */
	DLLAPI int	CALLTYPE ACM_PushPanelCtrl(int deck, int ctrl );

#ifdef __cplusplus
}
#endif

#endif // #ifdef ENABLE_API1
