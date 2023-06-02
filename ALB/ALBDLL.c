
#if defined _WIN32 || defined _WIN64
#include <Windows.h>
#include "../common/wintty.h"
#include "../common/winnet.h"
#include "../common/mtrace.h"

//#include "wintty.h"
//#include "winnet.h"
#include <io.h>
// global message number and window handle that DLL owner assigned by obsoleted API DEV_EnableEventMessage
// use DEV_EnableEventMessageEx instead
HWND	m_hWnd = NULL;
UINT		m_nMsg = 0;
#define TTY_GETS(fd, bs, sz, eoc)		tty_gets(fd, bs, sz, eoc)
#else 
#include "utils_mtrace.h" 
#include <error.h>
#include <errno.h>
#include <unistd.h>
#include "../utils/utils_net.h"
#include "../utils/utils_tty.h"
#define WSAGetLastError()			(errno)
#define winsock_startup()				(0)
#define winsock_cleanup()				(0)
#define Sleep(n)							usleep( (n)*1000 )
#define TTY_GETS(fd, bs, sz, eoc)		tty_gets(fd, bs, sz, 100, eoc, NULL)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>
#include "ALBLib.h"
#include "ALBDLL.h"

////////////////////////////// ALB library //////////////////////////////////

#define STR_VER_LIB		"1.0.0.9"
//#define ENABLE_LOG


/* 无论控制一台还是多台，默认使用多栏杆机控制动态库 */
/* ENABLE_CONTROL_ONE 此宏是为了兼容之前的单栏杆机动态库 */
//#define ENABLE_CONTROL_ONE // 单栏杆机控制动态库

// 单位：秒
#define HEARTBEAT_PERIOD	1
#define HEATBEAT_OVERTIME	5

#ifdef linux
static void *WorkThreadProc(void *lpParameter);
#define THREAD_RET	NULL
#else
static DWORD WINAPI WorkThreadProc(LPVOID lpParameter);
#define THREAD_RET		0
#endif

typedef struct {
#if defined _WIN32 || defined _WIN64
	HWND	hWnd;
	UINT		Msg;
#else
	int		nothing;			// Linux do not need this 
#endif
} UserData;
// ================================ Trace log end ================================

static BOOL _ALBCtrl( ALBHANDLE hALB, BOOL bOpen )
{
	char CMD[32] = "alb ";
	int len;
	int rc;
	int i;
	
	strcpy(CMD, bOpen ? "alb open\n" : "alb close\n" );
	len = (int)strlen( CMD );

	hALB->m_bReceiveOK = FALSE;
	switch( hALB->m_nInterface )
	{
	case INTERFACE_NET:
		if( hALB->m_nSocket == INVALID_SOCKET )
			return FALSE;

		rc = sock_write( hALB->m_nSocket, CMD, len );
		break;
		
	case INTERFACE_UART:
		if( hALB->m_fdCom == -1 )
			return FALSE;

		rc = tty_write( hALB->m_fdCom, CMD, len );
		break;
		
	default:
		return FALSE;
	}

	MTRACE_LOG(hALB->hLog, "[Note] Send: %s", CMD);

	for (i = 0; i < 10 && !hALB->m_bReceiveOK; i++)
	{
		Sleep(10);
	}

	return ((0 < rc) && (rc <= len) && hALB->m_bReceiveOK);
}

static void SendHeartBeat( ALBHANDLE hALB )
{
	char CMD[] = "heart beat\n";
	int len;
	int rc;

	len = (int)strlen( CMD );

	switch( hALB->m_nInterface )
	{
	case INTERFACE_NET:
		if( hALB->m_nSocket == INVALID_SOCKET )
			return;

		rc = sock_write( hALB->m_nSocket, CMD, len );
		break;
		
	case INTERFACE_UART:
		if( hALB->m_fdCom == -1 )
			return;

		rc = tty_write( hALB->m_fdCom, CMD, len );
		break;
		
	default:
		return;
	}
}

