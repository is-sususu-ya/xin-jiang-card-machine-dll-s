// Enabler
#define	ENABLE_TCP_CONNECT
#define	ENABLE_LOG


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

 
#if defined _WIN32 || defined _WIN64
#include <windows.h>
#include <WinUser.h>
#include <io.h>
#include "../common/wintty.h"
#include "../common/winnet.h" 
#include <winsock.h>
#define TTY_GETC(fd,t)				tty_getc(fd,t)
#define MTRACE_LOG		 			trace_log
#define MLOG_INIT(dir,bf)		    (NULL)
#define MLOG_CLOSE(h)
#define MLOG_SETPATH(h,p)
#define MLOG_ENABLE(h,bEn)
#define THREAD_ZERO_VAL		NULL
static BOOL __gbEnableLog = 1;
#else		// linux
#include "../utils/utils_tty.h"
#include "../utils/utils_net.h" 
#include <pthread.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>
static int __gbEnableLog = 1; 
#define MTRACE_LOG					trace_log
#define MLOG_INIT(dir,bf)		    (NULL)
#define MLOG_CLOSE(h)
#define MLOG_SETPATH(h,p)
#define MLOG_ENABLE(h,bEn)
#define winsock_startup()				(0)
#define winsock_cleanup()				(0) 
#define TTY_GETC(fd,t)					tty_getc(fd,t,0)
#define Sleep(n)						usleep( (n)*1000 )
#define THREAD_ZERO_VAL		0
#endif

#include "TFI.h"

#define	CMD_GET_INFO		0x02
#define	CMD_ALRAM_CTL		0x03
#define	CMD_CLEAR			0x04
#define	CMD_SET_MODE		0x05
#define	CMD_SHOW_LINE		0x20
#define CMD_SET_PICTURE		0x21
#define	CMD_SET_PASSLIGHT	0x33
#define	CMD_PIP_DISPLAY		0x24
#define	CMD_PIP_CLEAR		0x25

#define	MODE_ROW	1
#define	MODE_PIC	2
#define	MODE_MIX	3
#define	MODE_PAGE	4

#define ALLIN_LEFT 0
#define ALLIN_MIDDLE 1
#define ALLIN_RIGHT 2
#define ROLL_RIGHT_TO_LEFT 3
#define ROLL_LEFT_TO_RIGHT 4
#define ROLL_RIGHT_TO_LEFT_CIRCLE 5
#define ROLL_LEFT_TO_RIGHT_CIRCLE 6
#define Format(n, nRoll, nBlinktime, nRolltime) n = nRoll & 0x0f + (nBlinktime & 0x03) << 6 + nRolltime

#define PORT_SLAVE_UDP	8016
#define UDP_LOCAL_PORT	9000
#define	TCP_TYPE	2
#define	UDP_TYPE	1
#define	TTY_TYPE	0
typedef unsigned short	UInt16;  
typedef	unsigned char	UInt8;
   
#ifdef linux
static void *ProtocolThread(void* lpParameter);
#else
static DWORD WINAPI ProtocolThread(void* lpParameter);
DWORD ThreadId; 
#endif 
  
typedef struct 
{	
	int  size;
	int InterfaceType; 
	int bQuit;
#ifdef linux
	pthread_t	hThread;
	pthread_mutex_t hMutex;
	struct termios  termio_save;
#else
	HANDLE hThread;
	HANDLE hMutex;
	HWND	hWnd;
	int		nMsgNo; 
#endif
	HANDLE hLog;
	BOOL bLogEnable; 
	int fd;
	int nCOM; 
	int  nPort;
	char chip[20];
	DWORD m_DWIP; 
	char chModel[12];
	int nVersion;
	int nColor;	 
	int nDeepth; 
	int nLineH; 
	int nPixWitdh; 
	int nPixHeight; 
	int nCapability;
	TFI_CallBack ca;
} LENFONTCARD,*TFI_STATURE;

#ifdef WIN32
#define Mutex_Lock(h)			WaitForSingleObject( h->hMutex, INFINITE )
#define Mutex_Unlock(h)			ReleaseMutex(  h->hMutex )
#else
#define Mutex_Lock(h)			pthread_mutex_lock( & h->hMutex )
#define Mutex_Unlock(h)		pthread_mutex_unlock( & h->hMutex )
#endif
 
//#define	ENABLE_USE_CP5200
#define _IsValidHandle(h)		( (h)!=NULL && ( (LENFONTCARD *)h)->size == sizeof(LENFONTCARD) )

static int SendPacket(LENFONTCARD *h, char* msg, int len);
static int GetPacket(LENFONTCARD *h,char* buf, int len, int cmd, int tout);
static int ProcessCommand(LENFONTCARD *h, char* msg, int len,char *buf, int rlen);
#ifdef linux
static void *TFI_reconnect(void *lpParameter);
#define THREAD_RET	NULL
#else
static DWORD WINAPI TFI_reconnect(LPVOID lpParameter);
#define THREAD_RET	0
#endif

static char LOG_FILE[256] = { "./TFI.log" };
static char BKUP_FILE[256] = {"./TFI.logbak"};
#define MAX_LOGSIZE 1024000

static const char *time_stamp()
{ 
#ifdef linux
	static char timestr[64];
	char *p = timestr;
	struct timeval tv;
	gettimeofday( &tv, NULL ); 
	strftime( p, sizeof(timestr), "%y/%m/%d %H:%M:%S", localtime( &tv.tv_sec ) );
	sprintf( p + strlen(p), ".%03lu", tv.tv_usec/1000 );

#else
	static TCHAR timestr[64];
	SYSTEMTIME	tnow;
	GetLocalTime( &tnow );
	_stprintf( timestr, _T("%04d/%02d/%02d %02d:%02d:%02d.%03d"), 
				tnow.wYear, tnow.wMonth, tnow.wDay, 
				tnow.wHour, tnow.wMinute, tnow.wSecond, tnow.wMilliseconds );
#endif
	return timestr;
}

