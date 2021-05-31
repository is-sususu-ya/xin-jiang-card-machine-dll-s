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

	typedef void (* CALLTYPE ACCEventHandle)(int a, int b);

#ifndef linux
	DLLAPI BOOL CALLTYPE ACC_OpenDevice(int nCOM, int nBaudRate);
	DLLAPI void CALLTYPE ACC_SetEventMessage(HWND hWnd, UINT MsgNum);
#else 
	DLLAPI BOOL CALLTYPE ACC_OpenDevice(const char *ttyName, int nBaudRate);
#endif
	DLLAPI BOOL CALLTYPE ACC_SetEventCallBack(ACCEventHandle cb);
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
	DLLAPI int	CALLTYPE ACC_GetTxData(char *buf, int size);
	DLLAPI int	CALLTYPE ACC_GetRxData(char *buf, int size);
	DLLAPI int CALLTYPE ACC_GetKernelLog(char *buf, int size);

#ifdef __cplusplus
}
#endif

#endif