static void ProcessHeartBeatOverTime( ALBHANDLE hALB )
{
	MTRACE_LOG(hALB->hLog, "[Note] ALB Controler heart beat over time\n");

	switch( hALB->m_nInterface )
	{
	case INTERFACE_NET:
 
		if (hALB->m_nSocket == INVALID_SOCKET)   
			return;
		sock_close( hALB->m_nSocket );
		hALB->m_nSocket = INVALID_SOCKET;
		hALB->m_nALBStateThis.m_bOnline = FALSE;
		if( hALB->m_nCb != NULL ) 
		{ 
			MTRACE_LOG(hALB->hLog, "[Note] 上报离线事件\n"); 
			hALB->m_nCb( hALB, EVID_OFFLINE, 1 );  
		}
		hALB->m_nALBStateLast.m_bOnline = FALSE;
		break;
		
	case INTERFACE_UART:
		if( hALB->m_fdCom == -1 )
			return;
		break;
		
	default:
		break;
	}
}

static void ProcessDeviceRefuseConnect( ALBHANDLE hALB )
{
	switch( hALB->m_nInterface )
	{
	case INTERFACE_NET:
		hALB->m_nAccept = 2;
		sock_close( hALB->m_nSocket );
		hALB->m_nSocket = INVALID_SOCKET;
		hALB->m_nQuit = 1;
		if( hALB->m_nCb != NULL )
		{
			//MTRACE_LOG(hALB->hLog, "[Note] Call device event callback function!\n" );
			MTRACE_LOG(hALB->hLog, "[Note] 上报拒绝连接事件\n");

			hALB->m_nCb( hALB, EVID_REFUSE, 1 );

			//MTRACE_LOG(hALB->hLog, "[Note] Back from device event callback function!\n" );
		}
		break;
		
	case INTERFACE_UART:
		break;
		
	default:
		break;
	}
}

static void ProcessDeviceAcceptConnect( ALBHANDLE hALB )
{
	switch( hALB->m_nInterface )
	{
	case INTERFACE_NET:
		hALB->m_nAccept = 1;
		if( hALB->m_nCb != NULL )
		{
			MTRACE_LOG(hALB->hLog, "[Note] 上报接受连接事件\n");

			hALB->m_nCb( hALB, EVID_ACCEPT, 1 );
		}
		break;
		
	case INTERFACE_UART:
		break;
		
	default:
		break;
	}
}

