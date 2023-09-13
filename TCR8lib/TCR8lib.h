#ifndef _TCR8LIB_INCLUDED
#define _TCR8LIB_INCLUDED

#ifdef linux
#include "wintype.h" 
#include "utils_net.h"
#include "utils_tty.h"
#include <pthread.h>
#else 
#include <windows.h>
#include "../common/wintty.h"
#include "../common/winnet.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
 
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


typedef enum {
	tsIdle, tsBegin, tsRWDone, tsEJFail, tsOnExit } TRState;

#define MSGBUF_SIZE		128
typedef struct lane_msg {
	int		msg_state;					/* 0 -- free slot, 1 -- wait for ack from lane computer */
	int		msg_sndcnt;					// how many time has been send
	int		msg_len;
	char		msg_body[ MSGBUF_SIZE ];	/* last message body -- null terminated (for resending) */
	__int64	t_resend;					/* next time to resend if host is not responsed */
	time_t	t_queue;					/* queue time in sec */
	int		msg_naked;				/* number of NAKed times */
} LANEMSG;

#define MAX_BOX_PERCHANNEL	6 // 每个通道的卡盒座数量

// 通道卡盒信息用于存放多卡盒机芯的卡盒信息
typedef struct _CHANNEL_BOX_INFORMATION{
	DWORD nSn[MAX_BOX_PERCHANNEL];
	int nCap[MAX_BOX_PERCHANNEL];
	int nCount[MAX_BOX_PERCHANNEL];
}CHANNEL_BOX_INFORMATION;

// 通道天线位置卡片信息
typedef struct _CARD_ON_ANT_INFO{
	BYTE  nSeat;  // 天线位置卡片是从哪个卡座发出来的, 如果为0，则不知道天线位置卡是从哪个卡座上出来的，此种情况发生在机芯上电时卡道有卡
	DWORD nSn;    // 天线位置卡片所在的卡盒编号，如果为99999999，则不知道天线位置卡所在卡盒的编号，此种情况发生在机芯上电时卡道有卡
}CARD_ON_ANT_INFO;

#define TCR8_MAGICWORD	0xA1B2C3D4

typedef struct tagTCR8Struct 
{
	// machine attributes
	TRState	smTrans;				// Transaction state
	TRState smLastTrans;			// Last Transaction state
	short 	m_iState[4];
	short	m_iStateLast[4];
	DWORD 	m_nSerial[4];
	short   m_iCap[4];
	short 	m_iCounter[4];
	CHANNEL_BOX_INFORMATION m_nChannelBoxInfo[4];
	CARD_ON_ANT_INFO        m_nCardOnAntInfo[4];
	short 	m_iActive[2];
	long	m_hexVersion;		// kernel firmware version. 
	int	 	m_nChannel;			// Current active channel for transaction
	int	 	m_nButton;
	BOOL	m_bReceiveRecycleCMD; // for Jiangsu protocol
	int     m_nMachineType;     // Machine type: 0: Auto Dispenser; 1: Auto Collector 
	//  comm port
	int		m_tty;				// TCR8 protocol port TTY connection handle 
	int     m_nPort;				// COM port 1,2,3..
	char	m_stPort[32];
	int		m_nBaudrate;			// baudrate 9600 or 19200
	BOOL	m_bOnline;			// TCR8 is online 
	DWORD	m_dwIP;
	SOCKET	m_sockTCP; 
#ifdef linux
	pthread_t m_hThread;
	pthread_mutex_t m_hMutex;
	pthread_mutex_t m_hLogMutex;
#else
	HANDLE	m_hThread;	
	HANDLE		m_hMutex;						// Spin-lock to access the Protocol Packet ring buffer.
	HANDLE      m_hLogMutex;                    // Spin-lock to access the log file 
#endif
	BOOL	m_bQuit;
	void (*m_cbFxc)( void *, int, int);
	DWORD   m_dwEventMask; 
	time_t		m_tNextSync;					// Next time to sync the system clock with controller
	time_t  	t_lastInitial;					// last time we send initial packet
	INT64		m_tHeartBeat2Recv;				// last time I have hear heartbeat from controller. 
	LANEMSG		_laneMsg[10];					// Ring buffer of protocol message send to TCR8
	int			m_cSeq;							// Sequence of protocol packet to send ('0' ~ '9') 
	FILE		*fp;
	char		*m_strPath;						// log file full path 
	void		*m_pUsrData;  
	int m_iProtocol;
	int m_nExtendProtocol;
	DWORD	m_dwMagicWord;
} TCR8Obj, *TCR8HANDLE;