static void trace_log(HANDLE hObj, const char *fmt,...)
{
#ifdef ENABLE_LOG
	va_list		va;
	char		str[1024] = ""; 
	FILE *fp; 

	va_start(va, fmt);
	vsprintf(str, fmt, va);
	va_end(va);
 
	fp = fopen(LOG_FILE, "a");
	if (fp != NULL)
	{
		if (fmt[0] != '\t')
			fprintf(fp, "[%s] %s ", time_stamp(), str);
		else
			fprintf(fp, "%26s%s", " ", str + 1);
		printf( str );
	}
	if (fp != NULL && ftell(fp) > MAX_LOGSIZE)
	{
		fclose(fp);
		remove(BKUP_FILE);
		rename(LOG_FILE, BKUP_FILE);
		fp = fopen(LOG_FILE, "a");
	}
	if (fp != NULL)
		fclose(fp);
#else
#endif
}
  
static int HasInit = 0;

unsigned int get_thread_id()
{
#ifdef linux
	return pthread_self();
#else
	return GetCurrentThreadId();
#endif
}

#ifdef linux 
unsigned long GetTickCount()
{
	static signed long long begin_time = 0;
	static signed long long now_time;
	struct timespec tp;
	unsigned long tmsec = 0;
	if (clock_gettime(CLOCK_MONOTONIC, &tp) != -1)
	{
		now_time = tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
	}
	if (begin_time == 0)
		begin_time = now_time;
	tmsec = (unsigned long)(now_time - begin_time);
	return tmsec;
}
#endif

static void u16bcd( WORD u16Value, BYTE *bcd )
{
	BYTE nibble;

	nibble = (BYTE)( u16Value / 100 );
	bcd[0] = ((nibble/10) << 4) + (nibble%10);
	u16Value %= 100;
	nibble = (BYTE)u16Value;
	bcd[1] = ((nibble/10) << 4) + (nibble%10);
}

static WORD bcd2u16( BYTE *bcd )
{
	int i = 2;
	WORD u16 = 0;

	for( ;i--; )
	{
		u16 *= 100;
		u16 += (((*bcd) & 0xf0) >> 4 ) * 10 + ((*bcd) & 0x0f);
		bcd++;
	}
 	return u16;
}  

static BYTE CalculateXOR( BYTE xor, BYTE *param, int len )
{
	for( ;len--; )
		xor ^= *(param++);
	return xor;
}

static void ParseDeviceInfo(LENFONTCARD *h, char buf[])
{
	memcpy(h->chModel, &buf[7], 8);
	h->chModel[8] = '\0';
	h->nVersion = (((int)buf[15]) << 8) + buf[16];
	h->nPixWitdh = (((int)buf[17]) << 8) + buf[18];
	h->nPixHeight = (((int)buf[19]) << 8) + buf[20];
	h->nColor = buf[21];
	h->nDeepth = buf[22];
	h->nLineH = buf[23];
	h->nCapability = (((int)buf[24]) << 8) + buf[25];
	MTRACE_LOG(h->hLog, "--> Device Info: Model=%s, Version=%02x.%02x, Color=%d, Deepth=%d, LineH=%d, Resolution=%d x %d, Capability=0x%x\n",\
			h->chModel, h->nVersion>>8, h->nVersion & 0xff, h->nColor, h->nDeepth, h->nLineH, h->nPixWitdh, h->nPixHeight, h->nCapability );
}

_API_EXPORT HANDLE CALLING_CONV TFI_Create()
{
	LENFONTCARD *h;
	h = (LENFONTCARD*)malloc(sizeof(LENFONTCARD));
	memset(h,0,sizeof(LENFONTCARD));
	h->size = sizeof(LENFONTCARD);
	h->bLogEnable = __gbEnableLog;
	h->ca = NULL;
	return (HANDLE)h;
}