ALBHANDLE libALBOpen( const char *device, ALBCallBack pFunc, const char *logName )
{
	ALBHANDLE hALB;
	DWORD nIP;
	
	hALB = (ALBHANDLE)malloc( sizeof(ALBObject) );
	
	if( hALB == NULL )
	{
		return NULL;
	}
	
	memset( hALB, 0, sizeof(ALBObject) );

	hALB->m_nALBMagicWord = MAGIC_WORD_ALB;

#ifdef ENABLE_LOG
	hALB->hLog = MLOG_INIT( NULL, logName == NULL ? "ALBControllerLib" : logName );
#else
	hALB->hLog = NULL;
#endif
	
	MTRACE_LOG(hALB->hLog, "[Call] libALBOpen() device = %s \n", device == NULL ? "NULL" : device);
	MTRACE_LOG(hALB->hLog, "\tALB library Ver:%s\n", STR_VER_LIB);
	
	if( device == NULL )
	{
		MLOG_CLOSE( hALB->hLog );
		free( hALB );
		return NULL;
	}
	
	if( (nIP = inet_addr( device ) ) != INVALID_SOCKET )
	{
		int nEc;

		if( (nEc = winsock_startup()) != 0 )
		{
			MTRACE_LOG(hALB->hLog, "\t[Err] WSAStartup fail, error code = %d\n", nEc );
			goto Error;
		}
		hALB->m_nInterface = INTERFACE_NET;
		hALB->m_nIP = nIP;
 
		hALB->m_nSocket = sock_connect_tou( hALB->m_nIP, ALB_CONTROLLER_TCP_PORT ); 

		if( hALB->m_nSocket == INVALID_SOCKET )
		{
			MTRACE_LOG(hALB->hLog, "\t[Error] TCP connection failed WSAError = %d\n", WSAGetLastError() );
			winsock_cleanup();
			goto Error;
		}

		SendHeartBeat( hALB );
		hALB->m_tHeartBeatTime = time( NULL ) + HEARTBEAT_PERIOD;
		hALB->m_tHeartBeatOverTime = time( NULL ) + HEATBEAT_OVERTIME;
	}
#ifdef linux
	else if ( strncmp(device, "/dev/",  5)==0 )
	{
		hALB->m_fdCom = open(device, O_RDWR);
		if ( hALB->m_fdCom==-1 )
		{
			MTRACE_LOG(hALB->hLog, "tty device (%s) open failed - errno=%d\n", device, errno);
			goto Error;
		}
		hALB->m_nInterface = INTERFACE_UART;
		// set tty as raw mode 
		tty_raw(hALB->m_fdCom, &hALB->termio_save, 9600);
		hALB->m_nInterface = INTERFACE_UART;
}
#else
	else if ( strncmp(device, "COM", 3) == 0 )
	{
		int nComNum;

		nComNum = atoi( device+3 );
		
		if( nComNum <= 0 )
		{
			MTRACE_LOG(hALB->hLog, "\t[Err] Invalid COM port number - %s!\n", device );
			goto Error;
		}
	
		hALB->m_nInterface = INTERFACE_UART;
		hALB->m_nComNum = nComNum;
		hALB->m_fdCom = tty_openPort( nComNum );
		
		if( hALB->m_fdCom == -1 )
		{
			MTRACE_LOG(hALB->hLog, "\t[Err] COM port open failure!\n" );
			goto Error;
		}
		
		tty_raw( hALB->m_fdCom, 9600, 8, PRTY_NONE );
		tty_setReadTimeouts( hALB->m_fdCom, 20, 1, 100 );
	}
#endif
	
	hALB->m_nQuit = 0;
	hALB->m_nCb = pFunc;
#ifdef linux
		if ( pthread_create( &hALB->m_hWorkThread, NULL, WorkThreadProc, (void *)hALB) == -1 )
			goto Error;
#else
	hALB->m_hWorkThread = CreateThread(
									NULL,           // default security attributes
									0,              // use default stack size
			(LPTHREAD_START_ROUTINE)WorkThreadProc, // thread function
									(LPVOID)hALB,	// argument to thread function
									0,              // use default creation flags
									&hALB->m_nThreadId);	// returns the thread identifier
	if( hALB->m_hWorkThread == NULL )
	{
		MTRACE_LOG(hALB->hLog, "\t[Err]  Failed to create ALB DLL working thread: Error=%d\n", GetLastError() );
		goto Error;
	}
#endif

	// for TCP connection, we have to wait for controller granted the connection as controller does not support multiple connection
	if( hALB->m_nInterface == INTERFACE_NET )
	{
		time_t tOvertime;

		tOvertime = time( NULL ) + 5;

		//MTRACE_LOG(hALB->hLog, "Accept = %d\n", hALB->m_nAccept );
		while( hALB->m_nAccept == 0 && time( NULL ) < tOvertime  )
		{
			Sleep( 100 );
		}

		if( hALB->m_nAccept == 2 || hALB->m_nAccept == 0 )
		{
			MTRACE_LOG(hALB->hLog, "%s\n", hALB->m_nAccept == 2 ? "[Warning] Controller refuse the connection request!" : "[Warning] controller response timed-out!" );
			libALBClose( hALB );
			return NULL;
		}
	}

	return hALB;
	
Error:
	MLOG_CLOSE(hALB->hLog );
	free( hALB );
	return NULL;
}