// WPARM is event ID, 

#define EVID_POWERON 		1				// Machine Power On
#define EVM_POWERON			0x00000001
#define EVID_LINK			2				// Link event (param 1 Online, 0 offline)
#define EVM_LINK			0x00000002
#define EVID_PRESSKEY		3				// Button Pressed (param: channel)
#define EVM_PRESSKEY		0x00000004
#define EVID_OUTCARDOK		4				// Card ejected  (param: channel)
#define EVM_OUTCARDOK		0x00000008
#define EVID_OUTCARDFAIL	5				// Card ejected  (param: channel)
#define EVM_OUTCARDFAIL		0x00000010
#define EVID_TAKECARD 		6				// Card taken (param: channel)
#define EVM_TAKECARD		0x00000020
#define EVID_STATECHANGE 	7				// Channel state changed (param: channel)
#define EVM_STATECHANGE		0x00000040
#define EVID_COUNTCHANGE	8				// Channel count changed (param: channel)
#define EVM_COUNTCHANGE		0x00000080
#define EVID_RECYCLEOK		9				// Bad card recycled (param: channel)
#define EVM_RECYCLEOK		0x00000100
#define EVID_RECYCLEFAIL	10				// Bad card recycle failed (param: channel)
#define EVM_RECYCLEFAIL		0x00000200
#define EVID_PRESSPOLKEY	11				// Police key pressed (param: deck) - for ZJ only
#define EVM_PRESSPOLKEY		0x00000400
#define EVID_RWTIMEOUT		12				// wait for CSC R/W timed out
#define EVM_RWTIMEOUT		0x00000800
#define EVID_BUTTONIGNRD	13				// button ignored (param: deck<<8 + 'Button Pressed Ignored Code') 16-->8
#define EVM_BUTTONIGNRD		0x00001000
#define EVID_BOXLOAD		14				// cartridge loaded (param: channel)
#define EVM_BOXLOAD			0x00002000
#define EVID_BOXUNLOAD		15				// cartridge unload (param: channel)
#define EVM_BOXUNLOAD		0x00004000
#define EVID_SNCHANGED		16				// cartridge S/N changed (change by protocol command)
#define EVM_SNCHANGED		0x00008000 		// extend eject event (param: channel<<8 + seat; temporary )
#define EVM_OUTCARDOK_EX	0x00010000 		// extend recycle event ( param: channel<<8 + seat; temporary)
#define EVM_RECYCLEOK_EX	0x00020000
#define EVID_BOXEX			19              // 多卡盒机芯卡盒编号事件( param: channel<<13 + eventId<<8 + seat )
#define EVM_BOXEX			0x00040000
#define EVID_COLLECTOK		20				// Collect Card OK(param: channel)
#define EVM_COLLECTOK		0x00080000
#define EVID_COLLECTFAIL	21				// Collect Card FAIL(param: channel)
#define EVM_COLLECTFAIL		0x00100000
#define EVID_PULLBACKOK		22				// Pull back card from exit OK(param: channel)
#define EVM_PULLBACKOK		0x00200000
#define EVID_PULLBACKFAIL	23				// Pull back card from exit FAIL(param: channel)
#define EVM_PULLBACKFAIL	0x00400000 

// 废票回收
#define EVID_PAPERRECYCLE	24
#define EVM_PAPERRECYCLE	0x00800000 