_API_EXPORT BOOL CALLING_CONV TFI_Destroy(HANDLE h)
{
	LENFONTCARD *obj = (LENFONTCARD *)h;
	if (_IsValidHandle(obj))
	{
		free(obj);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

#ifdef linux
_API_EXPORT BOOL CALLING_CONV TFI_OpenCom (HANDLE h, const char *tty_device)
#else
_API_EXPORT BOOL CALLING_CONV TFI_OpenCom (HANDLE h, int iComID)
#endif
{
	char buf[30];
	LENFONTCARD *pTFI = (LENFONTCARD *)h;
	int tty;
	char log_buf[40] = "";
#ifdef linux
	int  iComID = 0;
#endif 
	sprintf(log_buf,"TFI_SP32_COM%d",iComID);
	if ( !_IsValidHandle(h) )
	{
		return FALSE;
	}
	if ( pTFI->bLogEnable )
		pTFI->hLog = MLOG_INIT(NULL, log_buf);

#ifndef linux
	MTRACE_LOG(pTFI->hLog, "TFI_OpenCom on COM%d \n", iComID);
	tty = tty_openPort(iComID);
#else
	tty = open(tty_device, O_RDWR);
#endif
	if(tty == -1)
	{
		MTRACE_LOG(pTFI->hLog, "--> COM%d  Open Failed \n",iComID);
		return FALSE;
	}
	else
	{
		MTRACE_LOG(pTFI->hLog, "-->  COM%d  Open Successed!\n",iComID);
#ifdef linux
		tty_raw(tty, &pTFI->termio_save, 9600);
#else
		tty_raw( tty, 9600, 8, PRTY_NONE );
		tty_setReadTimeouts( tty, 200, 1, 100 );
#endif
	}
	pTFI->fd = tty;
	pTFI->nCOM = iComID;
	pTFI->InterfaceType = TTY_TYPE;
#if defined _WIN32 || defined _WIN64
	pTFI->hMutex = CreateMutex(
		NULL,				// default security attributes
		FALSE,				// initially not owned
		NULL);				// mutex name (NULL for unname mutex)
#else
	pthread_mutex_init(&pTFI->hMutex, NULL);
#endif

#ifdef linux
	if (0 == pthread_create(&pTFI->hThread, NULL, ProtocolThread, pTFI))
		MTRACE_LOG(pTFI->hLog, "--> ProtocolThread create sucess\n");
	else
		MTRACE_LOG(pTFI->hLog, "--> ProtocolThread create failed \n");
#else
	MTRACE_LOG(pTFI->hLog,  "TFI_OpenCom success!\n");
	pTFI->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ProtocolThread, h, NULL, &ThreadId);
	if (pTFI->hThread)
		MTRACE_LOG(pTFI->hLog, "--> ProtocolThread create sucess\n"); 
	else
		MTRACE_LOG(pTFI->hLog, "--> ProtocolThread create failed \n");
#endif
	return TRUE;
}

_API_EXPORT BOOL CALLING_CONV TFI_OpenNet(HANDLE h, const char *chIP)
{
	LENFONTCARD *pTFI = (LENFONTCARD *)h;
	DWORD ip_hex;
	char buf[30];
	SOCKET fd;
	char log_buf[40];
    strcpy(pTFI->chip,chIP);
	if ((ip_hex = inet_addr(chIP)) == INADDR_NONE || !_IsValidHandle(h))
		return FALSE;
	if (pTFI->bLogEnable)
	{
		sprintf(log_buf, "TFI_SP32_%s", chIP);
		pTFI->hLog = MLOG_INIT(NULL, log_buf);
	}
	MTRACE_LOG(pTFI->hLog, "TFI_OpenNet   IP addr =  %s \n", chIP);
	if (winsock_startup() != 0)
	{
		MTRACE_LOG(pTFI->hLog, "--> WINSOCK library initialization failed!\n");
		return FALSE;
	}  
	pTFI->fd = INVALID_SOCKET;
	pTFI->nPort = PORT_SLAVE_UDP;
	pTFI->m_DWIP = ip_hex;
	pTFI->InterfaceType = TCP_TYPE;
#if defined _WIN32 || defined _WIN64
	pTFI->hMutex = CreateMutex(
		NULL,				// default security attributes
		FALSE,				// initially not owned
		NULL);				// mutex name (NULL for unname mutex)
#else
	pthread_mutex_init(&pTFI->hMutex, NULL);
#endif

#ifdef linux
	if (0 == pthread_create(&pTFI->hThread, NULL, ProtocolThread, pTFI))
		MTRACE_LOG(pTFI->hLog, "--> ProtocolThread create sucess\n"); 
	else
		MTRACE_LOG(pTFI->hLog, "--> ProtocolThread create failed \n");
#else
	MTRACE_LOG(pTFI->hLog,  "TFI_OpenNet open success!\n");
	pTFI->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ProtocolThread, h, NULL, &ThreadId);
	if (pTFI->hThread)
		MTRACE_LOG(pTFI->hLog, "--> ProtocolThread create sucess\n"); 
	else
		MTRACE_LOG(pTFI->hLog, "--> ProtocolThread create failed \n");
#endif
	return TRUE;
}

_API_EXPORT BOOL CALLING_CONV TFI_Close(HANDLE h) 
{
	LENFONTCARD *pTFI = (LENFONTCARD*)h;

	if(!_IsValidHandle(h))
	{
		return FALSE;
	} 
	MTRACE_LOG(pTFI->hLog,  "TFI_Close \r\n");
	pTFI->bQuit = 1;

	MTRACE_LOG(pTFI->hLog, "TFI_Close Close Log\n");

#if defined _WIN32 || defined _WIN64
	if (pTFI->hMutex)
		CloseHandle(pTFI->hMutex);
#else
	pthread_mutex_destroy(&pTFI->hMutex);
#endif
	if ((int)pTFI->hThread != 0)
	{
#ifdef linux
		pthread_cancel(pTFI->hThread);
		pthread_join(pTFI->hThread, NULL);
		pthread_mutex_destroy(&pTFI->hMutex);
#else
		TerminateThread(pTFI->hThread, 0);
#endif
	}
	if (pTFI->InterfaceType == TTY_TYPE)
	{
#ifdef linux
		tty_set(pTFI->fd, &pTFI->termio_save);
		close(pTFI->fd);
#else
		tty_close( pTFI->fd );
#endif
	}
	else
	{
		MTRACE_LOG(pTFI->hLog, "TFI_Close Close Socket\n");
		if (pTFI->fd > 0)
			sock_close(pTFI->fd);
#ifdef linux
#else
		MTRACE_LOG(pTFI->hLog, "TFI_Close Clean Socket\n");
		winsock_cleanup();
#endif
	}
	MTRACE_LOG(pTFI->hLog, "TFI_Close Finish\n");
	MLOG_CLOSE(pTFI->hLog);
	return TRUE;
}

_API_EXPORT BOOL CALLING_CONV  TFI_ClearAll(HANDLE h)
{
	LENFONTCARD *pTFI = (LENFONTCARD*)h;
	unsigned char dat[30];
	unsigned char *ptr = dat;
	unsigned char xor;
	unsigned char buf[20];
	int res;
	if(!_IsValidHandle(h))
		return FALSE;
	MTRACE_LOG(pTFI->hLog,  "TFI_ClearAll\n");
	*ptr++ = 0xa0;
	*ptr++ = 0xb0;
	*ptr++ = 0xc0;
	*ptr++ = 0xd0;
	*ptr++ = CMD_CLEAR;
	*ptr++ = 0;
	*ptr++ = 0;
	xor = CalculateXOR(0,dat, (int)(ptr-dat));
	*ptr++ = xor;

	res = ProcessCommand((LENFONTCARD *)h,(char*)dat, (int)(ptr-dat),(char*)buf,sizeof(buf));
	return res > 0;
}

_API_EXPORT BOOL CALLING_CONV TFI_GetDeviceInfo(HANDLE h, char *pInfo)
{
	LENFONTCARD *pTFI = (LENFONTCARD *)h;
	unsigned char dat[30];
	unsigned char *ptr = dat;
	unsigned char xor ;
	unsigned char buf[30];
	int res;

	if (!_IsValidHandle(h))
	{
		return FALSE;
	}
	*ptr++ = 0xa0;
	*ptr++ = 0xb0;
	*ptr++ = 0xc0;
	*ptr++ = 0xd0;
	*ptr++ = CMD_GET_INFO;
	*ptr++ = 0;
	*ptr++ = 0;
	xor = CalculateXOR(0, dat, (int)(ptr - dat));
	*ptr++ = xor;

	res = ProcessCommand(pTFI, (char *)dat, (int)(ptr - dat), (char *)buf, sizeof(buf));
	if (res)
	{
		memcpy((char *)pInfo, (char *)&buf[0], 26);
	}
	else
	{
		MTRACE_LOG(pTFI->hLog, "TFI_GetDeviceInfo --> FAILURE\n");
	}
	return res > 0;
}