BOOL libALBClose( ALBHANDLE hALB )
{
	int i;
	
	if( !IsValidHandle( hALB ) )
		return FALSE;
	
	MTRACE_LOG(hALB->hLog, "[Call] libALBClose() hALB = %p\n", hALB );

	if( hALB->m_nQuit == 0 )
		hALB->m_nQuit = 1;

	MTRACE_LOG(hALB->hLog, "[Note] Wait work thread quit!\n" );
	for( i = 0; i < 10; i ++ )
	{
		if( hALB->m_nQuit == 2 )
		{
			MTRACE_LOG(hALB->hLog, "[Note] Work thread quit!\n" );
			break;
		}
		Sleep( 100 );
	}

	if( hALB->m_nQuit != 2 )
	{
		MTRACE_LOG(hALB->hLog, "\t[Note] Work thread do not quit, Terminate it!\n" );
#ifdef linux
		pthread_cancel(  hALB->m_hWorkThread );
		pthread_join( hALB->m_hWorkThread, NULL);
#else
		TerminateThread( hALB->m_hWorkThread, 0 );
		hALB->m_hWorkThread = NULL;
#endif
	}


	if( hALB->m_nInterface == INTERFACE_NET )
	{
		if( hALB->m_nSocket != INVALID_SOCKET )
		{
			MTRACE_LOG(hALB->hLog, "\t[Note] Close sock!\n" );
			sock_close( hALB->m_nSocket );
		}

		MTRACE_LOG(hALB->hLog, "\t[Note] Winsock clear!\n" );
		winsock_cleanup();
	}
	else
	{
		MTRACE_LOG(hALB->hLog, "\t[Note] Close COM%d!\n", hALB->m_nComNum );
#ifdef linux
		// restore tty setting
		tty_set(hALB->m_fdCom, &hALB->termio_save);
		close(hALB->m_fdCom);
#else
		tty_close( hALB->m_fdCom );
#endif
	}
	
	MLOG_CLOSE(hALB->hLog);
/* shall be released by upper level (DLL wrap) code
	pUserData = (UserData *)(hALB->pUserData);
	if (pUserData != NULL)
	{
		free(pUserData);
		hALB->pUserData = NULL;
	}
*/
	free( hALB );
	
	return TRUE;
}

BOOL libALBCtrl( ALBHANDLE hALB, BOOL bOpen )
{
	BOOL bRC;
	int i;

	if( !IsValidHandle( hALB ) )
		return FALSE;
	
	MTRACE_LOG(hALB->hLog, "[Call] libALBCtrl() hALB = %p, bOpen = %s\n", hALB, bOpen ? "Open" : "Close" );
	
	for (i = 0; i < 3; i++)
	{
		bRC = _ALBCtrl(hALB, bOpen);
		if (bRC)
			break;
	}

	MTRACE_LOG(hALB->hLog, "[Note] Send command %s!\n", bRC ? "OK" : "Fail" );

	return bRC;
}

const char *GetBalustradeText( int nState)
{
	const char *ptr;

	switch (nState)
	{
	case 1:
		ptr = "栏杆正在下落";
		break;

	case 2:
		ptr = "栏杆下落到水平位置";
		break;

	case 3:
		ptr = "栏杆正在抬起";
		break;

	case 4:
		ptr = "栏杆抬起到竖直位置";
		break;

	default:
		ptr = "未知状态";
		break;
	}

	return ptr;
}

