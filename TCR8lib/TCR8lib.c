/*
 * TCR8lib.c
 *
 * Implement the TCR8 Auto Card Dispensor machine protocol. It will be the core for various DLL or ActiveX to comply the interface
 * requested by different proviences. It also server the core for high level class used by AP to control the machine.
 *
 * Author: Thomas Chang
 * Date: 2013-04-20
 *
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>

#include "TCR8lib.h"

#ifdef linux
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#define _tzset()
#define winsock_startup()
#define winsock_cleanup()
#define WSAGetLastError() strerror(errno)
#define GetLastError() strerror(errno)
#define Sleep(n) usleep(n * 1000)
#else
#pragma comment(lib, "ws2_32.lib");
#pragma comment(lib, "Gdi32.lib");
#endif

// 使能数据重发三次抛弃功能
// #define ENABLE_DROP_DATA_OVER_THREE_TIMES

//
#define ENABLE_ENGLISH 0

#ifdef linux
#define Mutex_LogLock(h) pthread_mutex_lock(&h->m_hLogMutex)
#define Mutex_LogUnlock(h) pthread_mutex_unlock(&h->m_hLogMutex)
#else
#define Mutex_LogLock(h) WaitForSingleObject(h->m_hLogMutex, INFINITE)
#define Mutex_LogUnlock(h) ReleaseMutex(h->m_hLogMutex)
#endif

#define TRACE_LOG TCR8_Log
#define MAX_LOGSIZE 8388608L
#define MAX_ROLLCOUNT 10

#ifdef _WIN32
#pragma warning(disable : 4996)
LANGID m_langId;
#define IsChinese() ((m_langId & 0x3ff) == LANG_CHINESE)
#else
#undef MAX_PATH
#define MAX_PATH 256
#define IsChinese() (1)
#endif

#define VERSION_TCR8LIBRATY "V1.1.8"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void log_rollforward(LPCTSTR strPath)
{
	char cLog1[MAX_PATH], cLog2[MAX_PATH];
	int i;
	char strFormat[MAX_PATH], *ptr;

	strcpy(strFormat, strPath);
	ptr = strstr(strFormat, ".log");
	strcpy(ptr, "%d.log");

	sprintf(cLog2, strFormat, MAX_ROLLCOUNT);
	unlink(cLog2); // unlink last one
	for (i = MAX_ROLLCOUNT - 1; i > 0; i--)
	{
		sprintf(cLog1, strFormat, i);
		rename(cLog1, cLog2);
		strcpy(cLog2, cLog1);
	}
	rename(strPath, cLog1);
}

int64_t GetSystemTime64() // get system time (UTC) in msec
{
#ifdef linux
	static int64_t begin_time = 0;
	int64_t now_time;
	struct timespec tp;
	UINT64 tmsec = 0;
	if (clock_gettime(CLOCK_MONOTONIC, &tp) != -1)
	{
		now_time = tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
	}
	if (begin_time == 0)
		begin_time = now_time;
	tmsec = (UINT64)(now_time - begin_time);
	return tmsec;
#else
	int64_t ft;
	GetSystemTimeAsFileTime((FILETIME *)&ft);
	return ft /= 10000; //  divided by 10000000 and multiply by 1000
#endif
}

static int GetTime2Resend(TCR8HANDLE h);
static int SendPacket(TCR8HANDLE h, const char *i_packet);
static int SendAck(TCR8HANDLE h, char cseq);
static int SendNak(TCR8HANDLE h, char cseq);
static void AckedPacket(TCR8HANDLE h, int nSeq);
static void NakedPacket(TCR8HANDLE h, int nSeq);
static BOOL IsValidPacket(TCR8HANDLE h, char *packet);
static BOOL IsEmptyQueue(TCR8HANDLE h);
static int SendInitialPacket(TCR8HANDLE h);
static int SendTimeSyncPacket(TCR8HANDLE h);
static int GetTime2Resend(TCR8HANDLE h);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef linux
static void ProtocolThread(void *lpParameter);
#else
static DWORD WINAPI ProtocolThread(LPVOID lpParameter);
#endif
DLLAPI TCR8HANDLE CALLTYPE TCR8_Create()
{
	int i;
	TCR8HANDLE h = malloc(sizeof(TCR8Obj));
	FILE *fp = NULL;
	if (h == NULL)
		return NULL;
	memset(h, 0, sizeof(TCR8Obj));
	h->m_iProtocol = -1;
	h->m_dwIP = INADDR_NONE;
	h->m_sockTCP = INVALID_SOCKET;
	h->m_tty = -1;
	h->m_cSeq = '9';
	h->m_dwMagicWord = TCR8_MAGICWORD;
	h->m_nExtendProtocol = 1;
	for (i = 0; i < 4; i++)
	{
		h->m_iState[i] = CSB_NOBOX;
		h->m_iStateLast[i] = CSB_NOBOX;
	}
#ifdef _WIN32
	m_langId = GetUserDefaultLangID();
#endif
	h->m_dwEventMask = T8EVM_ALL;

	return h;
}

DLLAPI void CALLTYPE TCR8_Destroy(TCR8HANDLE h)
{
	if (!_IsValidHandle(h))
		return;
	TCR8_CloseDevice(h); // incase user does not close it first
	if (h->m_strPath)
		free(h->m_strPath);
	if (h->fp)
		fclose(h->fp);
	if (h->m_pUsrData)
		h->m_pUsrData = NULL;
	if (h)
		free(h);
}

DLLAPI BOOL CALLTYPE TCR8_SetComPortString(TCR8HANDLE h, const char *dev_name, int nBaudrate)
{
	int nPort = 0;
	if (!_IsValidHandle(h))
		return FALSE;
	if (!_IsOpen(h))
	{
#ifdef linux
		strcpy(h->m_stPort, dev_name);
#else
		sscanf(dev_name, "COM%d", &nPort);
		h->m_nPort = nPort;
#endif
		h->m_nBaudrate = nBaudrate;
		return TRUE;
	}
	return FALSE;
}

#ifdef linux
DLLAPI BOOL CALLTYPE TCR8_SetComPort(TCR8HANDLE h, const char *dev_name, int nBaudrate)
{
	return TCR8_SetComPortString(h, dev_name, nBaudrate);
}
#else
DLLAPI BOOL CALLTYPE TCR8_SetComPort(TCR8HANDLE h, int port, int nBaudrate)
{
	char buffer[32] = { 0 };
	sprintf(buffer, "COM%d", port);
	return TCR8_SetComPortString(h, buffer, nBaudrate);
}
#endif
 
DLLAPI BOOL CALLTYPE TCR8_SetCallback(TCR8HANDLE h, void (*cb)(void *, int, int))
{
	if (!_IsValidHandle(h))
		return FALSE;

	h->m_cbFxc = cb;
	return TRUE;
}

static int TCR8SendData(TCR8HANDLE h, char *data, int len)
{
	if (_IsConnectWithCom(h))
		tty_write(h->m_tty, data, len);
	else
		sock_write(h->m_sockTCP, data, len);
}

DLLAPI BOOL CALLTYPE TCR8_OpenDevice(TCR8HANDLE h)
{
	DWORD dwThreadId;
	int port = 0;
	if (!_IsValidHandle(h))
		return FALSE;
	TRACE_LOG(h, "[Note] TCR8Lib Version: %s\n", VERSION_TCR8LIBRATY);
	if (_IsOpen(h))
	{
		if (_IsConnectWithCom(h))
		{
			TRACE_LOG(h, "\t[Error] The COM%d has been opened by TCR8 machine!\n", h->m_nPort);
		}
		else
		{
			struct in_addr address;
			address.s_addr = h->m_dwIP;
			TRACE_LOG(h, "\t[Error] The TCR8 machine has been connected on %s!\n", inet_ntoa(address));
		}
		return FALSE;
	}
#ifdef linux
	h->m_tty = open(h->m_stPort, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
	if (h->m_tty == -1)
	{
		TRACE_LOG(h, "\t[Error] The COM%s is opened fail!\n", h->m_stPort);
		return FALSE;
	}
	if (h->m_nBaudrate != 9600 && h->m_nBaudrate != 19200)
		h->m_nBaudrate = 9600;
	tty_raw(h->m_tty, NULL, h->m_nBaudrate);
	pthread_create(&h->m_hThread, NULL, ProtocolThread, h);
	pthread_mutex_init(&h->m_hMutex, NULL);
	pthread_mutex_init(&h->m_hLogMutex, NULL);
	return TRUE;
#else
	sscanf(h->m_stPort, "COM%d", &h->m_nPort);
	h->m_tty = tty_openPort(h->m_nPort);
	if (h->m_tty == -1)
	{
		TRACE_LOG(h, "\t[Error] The COM%d is opened fail!\n", h->m_nPort);
		return FALSE;
	}

	if (h->m_nBaudrate != 9600 && h->m_nBaudrate != 19200)
		h->m_nBaudrate = 9600;
	tty_raw(h->m_tty, h->m_nBaudrate, 8, 0);
	// create work thread
	h->m_hThread = CreateThread(
		NULL,									// default security attributes
		0,										// use default stack size
		(LPTHREAD_START_ROUTINE)ProtocolThread, // thread function
		h,										// argument to thread function
		CREATE_SUSPENDED,						// use default creation flags
		&dwThreadId);							// returns the thread identifier

	// create mutex handle
	h->m_hMutex = CreateMutex(
		NULL,  // default security attributes
		FALSE, // initially not owned
		NULL); // mutex name (NULL for unname mutex)

	h->m_hLogMutex = CreateMutex(
		NULL,  // default security attributes
		FALSE, // initially not owned
		NULL); // mutex name (NULL for unname mutex)
	if (!h->m_hThread || !h->m_hMutex || !h->m_hLogMutex)
	{

		if (h->m_hThread)
		{
			TerminateThread(h->m_hThread, 0);
			h->m_hThread = NULL;
		}

		if (h->m_hMutex)
		{
			DeleteObject(h->m_hMutex);
			h->m_hMutex = NULL;
		}

		if (h->m_hLogMutex)
		{
			DeleteObject(h->m_hLogMutex);
			h->m_hLogMutex = NULL;
		}
		TRACE_LOG(h, "\tFailed to create TCR8 work thread and/or Mutex!\n");
	}
	return h->m_hThread && h->m_hMutex && h->m_hLogMutex;
#endif
}

DLLAPI BOOL CALLTYPE TCR8_OpenDeviceNet(TCR8HANDLE h, const char *chIP)
{
	DWORD dwThreadId;
	if (!_IsValidHandle(h))
		return FALSE;
	TRACE_LOG(h, "[Note] TCR8Lib Version: %s\n", VERSION_TCR8LIBRATY);
	if (_IsOpen(h))
	{
		if (_IsConnectWithCom(h))
		{
			TRACE_LOG(h, "\t[Error] The COM%d has been opened by TCR8 machine!\n", h->m_nPort);
		}
		else
		{
			struct in_addr address;
			address.s_addr = h->m_dwIP;
			TRACE_LOG(h, "\t[Error] The TCR8 machine has been connected on %s!\n", inet_ntoa(address));
		}
		return FALSE;
	}
	if ((h->m_dwIP = inet_addr(chIP)) == INADDR_NONE)
		return FALSE;
#ifdef linux
	pthread_create(&h->m_hThread, NULL, ProtocolThread, h);
	pthread_mutex_init(&h->m_hMutex, NULL);
	pthread_mutex_init(&h->m_hLogMutex, NULL);
	return TRUE;
#else
	h->m_hThread = CreateThread(
		NULL,									// default security attributes
		0,										// use default stack size
		(LPTHREAD_START_ROUTINE)ProtocolThread, // thread function
		h,										// argument to thread function
		CREATE_SUSPENDED,						// use default creation flags
		&dwThreadId);							// returns the thread identifier

	// create mutex handle
	h->m_hMutex = CreateMutex(
		NULL,  // default security attributes
		FALSE, // initially not owned
		NULL); // mutex name (NULL for unname mutex)

	h->m_hLogMutex = CreateMutex(
		NULL,  // default security attributes
		FALSE, // initially not owned
		NULL); // mutex name (NULL for unname mutex)
	if (!h->m_hThread || !h->m_hMutex || !h->m_hLogMutex)
	{
		if (h->m_hThread)
		{
			TerminateThread(h->m_hThread, 0);
			h->m_hThread = NULL;
		}
		if (h->m_hMutex)
		{
			DeleteObject(h->m_hMutex);
			h->m_hMutex = NULL;
		}
		if (h->m_hLogMutex)
		{
			DeleteObject(h->m_hLogMutex);
			h->m_hLogMutex = NULL;
		}
		TRACE_LOG(h, "\tFailed to create TCR8 work thread and/or Mutex!\n");
	}
	return h->m_hThread && h->m_hMutex && h->m_hLogMutex;
#endif
}

DLLAPI BOOL CALLTYPE TCR8_CloseDevice(TCR8HANDLE h)
{
	int i;

	if (!_IsValidHandle(h))
		return FALSE;
	if (!_IsOpen(h))
		return FALSE;
	h->m_bQuit = TRUE;

	if (_IsConnectWithCom(h))
	{
		tty_close(h->m_tty); // close tty will cause the work thread's tty_getc return with error
		h->m_tty = -1;		 // and break out the loop
	}
	else
	{
		if (h->m_sockTCP != INVALID_SOCKET)
		{
			sock_close(h->m_sockTCP);
			h->m_sockTCP = INVALID_SOCKET;
		}
	}
#ifdef linux
	pthread_cancel(h->m_hThread);
	pthread_join(h->m_hThread, NULL);
	pthread_mutex_destroy(&h->m_hMutex);
	pthread_mutex_destroy(&h->m_hLogMutex);
#else
	ResumeThread(h->m_hThread);
	for (i = 0; i < 10 && h->m_hThread; i++)
		Sleep(5);
	if (h->m_hThread)
		TerminateThread(h->m_hThread, 0);
	h->m_hThread = NULL;
	DeleteObject(h->m_hMutex);
	h->m_hMutex = NULL;
	DeleteObject(h->m_hLogMutex);
	h->m_hLogMutex = NULL;
#endif
	return TRUE;
}

DLLAPI BOOL CALLTYPE TCR8_Run(TCR8HANDLE h)
{
	if (!_IsValidHandle(h))
		return FALSE;
#ifdef linux
	return TRUE;
#else
	if (h->m_hThread)
		return ResumeThread(h->m_hThread) != 0xFFFFFFFF;
#endif
	return FALSE;
}

DLLAPI BOOL CALLTYPE TCR8_Suspend(TCR8HANDLE h)
{
	if (!_IsValidHandle(h))
		return FALSE;
#ifdef linux
#else
	if (h->m_hThread)
		return SuspendThread(h->m_hThread) != 0xFFFFFFFF;
#endif
	return FALSE;
}

BOOL TCR8_PushPanelEx(TCR8HANDLE h, int gw, int ops)
{
	char cmd[8] = {"<xv#>"};
	BOOL bRC;
	if (!_IsValidHandle(h))
		return FALSE;
	if (gw == 1)
	{
		// 上工位用w控制
		cmd[2] = 'w';
		cmd[3] = ops ? 0x30 : 0x31;
		if (_IsJiangsuProtocol(h))
		{
			cmd[1] = 'w';
			cmd[2] = '#';
			cmd[4] = '>';
			cmd[5] = '\0';
		}
	}
	else
	{
		cmd[3] = ops ? 0x30 : 0x31;
		if (_IsJiangsuProtocol(h))
		{
			cmd[1] = 'v';
			cmd[2] = '#';
			cmd[4] = '>';
			cmd[5] = '\0';
		}
	}
	bRC = SendPacket(h, cmd) == 0;
	return bRC;
}

BOOL TCR8_PushPanel(TCR8HANDLE h, int ops)
{
	char cmd[8] = {"<xv#>"};
	BOOL bRC;
	if (!_IsValidHandle(h))
		return FALSE;
	cmd[3] = ops ? 0x30 : 0x31;
	if (_IsJiangsuProtocol(h))
	{
		cmd[1] = 'v';
		cmd[2] = '#';
		cmd[4] = '>';
		cmd[5] = '\0';
	}
	bRC = SendPacket(h, cmd) == 0;
	return bRC;
}

BOOL TCR8_QueryPaperRecycle(TCR8HANDLE h, int gw)
{
	char cmd[8] = {"<xy#>"};
	BOOL bRC;
	if (!_IsValidHandle(h))
		return FALSE;
	cmd[3] = gw == 1 ? 0x31 : 0x32;
	if (_IsJiangsuProtocol(h))
	{
		cmd[1] = 'y';
		cmd[2] = '#';
		cmd[4] = '>';
		cmd[5] = '\0';
	}
	bRC = SendPacket(h, cmd) == 0;
	return bRC;
}

static void TriggerOnPaperRecycle(TCR8HANDLE h, int nDeck, int nEvent)
{
	if (!_IsValidHandle(h))
		return;
	if (h->m_cbFxc && (h->m_dwEventMask & (0x01 << (EVID_PAPERRECYCLE - 1))))
	{
		TRACE_LOG(h, "[EVENT] EVID_PAPERRECYCLE - nDeck %d, nWhy %d!\n", nDeck, nEvent);
		h->m_cbFxc(h, EVID_PAPERRECYCLE, (nDeck << 8 | nEvent));
	}
}

static void TriggerOnFlipMotorChange(TCR8HANDLE h, int nDeck, int nEvent)
{
	if (!_IsValidHandle(h))
		return;
	if (h->m_cbFxc && (h->m_dwEventMask & (0x01 << (EVM_FLIP_MOTOR_CHANGE - 1))))
	{
		TRACE_LOG(h, "[EVENT] EVM_FLIP_MOTOR_CHANGE - nDeck %d, nWhy %d!\n", nDeck, nEvent);
		h->m_cbFxc(h, EVID_FLIP_MOTOR_CHANGE, (nDeck << 8 | nEvent));
	}
}

DLLAPI BOOL CALLTYPE TCR8_SetCartridgeInfo(TCR8HANDLE h, int nChannel, DWORD dwSN, int nCounter)
{
	BOOL rc = FALSE;

	if (!_IsValidHandle(h))
		return FALSE;

	if ((0 < nChannel && nChannel < 5) &&
		(0 <= dwSN && dwSN < 100000000) &&
		((0 <= nCounter) && (nCounter < 1000)))
	{
		char cmd[32];
		if (_IsJiangsuProtocol(h))
		{
			sprintf(cmd, "<jx%d%08d000%03d>", nChannel, dwSN, nCounter);
		}
		else
		{
			sprintf(cmd, "<xj%d%08d000%03d>", nChannel, dwSN, nCounter);
		}
		rc = SendPacket(h, cmd) == 0;
	}
	return rc;
}

DLLAPI BOOL CALLTYPE TCR8_EjectCard(TCR8HANDLE h, int nChannel)
{
	char cmd[8] = "<xb0>";
	BOOL bRC;

	if (!_IsValidHandle(h))
		return FALSE;

	StateEolve(h, tsRWDone);

	h->m_nButton = 0;
	if (0 <= nChannel && nChannel < 7)
		cmd[3] = nChannel + '0';

	if (_IsJiangsuProtocol(h))
	{
		cmd[1] = 'b';
		cmd[2] = '#';
		cmd[3] = '>';
		cmd[4] = '\0';
	}
	// 在发送出卡指令后，复位回收标记，防止返回上报错误
	h->m_bReceiveRecycleCMD = FALSE;
	bRC = SendPacket(h, cmd) == 0;

	return bRC;
}

DLLAPI BOOL CALLTYPE TCR8_ForceEjectCard(TCR8HANDLE h, int nChannel)
{
	char cmd[8] = "<xm0>";
	BOOL bRC;

	if (!_IsValidHandle(h))
		return FALSE;

	if (0 <= nChannel && nChannel < 7)
		cmd[3] = nChannel + '0';

	if (_IsJiangsuProtocol(h))
	{
		cmd[1] = 'm';
		cmd[2] = '#';
	}
	bRC = SendPacket(h, cmd) == 0;
	//	return SendPacket( h, "<xc0>" );
	return bRC;
}

DLLAPI BOOL CALLTYPE TCR8_RecycleCard(TCR8HANDLE h)
{
	char cmd[8] = "<xc0>";
	BOOL bRC;

	if (!_IsValidHandle(h))
		return FALSE;

	h->m_nButton = 0;

	StateEolve(h, tsIdle);

	if (_IsCollector(h))
		cmd[2] = 'b';

	if (_IsJiangsuProtocol(h))
	{
		char tmp;

		tmp = cmd[1];
		cmd[1] = cmd[2];
		cmd[2] = tmp;
		cmd[3] = '>';
		cmd[4] = '\0';
		h->m_bReceiveRecycleCMD = TRUE;
	}

	bRC = SendPacket(h, cmd) == 0;
	//	return SendPacket( h, "<xc0>" );
	return bRC;
}

DLLAPI BOOL CALLTYPE TCR8_ForceRecycleCard(TCR8HANDLE h, int nChannel)
{
	char cmd[8] = "<xn0>";
	BOOL bRC;

	if (!_IsValidHandle(h))
		return FALSE;

	if (0 <= nChannel && nChannel < 7)
		cmd[3] = nChannel + '0';

	if (_IsJiangsuProtocol(h))
	{
		cmd[1] = 'n';
		cmd[2] = '#';
	}

	bRC = SendPacket(h, cmd) == 0;
	return bRC;
}

DLLAPI BOOL CALLTYPE TCR8_CancelButton(TCR8HANDLE h)
{
	char cmd[8] = "<xg>";
	BOOL bRC;

	if (!_IsValidHandle(h))
		return FALSE;

	StateEolve(h, tsIdle);

	if (_IsJiangsuProtocol(h))
	{
		cmd[1] = 'g';
		cmd[2] = '#';
		cmd[3] = '>';
		cmd[4] = '\0';
	}

	h->m_nButton = 0;

	bRC = SendPacket(h, cmd) == 0;

	//	return SendPacket( h, "<xg>" );
	return bRC;
}

DLLAPI BOOL CALLTYPE TCR8_ReturnCard(TCR8HANDLE h, int nChannel)
{
	char cmd[8] = "<xc0>";
	BOOL bRC;

	if (!_IsValidHandle(h))
		return FALSE;

	StateEolve(h, tsIdle);

	if (0 <= nChannel && nChannel < 7)
		cmd[3] = nChannel + '0';

	if (_IsJiangsuProtocol(h))
	{
		cmd[1] = 'c';
		cmd[2] = '#';
		cmd[3] = '>';
		cmd[4] = '\0';
	}

	h->m_nButton = 0;

	bRC = SendPacket(h, cmd) == 0;

	return bRC;
}

DLLAPI BOOL CALLTYPE TCR8_CollectCard(TCR8HANDLE h, int nChannel)
{
	char cmd[8] = "<xd0>";
	BOOL bRC;

	if (!_IsValidHandle(h))
		return FALSE;

	StateEolve(h, tsRWDone);

	if (0 <= nChannel && nChannel < 7)
		cmd[3] = nChannel + '0';

	if (_IsJiangsuProtocol(h))
	{
		cmd[1] = 'd';
		cmd[2] = '#';
		cmd[3] = '>';
		cmd[4] = '\0';
	}

	bRC = SendPacket(h, cmd) == 0;

	return bRC;
}

DLLAPI BOOL CALLTYPE TCR8_SwitchChannel(TCR8HANDLE h, int nChannel)
{
	char cmd[8] = "<xi#>";
	BOOL bRC;

	if (!_IsValidHandle(h))
		return FALSE;

	cmd[3] = nChannel + '0';

	if (_IsJiangsuProtocol(h))
	{
		cmd[1] = 'i';
		cmd[2] = '#';
		cmd[3] = nChannel + '0';
		cmd[4] = '>';
		cmd[5] = '\0';
	}

	bRC = SendPacket(h, cmd) == 0;
	return bRC;
}

DLLAPI BOOL CALLTYPE TCR8_SwitchAntenna(TCR8HANDLE h, int nAnt)
{
	char cmd[8] = "<xh#>";
	BOOL bRC;

	if (!_IsValidHandle(h))
		return FALSE;

	cmd[3] = nAnt + '0';

	if (_IsJiangsuProtocol(h))
	{
		cmd[1] = 'h';
		cmd[2] = '#';
		cmd[3] = nAnt + '0';
		cmd[4] = '>';
		cmd[5] = '\0';
	}
	bRC = SendPacket(h, cmd) == 0;
	return bRC;
}

DLLAPI BOOL CALLTYPE TCR8_PlayAudio(TCR8HANDLE h, int nIndex)
{
	char cmd[8] = "<xd##>";
	BOOL bRC;

	if (!_IsValidHandle(h))
		return FALSE;

	if (_IsJiangsuProtocol(h))
	{
		cmd[1] = 'q'; // 江苏播放语音控制字
		cmd[2] = 'x';
		cmd[3] = nIndex + '0';
		cmd[4] = '>';
		cmd[5] = '\0';
	}
	else
	{
		cmd[3] = nIndex / 10 + '0';
		cmd[4] = nIndex % 10 + '0';
	}

	bRC = SendPacket(h, cmd) == 0;

	return bRC;
}

DLLAPI BOOL CALLTYPE TCR8_TriggerButton(TCR8HANDLE h, int nChannel)
{
	char cmd[8] = "<xl#>";
	BOOL bRC;

	if (!_IsValidHandle(h))
		return FALSE;

	if (_IsJiangsuProtocol(h))
	{
		cmd[1] = 'l';
		cmd[2] = '#';
		cmd[3] = nChannel + '0';
		cmd[4] = '>';
		cmd[5] = '\0';
	}

	if (nChannel < 0 || nChannel > 6)
		return FALSE;

	if (nChannel > 4)
	{
		nChannel = nChannel > 5 ? 2 : 1; // 1: 上工位按键 2: 下工位按键
	}
	else
	{
		if (nChannel != 0)
		{
			nChannel += 2; // 3~6 1#通道~4#通道按键
		}
	}

	cmd[3] = nChannel + '0';

	bRC = SendPacket(h, cmd) == 0;

	return bRC;
}

DLLAPI BOOL CALLTYPE TCR8_PullBackToAnt(TCR8HANDLE h, int nChannel)
{
	char cmd[] = "<xp0000>";
	BOOL bRC;

	if (!_IsValidHandle(h))
		return FALSE;

	if (_IsJiangsuProtocol(h))
	{
		cmd[1] = 'p';
		cmd[2] = 'x';
		cmd[3] = '>';
		cmd[4] = '\0';
	}
	else
	{
		if (nChannel > 4 || nChannel < 0)
			return FALSE;

		if (nChannel != 0)
			cmd[2 + nChannel] += 1;
	}

	bRC = SendPacket(h, cmd) == 0;

	return bRC;
}

DLLAPI BOOL CALLTYPE TCR8_EnableKernelLog(TCR8HANDLE h, BOOL bEnable)
{
	char cmd[] = "<xux>";
	BOOL bRC;
	if (!_IsValidHandle(h))
		return FALSE;
	cmd[3] = bEnable ? 0x31 : 0x30;
	if (_IsJiangsuProtocol(h))
	{
		cmd[1] = 'u';
		cmd[2] = '#';
		cmd[4] = '>';
		cmd[5] = '\0';
	}
	bRC = SendPacket(h, cmd) == 0;
	return bRC;
}

static int get_dirname(const char *path, char *buffer, int buf_size)
{
	int len = -1;
	const char *ptr;

	for (ptr = path + strlen(path); ptr >= path; ptr--)
	{
		if (*ptr == '/')
		{
			len = ptr - path;
			if (len >= buf_size)
				return -1;
			memcpy(buffer, path, len);
			buffer[len] = '\0';
			break;
		}
	}
	return len;
}

static int make_full_dir(const char *path)
{
	struct stat finfo;
	char updir[MAX_PATH];

	if (get_dirname(path, updir, MAX_PATH) <= 0)
	{
		return -1;
	}
	if ((0 == stat(updir, &finfo)) ||
		(0 == make_full_dir(updir)))
	{
		return mkdir(path, 0755);
	}
	else
	{
		return -1;
	}
}

DLLAPI BOOL CALLTYPE TCR8_EnableLog(TCR8HANDLE h, LPCTSTR strPath)
{
	printf("TCR8_EnableLog\r\n");
#ifdef linux
	char logDir[MAX_PATH];
	int len;
	if (!_IsValidHandle(h))
		return FALSE;
	if (h->m_strPath)
	{
		free(h->m_strPath);
		fclose(h->fp);
		h->fp = NULL;
	}
	if (strPath)
	{
		h->m_strPath = strdup(strPath);
		len = strlen(strPath);
		for (; (len <= MAX_PATH) && (len >= 0); len--)
		{
			if ((strPath[len] == '/') || (strPath[len] == '\\'))
				break;
		}
		memcpy(logDir, strPath, len);
		logDir[len] = '\0';
		make_full_dir(logDir);
	}
	else
		h->m_strPath = NULL;
#else
	char logDir[MAX_PATH];
	int len;
	if (!_IsValidHandle(h))
		return FALSE;
	if (h->m_strPath)
	{
		free(h->m_strPath);
		fclose(h->fp);
		h->fp = NULL;
	}
	if (strPath)
	{
		h->m_strPath = strdup(strPath);
		len = strlen(strPath);
		for (; (len <= MAX_PATH) && (len >= 0); len--)
		{
			if ((strPath[len] == '/') || (strPath[len] == '\\'))
				break;
		}
		memcpy(logDir, strPath, len);
		logDir[len] = '\0';
		if (!CreateDirectory(logDir, NULL))
			len = GetLastError(); // for debugger to see the error code
	}
	else
		h->m_strPath = NULL;
#endif
	return TRUE;
}

#ifdef linux
DLLAPI BOOL CALLTYPE TCR8_Log(TCR8HANDLE h, const char *fmt, ...)
#else
DLLAPI BOOL CALLTYPE TCR8_Log(TCR8HANDLE h, LPCTSTR fmt, ...)
#endif
{
	va_list va;
	char str[MAX_PATH] = "";
	int rc = 0;
#ifdef linux
	struct stat stat;
#else
	struct _stat stat;
#endif
	static char timestr[30];
	char *p = timestr;
	struct timeval tv;
	if (!_IsValidHandle(h))
		return FALSE;
	if (!h->m_strPath)
		return FALSE;
	Mutex_LogLock(h);
	va_start(va, fmt);
	vsprintf(str, fmt, va);
	va_end(va);
	if (h->fp != NULL)
	{
#ifdef linux
		fstat(fileno(h->fp), &stat);
#else
		_fstat(_fileno(h->fp), &stat);
#endif
		if (stat.st_size > MAX_LOGSIZE)
		{
			fclose(h->fp);
			h->fp = NULL;
			log_rollforward(h->m_strPath);
		}
	}
	if (h->fp == NULL)
	{
		h->fp = fopen(h->m_strPath, "a");
	}
	if (h->fp != NULL)
	{
#ifdef linux
		gettimeofday(&tv, NULL);
		strftime(p, sizeof(timestr), "%F %H:%M:%S", localtime(&tv.tv_sec));
		if (fmt[0] != '\t')
		{
			rc = fprintf(h->fp, "[%s] %s", timestr, str);
			printf("[%s] %s", timestr, str);
		}
		else
		{
			rc = fprintf(h->fp, "%26s%s", " ", str + 1);
			printf("%26s%s", " ", str + 1);
		}
#else
		SYSTEMTIME tnow;
		GetLocalTime(&tnow);
		if (fmt[0] != '\t')
		{
			rc = fprintf(h->fp, "[%04d/%02d/%02d %02d:%02d:%02d.%03d] %s",
						 tnow.wYear, tnow.wMonth, tnow.wDay, tnow.wHour, tnow.wMinute, tnow.wSecond, tnow.wMilliseconds, str);
		}
		else
		{
			rc = fprintf(h->fp, "%26s%s", " ", str + 1);
		}
#endif
		if (rc < 0)
		{
			fclose(h->fp);
			h->fp = NULL;
		}
		else
			fflush(h->fp);
	}
	Mutex_LogUnlock(h);
	return TRUE;
}

DLLAPI LPCTSTR CALLTYPE TCR8_GetButtonIgnoreText(int code)
{
	switch (code)
	{
	case IBT_NOCARD:
		return IsChinese() ? "无卡" : "None card";
	case IBT_FAILED:
	case IBT_NOTREADY:
		return IsChinese() ? "故障" : "Fault";
	case IBT_MASKPERIOD:
		return IsChinese() ? "按键屏蔽期间" : "During ignore period";
	case IBT_ERRLIMIT:
		return IsChinese() ? "连续坏卡" : "Continuously bad card";
	}
	return "Unknown";
}

DLLAPI LPCTSTR CALLTYPE TCR8_GetTransStateText(int st)
{
	TRState smTrans = (TRState)st;
	switch (smTrans)
	{
	case tsIdle:
		return IsChinese() ? "闲置" : "Idle";
	case tsBegin:
		return IsChinese() ? "等待读写通行卡" : "Wait for read/write";
	case tsRWDone:
		return IsChinese() ? "读写完成出卡" : "Read/write done,eject";
	case tsEJFail:
		return IsChinese() ? "出卡失败" : "Eject fail";
	case tsOnExit:
		return IsChinese() ? "等待司机取卡" : "Wait for button press";
	}
	return "Unknown";
}

DLLAPI LPCTSTR CALLTYPE TCR8_GetEventText(int nEventId, int nParam)
{
	static char buf[64];
	switch (nEventId)
	{
	case EVID_POWERON: // Machine Power On
		return IsChinese() ? "卡机上电" : "Power on";
	case EVID_LINK: // Link event (param 1 Online, 0 offline)
		if (IsChinese())
			return nParam ? "卡机连线" : "卡机离线";
		else
			return nParam ? "Connected" : "Disconnected";
	case EVID_PRESSKEY: // Button Pressed (param: channel)
		sprintf(buf, (IsChinese() ? "司机按取卡键 (%d通道)" : "Button pressed(%d channel)"), nParam);
		break;
	case EVID_OUTCARDOK: // Card ejected  (param: channel)
		sprintf(buf, (IsChinese() ? "卡推出卡机 (%d通道)" : "Eject OK(%d channel)"), nParam);
		break;
	case EVID_OUTCARDFAIL: // Card ejectfail  (param: channel)
		sprintf(buf, (IsChinese() ? "卡推出卡机失败 (%d通道)" : "Eject fail(%d channel)"), nParam);
		break;
	case EVID_TAKECARD: // Card taken (param: channel)
		sprintf(buf, (IsChinese() ? "卡取走 (%d通道)" : "Card taken(%d channel)"), nParam);
		break;
	case EVID_STATECHANGE: // Channel state changed (param: channel)
		sprintf(buf, (IsChinese() ? "通道状态改变 (%d通道)" : "Channel status change(%d channel)"), nParam);
		break;
	case EVID_COUNTCHANGE: // Channel count changed (param: channel)
		sprintf(buf, (IsChinese() ? "通道卡数改变 (%d通道)" : "Channel card count change(%d channel)"), nParam);
		break;
	case EVID_RECYCLEOK: // Bad card recycled (param: channel)
		sprintf(buf, (IsChinese() ? "卡回收成功 (%d通道)" : "Recycle OK(%d channel)"), nParam);
		break;

	case EVID_RECYCLEFAIL: // Bad card recycled (param: channel)
		sprintf(buf, (IsChinese() ? "卡回收失败 (%d通道)" : "Recycle OK(%d channel)"), nParam);
		break;
		/*
			case EVID_OUTCARDOK_EX:
				sprintf( buf, ( IsChinese() ? "卡推出卡机  (%d通道%d座号)" : "Eject OK(%d channel %d seat)" ), (nParam>>8), (nParam & 0xFF));
				break;

			case EVID_RECYCLEOK_EX:
				sprintf( buf, ( IsChinese() ? "卡回收成功 (%d通道%d座号)" : "Recycle OK(%d channel %d seat)" ), (nParam>>8), (nParam & 0xFF));
				break;
		*/
	case EVID_BOXEX:
		sprintf(buf, (IsChinese() ? "多卡盒机芯 %d# 事件(%d通道%d座号)" : "Multibox %d# event(%d channel %d seat)"), ((nParam & 0x1F) >> 8), ((nParam & 0x70) >> 13), (nParam & 0xFF));
		break;

	case EVID_PRESSPOLKEY: // Police key pressed (param: deck) - for ZJ only
		sprintf(buf, (IsChinese() ? "司机按军警键 (%s工位)" : "Police button pressed"), nParam == 1 ? (IsChinese() ? "上" : "Up") : (IsChinese() ? "下" : "Down"));
		break;
	case EVID_RWTIMEOUT: // wait for CSC R/W timed out
		return IsChinese() ? "等待读写通行卡超时" : "Wait for read/write timeout";
	case EVID_BUTTONIGNRD: // button ignored  nParam = nDeck << 8 + nIgnoreCode
		if (IsChinese())
			sprintf(buf, "%s工位按键忽略-%s", (nParam >> 8) == 1 ? "上" : "下", TCR8_GetButtonIgnoreText(nParam & 0xff));
		else
			sprintf(buf, "%sButton ignore-%s", (nParam >> 8) == 1 ? "Up" : "Down", TCR8_GetButtonIgnoreText(nParam & 0xff));
		break;
	case EVID_BOXLOAD:
		if (IsChinese())
			sprintf(buf, "卡盒装入 (%d通道)", nParam);
		else
			sprintf(buf, "Box load (%d channel)", nParam);
		break;
	case EVID_BOXUNLOAD:
		if (IsChinese())
			sprintf(buf, "卡盒卸下 (%d通道)", nParam);
		else
			sprintf(buf, "Box unload (%d channel)", nParam);
		break;
	case EVID_SNCHANGED:
		if (IsChinese())
			sprintf(buf, "卡盒改序号 (%d通道)", nParam);
		else
			sprintf(buf, "Box SN changed (%d channel)", nParam);
		break;

	case EVID_COLLECTOK:
		sprintf(buf, (IsChinese() ? "收卡成功 (%d通道)" : "Collect OK(%d channel)"), nParam);
		break;

	case EVID_COLLECTFAIL:
		sprintf(buf, (IsChinese() ? "收卡失败 (%d通道)" : "Collect fail(%d channel)"), nParam);
		break;

	case EVID_PULLBACKOK: // Bad card recycled (param: channel)
		sprintf(buf, (IsChinese() ? "卡被从卡口拉回成功 (%d通道)" : "Pull Back OK(%d channel)"), nParam);
		break;

	case EVID_PULLBACKFAIL: // Bad card recycled (param: channel)
		sprintf(buf, (IsChinese() ? "卡被从卡口拉回失败 (%d通道)" : "Pull Back OK(%d channel)"), nParam);
		break;

	case EVID_THREADEXIT: // workthread exir abnormally
		if (IsChinese())
			return "工作线程结束-无法再控制卡机";
		else
			return "Work thread end---can't control machine";
	case EVID_TX:
		if (IsChinese())
			return "协议帧TX";
		else
			return "Protocol Tx";
	case EVID_RX:
		if (IsChinese())
			return "协议帧RX";
		else
			return "Protocol Rx";
	case EVID_KERNELLOG:
		if (IsChinese())
			return "收到卡机日志帧";
		else
			return "Receive machine log";
	default:
		if (IsChinese())
			sprintf(buf, "未知事件代号: %d, 参数: %d", nEventId, nParam);
		else
			sprintf(buf, "Unknown event code: %d, parameter: %d", nEventId, nParam);
	}
	return buf;
}

