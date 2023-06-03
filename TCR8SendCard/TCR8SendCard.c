
#include <string.h>
#include "../TCR8lib/TCR8lib.h" 
#include "TCR8SendCard.h"

#define TCR8LOG_DEFAULT_PATH	"./RunwellTCR8Dll.log"

#define Sleep(n)	usleep(n*1000)

#undef MAX_PATH
#define MAX_PATH 256

typedef struct tagTCR8UserData {
#ifndef linux
	HWND	hWnd;
	UINT	Msg; 
#endif
	ACM_EventCallBack pEventCB; 
	char    TxData[64];
	char    RxData[32];
	char    TCR8Log[256]; 
	BOOL	bContainCardOnAnt;  // 设置卡数的时候，传入的参数是否包含天线位置的卡，默认不包含
	int		nStatusChange;  // 卡机刚连上时收到的状态变化消息记录，有的车道软件在收到离线帧时就会设置卡数，这样会导致卡数设置有问题
							// bit0 ~ 3 分别表示 通道1 ~ 通道4的状态变化，如果这4位都置位了就说明，卡机状态都已经上报，可以进行设置卡数
}TCR8UserData;

#define STRING_DLL_VRESION	"1.4.0"
  
TCR8HANDLE   m_hTCR8 = NULL;
TCR8UserData m_TCR8UserData = {0};
 
