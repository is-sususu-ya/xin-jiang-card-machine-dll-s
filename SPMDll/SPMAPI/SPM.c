#include "SPM.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef linux
#include <pthread.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "./utils/utils_tty.h"
#include "./utils/utils_net.h"
#include "../utils/utils_mtrace.h"
#else
#include "common\mtrace.h"
#include "common\winnet.h"
#include "common\wintty.h"
#endif

#define TCP_TYPE 1
#define UDP_TYPE 2
#define TTY_TYPE 3

#define PORT_SLAVE_UDP 8016
#define UDP_LOCAL_PORT 9000

typedef struct tagObject
{
	int size;
	int fd;
	int bQuit;
	int type;
	int nPort;
	char dev_name[16];
	int qr_index;
	uint32_t di_this;
	HANDLE hLog;
	BOOL onLine;
	SPM_CallBack cbfx;
#ifdef linux
	pthread_t hThread;
	pthread_mutex_t hMutex;
	struct termios termio_save;
#else
	HANDLE hThread;
	HANDLE hMutex;
	HWND hWnd;
	int nMsgNo;
#endif
	char qr_code[256];
	int connect_type;
	int res_id;
	int res_code;
	char response[2046];
} OBJECT_S;

#define DEF_RSU_BAUDRATE 9600

#define INVLAID_OBJ(h) ((h == NULL) || ((OBJECT_S *)h)->size != sizeof(OBJECT_S))

#ifdef WIN32
#define Mutex_Lock(h) WaitForSingleObject(h->hMutex, INFINITE)
#define Mutex_Unlock(h) ReleaseMutex(h->hMutex)
#else
#define Mutex_Lock(h) pthread_mutex_lock(&h->hMutex)
#define Mutex_Unlock(h) pthread_mutex_unlock(&h->hMutex)
#endif

static uint8_t getCrc(uint8_t *data, int len);
static int32_t bcd2len(uint8_t *bcd);
static void len2bcd(int32_t value, uint8_t *bcd);
static int32_t create_package(int type, uint32_t cmd, const uint8_t *buf, uint32_t len, uint8_t *dst, uint32_t max_len);

static int AckPacket(OBJECT_S *pSPM, int cmd);
static int ReceivePacket(OBJECT_S *pSPM, uint8_t *msg, int size);
static int SendPacket(OBJECT_S *pSPM, char *msg, int len);

static int callInitSuccess = 0;

#ifdef linux
static void *ProtocolThread(void *h);
#else
static DWORD WINAPI ProtocolThread(HANDLE h);
#endif

#ifdef linux
unsigned long long GetTickCount()
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
	tmsec = (unsigned long long)(now_time - begin_time);
	return tmsec;
}
#endif

EXPAPI HANDLE CALLTYPE SPM_Create()
{
	OBJECT_S *pSPM = malloc(sizeof(OBJECT_S));
	char log_buf[40] = "";
	memset(pSPM, 0, sizeof(OBJECT_S));
	pSPM->size = sizeof(OBJECT_S);
	return pSPM;
}