LPCTSTR TCR8_GetMultiBoxEventText(int nEventCode)
{
	char *ptr;

	switch (nEventCode)
	{
	case 1:
		ptr = IsChinese() ? "卡盒装入" : "Box load";
		break;

	case 2:
		ptr = IsChinese() ? "卡盒卸下" : "Box unload";
		break;

	case 3:
		ptr = IsChinese() ? "卡盒信息更新" : "Box information update";
		break;

	case 4:
		ptr = IsChinese() ? "卡盒将满" : "Box about full";
		break;

	case 5:
		ptr = IsChinese() ? "卡盒将空" : "Box about empty";
		break;

	case 6:
		ptr = IsChinese() ? "卡盒已满" : "Box full";
		break;

	case 7:
		ptr = IsChinese() ? "卡盒已空" : "Box empty";
		break;

	case 8:
		ptr = IsChinese() ? "卡盒锁打开" : "Box unlock";
		break;

	case 9:
		ptr = IsChinese() ? "卡盒锁关闭" : "Box lock";
		break;

	case 10:
		ptr = IsChinese() ? "卡盒通信故障" : "Box community fail";
		break;

	case 11:
		ptr = IsChinese() ? "卡盒通信正常" : "Box community ok";
		break;

	case 12:
		ptr = IsChinese() ? "卡盒信息需要设置" : "Box information not ready";
		break;

	default:
		ptr = IsChinese() ? "未知卡盒事件" : "Unknown box event";
		break;
	}

	return ptr;
}