static void ACM_EventHandle( void *hMachine, int nEventId, int nParam )
{
	TCR8HANDLE hTCR8 = (TCR8HANDLE)hMachine;
	TCR8UserData *pUserData = (TCR8UserData *)hTCR8->m_pUsrData;

	if( (void *)nParam != NULL )
	{
		int len;

		switch(nEventId)
		{
		case EVID_STATECHANGE:
			m_TCR8UserData.nStatusChange |= 0x01 << (nParam -1);
			break;

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

	if((nEventId == EVID_LINK && nParam) || nEventId == EVID_POWERON)
	{
		TCR8_EnableKernelLog( hTCR8, TRUE );
	}

#ifndef linux
	if( m_TCR8UserData.hWnd != NULL )
	{
		TCR8_Log( hTCR8, "PostMessage() hWnd = %p, Msg = %d, wParam(nEventId) = %d, lParam(nParam) = %d\n", m_TCR8UserData.hWnd, m_TCR8UserData.Msg, nEventId, nParam );
		PostMessage( pUserData->hWnd, pUserData->Msg, nEventId, nParam );
	}
#endif 
	if( m_TCR8UserData.pEventCB != NULL )
	{
		TCR8_Log( hTCR8, "【Note】 调用用户注册的卡机事件回调函数( nEventId = %d, nEventParam = %d)\n", nEventId, nParam );
		m_TCR8UserData.pEventCB( nEventId, nParam );
		TCR8_Log( hTCR8, "【Note】 从用户注册的卡机事件回调函数返回\n");
	} 
}
 
#ifndef linux
DLLAPI void CALLTYPE ACM_SetEventMessage(HWND hWnd, UINT MsgNum)
{
	m_TCR8UserData.hWnd = hWnd;
	m_TCR8UserData.Msg  = MsgNum;
}
#endif

DLLAPI void CALLTYPE ACM_SetEventCallBackFunc(ACM_EventCallBack cb)
{
	m_TCR8UserData.pEventCB = cb;
}

#ifndef linux
DLLAPI BOOL CALLTYPE ACM_OpenDevice(int nCOM, int nBaudRate)
#else
DLLAPI BOOL CALLTYPE ACM_OpenDevice(const char *ttyDev, int nBaudRate)
#endif
{
	FILE *fp = NULL;
	char chDllPath[256];
	char drive[64];
	char dir[256]; 
#ifndef linux
	HMODULE hDll;
#else
	printf("input :%s %d \r\n", ttyDev, nBaudRate );
#endif
	if( m_hTCR8 != NULL )
	{
		TCR8_Log( m_hTCR8, "【上位机重复调用】 ACM_OpenDevice()，先关闭上次打开的设备!\n" );
		TCR8_Destroy( m_hTCR8 );
	}
	m_hTCR8 = TCR8_Create();

	if( m_hTCR8 == NULL )
		return FALSE;
 
	TCR8_EnableLog( m_hTCR8, TCR8LOG_DEFAULT_PATH );
#ifndef linux
	TCR8_Log( m_hTCR8, "【上位机调用】ACM_OpenDevice() nCOM = %d, nBaudRate = %d.\n", nCOM, nBaudRate );
#else
	TCR8_Log( m_hTCR8, "【上位机调用】ACM_OpenDevice() nCOM = %s, nBaudRate = %d.\n", ttyDev, nBaudRate );
#endif
	TCR8_Log( m_hTCR8, "\tVersion %s\n", STRING_DLL_VRESION );
 
	m_TCR8UserData.bContainCardOnAnt = FALSE;
	m_TCR8UserData.nStatusChange = 0;

#ifndef linux
	// 获取动态库所在目录
	hDll = GetModuleHandle("AutoCardMachine.dll");
	if( hDll == NULL )
	{
		TCR8_Log( m_hTCR8, "\t--> 获取 AutoCardMachine.dll 句柄失败(错误代码: %d)!\n", GetLastError() );
	}
	else
	{
		if( GetModuleFileName(hDll, chDllPath, MAX_PATH) == 0 )
		{
			TCR8_Log( m_hTCR8, "\t--> 获取 AutoCardMachine.dll 路径失败(错误代码: %d)!\n", GetLastError() );
		}
		else
		{
			_splitpath( chDllPath, drive, dir, NULL,NULL );

			TCR8_Log( m_hTCR8, "\t动态库所在路径为: %s%s\n", drive, dir );

			sprintf(chDllPath,"%s%s\\AutoCardMachine.ini", drive, dir);

			if( (fp = fopen( chDllPath/*"d:/lane/ddl/AutoCardMachine.ini"*/, "r")) == NULL )
			{
				TCR8_Log( m_hTCR8, "\t--> 打开 %s 失败!\n", chDllPath );
			}
			else
			{
				char line[80];
				char *ptr;

				while ( fgets( line, sizeof(line), fp ) != NULL )
				{
					if ( line[0] == '#' ) continue;
					if ( (ptr = strstr( line, "bContainCardOnAnt=" )) != NULL )
					{
						ptr += strlen( "bContainCardOnAnt=" );
						TCR8_Log( m_hTCR8, "\tbContainCardOnAnt=%s\n", ptr );
						while ( *ptr && !isalpha( *ptr ) ) ptr++; 
					#ifdef linux
						if( strncasecmp( ptr, "TRUE", 4 ) == 0 )
					#else
						if( _strnicmp( ptr, "TRUE", 4 ) == 0 )
					#endif
						{
							m_TCR8UserData.bContainCardOnAnt = TRUE;
						}
					}
				}
				fclose( fp );
			}
		}
	}
#else
	// TODO
#endif
	// set user data and callback function
	_SetUserData( m_hTCR8, &m_TCR8UserData );
	_SetEventMask( m_hTCR8, (0xFFFF | EVM_THREADEXIT | EVM_PULLBACKOK | EVM_PULLBACKFAIL ) );

	TCR8_SetCallback( m_hTCR8, ACM_EventHandle );

	// set COM parameter 
#ifndef linux
	TCR8_SetComPort( m_hTCR8, nCOM, nBaudRate );
#else
	TCR8_SetComPort( m_hTCR8, ttyDev, nBaudRate );
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

DLLAPI BOOL CALLTYPE ACM_CloseDevice(void)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_CloseDevice()\n" );
	TCR8_SetCallback( m_hTCR8, NULL );
	TCR8_Destroy( m_hTCR8 );
	m_hTCR8 = NULL;
	return TRUE;
}

DLLAPI BOOL CALLTYPE ACM_IssueCard(void)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_IssueCard()\n" );
	return TCR8_EjectCard( m_hTCR8, 0 );
}

DLLAPI BOOL CALLTYPE ACM_RecycleCard(void)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_RecycleCard()\n" );
	return TCR8_RecycleCard( m_hTCR8 ); 
}

