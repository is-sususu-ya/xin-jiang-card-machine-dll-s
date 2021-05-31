#include <string.h>
#include "../TCR8lib/TCR8lib.h"
#include "TCR8CollectCard.h"

#define TCR8LOG_DEFAULT_PATH	"./RunwellTCR8CDll.log"

typedef struct tagTCR8UserData {
#ifndef linux
	HWND	hWnd;
	UINT	Msg;
#endif
	ACCEventHandle fxc_acc;
	char    TxData[64];
	char    RxData[32];
	char    TCR8Log[256];
}TCR8UserData;

#define STRING_DLL_VRESION	"1.0.0"
   
TCR8HANDLE   m_hTCR8 = NULL;
TCR8UserData m_TCR8UserData;

static void ACC_EventHandle( void *hMachine, int nEventId, int nParam )
{
	TCR8HANDLE hTCR8 = (TCR8HANDLE)hMachine;
	TCR8UserData *pUserData = (TCR8UserData *)hTCR8->m_pUsrData; 
	if( (void *)nParam != NULL )
	{
		int len; 
		switch(nEventId)
		{
		case EVID_TX:
			len = strlen( (char *)nParam );
			memcpy( pUserData->TxData, (char *)nParam, len+1 );
			break; 
		case EVID_RX:
			len = strlen( (char *)nParam );
			memcpy( pUserData->RxData, (char *)nParam, len+1 );
			break; 
		case EVID_KERNELLOG:
			len = strlen( (char *)nParam );
			memcpy( pUserData->TCR8Log, (char *)nParam, len+1 );
			break;
		}
	} 
#ifndef linux
	PostMessage( pUserData->hWnd, pUserData->Msg, nEventId, nParam );
#endif
	if( m_TCR8UserData.fxc_acc )
		m_TCR8UserData.fxc_acc( nEventId, nParam );
}

DLLAPI BOOL CALLTYPE ACC_SetEventCallBack(ACCEventHandle cb)
{
	m_TCR8UserData.fxc_acc = cb;
}
  
#ifndef linux
DLLAPI void CALLTYPE ACC_SetEventMessage(HWND hWnd, UINT MsgNum)
{
	m_TCR8UserData.hWnd = hWnd;
	m_TCR8UserData.Msg  = MsgNum;
}
#endif

#ifndef linux
DLLAPI BOOL CALLTYPE ACC_OpenDevice(int nCOM, int nBaudRate)
#else
DLLAPI BOOL CALLTYPE ACC_OpenDevice(const char *ttyName, int nBaudRate)
#endif
{
	DWORD dwEventMask;

	if( m_hTCR8 != NULL )
	{
		TCR8_Log( m_hTCR8, "����λ���ظ����á� ACC_OpenDevice()���ȹر��ϴδ򿪵��豸!\n" );
		TCR8_Destroy( m_hTCR8 );
	}
	m_hTCR8 = TCR8_Create();

	if( m_hTCR8 == NULL )
		return FALSE;

	// enable TCR8 log
	TCR8_EnableLog( m_hTCR8, TCR8LOG_DEFAULT_PATH );
#ifndef linux
	TCR8_Log( m_hTCR8, "����λ�����á�ACC_OpenDevice() nCOM = %d, nBaudRate = %d.\n", nCOM, nBaudRate );
#else  
	TCR8_Log( m_hTCR8, "����λ�����á�ACC_OpenDevice() COM = %s, nBaudRate = %d.\n", ttyName, nBaudRate );
#endif
	TCR8_Log( m_hTCR8, "\tVersion %s\n", STRING_DLL_VRESION );

	// set user data and callback function
	_SetUserData( m_hTCR8, &m_TCR8UserData );
	_SetMachineType( m_hTCR8, TCR8_COLLECTOR );

	dwEventMask = (T8EVM_ALL & ~(EVM_BUTTONIGNRD | EVM_OUTCARDOK_EX | EVM_RECYCLEOK_EX | EVM_BOXEX | EVM_THREADEXIT | EVM_TX | EVM_RX | EVM_KERNELLOG));

	_SetEventMask( m_hTCR8, dwEventMask );
	TCR8_SetCallback( m_hTCR8, ACC_EventHandle );

#ifndef linux
	// set COM parameter
	TCR8_SetComPort( m_hTCR8, nCOM, nBaudRate );
#else
	TCR8_SetComPort( m_hTCR8, ttyName, nBaudRate );
#endif
	
	// open TCR8 device
	if( !TCR8_OpenDevice( m_hTCR8 ) )
	{
		return FALSE;
	}

	// run TCR8 work thread
	if( !TCR8_Run( m_hTCR8 ) )
	{
		TCR8_Log( m_hTCR8, "[Error] -TCR8_Run- fail!.\n" );
		TCR8_Destroy( m_hTCR8 );
		return FALSE;
	} 
	return TRUE;
}