DLLAPI LPCTSTR CALLTYPE TCR8_GetAntennaEventText(int nEventCode)
{
	char *ptr;

	switch (nEventCode)
	{
	case 1:
		ptr = IsChinese() ? "天线位置卡被回收" : "Card on antenna is recycled";
		break;

	case 2:
		ptr = IsChinese() ? "天线位置卡被发出" : "Card on antenna is ejected";
		break;

	case 3:
		ptr = IsChinese() ? "卡到达天线位置" : "Card arrive antenna";
		break;

	case 4:
		ptr = IsChinese() ? "天线位置卡被人为拿走" : "Card is taken by human";
		break;

	case 5:
		ptr = IsChinese() ? "天线位置卡回收失败" : "Card on antenna is recycled fail";
		break;

	case 6:
		ptr = IsChinese() ? "天线位置卡发出失败" : "Card on antenna is eject fail";
		break;

	default:
		ptr = IsChinese() ? "未知天线位置卡片变化事件" : "Unknown antenna change event";
		break;
	}

	return ptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Events callback

static void TriggerOnPowerOn(TCR8HANDLE h)
{
	if (!_IsValidHandle(h))
		return;

	if (h->m_cbFxc && (h->m_dwEventMask & (0x01 << (EVID_POWERON - 1))))
	{
		TRACE_LOG(h, "[EVENT] EVID_POWERON\n");
		h->m_cbFxc(h, EVID_POWERON, 0);
	}
}

static void TriggerOnConection(TCR8HANDLE h, BOOL bOnline)
{
	if (!_IsValidHandle(h))
		return;

	if (h->m_cbFxc && (h->m_dwEventMask & (0x01 << (EVID_LINK - 1))))
	{
		TRACE_LOG(h, "[EVENT]  EVID_LINK - %s\n", bOnline ? "Online" : "Offline");
		h->m_cbFxc(h, EVID_LINK, bOnline);
	}
}

static void TriggerOnButtonPress(TCR8HANDLE h, int nDeck, int boxNo)
{
	if (!_IsValidHandle(h))
		return;

	if (h->m_cbFxc)
	{
		if (((boxNo == 5) || (boxNo == 6))) // 军警按键
		{
			if (h->m_dwEventMask & (0x01 << (EVID_PRESSPOLKEY - 1)))
			{
				TRACE_LOG(h, "[EVENT] EVID_PRESSPOLKEY (nDeck=%d)\n", nDeck);
				h->m_cbFxc(h, EVID_PRESSPOLKEY, nDeck);
			}
		}
		else
		{
			if (h->m_dwEventMask & (0x01 << (EVID_PRESSKEY - 1)))
			{
				TRACE_LOG(h, "[EVENT] EVID_PRESSKEY (channel=%d)\n", boxNo);
				h->m_cbFxc(h, EVID_PRESSKEY, boxNo);
			}
		}
	}
}

static void TriggerOnCardEject(TCR8HANDLE h, int boxNo, BOOL bOK)
{
	if (!_IsValidHandle(h))
		return;

	if (h->m_cbFxc)
	{
		if (h->m_dwEventMask & (0x01 << (EVID_OUTCARDOK - 1)) && bOK)
		{
			TRACE_LOG(h, "[EVENT] %s (boxNo=%d)\n", "EVID_OUTCARDOK", boxNo);
			h->m_cbFxc(h, EVID_OUTCARDOK, boxNo);
		}
		else if (h->m_dwEventMask & (0x01 << (EVID_OUTCARDFAIL - 1)) && !bOK)
		{
			TRACE_LOG(h, "[EVENT] %s (boxNo=%d)\n", "EVID_OUTCARDFAIL", boxNo);
			h->m_cbFxc(h, EVID_OUTCARDFAIL, boxNo);
		}
	}
}

static void TriggerOnCSCValidateTimeout(TCR8HANDLE h, int boxNo)
{
	if (!_IsValidHandle(h))
		return;

	if (h->m_cbFxc && (h->m_dwEventMask & (0x01 << (EVID_RWTIMEOUT - 1))))
	{
		TRACE_LOG(h, "[EVENT] EVID_RWTIMEOUT (boxNo=%d)\n", boxNo);
		h->m_cbFxc(h, EVID_RWTIMEOUT, boxNo);
	}
}

static void TriggerOnCSCTaken(TCR8HANDLE h, int boxNo)
{
	if (!_IsValidHandle(h))
		return;

	if (h->m_cbFxc && (h->m_dwEventMask & (0x01 << (EVID_TAKECARD - 1))))
	{
		TRACE_LOG(h, "[EVENT] EVID_TAKECARD (boxNo=%d)\n", boxNo);
		h->m_cbFxc(h, EVID_TAKECARD, boxNo);
	}
}

static void TriggerOnStateChange(TCR8HANDLE h, int boxNo)
{
	if (!_IsValidHandle(h))
		return;

	if (h->m_cbFxc && (h->m_dwEventMask & (0x01 << (EVID_STATECHANGE - 1))))
	{
		TRACE_LOG(h, "[EVENT] EVID_STATECHANGE (boxNo=%d)\n", boxNo);
		h->m_cbFxc(h, EVID_STATECHANGE, boxNo);
	}
}

static void TriggerOnCountChange(TCR8HANDLE h, int boxNo)
{
	if (!_IsValidHandle(h))
		return;

	if (h->m_cbFxc && (h->m_dwEventMask & (0x01 << (EVID_COUNTCHANGE - 1))))
	{
		TRACE_LOG(h, "[EVENT] EVID_COUNTCHANGE (boxNo=%d)\n", boxNo);
		h->m_cbFxc(h, EVID_COUNTCHANGE, boxNo);
	}
}

static void TriggerOnRecycle(TCR8HANDLE h, int boxNo, BOOL bOK)
{
	if (!_IsValidHandle(h))
		return;

	if (h->m_cbFxc)
	{
		if ((h->m_dwEventMask & (0x01 << (EVID_RECYCLEOK - 1))) && bOK)
		{
			TRACE_LOG(h, "[EVENT] %s (boxNo=%d)\n", "EVID_RECYCLEOK", boxNo);
			h->m_cbFxc(h, EVID_RECYCLEOK, boxNo);
		}
		else if ((h->m_dwEventMask & (0x01 << (EVID_RECYCLEFAIL - 1))) && !bOK)
		{
			TRACE_LOG(h, "[EVENT] %s (boxNo=%d)\n", "EVID_RECYCLEFAIL", boxNo);
			h->m_cbFxc(h, EVID_RECYCLEFAIL, boxNo);
		}
	}
}

static void TriggerOnPullBack(TCR8HANDLE h, int boxNo, BOOL bOK)
{
	if (!_IsValidHandle(h))
		return;

	if (h->m_cbFxc)
	{
		if ((h->m_dwEventMask & (0x01 << (EVID_PULLBACKOK - 1))) && bOK)
		{
			TRACE_LOG(h, "[EVENT] %s (boxNo=%d)\n", "EVID_PULLBACKOK", boxNo);
			h->m_cbFxc(h, EVID_PULLBACKOK, boxNo);
		}
		else if ((h->m_dwEventMask & (0x01 << (EVID_PULLBACKFAIL - 1))) && !bOK)
		{
			TRACE_LOG(h, "[EVENT] %s (boxNo=%d)\n", "EVID_PULLBACKFAIL", boxNo);
			h->m_cbFxc(h, EVID_PULLBACKFAIL, boxNo);
		}
	}
}

static void TriggerOnAbnormalExit(TCR8HANDLE h)
{
	if (!_IsValidHandle(h))
		return;

	if (h->m_cbFxc && (h->m_dwEventMask & (0x01 << (EVID_THREADEXIT - 1))))
	{
		TRACE_LOG(h, "[EVENT] EVID_THREADEXIT!\n");
		h->m_cbFxc(h, EVID_THREADEXIT, 0);
	}
}

static void TriggerOnButtonIgnore(TCR8HANDLE h, int nDeck, int nWhy)
{
	if (!_IsValidHandle(h))
		return;

	if (h->m_cbFxc && (h->m_dwEventMask & (0x01 << (EVID_BUTTONIGNRD - 1))))
	{
		TRACE_LOG(h, "[EVENT] EVID_BUTTONIGNRD - nDeck %d, nWhy %d!\n", nDeck, nWhy);
		h->m_cbFxc(h, EVID_BUTTONIGNRD, (nDeck << 8 | nWhy));
	}
}

static void TriggerOnCartridgeChange(TCR8HANDLE h, int boxNo)
{
	if (!_IsValidHandle(h))
		return;

	if (h->m_cbFxc)
	{
		BOOL bHasBox = _HasCartridge(h, boxNo - 1);

		if ((h->m_dwEventMask & (0x01 << (EVID_BOXLOAD - 1))) && bHasBox)
		{
			TRACE_LOG(h, "[EVENT] %s, Channel=%d\n", "EVID_BOXLOAD", boxNo);
			h->m_cbFxc(h, EVID_BOXLOAD, boxNo);
		}
		else if ((h->m_dwEventMask & (0x01 << (EVID_BOXUNLOAD - 1))) && !bHasBox)
		{
			TRACE_LOG(h, "[EVENT] %s, Channel=%d\n", "EVID_BOXUNLOAD", boxNo);
			h->m_cbFxc(h, EVID_BOXUNLOAD, boxNo);
		}
	}
}

static void TriggerOnChangedCartridgeSN(TCR8HANDLE h, int boxNo)
{
	if (!_IsValidHandle(h))
		return;

	if (h->m_cbFxc && (h->m_dwEventMask & (0x01 << (EVID_SNCHANGED - 1))))
	{
		TRACE_LOG(h, "[EVENT] EVID_SNCHANGED!\n");
		h->m_cbFxc(h, EVID_SNCHANGED, boxNo);
	}
}

static void TriggerOnProtocolPacket(TCR8HANDLE h, BOOL bIsTx, LPCTSTR text)
{
	if (!_IsValidHandle(h))
		return;

	if (h->m_cbFxc)
	{
		if ((h->m_dwEventMask & (0x01 << (EVID_TX - 1))) && bIsTx)
		{
			TRACE_LOG(h, "[EVENT] %s!\n", "EVID_TX");
			h->m_cbFxc(h, EVID_TX, (int)text);
		}
		else if ((h->m_dwEventMask & (0x01 << (EVID_RX - 1))) && !bIsTx)
		{
			TRACE_LOG(h, "[EVENT] %s!\n", "EVID_RX");
			h->m_cbFxc(h, EVID_RX, (int)text);
		}
	}
}

static void TriggerOnKernelLog(TCR8HANDLE h, LPCTSTR text)
{
	if (!_IsValidHandle(h))
		return;

	if (h->m_cbFxc && (h->m_dwEventMask & (0x01 << (EVID_KERNELLOG - 1))))
	{
		h->m_cbFxc(h, EVID_KERNELLOG, (int)text);
	}
}
/*
static void TriggerOnCardEjectEx( TCR8HANDLE h, int nChannel, int nSeat )
{
	if( !_IsValidHandle(h) )
		return;

	if ( h->m_cbFxc && (h->m_dwEventMask & (0x01 << (EVID_OUTCARDOK_EX-1))) )
	{
		TRACE_LOG(h, "[EVENT] EVID_OUTCARDOK_EX - nChannel %d, nSeat %d!\n", nChannel, nSeat );
		h->m_cbFxc( h, EVID_OUTCARDOK_EX, (nChannel<<8|nSeat) );
	}
}

static void TriggerOnCardRecycleEx( TCR8HANDLE h, int nChannel, int nSeat )
{
	if( !_IsValidHandle(h) )
		return;

	if ( h->m_cbFxc && (h->m_dwEventMask & (0x01 << (EVID_RECYCLEOK_EX-1))) )
	{
		TRACE_LOG(h, "[EVENT] EVID_RECYCLEOK_EX - nChannel %d, nSeat %d!\n", nChannel, nSeat );
		h->m_cbFxc( h, EVID_RECYCLEOK_EX, (nChannel<<8|nSeat) );
	}
}
*/

static void TriggerBoxExtendEvent(TCR8HANDLE h, int nChannel, int nSeat, int nEvent)
{
	if (!_IsValidHandle(h))
		return;

	if (h->m_cbFxc && (h->m_dwEventMask & (0x01 << (EVID_BOXEX - 1))))
	{
		TRACE_LOG(h, "[EVENT] EVID_BOXEX (Event ID: %d)- nChannel %d, nSeat %d!\n", nEvent, nChannel, nSeat);
		h->m_cbFxc(h, EVID_BOXEX, ((nChannel << 13) | ((nEvent & 0x1F) << 8) | nSeat));
	}
}

static void TriggerOnCollect(TCR8HANDLE h, int boxNo, BOOL bOK)
{
	if (!_IsValidHandle(h))
		return;

	if (h->m_cbFxc)
	{
		if ((h->m_dwEventMask & (0x01 << (EVID_COLLECTOK - 1))) && bOK)
		{
			TRACE_LOG(h, "[EVENT] %s (boxNo=%d)\n", "EVID_COLLECTOK", boxNo);
			h->m_cbFxc(h, EVID_COLLECTOK, boxNo);
		}
		else if ((h->m_dwEventMask & (0x01 << (EVID_COLLECTFAIL - 1))) && !bOK)
		{
			TRACE_LOG(h, "[EVENT] %s (boxNo=%d)\n", "EVID_COLLECTFAIL", boxNo);
			h->m_cbFxc(h, EVID_COLLECTFAIL, boxNo);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Protocol communication thread

#define HEARTBEAT_LOST 30000 // 30 sec.
#define MSG_STX '<'
#define MSG_ETX '>'
#define MSG_STX1 '{'
#define MSG_ETX1 '}'

static long VersionCode(const char *cCode)
{
	long vCode = 0L;
	const char *p = cCode;
	int nByte = 4;

	for (; nByte--;)
	{
		vCode <<= 8;
		vCode += (*p - '0') * 10 + *(p + 1) - '0';
		p += 2;
		if (*p == '.')
			p++;
	}
	return vCode;
}

static void SetMachineType(TCR8HANDLE h, int nType)
{
	if (!_IsValidHandle(h))
		return;

	h->m_nMachineType = nType;
}

static void ReportChannelStateChange(TCR8HANDLE h, int nChannel, int bitClear, int bitSet)
{
	if (!_IsValidHandle(h))
		return;

	if (0 < nChannel && nChannel < 5)
	{
		int i = nChannel - 1;

		h->m_iStateLast[i] = h->m_iState[i];
		if (bitClear != 0)
			h->m_iState[i] &= ~bitClear;
		if (bitSet != 0)
			h->m_iState[i] |= bitSet;

		if ((h->m_iStateLast[i] & 0xff) != (h->m_iState[i] & 0xff))
			TriggerOnStateChange(h, nChannel);

		if (_IsCounterChanged(h, i) || _IsCardJustOnAntenna(h, i))
		{
			TriggerOnCountChange(h, nChannel);
			_ResetCounterChanged(h, i);
		}
		/*
				// cartridge load/unload event
				if ( _HasCartridge(h,i) != _HadCartridge(h,i) )
					TriggerOnCartridgeChange(h,nChannel);
		*/
	}
}

static void SetChannelOfflineBit(TCR8HANDLE h, char *packet)
{
	char *ptr = packet + 5; // channel 1 status byte
	int i;

	if (!_IsValidHandle(h))
		return;

	for (i = 0; i < 4; i++, ptr += 5)
	{
		if (*ptr == 0x33)
			h->m_iState[i] |= CSB_OFFLINE;
		else
			h->m_iState[i] &= ~CSB_OFFLINE;
	}
}

static void ReportOnConnection(TCR8HANDLE h, BOOL bConnect)
{

	if (!_IsValidHandle(h))
		return;

	// send channel online of line event first
	h->m_bOnline = bConnect;
	TriggerOnConection(h, bConnect);
	/*
		int  i;
		Policy changed. Link broken means computer and TCR8 link broken
		the offline bit in each channel is offline between kernel board and channel control board.
		if ( !bConnect )
		{
			for (i=0; i<4; i++ )
				ReportChannelStateChange( h, i+1, 0, CSB_OFFLINE );
		}
		else
		{
			for (i=0; i<4; i++)
				ReportChannelStateChange( h, i+1, CSB_OFFLINE, 0 );
		}
	*/
}

static void CheckAndReportBoxEvent(TCR8HANDLE h, int Index, DWORD nSN, int nCount)
{
	if (!_IsValidHandle(h))
		return;

	if (nSN != h->m_nSerial[Index])
	{
		DWORD oldSN = h->m_nSerial[Index];
		h->m_nSerial[Index] = nSN;
		if (nSN != 0 && oldSN != 0)
		{
			// S/N number changed, not load/unload
			TriggerOnChangedCartridgeSN(h, Index + 1);
		}
		else if (nSN == 0)
		{
			// box unload
			h->m_iCounter[Index] = 0;
			ReportChannelStateChange(h, Index + 1, 0, CSB_NOBOX);
			TriggerOnCartridgeChange(h, Index + 1);
		}
		else
		{
			// Box load
			ReportChannelStateChange(h, Index + 1, CSB_NOBOX, 0);
			TriggerOnCartridgeChange(h, Index + 1);
		}
	}

	// counter
	if (h->m_iState[Index] & CSB_CSCONANT)
		nCount++;

	if (nCount != h->m_iCounter[Index] && nCount < 1000)
	{
		h->m_iCounter[Index] = nCount;
		TriggerOnCountChange(h, Index + 1);
	}
}

static void ProcessProtocolPacket(TCR8HANDLE h, const char *packet)
{
	int nBoxCode;
	int i, len = strlen(packet);
	char cmd;

	if (!_IsValidHandle(h))
		return;

	cmd = _IsJiangsuProtocol(h) ? packet[1] : packet[2];

	switch (cmd /*packet[2]*/)
	{
	case 'A': // Power On (Reset)
		TriggerOnPowerOn(h);
		StateEolve(h, tsIdle);
		break;

	case 'B': // Status
	{
		const char *ptr = packet + 3;
		BOOL bBadCSC, bFault, bOnExit, bOffline;
		int nOnAnt, nCount;
		short bitClear, bitSet;

		if (!_IsJiangsuProtocol(h))
		{
			h->m_iActive[0] = *(ptr++) - '0';
			h->m_iActive[1] = *(ptr++) - '0';
		}

		for (nBoxCode = 1; nBoxCode < 5; nBoxCode++)
		{
			bBadCSC = bFault = bOnExit = bOffline = FALSE;
			bitClear = bitSet = 0;
			switch (*ptr)
			{
			case 0x31:
				bFault = TRUE;
				break;
			case 0x32:
				bBadCSC = TRUE;
				break;
			case 0x33:
				bOffline = TRUE;
				break;
			}
			ptr++;
			ptr++; // Skip HasBox flag byte
			sscanf(ptr, "%03d", &nCount);
			ptr += 3;
			nOnAnt = 0;

			if (*ptr == '1')
				nOnAnt = 1;
			else if (*ptr == '2')
				bOnExit = TRUE;

			ptr++;
			if (h->m_iCounter[nBoxCode - 1] != nCount)
			{
				h->m_iCounter[nBoxCode - 1] = nCount;
				_SetCounterChanged(h, nBoxCode - 1);
			}
			if (bFault)
				bitSet = CSB_FAILED;
			else
				bitClear = CSB_FAILED;
			if (bBadCSC)
				bitSet |= CSB_BADCSC;
			else
				bitClear |= CSB_BADCSC;
			if (bOnExit)
				bitSet |= CSB_CSCONEXIT;
			else
				bitClear |= CSB_CSCONEXIT;
			if (bOffline)
				bitSet |= CSB_OFFLINE;
			else
				bitClear |= CSB_OFFLINE;
			if (nOnAnt > 0)
			{
				bitSet |= CSB_CSCONANT;
				bitClear |= CSB_CSCRECYCLE | CSB_CSCTAKEN;
			}
			else
				bitClear |= CSB_CSCONANT;
			if (nCount > nOnAnt)
				bitClear |= CSB_NOBOX;
			// do not report channel state if this is the first status packet when we connect machine.
			// as we will send initial packet and machine will feed with cartridge packet, version packet and status packet
			// then we can process all status packet after initialized.
			if (h->m_hexVersion)
			{
				ReportChannelStateChange(h, nBoxCode, bitClear, bitSet);
			}
		}
	}
	break;

	case 'C': // Card Eject for Auto Dispenser; Return Card for Auto Collector
		nBoxCode = (_IsJiangsuProtocol(h) ? packet[3] : packet[4]) - '0';

		if (packet[3] == (_IsJiangsuProtocol(h) ? '0' : '3')) // failed
		{
			//			nBoxCode = packet[4] - '0';
			StateEolve(h, tsEJFail);
			TriggerOnCardEject(h, nBoxCode, FALSE);
		}
		else if (!_IsJiangsuProtocol(h) && (packet[3] == '4')) // CSC R/W time out and transaction is reset by Machine
		{
			//			nBoxCode = packet[4] - '0';
			TriggerOnCSCValidateTimeout(h, nBoxCode);
			StateEolve(h, tsIdle);
		}
		else
		{
			if (!_IsJiangsuProtocol(h) || !(h->m_bReceiveRecycleCMD))
			{
				StateEolve(h, tsOnExit);
				//				nBoxCode = (_IsJiangsuProtocol(h) ? packet[3] : packet[4]) - '0';
				ReportChannelStateChange(h, nBoxCode, CSB_CSCONANT, CSB_CSCONEXIT);
				TriggerOnCardEject(h, nBoxCode, TRUE);
			}
			else if (h->m_bReceiveRecycleCMD) // 江苏协议回收卡命令下发后，卡机要发送一个出卡信息帧，所以在此上报回收卡完成消息。
			{
				h->m_bReceiveRecycleCMD = FALSE;
				TriggerOnRecycle(h, nBoxCode, TRUE);
			}
		}
		break;

	case 'D': // Button pressed
	{
		int channel = (_IsJiangsuProtocol(h) ? packet[3] : packet[4]);
		char deck;

		h->m_nChannel = channel - '0';

		if (!_IsJiangsuProtocol(h))
		{
			deck = packet[3];
		}
		else
		{
			deck = (channel < '6') ? '1' : '2'; // '5'： 上军警按键按下，'6'： 下军警按键按下
		}

		if (ENABLE_ENGLISH)
		{
			TRACE_LOG(h, "司机按键，工位%c, 通道%c, 流程状态:%s\n",
					  deck /*packet[3]*/,
					  channel /*packet[4]*/,
					  TCR8_GetTransStateText(h->smTrans));
		}
		else
		{
			TRACE_LOG(h, "Button pressed, deck %c, channel %c, transaction status:%s\n",
					  deck /*packet[3]*/,
					  channel /*packet[4]*/,
					  TCR8_GetTransStateText(h->smTrans));
		}

		/*if ( 0 < h->m_nChannel && h->m_nChannel < 7 )*/
		{
			StateEolve(h, tsBegin);
			//			Sleep( 100 );					// 2011-09-05  wait for antenna stable
			TriggerOnButtonPress(h, deck - '0' /*packet[3] - '0'*/, h->m_nChannel);
			h->m_nButton = h->m_nChannel < 3 ? 1 : 2;
		}
	}
	break;

	case 'E': // CSC removed
		StateEolve(h, tsIdle);
		nBoxCode = (_IsJiangsuProtocol(h) ? packet[3] : packet[4]) - '0';
		ReportChannelStateChange(h, nBoxCode, CSB_CSCONEXIT, CSB_CSCTAKEN);
		TriggerOnCSCTaken(h, nBoxCode);
		h->m_nChannel = 0; // Reset active channel
		break;

	case 'G': // Card recycled
		// TODO - Log the recycle event
		if (_IsJiangsuProtocol(h))
		{
			break;
		}

		if (ENABLE_ENGLISH)
		{
			TRACE_LOG(h, "\t==>通道%c 回收坏卡%s!\n", packet[4], (packet[3] != '3') ? "成功" : "失败");
		}
		else
		{
			TRACE_LOG(h, "\t==>Channel %c recycle bad card %s!\n", packet[4], (packet[3] != '3') ? "OK" : "Fail");
		}

		nBoxCode = packet[4] - '0';
		if (packet[3] != '3')
		{
			ReportChannelStateChange(h, nBoxCode, CSB_BADCSC | CSB_CSCONANT, CSB_CSCRECYCLE);
		}
		TriggerOnRecycle(h, nBoxCode, packet[3] != '3');
		break;

	case 'F': // Cartridge information
	case 'X':
		if (_IsJiangsuProtocol(h))
		{
			break;
		}

		if (_IsHuaNanKuaiSuProtocol(h) && (cmd == 'X'))
		{
			len--;
		}

		if (len == 36)
		{
			DWORD nSN[4];
			sscanf(packet + 3, "%08d%08d%08d%08d", nSN, nSN + 1, nSN + 2, nSN + 3);
			for (i = 0; i < 4; i++)
				CheckAndReportBoxEvent(h, i, nSN[i], 1000);
		}
		else if (len == 40)
		{
			DWORD nSN[4], nCount[4];
			sscanf(packet + 3, "%06d%03d06d%03d%06d%03d%06d%03d", nSN, nCount, nSN + 1, nCount + 1, nSN + 2, nCount + 2, nSN + 3, nCount + 3);
			for (i = 0; i < 4; i++)
				CheckAndReportBoxEvent(h, i, nSN[i], nCount[i]);
		}
		else if (len == 19)
		{
			DWORD nSN;
			int nCount;
			int nCap;

			sscanf(packet + 4, "%08d%03d%03d", &nSN, &nCap, &nCount);
			i = packet[3] - 0x31; // 0-based binary channel index
			if (0 <= i && i < 4)
			{
				h->m_iCap[i] = nCap;
				TRACE_LOG(h, "\t[Note] %d #卡盒容量: %d\n", i + 1, h->m_iCap[i]);
			}
			else
			{
				TRACE_LOG(h, "[Error] 信息帧 %c 上报的通道错误，上报的通道 = %d \n", cmd, packet[3] - 0x31);
			}

			CheckAndReportBoxEvent(h, i, nSN, nCount);
		}
		break;

	case 'H': // Button ignored
		if (_IsJiangsuProtocol(h))
		{
			break;
		}
		else
		{
			int nWhy = packet[4] - '0';
			int nMachine = packet[3] - '0'; // 1 -- upper machine, 2 lower machine
			switch (nWhy)
			{
			case IBT_NOCARD:
				if (ENABLE_ENGLISH)
				{
					TRACE_LOG(h, "\t==> %s 工位按键被忽略，无卡!\n", nMachine == 1 ? "上" : "下");
				}
				else
				{
					TRACE_LOG(h, "\t==> %s Button press is ignored, none card!\n", nMachine == 1 ? "Up" : "Down");
				}
				break;
			case IBT_FAILED:
			case IBT_NOTREADY:
				if (ENABLE_ENGLISH)
				{
					TRACE_LOG(h, "\t==> %s 工位按键被忽略，故障!\n", nMachine == 1 ? "上" : "下");
				}
				else
				{
					TRACE_LOG(h, "\t==> %s Button press is ignored, fault!\n", nMachine == 1 ? "Up" : "Down");
				}
				break;
			case IBT_MASKPERIOD:
				if (ENABLE_ENGLISH)
				{
					TRACE_LOG(h, "\t==> %s 工位按键被忽略，屏蔽期间!\n", nMachine == 1 ? "上" : "下");
				}
				else
				{
					TRACE_LOG(h, "\t==> %s Button press is ignored, during ignore period!\n", nMachine == 1 ? "Up" : "Down");
				}
				break;
			case IBT_ERRLIMIT:
				if (ENABLE_ENGLISH)
				{
					TRACE_LOG(h, "\t==> %s 工位按键被忽略，连续坏卡中断流程!\n", nMachine == 1 ? "上" : "下");
				}
				else
				{
					TRACE_LOG(h, "\t==> %s Button press is ignored, continuously bad card, terminate the process!\n", nMachine == 1 ? "Up" : "Down");
				}
				break;
			}
			TriggerOnButtonIgnore(h, nMachine, nWhy);
		}
		break;

	case 'I': // 通道扩展状态
			  /*{
				  int nChannel;
				  int nSeat;
				  DWORD nSN;
				  int nCount, nCap;
	  
				  nChannel = packet[3] - '0';
	  
				  if( (nChannel < 1) || (nChannel > 4) )
					  break;
	  
				  nSeat = packet[4] - '0';
	  
				  if( (nSeat < 1) || (nSeat > MAX_BOX_PERCHANNEL) )
					  break;
	  
				  sscanf( packet+5, "%08d%03d%03d", &nSN, &nCap, &nCount );
	  
				  nChannel -= 1;
				  nSeat -= 1;
	  
				  h->m_nChannelBoxInfo[ nChannel ].nSn[ nSeat ] = nSN;
				  h->m_nChannelBoxInfo[ nChannel ].nCap[ nSeat ] = nCap;
				  h->m_nChannelBoxInfo[ nChannel ].nCount[ nSeat ] = nCount;
	  
				  TRACE_LOG( h, "[Note] %d# channel %d cartridge sn: %d, cap: %d, count: %d\n", nChannel+1, nSeat+1, nSN, nCap, nCount );
			  }*/
		if (_IsCollector(h))
		{
			if (ENABLE_ENGLISH)
			{
				TRACE_LOG(h, "\t==>通道%c 收卡%s!\n", packet[4], (packet[3] != '3') ? "成功" : "失败");
			}
			else
			{
				TRACE_LOG(h, "\t==>Channel %c collect card %s!\n", packet[4], (packet[3] != '3') ? "OK" : "Fail");
			}

			nBoxCode = packet[4] - '0';
			if (packet[3] != '3')
			{
				ReportChannelStateChange(h, nBoxCode, CSB_CSCONANT, 0);
			}
			TriggerOnCollect(h, nBoxCode, packet[3] != '3');
		}
		break;

	case 'J': // 天线位置卡片变化事件
	{
		int nChannel, nSeat, nEventCode;
		DWORD nSN;

		nChannel = packet[3] - '0';
		nSeat = packet[4] - '0';
		nEventCode = packet[5] - '0';

		sscanf(packet + 6, "%8d", &nSN);

		TRACE_LOG(h, "[Note] Antenna %d# Event (%s) (%d# channel %d# nSeat).\n",
				  nEventCode, TCR8_GetAntennaEventText(nEventCode), nChannel, nSeat);

		if (0 < nChannel && nChannel <= 4)
		{
			nChannel--;

			if (nEventCode == 3) // 卡片到达天线位置
			{
				h->m_nCardOnAntInfo[nChannel].nSeat = nSeat;
				h->m_nCardOnAntInfo[nChannel].nSn = nSN;
			}
			else
			{
				_ClearCardOnAntInfo(h, nChannel);
			}
		}
	}
	break;

	case 'K': // 多卡盒机芯卡盒事件
	{
		int nChannel, nSeat, nCap, nCount, nEventCode;
		DWORD nSN;

		nChannel = packet[3] - '0';
		nSeat = packet[4] - '0';

		sscanf(packet + 5, "%2d%8d%3d%3d", &nEventCode, &nSN, &nCap, &nCount);

		TRACE_LOG(h, "[Note] %d# channel %d# seat occur %d# event(%s) box SN: %d, Capacity: %d, Count: %d.\n",
				  nChannel, nSeat, nEventCode, TCR8_GetMultiBoxEventText(nEventCode), nSN, nCap, nCount);

		if ((0 < nChannel && nChannel <= 4) && (0 < nSeat && nSeat <= MAX_BOX_PERCHANNEL))
		{
			nChannel--;
			nSeat--;

			h->m_nChannelBoxInfo[nChannel].nSn[nSeat] = nSN;
			h->m_nChannelBoxInfo[nChannel].nCap[nSeat] = nCap;
			h->m_nChannelBoxInfo[nChannel].nCount[nSeat] = nCount;
		}

		TriggerBoxExtendEvent(h, nChannel + 1, nSeat + 1, nEventCode);
	}
	break;

	case 'P':
		if (ENABLE_ENGLISH)
		{
			TRACE_LOG(h, "\t==>通道%c 从卡口拉回卡%s!\n", packet[4], (packet[3] != '3') ? "成功" : "失败");
		}
		else
		{
			TRACE_LOG(h, "\t==>Channel %c recycle bad card %s!\n", packet[4], (packet[3] != '3') ? "OK" : "Fail");
		}

		nBoxCode = packet[4] - '0';
		if (packet[3] != '3')
		{
			ReportChannelStateChange(h, nBoxCode, 0, CSB_CSCONANT);
		}
		TriggerOnPullBack(h, nBoxCode, packet[3] != '3');
		break;

	case 'V': // Firmware Version "<#Vxx.xx.xx.xx>
		if (_IsJiangsuProtocol(h))
		{
			break;
		}
		else
		{
			h->m_hexVersion = VersionCode(packet + 3);
		}
		break;
	case 'Y':
	{
		int nChannel, nEvent;
		nChannel = packet[3] - '0';
		nEvent = packet[4] - '0'; // 事件号
		TriggerOnPaperRecycle(h, nChannel, nEvent);
	}
	break;
	case 'W':
	{
		int nChannel, nEvent;
		nChannel = packet[3] - '0';
		nEvent = packet[4] - '0'; // 事件号
		TriggerOnFlipMotorChange(h, nChannel, nEvent);
	}
	break;
	default:
		break;
	}
}

// 超时时间设置长点，不然容易收不到完整的数据帧，经过测试，一般字节间隔最多30ms左右
static inline int TCRGetChar(TCR8HANDLE h)
{
	int chr = 0;
	if (_IsConnectWithCom(h))
#ifdef linux
		chr = tty_getc(h->m_tty, 30, 0);
#else
	{
		chr = tty_getc(h->m_tty, 40);
	/*	if( chr >= 0 )
			TRACE_LOG(h, "-> %c\r\n", chr); */
	}
#endif
	else
		chr = sock_getc(h->m_sockTCP, 10);
	return chr;
}

/* 读取一个帧 */
static int ReadFramePacket(TCR8HANDLE h, char *buf)
{
	char *next = buf;
	int chr = 0;
	while ((chr = TCRGetChar(h)) >= 0 && chr != '<' && chr != '{')
		TRACE_LOG(h, "skip:[%#x]\r\n", chr);
	if (chr == '<')
	{
		*next++ = '<';
		while ((chr = TCRGetChar(h)) >= 0 && chr != '>')
			*next++ = chr;
		if (chr < 0)
			goto erro_frame;
		*next++ = '>';
		*next = '\0';
		return next - buf;
	}
	else if (chr == '{')
	{
		*next++ = '{';
		while ((chr = TCRGetChar(h)) >= 0 && chr != '}')
			*next++ = chr;
		if (chr < 0)
			goto erro_frame;
		*next++ = '}';
		*next = '\0';
		return next - buf;
	}
	else if (chr == -2)
	{
		if (!_IsConnectWithCom(h))
		{
			sock_close(h->m_sockTCP);
			h->m_sockTCP = 0;
			return -1;
		}
	}
erro_frame:
	if (chr == -2)
	{
		sock_close(h->m_sockTCP);
		h->m_sockTCP = 0;
		TRACE_LOG(h, "socket broken, skip:[%d][%s]\r\n", next - buf, buf);
	}
	return 0;
}

#define IsAckPacket(tcr8, buf) ((_IsJiangsuProtocol(tcr8) && buf[1] == '0') || (!_IsJiangsuProtocol(tcr8) && buf[2] == '0'))	 //( buf[2] == '0' )
#define IsNakPacket(tcr8, buf) ((_IsJiangsuProtocol(tcr8) && buf[1] == '1') || (!_IsJiangsuProtocol(tcr8) && buf[2] == '1'))	 //( buf[2] == '1' )
#define IsPowerOnPacket(tcr8, buf) ((_IsJiangsuProtocol(tcr8) && buf[1] == 'A') || (!_IsJiangsuProtocol(tcr8) && buf[2] == 'A')) //( buf[2] == 'A' )
#define IsStatusPacket(tcr8, buf) ((_IsJiangsuProtocol(tcr8) && buf[1] == 'B') || (!_IsJiangsuProtocol(tcr8) && buf[2] == 'B'))	 //( buf[2] == 'B' )

#define TCP_PORT 4018

#ifdef linux
static void ProtocolThread(void *lpParameter)
#else
static DWORD WINAPI ProtocolThread(LPVOID lpParameter)
#endif
{
	int64_t t_now;
	int tout;
	int i, ch;
	int nSkip = 0, szNextFrame = 0;
	int rlen;
	char buf[256] = {0};
	char *ptr;
	char packet_index;
	TCR8HANDLE h = (TCR8HANDLE)lpParameter;
	fd_set rfd_set;
	struct timeval tv;
	int nsel; 
	TRACE_LOG(h, "||===============<< TCR8 Protocol Thread Start >>====================||\n"); 
	if (h->m_dwIP != INADDR_NONE)
		winsock_startup();

	// Initial local stuffs
	for (i = 0; i < 4; i++)
	{
		h->m_iState[i] = CSB_NOBOX | CSB_OFFLINE;
		h->m_nSerial[i] = 0;
		h->m_iCounter[i] = 0;
	}
	h->m_iActive[0] = h->m_iActive[1] = 0;
	h->smTrans = h->smLastTrans = tsIdle;
	// set time zone
	_tzset();
	h->m_bOnline = FALSE;
	h->m_bQuit = FALSE;
	nSkip = 0;
	h->m_tHeartBeat2Recv = GetSystemTime64();
	for (; !h->m_bQuit;)
	{
		if ((tout = GetTime2Resend(h)) == 0)
			tout = 1000;
		if (!_IsConnectWithCom(h) && h->m_sockTCP <= 0)
		{
			h->m_sockTCP = sock_connect0(h->m_dwIP, TCP_PORT);
			if (h->m_sockTCP == INVALID_SOCKET)
			{
				TRACE_LOG(h, "[Error] sock_connect0 error code = %d", WSAGetLastError());
				Sleep(2000);
				continue;
			}
			TRACE_LOG(h, "[Error] sock_connect0 success \r\n");
			h->m_tHeartBeat2Recv = GetSystemTime64();
			ReportOnConnection(h, TRUE);
			continue;
		}
		/* check for machine offline -- w/o hearing the status packet over 20 sec */
		t_now = GetSystemTime64();
		if (t_now > h->m_tHeartBeat2Recv && h->m_bOnline)
		{
			if (ENABLE_ENGLISH)
			{
				TRACE_LOG(h, "[HEARTBEAT] - %d秒没有收到TCR8状态帧，设置卡机离线!\n", HEARTBEAT_LOST / 1000);
			}
			else
			{
				TRACE_LOG(h, "[HEARTBEAT] - %d seconds elapsed from last received TCR8 status, set machine off line!\n", HEARTBEAT_LOST / 1000);
			}
			ReportOnConnection(h, FALSE);
		}
		if (time(NULL) > h->m_tNextSync && h->m_bOnline)
		{
			if (ENABLE_ENGLISH)
			{
				TRACE_LOG(h, "[时钟同步] ·发送时钟同步帧，同步TCR8和计算机时间!\n");
			}
			else
			{
				TRACE_LOG(h, "[Clock Synchronization] ·Send Clock Synchronization cmd, synchronize TCR8 and host time!\n");
			}
			SendTimeSyncPacket(h);
			h->m_tNextSync = time(NULL) + (86400 - time(NULL) % 86400); // Next time to sync the system clock is End Of Day
		}
		memset(buf, 0, sizeof(buf));
		rlen = ReadFramePacket(h, buf);
		if (rlen <= 0)
			continue;
		if (buf[0] == MSG_STX1)
		{
			TRACE_LOG(h, "Kernel commetn:[%s]\r\n", buf);
			TriggerOnKernelLog(h, buf);
			continue;
		}
		TRACE_LOG(h, "RX[%s]\r\n", buf);
		if (IsValidPacket(h, buf))
		{
			packet_index = _IsJiangsuProtocol(h) ? buf[2] : buf[1];

			if (ENABLE_ENGLISH)
			{
				TRACE_LOG(h, "[协议帧Rx] %s\n", buf);
			}
			else
			{
				TRACE_LOG(h, "[Protocol Rx] %s\n", buf);
			}
			TriggerOnProtocolPacket(h, FALSE, buf);
			h->m_tHeartBeat2Recv = GetSystemTime64() + HEARTBEAT_LOST;
			if (IsAckPacket(h, buf))
				AckedPacket(h, /*buf[1]*/ packet_index - '0');
			else if (IsNakPacket(h, buf))
				NakedPacket(h, packet_index - '0');
			else
			{
				if (IsStatusPacket(h, buf))
				{
					if (!h->m_bOnline)
					{
						SetChannelOfflineBit(h, buf);
						ReportOnConnection(h, TRUE);
						SendInitialPacket(h);
					}
				}
				else if (IsPowerOnPacket(h, buf))
				{
					SendInitialPacket(h);
				}
				else
				{
					SendAck(h, packet_index /*buf[1]*/); // Ack it
				}
				ProcessProtocolPacket(h, buf);
			}
		}
		else
		{
			packet_index = _IsJiangsuProtocol(h) ? buf[2] : buf[1];
			if (1)
			{
				TRACE_LOG(h, "[协议帧错误-无效的协议帧] %s\n", buf);
			}
			else
			{
				TRACE_LOG(h, "[Protocol error-invalid protocol] %s\n", buf);
			}
			if (isdigit(packet_index /*buf[1]*/))
				SendNak(h, packet_index /*buf[1]*/); // NAK it
		}

	} // end for(;;)

	if (h->m_sockTCP != INVALID_SOCKET)
	{
		sock_close(h->m_sockTCP);
		h->m_sockTCP = INVALID_SOCKET;
	}

	// if break out the main loop abnormally, report event
	if (!h->m_bQuit)
	{
		TriggerOnAbnormalExit(h);
	}

	h->m_hThread = NULL;

	if (ENABLE_ENGLISH)
	{
		TRACE_LOG(h, "<<<< TCR8 协议线程结束 >>>>\n");
	}
	else
	{
		TRACE_LOG(h, "<<<< TCR8 protocol thread end >>>>\n");
	}

	if (h->m_dwIP != INADDR_NONE)
		winsock_cleanup();

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// Protocol communication functions with TCR8 control board
// lock/unlock mechanism for exclusive access to message TX list.
#ifdef linux
#define Mutex_Lock(h) pthread_mutex_lock(&h->m_hMutex)
#define Mutex_Unlock(h) pthread_mutex_unlock(&h->m_hMutex)
#else
#define Mutex_Lock(h) WaitForSingleObject(h->m_hMutex, INFINITE)
#define Mutex_Unlock(h) ReleaseMutex(h->m_hMutex)
#endif

#define MSGLEN_PWRON 4		  // <#A>		Power On
#define MSGLEN_STATUS 30	  // <#Bnnnn..>
#define MSGLEN_STATUS_EX 34	  // <xInnnn..> // 扩展机芯状态信息
#define MSGLEN_COLLECT 6	  // <#Inn>   CSC collected
#define MSGLEN_PBUTTON 6	  // <#Dnn>	Button is pressed
#define MSGLEN_EJECT 6		  // <#Cnn>	CSC ejected to exit
#define MSGLEN_TAKEN 6		  // <#Enn>	CSC removed
#define MSGLEN_RECYELE 6	  // <#Gnn>	CSC recycled
#define MSGLEN_BOXINF1 19	  // <#FnSSSSSSSSXXXCCC> (extended set - single cartridge)
#define MSGLEN_BOXINFEX 36	  // <#Faaaaaaaabbbbbbbbccccccccdddddddd> Cardtridge Serial number for 4 channel (extended set - old)
#define MSGLEH_BOXINF 40	  // <#FSSSSSScccSSSSSScccSSSSSScccSSSSSSccc> cartridge S/N and Counter for 4 channel (Basic set)
#define MSGLEN_BOXINF_HNKS 48 // <#XSSSSSSSScccSSSSSSSScccSSSSSSSScccSSSSSSSScccR> the length is no include xor
#define MSGLEN_BOXINF1_HNKS 19
#define MSGLEN_BOXINFEX_HNKS 36
#define MSGLEN_XBOXINF 22			// <#Knmeesssssssscccnnn> m# box information of n channel
#define MSGLEN_IGNORE 6				// <#Hnc>	Pushbutton ignored
#define MSGLEN_EVENT_CARD_ON_ANT 15 // <#Jcsennnnnnnn>  // 天线位置卡片变化事件
#define MSGLEN_PULLBACK 6			// <#Pnc>
#define MSGLEN_VERSION 15			// <#Vxx.xx.xx.xx>	Firmware version
#define MSGLEN_VERSION1 12			// <#Vaabbccdd> represent vesion aa.bb.cc.dd
#define MSGLEN_ACK 4				// <#0>
#define MSGLEN_NAK 4				// <#1>
#define MSGLEN_PWRON_JS 4			// <#A>		Power On
#define MSGLEN_STATUS_JS 28			// <#Bnnnn..>
#define MSGLEN_PBUTTON_JS 5			// <#Dnn>	Button is pressed
#define MSGLEN_EJECT_JS 5			// <#Cnn>	CSC ejected to exit
#define MSGLEN_TAKEN_JS 5			// <#Enn>	CSC removed
#define MSG_STX '<'
#define MSG_ETX '>'

#define MST_IDLE 0
#define MST_WAITSEND 1
#define MST_WAITACK 2
#define MST_TIMEOUT 1000 /* Time out to wait for ACKed (in # of msec.) */
#define MST_NAKLIMIT 3	 /* Same message NAK by host over 'MST_NAKLIMIT' times will be treated        \
						  * as ACKed. This can prevent message that not supported by controller will  \
						  * block following message exchange as it will be always NAKed by contoller. \
						  */

static char CalcXOR_HNKS(unsigned char *packet, int len)
{
	unsigned char XOR = 0;
	int i;

	for (i = 1; i <= len; i++)
	{
		XOR += (*(packet++) ^ i);
	}

	XOR = (XOR % 26) + 65;

	return XOR;
}

static char FillXor_HNKS(unsigned char *packet, int len)
{
	packet[len - 1] = CalcXOR_HNKS(packet + 1, len - 2);
	packet[len] = MSG_ETX;
	packet[len + 1] = '\0';

	return len + 1;
}

static BOOL IsWaitAck(TCR8HANDLE h, char cmd)
{
	if (_IsJiangsuProtocol(h) && ((cmd == 'q') || (cmd == 'r')))
		return FALSE;

	return TRUE;
}

static const char *get_packet_data(const char *pac)
{
	static char buffer[256] = {0};
	char *p = buffer;
	int len = strlen(pac);
	int i = 0;
	len = len > 256 ? 256 : len;
	while (len-- && *pac != '>')
		*p++ = *pac++;
	*p++ = '>';
	*p = '\0';
	return buffer;
}

int SendPacket(TCR8HANDLE h, const char *i_packet)
{
	int nseq;
	int len = strlen(i_packet);
	char packet[MSGBUF_SIZE];

	if (!_IsValidHandle(h))
		return -2;

	Mutex_Lock(h);
	strcpy(packet, i_packet);
	nseq = (h->m_cSeq - '0') + 1;
	if (nseq > 9)
		nseq = 0;

	/* check for queue full (too many un-acked message) */
	if (h->_laneMsg[nseq].msg_state == MST_WAITACK)
	{
		if (ENABLE_ENGLISH)
		{
			TRACE_LOG(h, "[TX队列] - 等待发送队列已满，信息帧%s忽略!\n", packet);
		}
		else
		{
			TRACE_LOG(h, "[TX Queue] - Tx Queue is full, information frame %s is ignored!\n", packet);
		}
		Mutex_Unlock(h);
		return -1;
	}
	else
	{
		char cmdCtrl;
		BOOL bWaitAck;

		h->m_cSeq = nseq + '0';

		if (_IsJiangsuProtocol(h))
		{
			packet[2] = h->m_cSeq;
			cmdCtrl = packet[1];
		}
		else
		{
			packet[1] = h->m_cSeq;
			cmdCtrl = packet[2];
		}

		bWaitAck = IsWaitAck(h, cmdCtrl);

		if (_IsHuaNanKuaiSuProtocol(h))
		{
			/*TRACE_LOG( h, "[Note] 填充协议之前，协议长度： %d，协议内容： %s\n", len, packet );*/
			FillXor_HNKS((unsigned char *)packet, len);
			len = strlen(packet);
			/*TRACE_LOG( h, "[Note] 填充协议之后，协议长度： %d，协议内容： %s\n", len, packet );*/
		}
		if (IsEmptyQueue(h) || !bWaitAck)
		{
			h->_laneMsg[nseq].msg_sndcnt = 0;
			h->_laneMsg[nseq].msg_state = bWaitAck ? MST_WAITACK : MST_IDLE;
			h->_laneMsg[nseq].msg_naked = 0;
			TCR8SendData(h, packet, len);
			h->_laneMsg[nseq].t_resend = GetSystemTime64() + MST_TIMEOUT;
			TriggerOnProtocolPacket(h, TRUE, packet);
			TRACE_LOG(h, "[协议帧Tx] %s\n", packet);
		}
		else
		{
			if (ENABLE_ENGLISH)
			{
				TRACE_LOG(h, "[TX队列] - 发送队列有等待发送/应答帧，帧 [%s]放在位置%d 等待发送!\n", packet, nseq);
			}
			else
			{
				TRACE_LOG(h, "[TX Queue] - There are wait sent /reply frame, frame [%s] is put at %d to send!\n", packet, nseq);
			}
			h->_laneMsg[nseq].msg_sndcnt = 0;
			h->_laneMsg[nseq].msg_state = MST_WAITSEND;
		}
		h->_laneMsg[nseq].t_queue = time(NULL);
		memcpy(h->_laneMsg[nseq].msg_body, packet, len + 1); /* 'len' does not include the NULL terminator. We need it */
		h->_laneMsg[nseq].msg_len = len;
	}
	Mutex_Unlock(h);
	return 0;
}

static int SendAck(TCR8HANDLE h, char cseq)
{
	/*static*/ char cAck[8] = "<x0>";
	int len;

	if (!_IsValidHandle(h))
		return -2;

	if (_IsJiangsuProtocol(h))
	{
		cAck[1] = '0';
		cAck[2] = 'x';
	}

	if ('0' <= cseq && cseq <= '9')
	{
		if (_IsJiangsuProtocol(h))
		{
			cAck[2] = cseq;
		}
		else
		{
			cAck[1] = cseq;
		}

		len = strlen(cAck);

		if (_IsHuaNanKuaiSuProtocol(h))
		{
			len = FillXor_HNKS((unsigned char *)cAck, len);
		}
		TCR8SendData(h, cAck, len);
		TriggerOnProtocolPacket(h, TRUE, cAck);
		if (ENABLE_ENGLISH)
		{
			TRACE_LOG(h, "[协议帧Tx] %s\n", cAck);
		}
		else
		{
			TRACE_LOG(h, "[Protocol frame Tx] %s\n", cAck);
		}
		return 0;
	}
	return -1;
}

static int SendNak(TCR8HANDLE h, char cseq)
{
	/*static */ char cNak[8] = "<x1>";
	int len;

	if (!_IsValidHandle(h))
		return -2;

	if (_IsJiangsuProtocol(h))
	{
		cNak[1] = '1';
		cNak[2] = 'x';
	}

	if ('0' <= cseq && cseq <= '9')
	{
		if (_IsJiangsuProtocol(h))
		{
			cNak[2] = cseq;
		}
		else
		{
			cNak[1] = cseq;
		}

		len = strlen(cNak);

		if (_IsHuaNanKuaiSuProtocol(h))
		{
			/*TRACE_LOG( h, "[Note] 填充协议之前，协议长度： %d，协议内容： %s\n", len, cNak );*/
			len = FillXor_HNKS((unsigned char *)cNak, len);
			/*TRACE_LOG( h, "[Note] 填充协议之后，协议长度： %d，协议内容： %s\n", len, cNak );*/
		}
		TCR8SendData(h, cNak, len);
		TriggerOnProtocolPacket(h, TRUE, cNak);
		if (1)
		{
			TRACE_LOG(h, "[协议帧Tx] %s\n", cNak);
		}
		else
		{
			TRACE_LOG(h, "[Protocol frame Tx] %s\n", cNak);
		}
		return 0;
	}
	return -1;
}

static void AckedPacket(TCR8HANDLE h, int nSeq)
{
	int i;

	if (!_IsValidHandle(h))
		return;

	Mutex_Lock(h);
	if (0 <= nSeq && nSeq < 10 && h->_laneMsg[nSeq].msg_state == MST_WAITACK)
	{
		h->_laneMsg[nSeq].msg_state = MST_IDLE; /* free this slot */
	}
	else
	{
		if (1 /*IsChinese()*/)
		{
			TRACE_LOG(h, "[协议错误]: 收到ACK %d, 但是此序号的协议帧并没有等待应答\n", nSeq);
		}
		else
		{
			TRACE_LOG(h, "[Protocol error]: received ACK %d, but the protocol frame this number does not wait for a reply\n", nSeq);
		}
	}
	/* check for pending message queue to be sent */
	for (i = 0; i < 10; i++)
	{
		if (++nSeq > 9)
			nSeq = 0;
		if (h->_laneMsg[nSeq].msg_state == MST_WAITSEND)
		{
			char cmdCtrl;
			BOOL bWaitAck;

			if (_IsJiangsuProtocol(h))
			{
				cmdCtrl = h->_laneMsg[nSeq].msg_body[1];
			}
			else
			{
				cmdCtrl = h->_laneMsg[nSeq].msg_body[2];
			}

			bWaitAck = IsWaitAck(h, cmdCtrl);

			h->_laneMsg[nSeq].msg_state = bWaitAck ? MST_WAITACK : MST_IDLE;
			h->_laneMsg[nSeq].msg_naked = 0;

			TCR8SendData(h, h->_laneMsg[nSeq].msg_body, h->_laneMsg[nSeq].msg_len);

			h->_laneMsg[nSeq].t_resend = GetSystemTime64() + MST_TIMEOUT; // Next second to resend if not acked
			TriggerOnProtocolPacket(h, TRUE, h->_laneMsg[nSeq].msg_body);
			if (1 /*IsChinese()*/)
			{
				TRACE_LOG(h, "[协议帧Tx] %s\n", h->_laneMsg[nSeq].msg_body);
			}
			else
			{
				TRACE_LOG(h, "[Protocole frame Tx] %s\n", h->_laneMsg[nSeq].msg_body);
			}
			break;
		}
	}
	Mutex_Unlock(h);
}

static void NakedPacket(TCR8HANDLE h, int nSeq)
{
	if (!_IsValidHandle(h))
		return;

	Mutex_Lock(h);
	if (0 <= nSeq && nSeq < 10 && h->_laneMsg[nSeq].msg_state == MST_WAITACK)
	{
		if (++h->_laneMsg[nSeq].msg_naked > MST_NAKLIMIT)
		{
			if (ENABLE_ENGLISH)
			{
				TRACE_LOG(h, "[协议错误]: 协议帧%s连续被负应答%d, 不再发送!\n",
						  h->_laneMsg[nSeq].msg_body, MST_NAKLIMIT);
			}
			else
			{
				TRACE_LOG(h, "[Protocol error]: protocol frame %s is negatived continuously %d times, don't send again!\n",
						  h->_laneMsg[nSeq].msg_body, MST_NAKLIMIT);
			}
			// NAKed for too many times, assume acked and removed from queue
			Mutex_Unlock(h);
			AckedPacket(h, nSeq);
			Mutex_Lock(h);
		}
		else
		{
			TCR8SendData(h, h->_laneMsg[nSeq].msg_body, h->_laneMsg[nSeq].msg_len);
			h->_laneMsg[nSeq].t_resend = GetSystemTime64() + MST_TIMEOUT;
			TriggerOnProtocolPacket(h, TRUE, h->_laneMsg[nSeq].msg_body);
			TRACE_LOG(h, "[协议帧Tx] %s\n", h->_laneMsg[nSeq].msg_body);
		}
	}
	else
	{
		if (ENABLE_ENGLISH)
		{
			TRACE_LOG(h, "[协议错误]: 收到NAK %d, 但是此序号的协议帧并没有等待应答\n", nSeq);
		}
		else
		{
			TRACE_LOG(h, "[Protocol error]: received NAK %d, but the protocol frame this number does not wait for a reply\n", nSeq);
		}
	}
	Mutex_Unlock(h);
}

static BOOL IsValidPacket(TCR8HANDLE h, char *packet)
{
	static struct msg_codelen
	{
		int code;
		int len;
	} MsgCodeLen[] = {
		{'A', MSGLEN_PWRON},
		{'A', MSGLEN_PWRON_JS}, // 江苏协议
		{'B', MSGLEN_STATUS},
		{'B', MSGLEN_STATUS_JS}, // 江苏协议
		{'C', MSGLEN_EJECT},
		{'C', MSGLEN_EJECT_JS}, // 江苏协议
		{'D', MSGLEN_PBUTTON},
		{'D', MSGLEN_PBUTTON_JS}, // 江苏协议
		{'E', MSGLEN_TAKEN},
		{'E', MSGLEN_TAKEN_JS}, // 江苏协议
		{'F', MSGLEN_BOXINFEX},
		{'F', MSGLEH_BOXINF},
		{'F', MSGLEN_BOXINF1},
		{'F', 11},
		{'X', MSGLEN_BOXINF_HNKS},	// 华南快速协议
		{'X', MSGLEN_BOXINF1_HNKS}, // 华南快速协议
		{'G', MSGLEN_RECYELE},
		{'H', MSGLEN_IGNORE},
		{'I', MSGLEN_STATUS_EX},		 // 扩展机芯状态信息
		{'I', MSGLEN_COLLECT},			 // 收卡机收卡结果
		{'J', MSGLEN_EVENT_CARD_ON_ANT}, // 天线位置卡片变化事件
		{'K', MSGLEN_XBOXINF},			 // 多卡盒机芯卡盒信息
		{'K', 17},						 // 多卡盒机芯卡盒信息
		{'P', MSGLEN_PULLBACK},			 // 拉回卡口卡结果
		{'V', MSGLEN_VERSION},
		{'V', MSGLEN_VERSION1},
		{'W', 6},
		{'Y', 6},
		{'0', MSGLEN_ACK},
		{'1', MSGLEN_NAK},
		{'Y', 6}, // 废票回收帧长度
		{'Y', 6}, // 废票回收操作
		{'W', 6}, // 废票回收帧长度
		{'W', 6}, // 废票回收操作
	};
	int i, len = strlen(packet);
	const char *end = strchr(packet, MSG_ETX);
	if (end)
	{
		len = end - packet + 1;
		packet[len] = '\0'; // 补充0字节结尾，显示帧内容不会有多余的信息
	}
	char cmd;
	char packet_index;

	if (!_IsValidHandle(h))
		return FALSE;

	if ((h->m_iProtocol == -1) && (packet[0] == MSG_STX) && (packet[len - 1] == MSG_ETX))
	{
		if (('0' <= packet[1]) && (packet[1] <= '9') && ('A' <= packet[2]) && (packet[2] <= 'Z'))
		{
			for (i = 0; i < sizeof(MsgCodeLen) / sizeof(struct msg_codelen); i++)
			{
				if (MsgCodeLen[i].code == packet[2])
				{
					if (MsgCodeLen[i].len == len)
					{
						h->m_iProtocol = PROTOCOL_NORMAL;
						TRACE_LOG(h, "[Note] 使用普通协议\n");
					}
					else if ((MsgCodeLen[i].len == len - 1) && (packet[len - 2] == CalcXOR_HNKS((unsigned char *)(packet + 1), len - 3)))
					{
						TRACE_LOG(h, "[Note] 使用华南快速协议\n");
						h->m_iProtocol = PROTOCOL_HNKS;
					}
					break;
				}
			}
		}
		else if (('0' <= packet[2]) && (packet[2] <= '9') && ('A' <= packet[1]) && (packet[1] <= 'Z'))
		{
			h->m_iProtocol = PROTOCOL_JIANGSU;
			TRACE_LOG(h, "[Note] 使用江苏协议\n");
		}
	}
	if (h->m_iProtocol == -1)
	{
		TRACE_LOG(h, "[Note] 协议未知!\n");
	}

	cmd = _IsJiangsuProtocol(h) ? packet[1] : packet[2];
	packet_index = _IsJiangsuProtocol(h) ? packet[2] : packet[1];

	if (packet[0] == MSG_STX && packet[len - 1] == MSG_ETX && packet_index >= '0' && packet_index <= '9')
	{
		BOOL bLen;
		BOOL bXor = TRUE;

		for (i = 0; i < sizeof(MsgCodeLen) / sizeof(struct msg_codelen); i++)
		{
			// 只要格式对，都认为是合法帧，不校验帧长度了
			bLen = TRUE; // (!_IsHuaNanKuaiSuProtocol(h) && (MsgCodeLen[i].len == len)) || (_IsHuaNanKuaiSuProtocol(h) && (MsgCodeLen[i].len == len - 1));

			if (_IsHuaNanKuaiSuProtocol(h))
			{
				bXor = (packet[len - 2] == CalcXOR_HNKS((unsigned char *)(packet + 1), len - 3));
			}
			if (MsgCodeLen[i].code == cmd && bLen && bXor)
				return TRUE;
		}
	}
	return FALSE;
}

// Note - Caller shall hold the lock of mutex when use this function
static BOOL IsEmptyQueue(TCR8HANDLE h)
{
	int i;

	if (!_IsValidHandle(h))
		return FALSE;

	for (i = 0; i < 10; i++)
	{
		if (h->_laneMsg[i].msg_state != MST_IDLE)
			return FALSE;
	}
	return TRUE;
}

static int SendInitialPacket(TCR8HANDLE h)
{
	time_t t_now = time(NULL);
	struct tm *dt_now;
	char packet[32];

	if (!_IsValidHandle(h))
		return -2;

	dt_now = localtime(&t_now);
	if (t_now - h->t_lastInitial > 3) // Don't send initial packet again within 3 sec.
	{
		if (_IsJiangsuProtocol(h))
		{
			sprintf(packet, "<ax>");
		}
		else
		{
			if (h->m_nExtendProtocol == 2) // 二级扩展
			{
				sprintf(packet, "<xa902%04d%02d%02d%02d%02d%02d>",
						dt_now->tm_year + 1900, dt_now->tm_mon + 1, dt_now->tm_mday,
						dt_now->tm_hour, dt_now->tm_min, dt_now->tm_sec);
			}
			else // 默认使用一级扩展
			{
				sprintf(packet, "<xa901%04d%02d%02d%02d%02d%02d>",
						dt_now->tm_year + 1900, dt_now->tm_mon + 1, dt_now->tm_mday,
						dt_now->tm_hour, dt_now->tm_min, dt_now->tm_sec);
			}
		}
		//		m_tNextSync = timeGetEODTime( t_now );			// Next time to sync the system clock is End Of Day
		h->t_lastInitial = t_now;

		if (_IsJiangsuProtocol(h))
		{
			h->m_hexVersion = 1;
		}

		return SendPacket(h, packet);
	}
	return 0;
}

static int SendTimeSyncPacket(TCR8HANDLE h)
{
	time_t t_now = time(NULL);
	struct tm *dt_now;
	char packet[32];

	if (!_IsValidHandle(h))
		return -2;

	if (_IsHuaNanKuaiSuProtocol(h))
		return 0;

	dt_now = localtime(&t_now);

	sprintf(packet, "<xt%04d%02d%02d%02d%02d%02d>",
			dt_now->tm_year + 1900, dt_now->tm_mon + 1, dt_now->tm_mday,
			dt_now->tm_hour, dt_now->tm_min, dt_now->tm_sec);

	if (_IsJiangsuProtocol(h))
	{
		packet[1] = 't';
		packet[2] = 'x';
	}

	return SendPacket(h, packet);
}

static int GetTime2Resend(TCR8HANDLE h)
{
	int i, t2resend = 0;
	int64_t tmsNow;

	if (!_IsValidHandle(h))
		return -2;

	Mutex_Lock(h);
	tmsNow = GetSystemTime64();
	for (i = 0; i < 10; i++)
	{
		if (h->_laneMsg[i].msg_state == MST_WAITACK || h->_laneMsg[i].msg_state == MST_WAITSEND)
		{
			if (tmsNow >= h->_laneMsg[i].t_resend)
			{
				h->_laneMsg[i].msg_sndcnt++;
				TCR8SendData(h, h->_laneMsg[i].msg_body, h->_laneMsg[i].msg_len);
				h->_laneMsg[i].t_resend = tmsNow + MST_TIMEOUT;
				TriggerOnProtocolPacket(h, TRUE, h->_laneMsg[i].msg_body);
				if (ENABLE_ENGLISH)
				{
					TRACE_LOG(h, "[协议帧Tx] %s\n", h->_laneMsg[i].msg_body);
				}
				else
				{
					TRACE_LOG(h, "[Protocol Tx] %s\n", h->_laneMsg[i].msg_body);
				}
			}
			if (h->_laneMsg[i].msg_sndcnt > 2)
			{
				TRACE_LOG(h, "[%s] 发送三次未响应，抛弃！\r\n", h->_laneMsg[i].msg_body);
				h->_laneMsg[i].msg_state = MST_IDLE;
				h->_laneMsg[i].msg_naked = 0;
			}
			t2resend = (int)(h->_laneMsg[i].t_resend - tmsNow);
			break;
		}
	}
	Mutex_Unlock(h);
	return t2resend;
}

DLLAPI BOOL CALLTYPE TCR8_IsOnline(TCR8HANDLE h)
{
	if (!_IsValidHandle(h))
		return 0;
	return h->m_bOnline;
}

DLLAPI BOOL CALLTYPE TCR8_GetCartridgeSN(TCR8HANDLE h, int i)
{
	if (!_IsValidHandle(h))
		return 0;
	return (h)->m_nSerial[i];
}

DLLAPI BOOL CALLTYPE TCR8_IsCardOnAntenna(TCR8HANDLE tcr8, int i)
{
	if (!_IsValidHandle(tcr8))
		return 0;
	return (tcr8)->m_iState[i] & CSB_CSCONANT;
}

DLLAPI BOOL CALLTYPE TCR8_GetChannelCount(TCR8HANDLE tcr8, int i)
{
	if (!_IsValidHandle(tcr8))
		return 0;
	return (tcr8)->m_iCounter[i];
}

DLLAPI BOOL CALLTYPE TCR8_GetChannelState(TCR8HANDLE tcr8, int i)
{
	if (!_IsValidHandle(tcr8))
		return 0;
	return (tcr8)->m_iState[i];
}

DLLAPI BOOL CALLTYPE TCR8_GetActive(TCR8HANDLE tcr8, int n)
{
	if (!_IsValidHandle(tcr8))
		return 0;
	return (tcr8)->m_iActive[n];
}

DLLAPI BOOL CALLTYPE TCR8_HasCartridge(TCR8HANDLE tcr8, int i)
{
	if (!_IsValidHandle(tcr8))
		return 0;
	return ((tcr8)->m_iState[i] & CSB_NOBOX) == 0;
}

DLLAPI BOOL CALLTYPE TCR8_GetKernelVersion(TCR8HANDLE tcr8)
{
	if (!_IsValidHandle(tcr8))
		return 0;
	return (tcr8)->m_hexVersion;
}