_API_EXPORT BOOL CALLING_CONV TFI_SendLine(HANDLE h, int nLineNo, COLORREF nColor, int nRollType, int nRolltime, const char *szText)
{
	LENFONTCARD *pTFI = (LENFONTCARD *)h;
	int res;
	unsigned char *head, *dat;
	unsigned char *ptr = NULL;
	unsigned char xor ;
	int len = 20 + (int)strlen(szText);
	unsigned char buf[20];
	if (!_IsValidHandle(h))
	{
		return FALSE;
	}
	MTRACE_LOG(pTFI->hLog, "TFI_SendLine : line# = %d color = 0x%x nRollType = %d  nRolltime = %d text = %s \n",
			   nLineNo, nColor, nRollType, nRolltime, szText == NULL ? "(null)" : szText);
	if (szText == NULL || nLineNo <= 0)
	{
		MTRACE_LOG(pTFI->hLog, "--> nLineNo and/or szText\n");
		return FALSE;
	}
	head = dat = (unsigned char *)malloc(len);
	if (head == NULL)
	{
		MTRACE_LOG(pTFI->hLog, "--> out of heap memory, malloc failed!\n");
		return FALSE;
	}
	memset(head, 0, len);
	ptr = dat;
	*ptr++ = 0xa0;
	*ptr++ = 0xb0;
	*ptr++ = 0xc0;
	*ptr++ = 0xd0;
	*ptr++ = CMD_SHOW_LINE;
	u16bcd((WORD)(strlen(szText) + 1 + 3 + 1), ptr);
	ptr += 2;
	*ptr++ = nLineNo;
	*ptr++ = (nRollType & 0x0f) | ((nRolltime & 0x03) << 6); //format_roll&0x0f + (blink_time&0x03)<<4 + (roll_time&0x03)<<6;
	*ptr++ = (unsigned char)(nColor & 0x000000ff);
	*ptr++ = (unsigned char)((nColor & 0x0000ff00) >> 8);
	*ptr++ = (unsigned char)((nColor & 0x00ff0000) >> 16);
	strcpy((char *)ptr, (char *)szText);
	ptr += strlen(szText);
	xor = CalculateXOR(0, dat, (int)(ptr - dat));
	*ptr++ = xor;
	res = ProcessCommand((LENFONTCARD *)h, (char *)dat, (int)(ptr - dat), (char *)buf, sizeof(buf));
	free(head);
	return res > 0;
}

_API_EXPORT BOOL CALLING_CONV TFI_SetMode(HANDLE h, BYTE mode, char *pRowType)
{
	LENFONTCARD *pTFI = (LENFONTCARD *)h;
	unsigned char dat[20] = {0xa0, 0xb0, 0xc0, 0xd0, 0x06, 0x01, 0x01, 0xe0};
	unsigned char *ptr = dat;
	unsigned char xor ;
	unsigned char buf[20];
	int res; 
	if (!_IsValidHandle(h))
	{ 
		return FALSE;
	} 
	*ptr++ = 0xa0;
	*ptr++ = 0xb0;
	*ptr++ = 0xc0;
	*ptr++ = 0xd0;
	*ptr++ = CMD_SET_MODE;
	*ptr++ = 0;
	*ptr++ = 9;
	*ptr++ = mode;
	memcpy((char *)ptr, (char *)pRowType, 8);
	ptr += 8;
	xor = CalculateXOR(0, dat, (int)(ptr - dat));
	*ptr++ = xor;
	MTRACE_LOG(pTFI->hLog, "TFI_SetMode mode=%d RowType[]={%d %d %d %d %d %d %d %d}\n",
			   mode, *(pRowType + 0), *(pRowType + 1), *(pRowType + 2), *(pRowType + 3), *(pRowType + 4), *(pRowType + 5), *(pRowType + 6), *(pRowType + 7));
	res = ProcessCommand((LENFONTCARD *)h, (char *)dat, (int)(ptr - dat), (char *)buf, sizeof(buf));
	if (res > 0)
	{
		MTRACE_LOG(pTFI->hLog, "--> SUCCESS\n");
	}
	else
	{
		MTRACE_LOG(pTFI->hLog, "--> FAILED!!!\n");
	}
	return res > 0;
}

_API_EXPORT BOOL CALLING_CONV TFI_AlarmSet(HANDLE h, BYTE bAlarm, BYTE nTime)
{
	LENFONTCARD *pTFI = (LENFONTCARD *)h;
	unsigned char dat[10] = {0xa0, 0xb0, 0xc0, 0xd0, 0x06, 0x01, 0x01, 0xe0};
	unsigned char *ptr = dat;
	unsigned char xor ;
	unsigned char buf[20];
	int res;
	if (!_IsValidHandle(h))
	{
		return FALSE;
	} 
	MTRACE_LOG(pTFI->hLog, "TFI_AlarmSet ALARM=%d, TIME=%d sec\n", bAlarm, nTime);
	*ptr++ = 0xa0;
	*ptr++ = 0xb0;
	*ptr++ = 0xc0;
	*ptr++ = 0xd0;
	*ptr++ = CMD_ALRAM_CTL;
	*ptr++ = 0;
	*ptr++ = 2;
	*ptr++ = bAlarm;
	*ptr++ = nTime % 4;
	xor = CalculateXOR(0, dat, (int)(ptr - dat));
	*ptr++ = xor;
	res = ProcessCommand((LENFONTCARD *)h, (char *)dat, (int)(ptr - dat), (char *)buf, sizeof(buf));
	if (res > 0)
	{
		MTRACE_LOG(pTFI->hLog, "--> SUCCESS\n");
	}
	else
	{
		MTRACE_LOG(pTFI->hLog, "--> FAILED!!!\n");
	}
	return res > 0;

}