EXPAPI BOOL CALLTYPE SPM_Destory(HANDLE h)
{
	if (INVLAID_OBJ(h))
		return FALSE;
	memset(h, 0, sizeof(OBJECT_S));
	free(h);
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_IsOnline(HANDLE h)
{
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	return pSPM->onLine;
}

EXPAPI BOOL CALLTYPE SPM_Open(HANDLE h, const char *devName)
{
	int port;
	int fd;
	char buffer[64] = {0};
	DWORD ThreadId;
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	sprintf(buffer, "SPM_%s", devName);
	pSPM->hLog = MLOG_INIT("/rwlog", buffer);
	if (pSPM->hLog)
		MLOG_SETLIMITCNT(pSPM->hLog, 500, 4);
	MTRACE_LOG(pSPM->hLog, "LOG INIT\r\n");
#ifdef linux
	if (strstr(devName, "/dev"))
	{
		pSPM->connect_type = TTY_TYPE;
		fd = open(devName, O_RDWR);
		if (fd < 0)
			return FALSE;
		tty_raw(fd, NULL, 9600);
	}
	else
	{
		if (inet_addr(devName) == INADDR_NONE)
			return FALSE;
		pSPM->connect_type = TCP_TYPE;
		fd = sock_connect(devName, 6018);
	}
#else
	if (strstr(devName, "COM"))
	{
		pSPM->connect_type = TTY_TYPE;
		sscanf(devName, "COM%d", &port);
		MTRACE_LOG(pSPM->hLog, "PORT:%d\r\n", port);
		fd = tty_openPort(port);
		if (fd < 0)
		{
			MTRACE_LOG(pSPM->hLog, "OPEN Port Error:%d %s\r\n", port, GetLastError());
			return FALSE;
		}
		tty_raw(fd, 9600, 8, 0);
		tty_setReadTimeouts(fd, 200, 1, 100);
		if (fd == -1)
		{
			MTRACE_LOG(pSPM->hLog, "%s Open Failed \n", devName);
			return FALSE;
		}
	}
	else
	{
		if (inet_addr(devName) == INADDR_NONE)
			return FALSE;
		pSPM->connect_type = TCP_TYPE;
		if (winsock_startup() != 0)
		{
			MTRACE_LOG(pSPM->hLog, "--> WINSOCK library initialization failed!\n");
			return FALSE;
		}
		fd = sock_connect(devName, 6018);
	}
#endif
	strcpy(pSPM->dev_name, devName);
	pSPM->fd = fd;
	pSPM->nPort = 6018;
#ifdef linux
	pthread_mutex_init(&pSPM->hMutex, NULL);
	if (0 == pthread_create(&pSPM->hThread, NULL, ProtocolThread, pSPM))
		MTRACE_LOG(pSPM->hLog, "--> ProtocolThread create sucess\n");
	else
		MTRACE_LOG(pSPM->hLog, "--> ProtocolThread create failed \n");
#else
	pSPM->hMutex = CreateMutex(NULL, FALSE, NULL);
	MTRACE_LOG(pSPM->hLog, "SPM_OpenCom success!\n");
	pSPM->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ProtocolThread, pSPM, NULL, &ThreadId);
	if (pSPM->hThread)
		MTRACE_LOG(pSPM->hLog, "--> ProtocolThread create sucess\n");
	else
		MTRACE_LOG(pSPM->hLog, "--> ProtocolThread create failed \n");
#endif
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_Close(HANDLE h)
{
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	pSPM->bQuit = 1;
	if (pSPM->bQuit == 1)
		return FALSE;
#if defined _WIN32 || defined _WIN64
	if (pSPM->hMutex)
		CloseHandle(pSPM->hMutex);
	TerminateThread(pSPM->hThread, 0);
	winsock_cleanup();
#else
	pthread_mutex_destroy(&pSPM->hMutex);
	pthread_cancel(pSPM->hThread);
	pthread_join(pSPM->hThread, NULL);
	pthread_mutex_destroy(&pSPM->hMutex);
#endif
	return TRUE;
}

#ifndef linux
EXPAPI BOOL CALLTYPE SPM_SetWinMsg(HANDLE h, HWND wnd, int nMsgNo)
{
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	pSPM->hWnd = wnd;
	pSPM->nMsgNo = nMsgNo;
	return TRUE;
}
#endif

EXPAPI BOOL CALLTYPE SPM_SetCallBack(HANDLE h, SPM_CallBack cb)
{
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	pSPM->cbfx = cb;
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_Led_ClearAll(HANDLE h)
{
	uint8_t buffer[128];
	int len;
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	len = create_package(0, 0x04, NULL, 0, buffer, sizeof(buffer));
	SendPacket(pSPM, buffer, len);
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_Led_SendLineText(HANDLE h, int nLineNumber, int color, int alignType, const char *text, int size)
{
	uint8_t buffer[1000] = {0};
	uint8_t tmp[1000] = {0};
	int len, total;
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	if (size > 900)
		return FALSE;
	tmp[0] = nLineNumber & 0xff;
	tmp[1] = (alignType & 0x0f) | 0x50; // 固定闪烁、滚动速度
	tmp[2] = color & 0xff;
	tmp[3] = (color >> 8) & 0xff;
	tmp[4] = (color >> 16) & 0xff;
	if (text && size > 0)
		memcpy(tmp + 5, text, size);
	total = size + 5;
	len = create_package(0, 0x20, tmp, total, buffer, sizeof(buffer));
	SendPacket(pSPM, buffer, len);
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_Voice_SendText(HANDLE h, int vol, const char *text, int size)
{
	uint8_t buffer[1024] = {0};
	uint8_t tmp[1024] = {0};
	int len, total;
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	if (size > 900)
		return FALSE;
	total = 0;
	tmp[0] = vol & 0xff;
	total += 1;
	if (text && size > 0)
	{
		total += size;
		memcpy(tmp + 1, text, size);
	}
	len = create_package(0, 0x50, tmp, total, buffer, sizeof(buffer));
	SendPacket(pSPM, buffer, len);
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_Lcd_ChangeContext(HANDLE h, int index, const char *text, int size)
{
	uint8_t buffer[1024] = {0};
	uint8_t tmp[1024] = {0};
	int len, total;
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	if (size > 900)
		return FALSE;
	total = 0;
	tmp[0] = index & 0xff;
	total += 1;
	if (text && size > 0)
	{
		memcpy(tmp + 1, text, size);
		total += size;
	}
	len = create_package(0, 0x60, tmp, total, buffer, sizeof(buffer));
	SendPacket(pSPM, buffer, len);
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_SyncTime(HANDLE h)
{
	uint8_t buffer[64] = {0};
	uint8_t tmp[32] = {0};
	int len, total;
	OBJECT_S *pSPM = (OBJECT_S *)h;
	static char timestr[64];
#ifdef linux
	char *p = timestr;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	strftime(p, sizeof(timestr), "%y%m%d%H%M%S", localtime(&tv.tv_sec));
#else
	SYSTEMTIME tnow;
	GetLocalTime(&tnow);
	sprintf((TCHAR)timestr, _T("%04d%02d%02d%02d%02d%02d"), tnow.wYear, tnow.wMonth, tnow.wDay, tnow.wHour, tnow.wMinute, tnow.wSecond);
#endif
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	total = 14;
	memcpy(tmp, timestr, 14);
	len = create_package(0, 0x01, tmp, 14, buffer, sizeof(buffer));
	SendPacket(pSPM, buffer, len);
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_Reboot(HANDLE h)
{
	uint8_t buffer[128];
	uint8_t tmp[32] = {0};
	int len;
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	tmp[0] = 0x03;
	len = create_package(0, 0x10, tmp, 1, buffer, sizeof(buffer));
	SendPacket(pSPM, buffer, len);
	return TRUE;
}

static BOOL SPM_HeartBeat(HANDLE h)
{
	uint8_t buffer[128];
	int len;
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	len = create_package(0, 0x00, NULL, 0, buffer, sizeof(buffer));
	SendPacket(pSPM, buffer, len);
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_EnableQrCode(HANDLE h, BOOL en)
{
	uint8_t buffer[128];
	uint8_t tmp[32] = {0};
	int len;
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	tmp[0] = en ? 0x01 : 0x02;
	len = create_package(0, 0x10, tmp, 1, buffer, sizeof(buffer));
	SendPacket(pSPM, buffer, len);
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_GetQrCodeEx(HANDLE h, int *index, char *buf)
{
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	*index = pSPM->qr_index;
	strcpy(buf, pSPM->qr_code);
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_GetQrCode(HANDLE h, char *buf)
{
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	strcpy(buf, pSPM->qr_code);
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_InitPhone(HANDLE h, char *server, int port, char *phoneId, char *password)
{
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	uint8_t buffer[256];
	uint8_t tmp[256] = {0};
	char text[1024] = {0};
	unsigned char recv_buf[2048] = {0};
	uint8_t *buf = recv_buf;
	int len = 0;
	DWORD ltSend = 0;
	sprintf(text, "%s;%d;%s;%s", server, port, phoneId, password);
	strcpy(tmp + 4, text);
	len += strlen(text) + 5;
	len = create_package(0, 0x80, tmp, len, buffer, sizeof(buffer));
	SendPacket(pSPM, buffer, len);
	ltSend = GetTickCount() + 5000;
	callInitSuccess = 0;
	// 简单操作，等待5秒
	while (ltSend > GetTickCount() && !callInitSuccess)
		usleep(100000);
	return callInitSuccess == 1; // 1:成功 其他失败
}

EXPAPI BOOL CALLTYPE SPM_CallPhone(HANDLE h, int index, char *phoneId, int timeout)
{
	uint8_t buffer[256];
	uint8_t tmp[256] = {0};
	char text[256] = {0};
	int len = 0;
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	sprintf(text, "%d;%s", index == 0 ? 0 : 1, phoneId);
	strcpy(tmp + 4, text);
	len += strlen(text) + 5;
	len = create_package(0, 0x81, tmp, 4, buffer, sizeof(buffer));
	SendPacket(pSPM, buffer, len);
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_AnswerPhone(HANDLE h, char *phoneId, int reply, int timeout)
{
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	uint8_t buffer[256];
	uint8_t tmp[256] = {0};
	char text[256] = {0};
	int len = 0;
	sprintf(text, "%s;%d", phoneId, reply);
	strcpy(tmp + 4, text);
	len += strlen(text) + 5;
	len = create_package(0, 0x82, tmp, 4, buffer, sizeof(buffer));
	SendPacket(pSPM, buffer, len);
	return TRUE;
}

static int gpio_output(OBJECT_S *pSPM, int type, int pin, int val)
{
	uint8_t buffer[128];
	uint8_t tmp[32] = {0};
	int len;
	tmp[0] = type;
	tmp[1] = pin & 0xff;
	tmp[2] = val & 0xff;
	tmp[3] = (val >> 8) & 0xff;
	tmp[4] = (val >> 16) & 0xff;
	tmp[5] = (val >> 24) & 0xff;
	len = create_package(0, 0x70, tmp, 6, buffer, sizeof(buffer));
	SendPacket(pSPM, buffer, len);
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_GpioOutPut(HANDLE h, int pin, int val)
{
	uint8_t buffer[128];
	uint8_t tmp[32] = {0};
	int len;
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	return gpio_output(pSPM, 0, pin, val > 0 ? 1 : 0);
}

EXPAPI BOOL CALLTYPE SPM_GpioPulse(HANDLE h, int pin, int tout)
{
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	return gpio_output(pSPM, 1, pin, tout);
}

EXPAPI BOOL CALLTYPE SPM_GpioPulseNegative(HANDLE h, int pin, int tout)
{
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	return gpio_output(pSPM, 2, pin, tout);
}

EXPAPI BOOL CALLTYPE SPM_ReadDi(HANDLE h, int *parma)
{
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	*parma = pSPM->di_this;
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_SetHttpProxyAddress(HANDLE h, int index, const char *url, int size)
{
	uint8_t tmp[300] = {0};
	uint8_t buffer[1024] = {0};
	int len;
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	tmp[0] = index & 0xff;
	strncpy(tmp + 1, url, size);
	len = create_package(0, 0x02, tmp, size + 1, buffer, sizeof(buffer));
	SendPacket(pSPM, buffer, len);
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_SetHttpProxyConfig(HANDLE h, int con_tout, int pro_tou)
{
	uint8_t tmp[300] = {0};
	uint8_t buffer[1024] = {0};
	int len;
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	tmp[0] = con_tout & 0xff;
	tmp[1] = pro_tou & 0xff;
	len = create_package(0, 0x03, tmp, 2, buffer, sizeof(buffer));
	SendPacket(pSPM, buffer, len);
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_SetHttpPostProxyRequest(HANDLE h, int index, int id, int type, const char *conten, int size)
{
	uint8_t tmp[1024] = {0};
	uint8_t buffer[1024] = {0};
	int len;
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	tmp[0] = index & 0xff;
	tmp[1] = id & 0xff;
	tmp[2] = type & 0xff;
	strncpy(tmp + 3, conten, size);
	len = create_package(0, 0x90, tmp, size + 3, buffer, sizeof(buffer));
	SendPacket(pSPM, buffer, len);
	MTRACE_LOG(pSPM->hLog, "发起Post请求：%s \n", conten);
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_SetHttpGetProxyRequest(HANDLE h, int id, const char *url, int size)
{
	uint8_t tmp[1024] = {0};
	uint8_t buffer[1024] = {0};
	int len;
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	tmp[0] = id & 0xff;
	strncpy(tmp + 1, url, size);
	len = create_package(0, 0x91, tmp, size + 1, buffer, sizeof(buffer));
	SendPacket(pSPM, buffer, len);
	MTRACE_LOG(pSPM->hLog, "发起Http请求：%s \n", url);
	return TRUE;
}

EXPAPI BOOL CALLTYPE SPM_GetHttpProxyResponse(HANDLE h, int *id, int *ret, char *content, int max_size)
{
	OBJECT_S *pSPM = (OBJECT_S *)h;
	if (INVLAID_OBJ(pSPM))
		return FALSE;
	*id = pSPM->res_id;
	*ret = pSPM->res_code;
	if (strlen(pSPM->response) > max_size)
		strncpy(content, pSPM->response, max_size);
	else
		strcpy(content, pSPM->response);
	return TRUE;
}

static void CALLTYPE NoticeEvent(OBJECT_S *pSPM, int code)
{
	MTRACE_LOG(pSPM->hLog, "上报事件号：%d \n", code);
	if (pSPM->cbfx)
		pSPM->cbfx(pSPM, code);
#ifdef WIN32
	if (pSPM->hWnd)
		PostMessage(pSPM->hWnd, pSPM->nMsgNo, (DWORD)pSPM, code);
#endif
}

#ifdef linux
#define TTY_GETC(fd, t) tty_getc(fd, t, 0)
#define TTY_GETS(fd, buf, size, eol) tty_gets(fd, buf, size, 50, eol, NULL)

#else
#define TTY_GETC(fd, t) tty_getc(fd, t)
#define TTY_GETS(fd, buf, size, eol) tty_gets(fd, buf, size, eol)
#endif

static int spm_ready(OBJECT_S *pSPM)
{
	if (pSPM->connect_type == TTY_TYPE)
		return tty_ready(pSPM->fd, 200);
	else
		return sock_dataready(pSPM->fd, 200);
}

static int spm_getc(OBJECT_S *pSPM)
{
	if (pSPM->connect_type == TTY_TYPE)
		return TTY_GETC(pSPM->fd, 200);
	else
		return sock_getc(pSPM->fd, 200);
}

static int spm_gets_until(OBJECT_S *pSPM, char *buf, int max_size, int eol)
{
	if (pSPM->connect_type == TTY_TYPE)
		return TTY_GETS(pSPM->fd, buf, max_size, eol);
	else
		return sock_read_until(pSPM->fd, buf, max_size, eol);
}

static int spm_gets_n_bytes(OBJECT_S *pSPM, char *buf, int size)
{
	if (pSPM->connect_type == TTY_TYPE)
		return tty_read_n_bytes(pSPM->fd, buf, size);
	else
		return sock_read_n_bytes(pSPM->fd, buf, size);
}
static void spm_sleep(int ms)
{
#ifdef linux
	usleep(ms * 1000);
#else
	Sleep(ms);
#endif
}

static int get_param(OBJECT_S *pSPM, uint8_t *buf, int len, uint8_t *param)
{
	int size = bcd2len(buf + 5);
	if (size > 1024)
	{
		MTRACE_LOG(pSPM->hLog, "长度超限[%d]！\r\n", size);
		size = 1024;
	}
	memcpy(param, buf + 7, size);
	return size;
}

static void CloseSocket(OBJECT_S *pSPM)
{
	sock_close(pSPM->fd);
	pSPM->fd = INVALID_SOCKET;
}

#ifdef linux
static void *ProtocolThread(void *h)
#else
static DWORD WINAPI ProtocolThread(HANDLE h)
#endif
{
	int len;
	OBJECT_S *pSPM = (OBJECT_S *)h;
	DWORD ltLastHeard = GetTickCount();
	int last_stat = 0;
	int this_stat = 0;
	uint8_t param[1024];
	char text[1024] = {0};
	int size;
	DWORD ltLastHeartBeat = GetTickCount();
	int cmd = 0;
	unsigned char recv_buf[2048] = {0};
	uint8_t *buf = recv_buf;
	if (pSPM->connect_type == TTY_TYPE)
	{
		NoticeEvent(pSPM, SPM_EVT_ONLINE);
		last_stat = 1;
		this_stat = 1;
	}
	pSPM->onLine = 0;
	while (!pSPM->bQuit)
	{
		if (this_stat != last_stat)
		{
			pSPM->onLine = this_stat;
			MTRACE_LOG(pSPM->hLog, "状态不一致【%d】 【%d】，上报设备状态-> %s ！\r\n", this_stat, last_stat, this_stat == 0 ? "离线" : "在线");
			NoticeEvent(pSPM, this_stat ? SPM_EVT_ONLINE : SPM_EVT_OFFLINE);
			last_stat = this_stat;
			MTRACE_LOG(pSPM->hLog, "状态切换至：%d \r\n", last_stat);
		}
		if (pSPM->connect_type != TTY_TYPE && pSPM->fd < 0)
		{
			pSPM->fd = sock_connect(pSPM->dev_name, pSPM->nPort);
			if (pSPM->fd == INVALID_SOCKET)
			{
				MTRACE_LOG(pSPM->hLog, "Connected to [%s] [%d] Led Failed!\n", (pSPM->dev_name), pSPM->nPort);
				spm_sleep(500);
				this_stat = 0;
				pSPM->onLine = 0;
				continue;
			}
			ltLastHeard = GetTickCount();
			this_stat = 1;
			MTRACE_LOG(pSPM->hLog, "Thread Connected to Led Success!\n");
			continue;
		}
		if (GetTickCount() > ltLastHeartBeat)
		{
			MTRACE_LOG(pSPM->hLog, "发送心跳！\r\n");
			ltLastHeartBeat = GetTickCount() + 5000; // 5秒一次心跳
			SPM_HeartBeat(pSPM);
		}
		// 超过10秒还未收到响应，表示设备离线
		if (GetTickCount() > (ltLastHeard + 12000))
		{
			MTRACE_LOG(pSPM->hLog, "长时间没收到信息，认定设备离线！\r\n");
			CloseSocket(pSPM);
			this_stat = 0;
		}
		if (spm_ready(pSPM))
		{
			if ((len = ReceivePacket(pSPM, recv_buf, sizeof(recv_buf))) < 0 && pSPM->connect_type == TCP_TYPE)
			{
				MTRACE_LOG(pSPM->hLog, "设备离线！\r\n");
				CloseSocket(pSPM);
				this_stat = 0;
				continue;
			}
			if (buf[0] == 0xa0 || buf[0] == 0xa1)
			{
				MTRACE_LOG(pSPM->hLog, "更新时间戳！\r\n");
				this_stat = 1;
				ltLastHeard = GetTickCount();
				// 收到发来的数据帧，响应
				if (buf[0] == 0xa0)
				{
					cmd = buf[4]; // 获取cmd，返回响应
					AckPacket(pSPM, cmd);
					memset(param, 0, sizeof(param));
					size = get_param(pSPM, buf, len, param);
					switch (buf[4])
					{
					case 0x40:
						memset(pSPM->qr_code, 0, sizeof(pSPM->qr_code));
						pSPM->qr_index = param[0];
						memcpy(pSPM->qr_code, param + 1, size - 1);
						MTRACE_LOG(pSPM->hLog, "收到上报的二维码讯息 【%s】- 长度：%d ！！\r\n", pSPM->qr_code, size - 1);
						NoticeEvent(pSPM, SPM_EVT_QRCODE);
						break;
					case 0x80:
						memcpy(&pSPM->di_this, (unsigned char *)param, 4);
						MTRACE_LOG(pSPM->hLog, "收到IO变化帧[%d]!\r\n", pSPM->di_this);
						NoticeEvent(pSPM, SPM_EVT_IOCHANGE);
						break;

					case 0x09:
						MTRACE_LOG(pSPM->hLog, "收到Http响应码！\r\n");
						pSPM->res_id = param[0];
						pSPM->res_code = param[1]; // 返回码
						memset(pSPM->response, 0, sizeof(pSPM->response));
						MTRACE_LOG(pSPM->hLog, " last_stat 【%d】！！\r\n", last_stat);
						if (size > 2)
						{
							memcpy(pSPM->response, (char *)param + 2, size - 2);
							MTRACE_LOG(pSPM->hLog, "ID: [%d] \r\n响应码: %d \r\n ", pSPM->res_id, pSPM->res_code);
							strncpy(text, pSPM->response, 256); // 限制，日至最多显示256个字节
							MTRACE_LOG(pSPM->hLog, "响应内容：\r\n%s\r\n", text);
							NoticeEvent(pSPM, SPM_EVT_HTTP_RESPONSE);
						}
						else
						{
							MTRACE_LOG(pSPM->hLog, "返回长度太短【%d】 \r\n", size);
						}
						MTRACE_LOG(pSPM->hLog, " last_stat 【%d】！！\r\n", last_stat);
						break;
					case 0x90:
						MTRACE_LOG(pSPM->hLog, "收到NUC端的连线通知！\r\n");
						NoticeEvent(pSPM, SPM_EVT_CALL);
						break;
					case 0x91:
						MTRACE_LOG(pSPM->hLog, "对讲数据初始化成功通知！\r\n");
						callInitSuccess = param[0] == 0 ? 1 : 2; // 0成功，其他失败
						break;									 // 通知NUC端初始化成功
					default:
						MTRACE_LOG(pSPM->hLog, "未实现的内容【%d】！！\r\n", buf[4]);
						break;
					}
				}
				else
				{
					// 收到响应帧，不做处理！
				}
			}
			else
			{
				MTRACE_LOG(pSPM->hLog, "不支持的帧协议，不做处理！\r\n");
			}
		}
	}
	return 0;
}

static int AckPacket(OBJECT_S *pSPM, int cmd)
{
	uint8_t buffer[128];
	uint8_t tmp[32] = {0};
	int len;
	len = create_package(0, cmd, NULL, 0, buffer, sizeof(buffer));
	SendPacket(pSPM, buffer, len);
	return TRUE;
}

static const char *show_hex(uint8_t *buf, int size);

static int ReceivePacket(OBJECT_S *pSPM, uint8_t *msg, int size)
{
	int rc, len, total;
	uint8_t crc;
	rc = spm_getc(pSPM);
	if (rc < 0)
		return -1;
	if (rc == 0)
		return 0;
	if (rc == 0xa0 || rc == 0xa1)
	{
		msg[0] = rc & 0xff;
		len = spm_gets_n_bytes(pSPM, msg + 1, 6);
		if (len < 6)
			return -1;
		len = bcd2len(msg + 5);
		rc = len + 1;
		total = len + 8;
		len = spm_gets_n_bytes(pSPM, msg + 7, rc);
		if (len < rc)
			return -1;
		crc = getCrc(msg, total - 1);
		if (crc != msg[total - 1])
		{
			MTRACE_LOG(pSPM->hLog, "CRC解析错误！\r\n");
			return -1;
		}
		MTRACE_LOG(pSPM->hLog, "帧解析正常， 帧长度 %d -> %s ！\r\n", total, show_hex(msg, total));
		return total;
	}
	else if (rc == '<')
	{
		msg[0] = '<';
		len = spm_gets_until(pSPM, msg + 1, size - 1, '>');
	}
	else
	{
		MTRACE_LOG(pSPM->hLog, "接收缴费机数据！\r\n");
	}
}

static const char *show_hex(uint8_t *buf, int size)
{
	int32_t i;
	static char text[4096] = {0};
	int8_t *p = text;
	uint8_t tmp;
	for (i = 0; i < size; i++)
	{
		tmp = buf[i];
		sprintf(p, "%02X ", tmp);
		p += 3;
	}
	return text;
}

static int SendPacket(OBJECT_S *pSPM, char *msg, int len)
{
	int res;
	// printf("SEND:%s\r\n", show_hex(msg, len));
	Mutex_Lock(pSPM);
	if (pSPM->connect_type == TTY_TYPE)
	{
		res = tty_write(pSPM->fd, msg, len);
	}
	else
	{
		if (pSPM->fd == INVALID_SOCKET)
		{
			MTRACE_LOG(pSPM->hLog, "Socket Not Connected Yet!\n");
			Mutex_Unlock(pSPM);
			return FALSE;
		}
		if (pSPM->connect_type == TCP_TYPE && sock_iqueue(pSPM->fd) > 0)
			sock_drain(pSPM->fd);
		res = sock_write(pSPM->fd, msg, len);
	}
	Mutex_Unlock(pSPM);
	return res;
}

static void len2bcd(int32_t value, uint8_t *bcd)
{
	uint8_t *p = bcd;
	uint8_t nibble;
	nibble = (uint8_t)(value / 100);
	bcd[0] = ((nibble / 10) << 4) + (nibble % 10);
	value %= 100;
	nibble = (uint8_t)value;
	bcd[1] = ((nibble / 10) << 4) + (nibble % 10);
}

static int32_t bcd2len(uint8_t *bcd)
{
	int i = 2;
	int32_t value = 0;
	for (; i--;)
	{
		value *= 100;
		value += (((*bcd) & 0xf0) >> 4) * 10 + ((*bcd) & 0x0f);
		bcd++;
	}
	return value;
}

static uint8_t getCrc(uint8_t *data, int len)
{
	int i = 0;
	uint8_t crc = 0;
	for (i = 0; i < len; i++)
		crc ^= data[i];
	return crc;
}

/**
 * @brief Create a package object
 *
 * @param type 0请求帧，1响应帧
 * @param cmd
 * @param buf
 * @param len
 * @param dst
 * @param max_len
 * @return int32_t
 */
static int32_t create_package(int type, uint32_t cmd, const uint8_t *buf, uint32_t len, uint8_t *dst, uint32_t max_len)
{
	int32_t i = 0;
	uint8_t crc = 0;
	uint8_t *p = dst;
	if (type == 0)
	{
		p[0] = 0xa0;
		p[1] = 0xb0;
		p[2] = 0xc0;
		p[3] = 0xd0;
		p[4] = cmd & 0xff;
	}
	else
	{
		p[0] = 0xa1;
		p[1] = 0xb1;
		p[2] = 0xc1;
		p[3] = 0xd1;
		p[4] = cmd & 0xff; // ack 代码需要设置最高位8
	}
	len2bcd(len, p + 5);
	if (buf && len > 0)
		memcpy(p + 7, buf, len);
	p += 7;
	p += len;
	for (i = 0; i < p - dst; i++)
	{
		crc ^= dst[i];
	}
	p[0] = crc;
	p++;
	return p - dst;
}