static void ProcessALBMsg( ALBHANDLE hALB, const char *msg )
{
	#define LEN_MESSAGE	6

	const char *Message[LEN_MESSAGE] = {
		"ok",
		"fail",
		"status ",
		"heart beat",
		"accept connect",
		"refuse connect",
	};
	int i;
	const char *ptr;
	int len;
	int nState;

	len = (int)strlen(msg);

	for( i = 0; i < LEN_MESSAGE; i++ )
	{
		ptr = strstr( msg, Message[i] );

		if( ptr == NULL )
		{
			continue;
		}
		
		ptr += strlen(Message[i]);

		hALB->m_tHeartBeatOverTime = time( NULL ) + HEATBEAT_OVERTIME; // 任何信息帧过来都更新心跳包超时时间

		switch( i )
		{
			/* operate result */
			case 0:
				hALB->m_bReceiveOK = TRUE;
			case 1:
				MTRACE_LOG(hALB->hLog, "[Read] %s", msg);
				break;

			/* status change */
			case 2:
				if (hALB->m_nCb != NULL && !hALB->m_nALBStateThis.m_bOnline)
				{
					hALB->m_nALBStateThis.m_bOnline = TRUE;
					MTRACE_LOG(hALB->hLog, "[Note] 上报连线事件\n");
					hALB->m_nCb(hALB, EVID_ONLINE, 1);
					hALB->m_nALBStateLast.m_bOnline = TRUE;
				}
				//MTRACE_LOG(hALB->hLog, "[Read] %s", msg );
				hALB->m_nALBStateThis.m_bBCoil = *ptr == '1' ? VEHICLE_LEAVE : VEHICLE_ARRIVE; // 栏杆机线圈(后线圈)车状态

				ptr += 2;
				nState = *ptr - '0';
				// 滤除正在抬杆和正在落杆的状态，因为这2个状态是猜测的，不一定准确，而且容易误导人（例如下了抬杆指令，但是栏杆没有抬，此时控制器会上报正在抬杆）
				if (nState != 1 && nState != 3)  
					hALB->m_nALBStateThis.m_nBalustrade = nState; // 栏杆状态

				ptr += 2;
				hALB->m_nALBStateThis.m_bFCoil = *ptr == '1' ? VEHICLE_LEAVE : VEHICLE_ARRIVE; // 抓拍线圈(前线圈)车状态

				if( hALB->m_nALBStateLast.m_bBCoil != hALB->m_nALBStateThis.m_bBCoil )
				{
					MTRACE_LOG(hALB->hLog, "[Note] 车辆-%s-后线圈\n", hALB->m_nALBStateThis.m_bBCoil ? "到达" : "离开" );
					if( hALB->m_nCb != NULL )
					{
						//MTRACE_LOG(hALB->hLog, "[Note] Call device event callback function!\n" );

						hALB->m_nCb( hALB, EVID_BCOIL, hALB->m_nALBStateThis.m_bBCoil );

						//MTRACE_LOG(hALB->hLog, "[Note] Back from device event callback function!\n" );
					}
					hALB->m_nALBStateLast.m_bBCoil = hALB->m_nALBStateThis.m_bBCoil;
				}

				if( hALB->m_nALBStateLast.m_nBalustrade != hALB->m_nALBStateThis.m_nBalustrade )
				{
					MTRACE_LOG(hALB->hLog, "[Note] 栏杆状态变化，当前状态为: %s\n", GetBalustradeText(hALB->m_nALBStateThis.m_nBalustrade));
					if( hALB->m_nCb != NULL )
					{
						//MTRACE_LOG(hALB->hLog, "[Note] Call device event callback function!\n" );

						hALB->m_nCb( hALB, EVID_BALUSTRADE, hALB->m_nALBStateThis.m_nBalustrade );

						//MTRACE_LOG(hALB->hLog, "[Note] Back from device event callback function!\n" );
					}
					hALB->m_nALBStateLast.m_nBalustrade = hALB->m_nALBStateThis.m_nBalustrade;
				}

				if( hALB->m_nALBStateLast.m_bFCoil != hALB->m_nALBStateThis.m_bFCoil )
				{
					MTRACE_LOG(hALB->hLog, "[Note] 车辆-%s-前线圈\n", hALB->m_nALBStateThis.m_bFCoil ? "到达" : "离开");
					if( hALB->m_nCb != NULL )
					{
						//MTRACE_LOG(hALB->hLog, "[Note] Call device event callback function!\n" );

						hALB->m_nCb( hALB, EVID_FCOIL, hALB->m_nALBStateThis.m_bFCoil );

						//MTRACE_LOG(hALB->hLog, "[Note] Back from device event callback function!\n" );
					}
					hALB->m_nALBStateLast.m_bFCoil = hALB->m_nALBStateThis.m_bFCoil;
				}
				break;

			case 3: // heart beat
				//hALB->m_tHeartBeatOverTime = time( NULL ) + HEATBEAT_OVERTIME;
				//MTRACE_LOG(hALB->hLog, "[Note] Receive ALB Controler heart beat!\n" );
				if (hALB->m_nCb != NULL && !hALB->m_nALBStateThis.m_bOnline)
				{
					hALB->m_nALBStateThis.m_bOnline = TRUE;
					MTRACE_LOG(hALB->hLog, "[Note] 上报连线事件\n");
					hALB->m_nCb(hALB, EVID_ONLINE, 1);
					hALB->m_nALBStateLast.m_bOnline = TRUE;
				}
				break;

			case 4: // connection accepted
				MTRACE_LOG(hALB->hLog, "[Note] ALB Controler accept connect!\n" );
				ProcessDeviceAcceptConnect( hALB );
				break;

			case 5: // connection denied
				MTRACE_LOG(hALB->hLog, "[Note] ALB Controler refuse connect!\n" );
				ProcessDeviceRefuseConnect( hALB );
				break;

			default:
				MTRACE_LOG(hALB->hLog, "[Error] Unknown message: %s!\n", msg );
				break;
		}
	}
}

