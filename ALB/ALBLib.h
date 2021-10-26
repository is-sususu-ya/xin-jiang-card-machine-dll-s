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
	int m_nBalustrade; // ����״̬
	BOOL m_bBCoil;   // ������Ȧ(����Ȧ)��״̬
	BOOL m_bFCoil;   // ץ����Ȧ(ǰ��Ȧ)��״̬
}DEVICESTATUS, *PDEVICESTATUS;

typedef struct tagALB {
	DWORD m_nALBMagicWord;
	
	int m_nInterface; // 1: ����; 2: ����
	
	SOCKET m_nSocket; // socket ���
	DWORD  m_nIP; // �豸IP
	//BOOL   m_bReconnect; // ������ǣ�����豸�ܾ����Ӻ󣬶�̬��Ͳ�����������
	int    m_nAccept;  // 0: ��ʼ��ֵ�� 1: �������� 2: �ܾ�����
	
	int m_fdCom; // ���ھ��
	int m_nComNum; // ���ں�

	ALBCallBack m_nCb;  // ���˻��¼��ص�����

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

	time_t m_tHeartBeatTime;     // ������������ʱ��
	time_t m_tHeartBeatOverTime; // �����������ĳ�ʱʱ��
	
	// log handle
	HANDLE hLog;

	BOOL m_bReceiveOK; // �յ����Ƶ�"OK"��Ӧ
}  ALBObject, *ALBHANDLE;

#define IsValidHandle(h)		(((h) != NULL) && ((h)->m_nALBMagicWord == MAGIC_WORD_ALB))
#define IsDeviceOnline(h)		((h)->m_nALBStateThis.m_bOnline)

#define INTERFACE_NET		1
#define INTERFACE_UART		2

#define ALB_CONTROLLER_TCP_PORT	4001	// �豸�˿ں�

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