#define EVID_FLIP_MOTOR_CHANGE	25				// Flip motor change (param: channel)
#define EVM_FLIP_MOTOR_CHANGE	0x01000000 

#define EVID_THREADEXIT		29				// workthread exir abnormally
#define EVM_THREADEXIT		0x10000000
#define EVID_TX				30				// protocol packet send to TCR8 machine
#define EVM_TX				0x20000000
#define EVID_RX				31				// protocol packet received from TCR8 machine
#define EVM_RX				0x40000000
#define EVID_KERNELLOG		32				// kernel firmware log output
#define EVM_KERNELLOG		0x80000000
#define T8EVM_DEFAULT		0x00ffffff		// default event mask
#define T8EVM_ALL			0xffffffff

#define EVID_BOXEX_SUB_ID_LOAD			1  // 卡盒装入
#define EVID_BOXEX_SUB_ID_UNLOAD		2  // 卡盒卸下
#define EVID_BOXEX_SUB_ID_INFOUPDATE	3  // 卡盒信息更新
#define EVID_BOXEX_SUB_ID_ABFULL		4  // 卡盒将满
#define EVID_BOXEX_SUB_ID_ABEMPTY		5  // 卡盒将空
#define EVID_BOXEX_SUB_ID_FULL			6  // 卡盒已满
#define EVID_BOXEX_SUB_ID_EMPTY			7  // 卡盒已空
#define EVID_BOXEX_SUB_ID_UNLOCK		8  // 卡盒开锁
#define EVID_BOXEX_SUB_ID_LOCK			9  // 卡盒关锁
#define EVID_BOXEX_SUB_ID_CFAIL			10  // 卡盒通信失败
#define EVID_BOXEX_SUB_ID_CNORMAL		11  // 卡盒通信正常
#define EVID_BOXEX_SUB_ID_NEEDSET		12  // 卡盒信息需要设置