#define END_OF_CHAR		'\n'
#ifdef linux
static void * WorkThreadProc(void *lpParameter)
#else
static DWORD WINAPI WorkThreadProc(LPVOID lpParameter)
#endif
{
	ALBHANDLE hALB;
	char buf[64];
	int i = 0;
	time_t nTime;
	
	hALB = (ALBHANDLE)lpParameter;
	assert(hALB!=NULL);

	MTRACE_LOG(hALB->hLog,"================ Work Thread Start! ================\n" );
	
	for( ;!hALB->m_nQuit; )
	{
		///////////////////////////////////////////////////////////////////
		if( (hALB->m_nInterface == INTERFACE_NET) && (hALB->m_nSocket == INVALID_SOCKET) )
		{
			i++;
			MTRACE_LOG(hALB->hLog, "[Note] Connect to ALB controller - retry count=%d!\n", i );
			hALB->m_nSocket = sock_connect0( hALB->m_nIP, ALB_CONTROLLER_TCP_PORT );
			if( hALB->m_nSocket == INVALID_SOCKET )
			{
				Sleep( 5000 );
				continue;
			} 
			MTRACE_LOG(hALB->hLog, "[Note] TCP connection to ALB controller established!\n", i );  
			hALB->m_tHeartBeatTime = time( NULL ) + HEARTBEAT_PERIOD;
			SendHeartBeat( hALB );
			hALB->m_tHeartBeatOverTime = time( NULL ) + HEATBEAT_OVERTIME; //  update next heart-beat time
		}
		///////////////////////////////////////////////////////////////////

		if( hALB->m_nInterface == INTERFACE_NET )
		{
			fd_set	rfd_set;
			struct	timeval tv;
			int nsel;
			int err;

			FD_ZERO( &rfd_set );
			FD_SET( hALB->m_nSocket, &rfd_set );

			tv.tv_sec = 0;
			tv.tv_usec = 100000;		// 100 msec
			
			nsel = select( 0, &rfd_set, NULL, NULL, &tv );
			if( nsel == SOCKET_ERROR && !(hALB->m_nQuit) )
			{
				MTRACE_LOG(hALB->hLog,"[WINSOCK ERROR] select error code = %d\n", WSAGetLastError() );
				sock_close( hALB->m_nSocket );
				hALB->m_nSocket = INVALID_SOCKET;
				i = 0;
				Sleep( 5000 );
				continue;
			}

			// read tcp data and process it
			if( nsel > 0 && FD_ISSET( hALB->m_nSocket, &rfd_set ) )
			{ 
				err = sock_read_n_bytes_tout( hALB->m_nSocket, buf, sizeof(buf), 50  ); 
				if( err > 0 )
				{
					ProcessALBMsg( hALB, buf );
				}
				else if( err <=0 && !(hALB->m_nQuit) )
				{
					MTRACE_LOG(hALB->hLog,"TCP broken, reconnect controller...\n" );
					sock_close( hALB->m_nSocket );
					hALB->m_nSocket = INVALID_SOCKET;
					i = 0;
					continue;
				}
			}
		}
		else		// COM port connection with ALB controller
		{
			if( tty_ready( hALB->m_fdCom, 0 ) )
			{
				int len = TTY_GETS(hALB->m_fdCom, buf, sizeof(buf), END_OF_CHAR);
				if ( len > 0 && buf[len-1]==END_OF_CHAR )
				{
					ProcessALBMsg( hALB, buf );
				}
			}
		}

		nTime = time( NULL );

		if( hALB->m_nInterface == INTERFACE_NET && nTime > hALB->m_tHeartBeatOverTime )
		{
			ProcessHeartBeatOverTime( hALB );
			i = 0;
		}

		if( hALB->m_nInterface == INTERFACE_NET && nTime >= hALB->m_tHeartBeatTime )
		{
			hALB->m_tHeartBeatTime = nTime + HEARTBEAT_PERIOD;
			SendHeartBeat( hALB );
		}   
		Sleep( 100 );
	} 
	if (hALB->m_nInterface == INTERFACE_NET && hALB->m_nSocket > 0)
	{
		sock_close(hALB->m_nSocket);
		hALB->m_nSocket = INVALID_SOCKET;
	} 
	if( hALB->m_nQuit == 1 )
	{
		hALB->m_nQuit = 2;
		MTRACE_LOG(hALB->hLog,">>>>>>>>>>>>>>>> Work Thread End! <<<<<<<<<<<<<<<<\n" );
	}
	else
	{
		MTRACE_LOG(hALB->hLog,"[Error]  Working thread terminated abnormally!\n" );
	}

	return THREAD_RET;
}
////////////////////////////////////////////////////////////////