DLLAPI BOOL CALLTYPE ACC_CloseDevice(void)
{
	TCR8_Log( m_hTCR8, "����λ�����á� ACC_CloseDevice()\n" );
	TCR8_SetCallback( m_hTCR8, NULL );
	TCR8_Destroy( m_hTCR8 );
	m_hTCR8 = NULL;
	return TRUE;
}

DLLAPI BOOL CALLTYPE ACC_CollectCard(void)
{
	TCR8_Log( m_hTCR8, "����λ�����á� ACC_CollectCard()\n" );
	return TCR8_CollectCard( m_hTCR8, 0 );
}

DLLAPI BOOL CALLTYPE ACC_RecycleCard(void)
{
	TCR8_Log( m_hTCR8, "����λ�����á� ACC_RecycleCard()\n" );
	return TCR8_RecycleCard( m_hTCR8 ); 
}

DLLAPI BOOL CALLTYPE ACC_ReturnCard(void)
{
	TCR8_Log( m_hTCR8, "����λ�����á� ACC_ReturnCard()\n" );
	return TCR8_ReturnCard( m_hTCR8, 0 );
}

DLLAPI BOOL CALLTYPE ACC_SwitchAntenna(int nPosition)
{
	TCR8_Log( m_hTCR8, "����λ�����á� ACC_SwitchAntenna() nPosition = %d\n", nPosition );
	return TCR8_SwitchAntenna( m_hTCR8, nPosition );
}

DLLAPI BOOL CALLTYPE ACC_IsOnline(void)
{
	TCR8_Log( m_hTCR8, "����λ�����á� ACC_IsOnline()\n" );
	return _IsOnline( m_hTCR8 );
}

DLLAPI BOOL CALLTYPE ACC_ForceReturn(int nChannel)
{
	TCR8_Log( m_hTCR8, "����λ�����á� ACC_ForceReturn() nChannel = %d\n", nChannel );
	return FALSE;
	//return TCR8_ForceEjectCard( m_hTCR8, nChannel );
}

DLLAPI BOOL CALLTYPE ACC_ForceRecycle(int nChannel)
{
	TCR8_Log( m_hTCR8, "����λ�����á� ACC_ForceRecycle() nChannel = %d\n", nChannel );
	return FALSE;
	//return TCR8_ForceRecycleCard( m_hTCR8, nChannel );
}

DLLAPI BOOL CALLTYPE ACC_GetFirmwareVer(DWORD *ver)
{
	TCR8_Log( m_hTCR8, "����λ�����á� ACC_GetFirmwareVer() ver = %d\n", ver );
	if( ver != NULL )
	{
		*ver = _GetKernelVersion( m_hTCR8 );
		return TRUE;
	} 
	return FALSE;
}

DLLAPI BOOL CALLTYPE ACC_SetBoxSN(int nChannel, DWORD dwSN)
{
	TCR8_Log( m_hTCR8, "����λ�����á� ACC_SetBoxSN() nChannel = %d, dwSN = %d\n", nChannel, dwSN );
	return TCR8_SetCartridgeInfo( m_hTCR8, nChannel, dwSN, 999 );
}

DLLAPI BOOL CALLTYPE ACC_GetBoxSN(int nChannel, DWORD *dwSN)
{
	TCR8_Log( m_hTCR8, "����λ�����á� ACC_GetBoxSN() nChannel = %d, dwSN = %d\n", nChannel, dwSN );

	if( (0 < nChannel && nChannel < 5) && (dwSN != NULL) )
	{
		*dwSN = _GetCartridgeSN( m_hTCR8, (nChannel - 1) );
		return TRUE;
	} 
	return FALSE;
}
 