/*
#define EVID_POWERON 		1				// Machine Power On
#define EVID_LINK			2				// Link event (param 1 Online, 0 offline)
#define EVID_PRESSKEY		3				// Button Pressed (param: channel)
#define EVID_OUTCARDOK		4				// Card ejected  (param: channel)
#define EVID_OUTCARDFAIL	5				// Card ejected  (param: channel)
#define EVID_TAKECARD 		6				// Card taken (param: channel)
#define EVID_STATECHANGE 	7				// Channel state changed (param: channel)
#define EVID_COUNTCHANGE	8				// Channel count changed (param: channel)
#define EVID_RECYCLEOK		9				// Bad card recycled (param: channel)
#define EVID_RECYCLEFAIL	10				// Bad card recycle failed (param: channel)
#define EVID_PRESSPOLKEY	11				// Police key pressed (param: deck) - for ZJ only
#define EVID_RWTIMEOUT		12				// wait for CSC R/W timed out
#define EVID_BUTTONIGNRD	13				// button ignored (param: deck<<8 + 'Button Pressed Ignored Code') 16-->8
#define EVID_BOXLOAD		14				// cartridge loaded (param: channel)
#define EVID_BOXUNLOAD		15				// cartridge unload (param: channel)
#define EVID_SNCHANGED		16				// cartridge S/N changed (change by protocol command)
#define EVID_CHFAULT		17              // channel fault
//#define EVID_CHNORMAL		18              // channel normal
#define EVID_WORKCH_SWITCHOK	19			// working switch ok
#define EVID_THREADEXIT		29				// workthread exir abnormally
#define EVID_TX				30				// protocol packet send to TCR8 machine
#define EVID_RX				31				// protocol packet received from TCR8 machine
#define EVID_KERNELLOG		32				// kernel firmware log output
#define T8EVM_DEFAULT		0x00ffffff		// default event mask
#define T8EVM_ALL			0xffffffff
*/
#ifdef __cplusplus
extern "C" {
#endif

// initial/uninitial
DLLAPI TCR8HANDLE CALLTYPE  TCR8_Create();
DLLAPI void CALLTYPE TCR8_Destroy(TCR8HANDLE h); 
/*
	多平台兼容接口，以字符串形式接入
	dev_name support 
		Windows:
			COM1 | 192.168.1.123
		Linux:
			/dev/ttyAMA3 | 192.168.1.123 
*/  
DLLAPI BOOL CALLTYPE TCR8_SetComPortString(TCR8HANDLE h, const char *dev_name, int nBaudrate);

// 兼容考虑，不破坏原始的接口
#ifdef linux
DLLAPI BOOL CALLTYPE TCR8_SetComPort(TCR8HANDLE h, const char *dev_name, int nBaudrate);
#else
DLLAPI BOOL CALLTYPE TCR8_SetComPort(TCR8HANDLE h, int port, int nBaudrate);
#endif
 
DLLAPI BOOL CALLTYPE TCR8_SetCallback( TCR8HANDLE h, void (*)( void *, int, int) ); 
/* OpenDevice as serial mode */
DLLAPI BOOL CALLTYPE TCR8_OpenDevice(TCR8HANDLE h);
/* OpenDevice as net mode */
DLLAPI BOOL CALLTYPE TCR8_OpenDeviceNet(TCR8HANDLE h, const char *chIP);

DLLAPI BOOL CALLTYPE TCR8_CloseDevice(TCR8HANDLE h);
DLLAPI BOOL CALLTYPE TCR8_Run(TCR8HANDLE h);
DLLAPI BOOL CALLTYPE TCR8_Suspend(TCR8HANDLE h); 
DLLAPI BOOL CALLTYPE TCR8_EjectCard(TCR8HANDLE h, int nChannel );
DLLAPI BOOL CALLTYPE TCR8_ForceEjectCard( TCR8HANDLE h, int nChannel );
DLLAPI BOOL CALLTYPE TCR8_RecycleCard(TCR8HANDLE h);
DLLAPI BOOL CALLTYPE TCR8_ForceRecycleCard( TCR8HANDLE h, int nChannel );
DLLAPI BOOL CALLTYPE TCR8_CancelButton(TCR8HANDLE h);
DLLAPI BOOL CALLTYPE TCR8_SwitchChannel(TCR8HANDLE h, int nChannel );
DLLAPI BOOL CALLTYPE TCR8_SwitchAntenna(TCR8HANDLE h, int nAnt );
DLLAPI BOOL CALLTYPE TCR8_ReturnCard( TCR8HANDLE h, int nChannel );
DLLAPI BOOL CALLTYPE TCR8_CollectCard( TCR8HANDLE h, int nChannel );
DLLAPI BOOL CALLTYPE TCR8_TriggerButton( TCR8HANDLE h, int nChannel );
DLLAPI BOOL CALLTYPE TCR8_PullBackToAnt( TCR8HANDLE h, int nChannel ); 
/**
 * @brief  面板控制，下工位
 * 
 * @param h 
 * @param ops 
 * @return * BOOL 
 */
BOOL TCR8_PushPanel(TCR8HANDLE h, int ops );
/**
 * @brief 带工位的面板控制，gw： 1: 上工位，2 下工位
 * 
 * @param h 
 * @param gw 工位信息
 * @param ops  伸出或缩回
 * @return BOOL 
 */
BOOL TCR8_PushPanelEx(TCR8HANDLE h, int gw, int ops );

BOOL TCR8_QueryPaperRecycle(TCR8HANDLE h, int gw);
DLLAPI BOOL CALLTYPE TCR8_SetCartridgeInfo( TCR8HANDLE h, int nChannel, DWORD dwSN, int nCount ); 
DLLAPI BOOL CALLTYPE TCR8_EnableLog( TCR8HANDLE h, LPCTSTR strPath );		// if strPath is NULL means disable.
DLLAPI BOOL CALLTYPE TCR8_EnableKernelLog( TCR8HANDLE h, BOOL bEnable );
DLLAPI BOOL CALLTYPE TCR8_Log(TCR8HANDLE h, LPCTSTR fmt,...);
DLLAPI LPCTSTR CALLTYPE TCR8_GetEventText( int nEventId, int nParam );
DLLAPI LPCTSTR CALLTYPE TCR8_GetTransStateText(int st);
DLLAPI LPCTSTR CALLTYPE TCR8_GetButtonIgnoreText( int code );
DLLAPI BOOL CALLTYPE TCR8_PlayAudio(TCR8HANDLE h, int nIndex );

#ifdef __cplusplus
}
#endif