////////////////////////////// ALB DLL //////////////////////////////////
// callback from under layer ALB working thread.
static void my_callback( void *h, int nEventId, int nParam )
{
	ALBHANDLE hALB = (ALBHANDLE)h;
	UserData *pUserData;
	
	pUserData = (UserData *)(hALB->pUserData);

#if defined _WIN32 || defined _WIN64
	if( pUserData != NULL && pUserData->hWnd != NULL )
	{
		PostMessage( pUserData->hWnd, pUserData->Msg, (WPARAM)h, (nEventId << 8) | nParam );
		MTRACE_LOG(hALB->hLog, "[Note] PostMessage( m_hWnd = %p, m_nMsg = %d, h = %p, nParam = %d )\n", pUserData->hWnd, pUserData->Msg, h, (nEventId << 8) | nParam );
	}
#endif
	/*	 Window handle and message number assigned by obsoleted APi DEV_EnableEventMessage are no longer supported. 
	if( m_hWnd != NULL )
	{
		PostMessage( m_hWnd, m_nMsg, (WPARAM)h, (nEventId << 8) | nParam );
		MTRACE_LOG(hALB->hLog, "[Note] PostMessage( m_hWnd = %p, m_nMsg = %d, wParam = %p, lParam = %d )\n", m_hWnd, m_nMsg, h, (nEventId << 8) | nParam );
	}
	*/

}


_API_EXPORT HANDLE CALLTYPE DEV_Open( const char *str )
{
	ALBHANDLE hALB;
	UserData *pUserData;
	char logbasename[64] = "ALBDLL-";

	if( str == NULL )
		return NULL;

	strcat(logbasename, str); //  strncat(logbasename, str, sizeof(logbasename)-1);
	hALB = (HANDLE)libALBOpen( str, my_callback, logbasename );
	if( hALB == NULL )
		return NULL;

	pUserData = (UserData *)malloc( sizeof(UserData) );
	if( pUserData == NULL )
	{
		MTRACE_LOG(hALB->hLog, "\t【SYSTEM ERROR】 failed to allocate user data!\n" );
		libALBClose( hALB );
		return NULL;
	}

	memset( pUserData, 0, sizeof(UserData) );

	hALB->pUserData = pUserData;

	return (HANDLE)hALB;
}

_API_EXPORT BOOL CALLTYPE DEV_Close( HANDLE h )
{
	ALBHANDLE hALB = (ALBHANDLE)h;
	UserData *pUserData;

	if (!IsValidHandle(hALB))
		return FALSE;

	pUserData = (UserData *)(hALB->pUserData);
	if (pUserData != NULL)
	{
		free(pUserData);
		hALB->pUserData = NULL;
	}

	return libALBClose( hALB );
}

_API_EXPORT BOOL CALLTYPE DEV_ALB_Ctrl( HANDLE h, BOOL bOpen )
{
	ALBHANDLE hALB = (ALBHANDLE)h;

	return libALBCtrl( hALB, bOpen );
}