_API_EXPORT BOOL CALLING_CONV TFI_PlayDu(HANDLE h,int vol)
{
	LENFONTCARD *pTFI = (LENFONTCARD *)h;
	int res = 0;
    unsigned char dat[40] = {"[v3][x1]sound123"};
	dat[2] = vol + '0';
	dat[2] = (dat[2] < '0' || dat[2] > '8') ? '3' : dat[2];
	char buffer[129] = {0};
	buffer[0] = '<';
	buffer[1] = 'A';
	strcpy( buffer + 2, dat );
	int len = strlen( buffer );
	buffer[len] = '>';
    res = SendPacket((LENFONTCARD *)h,(char*)buffer,len + 1);
	if (res > 0)
	{
		MTRACE_LOG(pTFI->hLog, "--> SUCCESS\n");
	}
	else
	{
		MTRACE_LOG(pTFI->hLog, "--> FAILED!!!\n");
	}
    return res>0;
}

_API_EXPORT BOOL CALLING_CONV TFI_SendVoice(HANDLE h, int vul, const char *stVoice)
{
	char dat[1024] = {0x3c, 'A', '[', 'v' , '3', ']', '[', 's' , '5', ']', '[', 't', '5', ']', '[', 'm' , '3', ']'};
	dat[4] = '0' + ( ( vul < 0 || vul > 7 ) ? 3 : vul );
	LENFONTCARD *pTFI = (LENFONTCARD *)h;

	char *ptr = dat + 18;
	int res;

	if (!_IsValidHandle(h))
	{
		return FALSE;
	}
	MTRACE_LOG(pTFI->hLog, "TFI_SendVoice text to speach=[%s]\n", stVoice);
	if (strlen(stVoice) > sizeof(dat) - 24)
	{
		MTRACE_LOG(pTFI->hLog, "==> Failure \n");
		return FALSE;
	}
	strcpy((char *)ptr, (char *)stVoice);
	ptr += strlen(stVoice);
	*ptr++ = 0x00;
	*ptr++ = 0x3e;
	res = SendPacket((LENFONTCARD *)h, (char *)dat, (int)(ptr - dat));
	return res > 0;
}

_API_EXPORT BOOL CALLING_CONV TFI_EnableLog(HANDLE h, BOOL bEnable)
{
	LENFONTCARD *pTFI = (LENFONTCARD *)h;

	if (h == NULL)
	{
		__gbEnableLog = bEnable;
	}
	else if (_IsValidHandle(h))
	{
		pTFI->bLogEnable = bEnable;
		if (pTFI->hLog)
			MLOG_ENABLE(pTFI->hLog, bEnable);
	}
	else
		return FALSE;
	return TRUE;
}
_API_EXPORT BOOL CALLING_CONV TFI_ClearPipMode(HANDLE h, BYTE nLineNo)
{
	int res;
	unsigned char *ptr = NULL;
	unsigned char xor ;
	unsigned char dat[20];
	unsigned char buf[20];
	LENFONTCARD *pTFI = (LENFONTCARD *)h;
	ptr = dat;
	if (!_IsValidHandle(h))
	{
		MTRACE_LOG(pTFI->hLog, "TFI_CLEAR_PIP_MODE - Invalid Handle\n");
		return FALSE;
	}
	MTRACE_LOG(pTFI->hLog, "TFI_CLEAR_PIP_MODE : line# = %d \n", nLineNo);
	if (nLineNo < 0)
	{
		MTRACE_LOG(pTFI->hLog, "--> nLineNo \n");
		return FALSE;
	}
	*ptr++ = 0xa0;
	*ptr++ = 0xb0;
	*ptr++ = 0xc0;
	*ptr++ = 0xd0;
	*ptr++ = CMD_PIP_CLEAR;
	*ptr++ = 0;
	*ptr++ = 1;
	*ptr++ = nLineNo;
	xor = CalculateXOR(0, dat, ptr - dat);
	*ptr++ = xor;
	res = ProcessCommand((LENFONTCARD *)h, (char *)dat, ptr - dat, (char *)buf, sizeof(buf));
	if (res > 0)
	{
		MTRACE_LOG(pTFI->hLog, "--> SUCCESS\n");
	}
	else
	{
		MTRACE_LOG(pTFI->hLog, "--> FAILED!!!\n");
	}
	return res > 0;
}
_API_EXPORT BOOL CALLING_CONV TFI_DisplayPipMode(HANDLE h, BYTE nLineNo, SHORT x, SHORT y, BYTE lineH, COLORREF nColor, const char *szText)
{
	int res;
	unsigned char *head, *dat;
	unsigned char *ptr = NULL;
	unsigned char xor ;
	unsigned char buf[20];
	LENFONTCARD *pTFI = (LENFONTCARD *)h;
	if (!_IsValidHandle(h))
	{
		MTRACE_LOG(pTFI->hLog, "TFI_DISPLAY_PIP_MODE - Invalid Handle\n");
		return FALSE;
	}
	MTRACE_LOG(pTFI->hLog, "TFI_DISPLAY_PIP_MODE : line# = %d color = 0x%x x = %d  y = %d lineH = %d text = %s \n",
			   nLineNo, nColor, x, y, lineH, szText == NULL ? "(null)" : szText);
	if (szText == NULL || nLineNo <= 0)
	{
		MTRACE_LOG(pTFI->hLog, "-->??°é?? nLineNo and/or szText\n");
		return FALSE;
	}
	head = dat = (unsigned char *)malloc(20 + strlen(szText));
	if (head == NULL)
	{
		MTRACE_LOG(pTFI->hLog, "--> out of heap memory, malloc failed!\n");
		return FALSE;
	}
	memset(head, 0, 20 + strlen(szText));
	ptr = dat;
	*ptr++ = 0xa0;
	*ptr++ = 0xb0;
	*ptr++ = 0xc0;
	*ptr++ = 0xd0;
	*ptr++ = CMD_PIP_DISPLAY;
	u16bcd((WORD)(strlen(szText) + 1 + 3 + 1 + 4), ptr);
	ptr += 2;
	*ptr++ = nLineNo;
	*ptr++ = (char)((x & 0xff00) >> 8);
	*ptr++ = (char)((x & 0x00ff));
	*ptr++ = (char)((y & 0xff00) >> 8);
	*ptr++ = (char)((y & 0x00ff));
	*ptr++ = lineH;
	*ptr++ = (unsigned char)(nColor & 0x000000ff);
	*ptr++ = (unsigned char)((nColor & 0x0000ff00) >> 8);
	*ptr++ = (unsigned char)((nColor & 0x00ff0000) >> 16);
	//	bcopy((char*)ptr,(char*)szText,strlen(szText));
	memcpy((char *)ptr, (char *)szText, strlen(szText));
	ptr += strlen(szText);
	xor = CalculateXOR(0, dat, ptr - dat);
	*ptr++ = xor;
	res = ProcessCommand((LENFONTCARD *)h, (char *)dat, ptr - dat, (char *)buf, sizeof(buf));
	free(head);
	if (res > 0)
	{
		MTRACE_LOG(pTFI->hLog, "--> SUCCESS\n");
	}
	else
	{
		MTRACE_LOG(pTFI->hLog, "--> FAILED!!!\n");
	}
	return res > 0;
}