// Channel Status bits
#define CSB_CSCONANT			0x01
#define CSB_CSCONEXIT			0x02
#define CSB_CSCTAKEN			0x04
#define CSB_CSCRECYCLE			0x08
#define CSB_BADCSC				0x10
#define CSB_FAILED				0x20
#define CSB_OFFLINE				0x40
#define CSB_NOBOX				0x80
#define CSB_COUNTERCHANGE		0x100

// Button Pressed Ignored Code
#define IBT_DELAY				0
#define IBT_NOCARD				1
#define IBT_FAILED				2
#define IBT_NOTREADY			3
#define IBT_DOWNMODE			4
#define IBT_MASKPERIOD			5
#define IBT_ERRLIMIT			6

// protocol 
#define PROTOCOL_NORMAL		1 // 普通协议
#define PROTOCOL_JIANGSU	2 // 江苏协议
#define PROTOCOL_HNKS		3 // 华南快速协议

// machine type 
#define TCR8_DISPENSER		0
#define TCR8_COLLECTOR		1

// psuedo functions
#define _IsValidHandle(tcr8)			( (tcr8 != NULL) && ((tcr8)->m_dwMagicWord == TCR8_MAGICWORD) )
#define _IsCollector(tcr8)				( ((tcr8)->m_nMachineType == TCR8_COLLECTOR) )
#define _IsOpen(tcr8)					( ((tcr8)->m_tty >= 0) || ((tcr8)->m_sockTCP != INVALID_SOCKET) )
#define _IsConnectWithCom(tcr8)			( (tcr8)->m_tty >= 0 )
#define _IsConnectWithNet(tcr8)			( (tcr8)->m_sockTCP != INVALID_SOCKET )
#define _IsOnline(tcr8)					( (tcr8)->m_bOnline )
#define _IsCardOnAntenna(tcr8,i)		((tcr8)->m_iState[i] & CSB_CSCONANT)
#define _IsCardJustOnAntenna(tcr8,i)	(((tcr8)->m_iState[i] & CSB_CSCONANT) && !((tcr8)->m_iStateLast[i] & CSB_CSCONANT))
#define _HasBadCard(tcr8,i)				((tcr8)->m_iState[i] & CSB_BADCSC)
#define _IsChannelFail(tcr8,i)			((tcr8)->m_iState[i] & CSB_FAILED)
#define _IsChannelJustFail(tcr8,i)		(!((tcr8)->m_iStateLast[i] & CSB_FAILED) && ((tcr8)->m_iState[i] & CSB_FAILED))
#define _IsChannelOffline(tcr8,i)		((tcr8)->m_iState[i] & CSB_OFFLINE)
#define _HasCartridge(tcr8,i)			( ((tcr8)->m_iState[i] & CSB_NOBOX) == 0 )
#define _HadCartridge(tcr8,i)			( ((tcr8)->m_iStateLast[i] & CSB_NOBOX) == 0 )
#define _AnyAbnormal(tcr8,i)			( ((tcr8)->m_iState[i] & (CSB_BADCSC|CSB_FAILED|CSB_OFFLINE)) != 0 )
#define _IsCounterChanged(tcr8,i)		( (tcr8)->m_iState[i] & CSB_COUNTERCHANGE )
#define _SetCounterChanged(tcr8,i) 		( (tcr8)->m_iState[i] |= CSB_COUNTERCHANGE )
#define _ResetCounterChanged(tcr8,i) 	( (tcr8)->m_iState[i] &= ~CSB_COUNTERCHANGE )
#define _CanPrepareCard(tcr8,i)			(_HasCartridge(tcr8,i) && !_IsCardOnAntenna(tcr8,i) && !_AnyAbnormal(tcr8,i) )
#define _IsChannelReady(tcr8,i)			(_IsCardOnAntenna(tcr8,i) && !_AnyAbnormal(tcr8,i) )
#define _IsChannelNotAvailable(tcr8,i) 	(_AnyAbnormal(tcr8,i) || (!_HasCartridge(tcr8,i) && !_IsCardOnAntenna(tcr8,i)) )
#define _IsJiangsuProtocol(tcr8)		( (tcr8)->m_iProtocol == PROTOCOL_JIANGSU)
#define _IsHuaNanKuaiSuProtocol(tcr8)	( (tcr8)->m_iProtocol == PROTOCOL_HNKS)