#if defined _WIN32 || defined _WIN64
// obsoleted API, don't use. No effect
_API_EXPORT void CALLTYPE DEV_EnableEventMessage( HWND hWnd, UINT MsgID )
{
	m_hWnd = hWnd;
	m_nMsg = MsgID;
}

_API_EXPORT BOOL CALLTYPE DEV_EnableEventMessageEx( HANDLE h, HWND hWnd, UINT MsgID )
{
	ALBHANDLE hALB = (ALBHANDLE)h;
	UserData  *pUserData;

	if( !IsValidHandle( hALB ) )
		return FALSE;

	MTRACE_LOG(hALB->hLog, "【DEV_EnableEventMessageEx】 - h = %p, hWnd = %p, MsgID = %ld\n", h, hWnd, MsgID );

	pUserData = (UserData  *)(hALB->pUserData);
	assert(pUserData!=NULL);

	pUserData->hWnd = hWnd;
	pUserData->Msg  = MsgID;

	return TRUE;
}
#endif 
//#else		// linux
_API_EXPORT BOOL CALLTYPE DEV_SetEventHandle( HANDLE h, DEVEventCallBack pCallBack )
{
	ALBHANDLE hALB = (ALBHANDLE)h;

	if( !IsValidHandle( hALB ) )
		return FALSE;

	MTRACE_LOG(hALB->hLog, "【DEV_SetEventHandle】- h = %p, pCallBack = %p\n", h, pCallBack );

	hALB->m_nCb = (ALBCallBack)pCallBack;

	return TRUE;
}
//#endif

_API_EXPORT BOOL CALLTYPE DEV_GetStatus( HANDLE h, DWORD *dwStatus )
{
	ALBHANDLE hALB = (ALBHANDLE)h;

	if( !IsValidHandle( hALB ) )
		return FALSE;

	MTRACE_LOG(hALB->hLog, "【DEV_GetStatus】 -  h = %p, dwStatus = %p \n", h, dwStatus );

	//if( !IsDeviceOnline( hALB ) )
	//	return FALSE;

	if( dwStatus == NULL )
		return TRUE;

	*dwStatus = (hALB->m_nALBStateThis.m_nBalustrade ) & 0x0F;

	// 去掉落杆中和抬杆中这2种状态，因为这2种状态是猜测的，不准确容易引起误导
	if (*dwStatus == 1 || *dwStatus == 3)
		*dwStatus = 0;

	// font loop status
	if( hALB->m_nALBStateThis.m_bFCoil )
	{
		*dwStatus |= 0x01 << 4;
	}

	// rear loop status
	if( hALB->m_nALBStateThis.m_bBCoil )
	{
		*dwStatus |= 0x01 << 5;
	}

	// network controller online status
	if( hALB->m_nALBStateThis.m_bOnline )
	{
		*dwStatus |= 0x01 << 6;
	}

	MTRACE_LOG(hALB->hLog, "\t[Note] DEV_GetStatus 返回: 0x%04X\n", *dwStatus);

	return TRUE;
}

//////////////////////////////////////////////////////////////// 

#ifdef _DEBUG
 
static void onAlbEvent(HANDLE h,int nEventId, int *param )
{
	printf("device event:%d \r\n", nEventId ); 
}

int main(int argc, char *argv[])
{
	char input[32];
	HANDLE alb = NULL;
	time_t tick;
	while (1)
	{
		fgets(input, sizeof(input), stdin);
		switch (input[0])
		{
		case 'a':
			tick = time(NULL);
			printf("开始连接控制！\r\n");
			if ( alb != NULL )
			{
				DEV_Close(alb);
			}
			alb = DEV_Open("192.168.1.190"); 
			printf("结束连接，时长：%d \r\n", time(NULL)- tick);
			if (alb)
			{
				DEV_SetEventHandle(alb, onAlbEvent);
			}
			break;
		case 'b':
			DEV_ALB_Ctrl(alb, 1);
			break;
		case 'c':
			DEV_ALB_Ctrl(alb, 0);
			break;
		default:
			break;
		}
	}
} 
#endif 