DLLAPI BOOL CALLTYPE ACM_Reject(void)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_Reject()\n" );
	return TCR8_CancelButton( m_hTCR8 );
}

DLLAPI BOOL CALLTYPE ACM_SwitchAntenna(int nPosition)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_SwitchAntenna() nPosition = %d\n", nPosition );
	return TCR8_SwitchAntenna( m_hTCR8, nPosition );
}

DLLAPI BOOL CALLTYPE ACM_IsOnline(void)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_IsOnline()\n" );
	return _IsOnline( m_hTCR8 );
}

DLLAPI BOOL CALLTYPE ACM_SwitchChannel(int nChannel)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_SwitchChannel() nChannel = %d\n", nChannel );
	return TCR8_SwitchChannel( m_hTCR8, nChannel );
}

DLLAPI BOOL CALLTYPE ACM_ForceEject(int nChannel)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_ForceEject() nChannel = %d\n", nChannel );
	//return FALSE;
	return TCR8_ForceEjectCard( m_hTCR8, nChannel );
}

DLLAPI BOOL CALLTYPE ACM_ForceRecycle(int nChannel)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_ForceRecycle() nChannel = %d\n", nChannel );
	//return FALSE;
	return TCR8_ForceRecycleCard( m_hTCR8, nChannel );
}

DLLAPI BOOL CALLTYPE ACM_GetFirmwareVer(DWORD *ver)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_GetFirmwareVer() ver = %p\n", ver );
	if( ver != NULL )
	{
		*ver = _GetKernelVersion( m_hTCR8 );
		return TRUE;
	}

	return FALSE;
}

DLLAPI BOOL CALLTYPE ACM_SetBoxSN(int nChannel, DWORD dwSN)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_SetBoxSN() nChannel = %d, dwSN = %d\n", nChannel, dwSN );
	return TCR8_SetCartridgeInfo( m_hTCR8, nChannel, dwSN, 999 );
}

DLLAPI BOOL CALLTYPE ACM_GetBoxSN(int nChannel, DWORD *dwSN)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_GetBoxSN() nChannel = %d, dwSN = %p\n", nChannel, dwSN );

	if( (0 < nChannel && nChannel < 5) && (dwSN != NULL) )
	{
		*dwSN = _GetCartridgeSN( m_hTCR8, (nChannel - 1) );
		return TRUE;
	}

	return FALSE;
}

DLLAPI BOOL CALLTYPE ACM_SetCardCounter(int nChannel, int nCount)
{
	int i = 0;

	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_SetCardCounter() nChannel = %d, nCount = %d\n", nChannel, nCount );

	if( m_TCR8UserData.bContainCardOnAnt )
	{
		while( !(m_TCR8UserData.nStatusChange & (0x01 << (nChannel-1))) && i < 1000 )
		{
			Sleep( 10 );
			i++;
		}

		if( nCount > 0 && _IsCardOnAntenna( m_hTCR8, nChannel-1) )
			nCount --;
	}

	return TCR8_SetCartridgeInfo( m_hTCR8, nChannel, 0, nCount );
}

DLLAPI BOOL CALLTYPE ACM_GetCardCounter(int nChannel, int *nCount)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_GetCardCounter() nChannel = %d, nCount = %p\n", nChannel, nCount );
	if( (0 < nChannel && nChannel < 5) && (nCount != NULL) )
	{
		*nCount = _GetChannelCount( m_hTCR8, (nChannel-1) ) - (_IsCardOnAntenna( m_hTCR8, (nChannel-1) ) ? 1 : 0 );
		return TRUE;
	}

	return FALSE;
}