_API_EXPORT BOOL CALLING_CONV TFI_SetPicture(HANDLE h,COLORREF nColor, int x,int bOpen)
{
	LENFONTCARD *pTFI = (LENFONTCARD *)h;
	unsigned char dat[20] = {0xa0, 0xb0, 0xc0, 0xd0, 0x06, 0x01, 0x01, 0xe0};
	unsigned char *ptr = dat;
	unsigned char xor ;
	unsigned char buf[20];
	int res;

	if (!_IsValidHandle(h))
	{
		//TRACE_LOG("TFI_AlarmSet - Invalid Handle\n");
		return FALSE;
	}

	MTRACE_LOG(pTFI->hLog, "[TFI_SetPicture bOpen=%d, x=%d\n", bOpen, x);
	*ptr++ = 0xa0;
	*ptr++ = 0xb0;
	*ptr++ = 0xc0;
	*ptr++ = 0xd0;
	*ptr++ = CMD_SET_PICTURE;
	*ptr++ = 0;
	*ptr++ = 5;
	*ptr++ = bOpen;
	*ptr++ = ( 0xff & x );
	*ptr++ = (unsigned char)(nColor & 0x0000ff);
	*ptr++ = (unsigned char)((nColor & 0x00ff00) >> 8);
	*ptr++ = (unsigned char)((nColor & 0xff0000)>>16);
	xor = CalculateXOR(0, dat, (int)(ptr - dat));
	*ptr++ = xor;
	res = ProcessCommand((LENFONTCARD *)h, (char *)dat, (int)(ptr - dat), (char *)buf, sizeof(buf));
	if (res > 0)
	{
		MTRACE_LOG(pTFI->hLog, "--> SUCCESS\n");
	}
	else
	{
		MTRACE_LOG(pTFI->hLog, "--> FAILED!!!\n");
	}
	return 0; 
}
 
_API_EXPORT BOOL CALLING_CONV TFI_SetPassLight(HANDLE h, int x, int y, BYTE bPass)
{
	LENFONTCARD *pTFI = (LENFONTCARD *)h;
	unsigned char dat[20] = {0xa0, 0xb0, 0xc0, 0xd0, 0x06, 0x01, 0x01, 0xe0};
	unsigned char *ptr = dat;
	unsigned char xor ;
	unsigned char buf[20];
	int res;
	if (!_IsValidHandle(h))
	{
		//TRACE_LOG("TFI_AlarmSet - Invalid Handle\n");
		return FALSE;
	}
	MTRACE_LOG(pTFI->hLog, "TFI_SetPassLight bPass=%d, x=%d y=%d\n", bPass, x, y);
	*ptr++ = 0xa0;
	*ptr++ = 0xb0;
	*ptr++ = 0xc0;
	*ptr++ = 0xd0;
	*ptr++ = CMD_SET_PASSLIGHT;
	*ptr++ = 0;
	*ptr++ = 5;
	*ptr++ = (unsigned char)((x & 0xff00) >> 8);
	*ptr++ = (unsigned char)((x & 0x00ff));
	*ptr++ = (unsigned char)((y & 0xff00) >> 8);
	*ptr++ = (unsigned char)((y & 0x00ff));
	*ptr++ = bPass;
	xor = CalculateXOR(0, dat, (int)(ptr - dat));
	*ptr++ = xor;
	res = ProcessCommand((LENFONTCARD *)h, (char *)dat, (int)(ptr - dat), (char *)buf, sizeof(buf));
	if (res > 0)
	{
		MTRACE_LOG(pTFI->hLog, "--> SUCCESS\n");
	}
	else
	{
		MTRACE_LOG(pTFI->hLog, "--> FAILED!!!\n");
	}
	return res > 0;
} 
_API_EXPORT BOOL CALLING_CONV TFI_SetLogPath(HANDLE h, const char *Path)
{
	LENFONTCARD *pTFI = (LENFONTCARD *)h;
	if (h == NULL)
		MLOG_SETPATH(h, Path); //  
	else if (_IsValidHandle(h))
		MLOG_SETPATH(pTFI->hLog, Path);
	else
		return FALSE;
	return TRUE;
}

////////////////////////////////////////////////////

static const char *show_hex(const char *ch, int rlen)
{
    int i = 0;
    char *ptr = (char *)ch;
    static char buf[640];
    char *off = buf;
    unsigned char val;
    memset(buf, 0, sizeof(buf));
    int len = rlen > 200 ? 200 : rlen;
    for (i = 0; i < len; i++)
    {
        val = *ptr++;
        sprintf(off, "%02x, ", val);
        off += 3;
    }
    *(off - 1) = '\0';
    return buf;
}

