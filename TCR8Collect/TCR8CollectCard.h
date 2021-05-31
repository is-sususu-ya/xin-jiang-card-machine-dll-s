#ifndef TCR8DLL_COLLECT_INCLUDED_
#define TCR8DLL_COLLECT_INCLUDED_
 
#ifndef linux 
#define DLLAPI		__declspec(dllexport)
#define CALLTYPE	__stdcall  
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

	DLLAPI void CALLTYPE ACC_SetEventMessage(HWND hWnd, UINT MsgNum);
	DLLAPI BOOL CALLTYPE ACC_OpenDevice(int nCOM, int nBaudRate);
	DLLAPI BOOL CALLTYPE ACC_CloseDevice(void);
	DLLAPI BOOL CALLTYPE ACC_CollectCard(void);
	DLLAPI BOOL CALLTYPE ACC_RecycleCard(void);
	DLLAPI BOOL CALLTYPE ACC_ReturnCard(void);
	DLLAPI BOOL CALLTYPE ACC_SwitchAntenna(int nPosition);
	DLLAPI BOOL CALLTYPE ACC_IsOnline(void);
	DLLAPI BOOL CALLTYPE ACC_ForceReturn(int nChannel);
	DLLAPI BOOL CALLTYPE ACC_ForceRecycle(int nChannel);
	DLLAPI BOOL CALLTYPE ACC_GetFirmwareVer(DWORD *ver);
	DLLAPI BOOL CALLTYPE ACC_SetBoxSN(int nChannel, DWORD dwSN);
	DLLAPI BOOL CALLTYPE ACC_GetBoxSN(int nChannel, DWORD *dwSN);
	DLLAPI BOOL CALLTYPE ACC_SetCardCounter(int nChannel, int nCount);
	DLLAPI BOOL CALLTYPE ACC_GetCardCounter(int nChannel, int *nCount);
	DLLAPI BOOL CALLTYPE ACC_GetChannelState(int nChannel, int *nState);
	DLLAPI BOOL CALLTYPE ACC_IsBoxLoad(int nChannel);
	DLLAPI void CALLTYPE ACC_SetEventMessage(HANDLE h, HWND hWnd, UINT MsgNum);
	DLLAPI BOOL CALLTYPE ACC_OpenDevice(int nCOM, int nBaudRate);
	DLLAPI BOOL CALLTYPE ACC_CloseDevice(void);
	DLLAPI BOOL CALLTYPE ACC_IssueCard(void);
	DLLAPI BOOL CALLTYPE ACC_RecycleCard(void);
	DLLAPI BOOL CALLTYPE ACC_Reject(void);
	DLLAPI BOOL CALLTYPE ACC_SwitchAntenna(int nPosition);
	DLLAPI BOOL CALLTYPE ACC_IsOnline(void);
	DLLAPI BOOL CALLTYPE ACC_SwitchChannel(int nChannel);
	DLLAPI BOOL CALLTYPE ACC_ForceEject(int nChannel);
	DLLAPI BOOL CALLTYPE ACC_ForceRecycle(int nChannel);
	DLLAPI BOOL CALLTYPE ACC_GetFirmwareVer(DWORD *ver);
	DLLAPI BOOL CALLTYPE ACC_GetBoxSN(int nChannel, DWORD *dwSN);
	DLLAPI BOOL CALLTYPE ACC_SetCartridgeInfo(int nChannel, DWORD dwSN, int nCount);
	DLLAPI BOOL CALLTYPE ACC_GetCardCounter(int nChannel, int *nCount);
	DLLAPI BOOL CALLTYPE ACC_GetChannelState(int nChannel, int *nState);
	DLLAPI BOOL CALLTYPE ACC_GetActiveChannel(int nPosition, int *nActiveChannel);
	DLLAPI BOOL CALLTYPE ACC_IsBoxLoad(int nChannel);
	DLLAPI LPCTSTR CALLTYPE ACC_GetEventText(int nEventId, int nParam);
	DLLAPI LPCTSTR CALLTYPE ACC_GetTransStateText(int st);
	DLLAPI LPCTSTR CALLTYPE ACC_GetButtonIgnoreText(int code);
	DLLAPI int	CALLTYPE ACC_GetTxData(char *buf, int size);
	DLLAPI int	CALLTYPE ACC_GetRxData(char *buf, int size);
	DLLAPI int CALLTYPE ACC_GetKernelLog(char *buf, int size);

#ifdef __cplusplus
}
#endif

#endif