DLLAPI BOOL CALLTYPE ACC_SetCardCounter(int nChannel, int nCount)
{
	DWORD dwSN = 0;

	TCR8_Log( m_hTCR8, "����λ�����á� ACC_SetCardCounter() nChannel = %d, nCount = %d\n", nChannel, nCount );

	if( 0 < nChannel && nChannel < 5 )
	{
		dwSN = _GetCartridgeSN( m_hTCR8, (nChannel-1) );
	}

	return TCR8_SetCartridgeInfo( m_hTCR8, nChannel, dwSN, nCount );
}

DLLAPI BOOL CALLTYPE ACC_GetCardCounter(int nChannel, int *nCount)
{
	TCR8_Log( m_hTCR8, "����λ�����á� ACC_GetCardCounter() nChannel = %d, nCount = %d\n", nChannel, nCount );
	if( (0 < nChannel && nChannel < 5) && (nCount != NULL) )
	{
		*nCount = _GetChannelCount( m_hTCR8, (nChannel-1) ) - (_IsCardOnAntenna( m_hTCR8, (nChannel-1) ) ? 1 : 0 );
		return TRUE;
	}

	return FALSE;
}

DLLAPI BOOL CALLTYPE ACC_GetChannelState(int nChannel, int *nState)
{ 
	if( (0 < nChannel && nChannel < 5) && (nState != NULL) )
	{
		*nState = _GetChannelState( m_hTCR8, (nChannel-1) );
		return TRUE;
	}
	else
	{
		TCR8_Log( m_hTCR8, "����λ�����á� ACC_GetChannelState() nChannel = %d, nState = %d\n", nChannel, nState );
	} 
	return FALSE;
}

DLLAPI BOOL CALLTYPE ACC_IsBoxLoad(int nChannel)
{
	TCR8_Log( m_hTCR8, "����λ�����á� ACC_IsBoxLoad() nChannel = %d\n", nChannel );
	if( 0 < nChannel && nChannel < 5 )
	{
		return _HasCartridge( m_hTCR8, (nChannel-1) );
	}

	return FALSE;
}
 
DLLAPI int	CALLTYPE ACC_GetTxData( char *buf, int size )
{
	int len = 0;
	TCR8UserData *pUserData = (TCR8UserData *)m_hTCR8->m_pUsrData; 
	TCR8_Log( m_hTCR8, "����λ�����á� ACC_GetTxData() buf = %p, size = %d\n", buf, size ); 
	if( buf == NULL )
		return 0; 
	len = strlen( pUserData->TxData );
	if( len > 0 )
	{
		if ( len > size )
			len = size;
		strncpy( buf, pUserData->TxData, size );
	} 
	return len;
}

DLLAPI int	CALLTYPE ACC_GetRxData( char *buf, int size )
{
	int len = 0;
	TCR8UserData *pUserData = (TCR8UserData *)m_hTCR8->m_pUsrData; 
	TCR8_Log( m_hTCR8, "����λ�����á� ACC_GetRxData() buf = %p, size = %d\n", buf, size ); 
	if( buf == NULL )
		return 0; 
	len = strlen( pUserData->RxData );
	if( len > 0 )
	{
		if ( len > size )
			len = size;
		strncpy( buf, pUserData->RxData, size );
	} 
	return len;
}
 
DLLAPI int CALLTYPE ACC_GetKernelLog( char *buf, int size )
{
	int len = 0;
	TCR8UserData *pUserData = (TCR8UserData *)m_hTCR8->m_pUsrData; 
	TCR8_Log( m_hTCR8, "����λ�����á� ACC_GetKernelLog() buf = %p, size = %d\n", buf, size ); 
	if( buf == NULL )
		return 0; 
	len = strlen( pUserData->TCR8Log );
	if( len > 0 )
	{
		if ( len > size )
			len = size;
		strncpy( buf, pUserData->TCR8Log, size );
	}

	return len;
}