static int ProcessCommand(LENFONTCARD *pTFI, char *msg, int len, char *buf, int rlen)
{
	SendPacket(pTFI, msg, len);
	return TRUE;
}
 
static char QR_code_buf[256];

_API_EXPORT BOOL CALLING_CONV TFI_SetCallBack(HANDLE h,TFI_CallBack cb)
{
	LENFONTCARD *pTFI = (LENFONTCARD *)h;
	if (!_IsValidHandle(pTFI))
	{
		return FALSE;
	}
	pTFI->ca = cb;
	return TRUE;
}

#if defined _WIN32 || defined _WIN64
_API_EXPORT BOOL CALLING_CONV TFI_SetWinMsg(HANDLE h, HWND hwnd, int msgno)
{
	LENFONTCARD *pTFI = (LENFONTCARD *)h;
	if (!_IsValidHandle(pTFI))
	{
		return FALSE;
	}
	pTFI->hWnd = hwnd;
	pTFI->nMsgNo = msgno;
	return TRUE;
}
#endif

_API_EXPORT int CALLING_CONV TFI_GetQrCode(HANDLE h, char *qrcode)
{
	strcpy(qrcode, QR_code_buf);
	return strlen(QR_code_buf);
}

static int is_qr_valid( const char *ptr, int len )
{
	unsigned char ch;
	int i = 0;
	if( len <= 3 )
		return 0;
	for( i = 0; i < len-1; i++ )
	{
		ch = *ptr++;
		if( ch < 0x20 || ch > 0x7D )
			return 0;
	}
	return 1;
}

static int led_gets(LENFONTCARD *pTFI, char *buf, int size)
{
	if (pTFI->InterfaceType == TTY_TYPE)
		return  tty_read(pTFI->fd, buf, size );
	else
		return  sock_read(pTFI->fd, buf, size);
}

#ifdef linux 
#define TTY_GETS(fd, buf, size, eol )     tty_gets(fd, buf, size, 50, eol, NULL )
#else
#define TTY_GETS(fd, buf, size, eol)        tty_gets(fd, buf, size, eol)
#endif

static int led_ready(LENFONTCARD *pTFI )
{
	if (pTFI->InterfaceType == TTY_TYPE)
		return tty_ready(pTFI->fd, 200 );
	else
		return sock_dataready(pTFI->fd, 200);
}

static int led_getc(LENFONTCARD *pTFI)
{
	if (pTFI->InterfaceType == TTY_TYPE)
		return TTY_GETC(pTFI->fd, 200 );
	else
		return sock_getc(pTFI->fd, 200);
}

static int led_gets_until(LENFONTCARD *pTFI, char *buf, int max_size, int eol)
{
	if (pTFI->InterfaceType == TTY_TYPE)
		return TTY_GETS(pTFI->fd, buf, max_size, eol );
	else
		return sock_read_until(pTFI->fd, buf, max_size, eol);
}

static void ProcessQrCode(char *buf)
{
	char buffer[256] = {0};
	int i = 0;
	int j = 0;
	int len = strlen(buf);
	if (len < 2)
		return;
	for (i = 0; i < len; i++)
	{
		if ( buf[i] == '\r' || buf[i] == '\n' || buf[i] == 0x03 )
			break;
		if (0x20 <= buf[i] && buf[i] < 0x7E)
		{
			buffer[j++] = buf[i];
		}
	}
	buffer[j] = '\0';
	memset(buf, 0, len);
	strcpy(buf, buffer);
}

static void NoticeEvent(LENFONTCARD *pTFI, int code)
{
	if (pTFI->ca)
		pTFI->ca(pTFI, code);
#ifdef WIN32
	if (pTFI->hWnd)
		PostMessage(pTFI->hWnd, pTFI->nMsgNo, (DWORD)pTFI, code);
#endif
	MTRACE_LOG(pTFI->hLog, "上报事件号：%d \n", code );
}