// get
#define _GetCOMPort(tcr8)				( (tcr8)->m_nPort )
#define _GetBaudRate(tcr8)				( (tcr8)->m_nBaudrate )
#define _GetChannelState(tcr8,i)		( (tcr8)->m_iState[i] )
#define _GetChannelLastState(tcr8,i)	( (tcr8)->m_iStateLast[i] )
#define _GetChannelCount(tcr8,i)		( (tcr8)->m_iCounter[i] )
#define _GetChannelCap(tcr8,i)			( (tcr8)->m_iCap[i] )
#define _GetCartridgeSN(tcr8,i)			( (tcr8)->m_nSerial[i] )
#define _GetUserData(tcr8)				( (tcr8)->m_pUsrData )
#define _GetTransState(tcr8)			( (tcr8)->smTrans )
#define _GetTransChannel(tcr8)			( (tcr8)->m_nChannel )
#define _GetKernelVersion(tcr8)			( (tcr8)->m_hexVersion )
#define _GetActive(tcr8,n)				( (tcr8)->m_iActive[n] )		// get active channel of specified deck (0 upper, 1 lower)
#define _GetGivenCartridgeSn( tcr8, ch, seat )		( (tcr8)->m_nChannelBoxInfo[ch].nSn[seat] )
#define _GetGivenCartridgeCap( tcr8, ch, seat )		( (tcr8)->m_nChannelBoxInfo[ch].nCap[seat] )
#define _GetGivenCartridgeCount( tcr8, ch, seat )	( (tcr8)->m_nChannelBoxInfo[ch].nCount[seat] )
#define _GetCardOnAntBoxId( tcr8, ch )				( (tcr8)->m_nCardOnAntInfo[ch].nSn )

// set
#define _SetBaudRate(tcr8,baudrate)			( (tcr8)->m_nBaudrate = (baudrate == 9600 ? 9600 : 19200) )
#define _SetUserData(tcr8,p)				( (tcr8)->m_pUsrData=p )
#define _SetExtendProtocol( tcr8, ex )		( (tcr8)->m_nExtendProtocol = ex )
#define _ClearCardOnAntInfo( tcr8, ch )		do{ (tcr8)->m_nCardOnAntInfo[ch].nSeat = 0; (tcr8)->m_nCardOnAntInfo[ch].nSn = 0; }while(0)
#define _SetMachineType( tcr8, nType )		( (tcr8)->m_nMachineType = nType )
#define _SetEventMask( tcr8, dwMask )		( (tcr8)->m_dwEventMask = dwMask )
#define StateEolve(h, newState ) \
	do { \
		h->smLastTrans = h->smTrans; \
		h->smTrans = newState; \
	} while (0)

// Last Error Code
#define ERR_NONE		0
#define ERR_JAM			1		// Card Jam
#define ERR_COMM		2		// Offline
#define ERR_FAIL		3		// Machine internal failure
#define ERR_BOX			4		// Cartridge failure
 
#endif