DLLAPI BOOL CALLTYPE ACM_GetChannelState(int nChannel, int *nState)
{
	//TCR8_Log( m_hTCR8, "【上位机调用】 ACM_GetChannelState() nChannel = %d, nState = %d\n", nChannel, nState );
	if( (0 < nChannel && nChannel < 5) && (nState != NULL) )
	{
		*nState = _GetChannelState( m_hTCR8, (nChannel-1) );
		return TRUE;
	}
	else
	{
		TCR8_Log( m_hTCR8, "【上位机调用】 ACM_GetChannelState() nChannel = %d, nState = %p\n", nChannel, nState );
	}

	return FALSE;
}

DLLAPI BOOL CALLTYPE ACM_GetActiveChannel(int nPosition, int *nActiveChannel)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_GetActiveChannel() nPosition = %d, nActiveChannel = %p\n", nPosition, nActiveChannel );
	if( ( (1 == nPosition) || (2 == nPosition) ) && (nActiveChannel != NULL) )
	{
		*nActiveChannel = _GetActive( m_hTCR8, (nPosition-1) );
		return TRUE;
	}

	return FALSE;
}

DLLAPI BOOL CALLTYPE ACM_IsBoxLoad(int nChannel)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_GetActiveChannel() nChannel = %d\n", nChannel );
	if( 0 < nChannel && nChannel < 5 )
	{
		return _HasCartridge( m_hTCR8, (nChannel-1) );
	}

	return FALSE;
}

DLLAPI LPCTSTR CALLTYPE ACM_GetEventText( int nEventId, int nParam )
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_GetEventText() nEventId = %d, nParam = %d\n", nEventId, nParam );
	return TCR8_GetEventText( nEventId, nParam );
}

DLLAPI LPCTSTR CALLTYPE ACM_GetTransStateText(int st)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_GetEventText() st = %d\n", st );

	return TCR8_GetTransStateText( st );
}

DLLAPI LPCTSTR CALLTYPE ACM_GetButtonIgnoreText( int code )
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_GetButtonIgnoreText() code = %d\n", code );
	return TCR8_GetButtonIgnoreText( code );
}

DLLAPI int	CALLTYPE ACM_GetTxData( char *buf, int size )
{
	int len = 0;
	TCR8UserData *pUserData = (TCR8UserData *)m_hTCR8->m_pUsrData;

	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_GetTxData() buf = %p, size = %d\n", buf, size );

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

DLLAPI int	CALLTYPE ACM_GetRxData( char *buf, int size )
{
	int len = 0;
	TCR8UserData *pUserData = (TCR8UserData *)m_hTCR8->m_pUsrData;

	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_GetRxData() buf = %p, size = %d\n", buf, size );

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

DLLAPI int CALLTYPE ACM_GetKernelLog( char *buf, int size )
{
	int len = 0;
	TCR8UserData *pUserData = (TCR8UserData *)m_hTCR8->m_pUsrData;

	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_GetRxData() buf = %p, size = %d\n", buf, size );

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

DLLAPI BOOL CALLTYPE ACM_PlayAudio(int nIndex)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_PlayAudio(%d)\n", nIndex );
	
	if( (0 <= nIndex) && (nIndex <= 9) )
	{
		return TCR8_PlayAudio( m_hTCR8, nIndex );
	}

	return FALSE;
}

BOOL CALLTYPE ACM_TriggerButton(int nChannel)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_TriggerButton(%d)\n", nChannel );

	return TCR8_TriggerButton( m_hTCR8, nChannel );
}

BOOL CALLTYPE ACM_PullBackToAnt(int nChannel)
{
	TCR8_Log( m_hTCR8, "【上位机调用】 ACM_PullBackToAnt(%d)\n", nChannel );

	return TCR8_PullBackToAnt( m_hTCR8, nChannel );
} 