#ifdef linux
static void *ProtocolThread(void* h)
#else
static DWORD WINAPI ProtocolThread( HANDLE h)
#endif 
{
	int max_fd; 
	int len;
	int i, error_happend;
	unsigned char code_buf[256];
	int ch;
	unsigned char snd_buf[512] = { 0 };
	LENFONTCARD *pTFI = (LENFONTCARD*)h;   
	char recv_buf[1024] = { 0 };
	DWORD ltNextHb = GetTickCount();
	DWORD last_tick = GetTickCount();
	DWORD ltLastHeard = GetTickCount();
	int last_stat = 0;
	int online_last, online_this = 0;
	int ping_ret = 0;
	online_last = online_this = 0;
	if (pTFI->InterfaceType == TTY_TYPE)
		NoticeEvent(pTFI, TFI_EVT_ONLINE );
	else
		pTFI->fd = INVALID_SOCKET; 
	while (!pTFI->bQuit)
	{
		if (online_this != online_last)
		{
			online_last = online_this; 
			NoticeEvent(pTFI, online_this?TFI_EVT_OFFLINE:TFI_EVT_OFFLINE);
		}
		if (pTFI->InterfaceType != TTY_TYPE && pTFI->fd < 0)
		{
			pTFI->fd = sock_connect0(pTFI->m_DWIP, pTFI->nPort);
		
			if (pTFI->fd == INVALID_SOCKET)
			{
				MTRACE_LOG(pTFI->hLog, "Connected to [%x] [%d] Led Failed!\n", (pTFI->m_DWIP), pTFI->nPort);
				online_this = 1;
#ifdef linux
				usleep(5000000);
#else
				Sleep(5000);
#endif
				continue;
			}
			ltLastHeard = GetTickCount();
			if ( last_tick > GetTickCount() )
			{
				MTRACE_LOG(pTFI->hLog, "Last Connected  was Too Closed, Wait For a second!!\n");
#ifdef linux
				usleep(5000000);
#else
				Sleep(5000);
#endif
			}
			last_tick = GetTickCount() + 5000;
			MTRACE_LOG(pTFI->hLog, "Thread %d Connected to Led Success!\n", get_thread_id() );
		}
		if (pTFI->InterfaceType != TTY_TYPE)
		{ 
			if (ltNextHb < GetTickCount())
			{
				TFI_GetDeviceInfo(pTFI, snd_buf);
				ltNextHb = GetTickCount() + 5000;
			} 
		} 
		if (ltLastHeard + 15000 < GetTickCount())
		{
			MTRACE_LOG(pTFI->hLog, "Too Long Not heard from device, offline!\r\n");
			if (pTFI->fd)
			{
				sock_close(pTFI->fd);
				pTFI->fd = INVALID_SOCKET;
				online_this = 0;
				continue;
			}
		}
		if( !led_ready( pTFI ) )
			continue;
		ch = led_getc(pTFI ); 
		if ( ch < 0 && pTFI->InterfaceType != TTY_TYPE)
		{
			sock_close(pTFI->fd);
			pTFI->fd = INVALID_SOCKET;
			MTRACE_LOG(pTFI->hLog, "[%d] socket has closed!!\n", __LINE__ );
			continue;
		}
		ltLastHeard = GetTickCount();
		if (ch <= 0)
		{
			continue;
		}
		if (ch == 0xf1)
		{
			error_happend = 0;
			len = 0;
			for (i = 0; i < 256; i++)
			{
				ch = led_getc(pTFI);
				if (ch < 0)
				{
					error_happend = 1;
					break;
				}
				if (ch == 0xff || ch == 0x0a)
					break;
				QR_code_buf[len++] = ch;
			}
			if (error_happend)
			{
				if (ch < 0 && pTFI->InterfaceType != TTY_TYPE)
				{
					sock_close(pTFI->fd);
					pTFI->fd = INVALID_SOCKET;
					MTRACE_LOG(pTFI->hLog, "[%d] socket has closed!!\n", __LINE__);
				}
				continue;
			}
			QR_code_buf[len] = '\0';
			MTRACE_LOG(pTFI->hLog, "RECV %s\r\n", show_hex(QR_code_buf, len));
			ProcessQrCode(QR_code_buf);
			MTRACE_LOG(pTFI->hLog, "--> QR_code_buf = [%s], len= %d \r\n", QR_code_buf, strlen(QR_code_buf));
			MTRACE_LOG(pTFI->hLog, "QRCODE %s\r\n", show_hex(QR_code_buf, len));
			NoticeEvent(pTFI, TFI_EVT_QRCODE);
			NoticeEvent(pTFI, TFI_EVT_MIGRATE_QRCODE);
		}
		else if (0xa1 == ch)
		{
			code_buf[0] = 0xa1;
			error_happend = 0;
			for (i = 0; i<6; i++)
			{
				ch = led_getc(pTFI);
				if (ch < 0 )
				{
					error_happend = 1;
					break;
				}
				code_buf[i] = ch & 0xff;
			}
			if (error_happend)
			{
				if (ch < 0 && pTFI->InterfaceType != TTY_TYPE)
				{
					sock_close(pTFI->fd);
					pTFI->fd = INVALID_SOCKET;
					MTRACE_LOG(pTFI->hLog, "[%d] socket has closed!!\n", __LINE__);
				}
				continue;
			}
			if (code_buf[0] == 0xb1 && code_buf[1] == 0xc1 && code_buf[2] == 0xd1 && code_buf[3] == 0x40)
			{
				int len = ((code_buf[4] & 0xf0) >> 4) * 1000 + (code_buf[4] & 0xf) * 100 + ((code_buf[5] & 0xf0) >> 4) * 10 + (code_buf[5] & 0xf);
				len++;
				for (i = 0; i < len; i++)
				{
					ch = led_getc(pTFI);
					if (ch == -1)
					{
						error_happend = 1;
						break;
					}
					QR_code_buf[i] = ch & 0xff;
				}
				if (error_happend)
				{
					if (ch < 0 && pTFI->InterfaceType != TTY_TYPE)
					{
						sock_close(pTFI->fd);
						pTFI->fd = INVALID_SOCKET;
						MTRACE_LOG(pTFI->hLog, "[%d] socket has closed!!\n", __LINE__);
					}
					continue;
				}
				QR_code_buf[len - 1] = '\0';
				len -= 2;
				MTRACE_LOG(pTFI->hLog, "--> 老款费显程序 QR_code_buf = [%s], len= %d \n", QR_code_buf, len - 1);
				if (is_qr_valid(QR_code_buf, len - 1))
					NoticeEvent(pTFI, TFI_EVT_QRCODE); 
			}
			else{
				// ignore
			}
		}
		else if (0xe3 == ch)
		{  
			MTRACE_LOG(pTFI->hLog, "--> button 0xe3 \n");
			ch = led_getc(pTFI);
			if (0xe4 == ch)
			{
				MTRACE_LOG(pTFI->hLog, "--> button 0xe4 \n");
				NoticeEvent(pTFI, TFI_EVT_BTN_NO1 );
			}
		}
		else if(0xe5 == ch)
		{
			MTRACE_LOG(pTFI->hLog, "--> button 0xe5 \n");
			ch = led_getc(pTFI);
			if (0xe6 == ch)
			{
				MTRACE_LOG(pTFI->hLog, "--> button 0xe6 \n");
				NoticeEvent(pTFI, TFI_EVT_BTN_NO2);
			}
		}
		else
		{

		} 
	}
	return 0;
}

static int SendPacket(LENFONTCARD *pTFI, char *msg, int len)
{
	int res;
	Mutex_Lock(pTFI);
	if (pTFI->InterfaceType == TTY_TYPE)
	{
		res = tty_write(pTFI->fd, msg, len);
	}
	else
	{
		if (pTFI->fd == INVALID_SOCKET)
		{
			MTRACE_LOG(pTFI->hLog, "Socket Not Connected Yet!\n" );
			NoticeEvent(pTFI, TFI_EVT_OFFLINE );
			Mutex_Unlock(pTFI);
			return FALSE;
		}
		if (pTFI->InterfaceType == TCP_TYPE && sock_iqueue(pTFI->fd) > 0)
			sock_drain(pTFI->fd);
		res = sock_write(pTFI->fd, msg, len);
	}
	Mutex_Unlock(pTFI);
	return res;
}

