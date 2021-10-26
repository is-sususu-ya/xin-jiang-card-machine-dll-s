#ifndef __ALBLIB_H__
#define __ALBLIB_H__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#if defined _WIN32 || defined _WIN64
#include <windows.h>
#else
#include "wintype.h"
#include <pthread.h>
#include <termios.h>
#endif

#ifdef __cplusplus
extern "C"{
#endif

#define MAGIC_WORD_ALB		0xAABBCCDD

typedef void ( * ALBCallBack)( void *h, int nEventId, int nParam );

typedef struct tagDeviceStatus{
	BOOL m_bOnline;
	int m_nBalustrade; // 栏杆状态
	BOOL m_bBCoil;   // 栏杆线圈(后线圈)车状态
	BOOL m_bFCoil;   // 抓拍线圈(前线圈)车状态
}DEVICESTATUS, *PDEVICESTATUS;

typedef struct tagALB {
	DWORD m_nALBMagicWord;
	
	int m_nInterface; // 1: 网口; 2: 串口
	
	SOCKET m_nSocket; // socket 句柄
	DWORD  m_nIP; // 设备IP
	//BOOL   m_bReconnect; // 重连标记，如果设备拒绝连接后，动态库就不再重新连接
	int    m_nAccept;  // 0: 初始化值， 1: 接受连接 2: 拒绝连接
	
	int m_fdCom; // 串口句柄
	int m_nComNum; // 串口号

	ALBCallBack m_nCb;  // 栏杆机事件回调函数

	void *pUserData;
#if defined _WIN32 || defined _WIN64
	HANDLE m_hWorkThread;
	DWORD  m_nThreadId;
#else // linux
	pthread_t		m_hWorkThread;
	struct termios  termio_save;
#endif
	int    m_nQuit;
	
	DEVICESTATUS m_nALBStateThis;
	DEVICESTATUS m_nALBStateLast;

	time_t m_tHeartBeatTime;     // 发送心跳包的时间
	time_t m_tHeartBeatOverTime; // 接收心跳包的超时时间
	
	// log handle
	HANDLE hLog;

	BOOL m_bReceiveOK; // 收到控制的"OK"回应
}  ALBObject, *ALBHANDLE;

#define IsValidHandle(h)		(((h) != NULL) && ((h)->m_nALBMagicWord == MAGIC_WORD_ALB))
#define IsDeviceOnline(h)		((h)->m_nALBStateThis.m_bOnline)

#define INTERFACE_NET		1
#define INTERFACE_UART		2

#define ALB_CONTROLLER_TCP_PORT	4001	// 设备端口号

#define EVID_BALUSTRADE	1
#define EVID_FCOIL		2
#define EVID_BCOIL		3
#define EVID_ACCEPT		96
#define EVID_REFUSE		97
#define EVID_ONLINE		98
#define EVID_OFFLINE	99

#define VEHICLE_ARRIVE		1
#define VEHICLE_LEAVE		0

extern ALBHANDLE libALBOpen( const char *str, ALBCallBack pFunc, const char *logName );
extern BOOL libALBClose( ALBHANDLE hALB );
extern BOOL libALBCtrl( ALBHANDLE hALB, BOOL bOpen );

#ifdef __cplusplus
}
#endif

#endif // #ifndef __ALBLIB_H__
