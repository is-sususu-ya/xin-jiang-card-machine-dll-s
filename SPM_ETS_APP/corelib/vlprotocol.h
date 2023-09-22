

// 车道主要上发的消息有如下类型
// 1. 车道IO信息，是线圈状态
//   >1. 线圈信号，前线圈触发
//	 >2. 抬落杆反馈
// 2. 交易信息
//	>1. 车牌识别抓拍信息，凭证获取信息（OBU，RFID等）
//  >2. 车牌图片信息
//  >3. 车辆处理信息，输入车牌号，临时车入场请求，车辆收费请求，车辆倒车状态
// 2. 车牌图片信息
// 3. 
//
#ifndef _TYPEDEF_HI_U8_
#define _TYPEDEF_HI_U8_
typedef unsigned char HI_U8;
#endif

#ifndef _TYPEDEF_HI_S8_
#define _TYPEDEF_HI_S8_
typedef signed char HI_S8;
#endif


#ifndef _TYPEDEF_HI_U32_
#define _TYPEDEF_HI_U32_
typedef unsigned int HI_U32;
#endif

#ifndef _TYPEDEF_HI_S32_
#define _TYPEDEF_HI_S32_
typedef signed int HI_S32;
#endif

#ifndef _TYPEDEF_HI_S16
#define _TYPEDEF_HI_S16
typedef signed short HI_S16;
#endif

#ifndef _TYPEDEF_HI_U16
#define _TYPEDEF_HI_U16
typedef unsigned short HI_U16;
#endif




#ifdef linux

#define TRUE	1
#define FALSE	0

#define SOCKET_ERROR    (-1)
#define DLLAPI
#define CALLTYPE
//typedef int						BOOL;
#ifndef _TYPEDEF_BOOL_
#define _TYPEDEF_BOOL_
typedef int BOOL;
#endif

#ifndef _TYPEDEF_SOCKET_
#define _TYPEDEF_SOCKET_
typedef int						SOCKET;
#endif
//typedef unsigned char BYTE;
#ifndef _TYPEDEF_BYTE_
#define _TYPEDEF_BYTE_
typedef unsigned char BYTE;
#endif
//typedef unsigned long	DWORD;
#ifndef _TYPEDEF_DWORD_
#define _TYPEDEF_DWORD_
typedef unsigned long DWORD;
#endif
//typedef void *				PVOID;
#ifndef _TYPEDEF_PVOID_
#define _TYPEDEF_PVOID_
typedef void *				PVOID;
#endif
//typedef void *				HANDLE;
#ifndef _TYPEDEF_HANDLE_
#define _TYPEDEF_HANDLE_
typedef void * HANDLE;		
#endif

#ifndef _TYPEDEF_LPCTSTR_
#define _TYPEDEF_LPCTSTR_
typedef const char* LPCTSTR;
#endif

#ifndef _TYPEDEF_LPSTR_
#define _TYPEDEF_LPSTR_
typedef  char* LPSTR;
#endif

#define	IN
#define OUT

#endif

#ifndef VLPPROTOCOL_HEADER_H
#define VLPPROTOCOL_HEADER_H

#define PORT_LISTEN		6008		// TCP
#define PORT_SEARCH		6009		// UDP

// [Data Type]
#if 0 		// for big endian CPU architecture. we want to byte stream sending in sequence of AA BB CC xx

#else		// for little endian CPU architechure.

#define DTYP_MAGIC		0x00ccbbaa
#define DTYP_HBEAT		0x00ccbbaa		// 车道心跳帧数据
#define DTYP_CONF		0x01ccbbaa		// 车道配置数据
#define DTYP_LANE_STAT	0x02ccbbaa		// 车道状态数据
#define DTYP_LOGIN		0x03ccbbaa		// 车道登录成功
#define DTYP_LOGOUT		0x04ccbbaa		// 车道登出事件
#define DTYP_IMAGE		0x05ccbbaa		// camera -> host
#define DTYP_TRANS		0x06ccbbaa		// 车道交易请求
#define DTYP_CTRL		0x07ccbbaa		// 控制字段
#define DTYP_ACK		0x10ccbbaa
#define DTYP_QR			0x11ccbbaa		// QRData
#define DTYP_DEV		0x12ccbbaa		// device info 
#define DTYP_EVT		0x13ccbbaa		// event info
#define DTYP_LED		0x14ccbbaa
#define DTYP_TEXT		0x15ccbbaa

#define DTYP_TIME		0x55ccbbaa		// host -> camera

#define DTYP_MASK		0x00ffffff

#endif

#define CTRL_IMLEVEL			2		// output itermidiate results level. param is level (0~3)

#define DID_LOGIN				0x01

// 显示字符串
#define CTRL_LED_DIS		3
#define CTRL_DEVICE_INFO	4
#define CTRL_SCROLL_PAGE	5
#define CTRL_IO_SET         6
#define CTRL_IO_PULSE		7
// 事件上报
#define EVT_KEYBOARD		1


// LED控制
#define LED_SETPICTURE		1

// 设备状态信息
#define DEV_STATUS			1

// 车辆入场请求
#define TRANS_ENTRY_COMMING		1
// 车辆输入车牌请求
#define TRANS_ENTRY_INPUT_PLAE	2
// 车辆倒车
#define TRANS_CAR_LEAVE			3
// 车辆闯关
#define TRANS_CAR_FORCE_LEAVE	4
// 车辆离场无入口匹配
#define TRANS_CAR_NO_RECORDER	5
// 收费操作
#define TRANS_INCHARGE			6

typedef struct tagTransData
{
	int ulid;
	int TranSn;
	int InSn;
	char plate[15];
	char looptime[25];
	char strVoucherType[10];
	char VoucherNum[64];
	char CaptureImageName[128];
}TransData;
	
typedef struct tagTimeAttr
{
	HI_S16	year;			// 2013
	HI_S16	month;		// 1~12
	HI_S16	day;			// 1~31
	HI_S16 	hour;			// 0 ~ 23
	HI_S16	minute;		// 0 ~ 59
	HI_S16	second;		// 0 ~ 59
	HI_S16	msec;			// 0 ~ 999
} TimeAttr;

typedef struct tagTextAttr{
	char		text[32];
} TextAttr;

typedef struct tagVerAttr {
	char 	strVersion[16];		// 主程序版本号
	int		algver;					// 算法版本号
	int 	param[3];				// 0: 主程序版本号二进制值，1:摄像机型号+Sensor型号，2:条件编译使能的capability
} VerAttr;

typedef struct tagEvtAttr {
	int 	param;
	char  text[28];
} EvtAttr;

typedef struct tagCtrlAttr
{
	HI_S32	code;
	HI_S32	param;
	HI_U8	option[24];
} CtrlAttr;

typedef struct tagLEDAttr
{
	HI_S32	code;
	HI_S32	param;
	HI_S32	option[6];
} LEDAttr;

typedef struct tagLogInAttr
{  
	HI_S32	ulid;				 // 车道id，登录时，该id必须和车道匹配,
	HI_S32	id;					 // 用户id，
	HI_S32	success;
	char	description[20];  	 // 登录描述
}LogInAttr;

typedef struct tagLaneStatusAttr{
	HI_S32 ulid;
	HI_S32 di_last;
	HI_S32 di_now;
	HI_U8  option[20];
}LaneStatusAttr;

typedef struct tagTansAttr
{
	HI_S32	type;
	HI_S32	code;
	HI_S32	param;
	HI_U8	option[20];
}TransAttr;

typedef struct tagDeviceInfoAttr
{
	HI_S32	type;
	HI_U8	option[28];
}DeviceInfoAttr;


typedef struct tagDataHeader
{
	HI_S32 DataType;
	HI_S32 DataId;
	union {
		VerAttr		verAttr;		
		CtrlAttr 	ctrlAttr;
		LEDAttr		ledAttr;
		EvtAttr		evtAttr;
		TimeAttr	timeAttr;
		TextAttr	textAttr;
		LogInAttr	logInAttr;
		LaneStatusAttr statAttr;
		TransAttr	transAttr;
		DeviceInfoAttr devAttr; 
	};
	HI_S32	size;
}DataHeader;

#define LOGIN_HEADER_INTI( hdr, i, u, d ) \
	do{\
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_LOGIN; \
		hdr.logInAttr.ulid = i; \
		hdr.logInAttr.id = u; \
		strcpy(hdr.logInAttr.description, d );\
	}while(0)

#define DEV_INFO_INTI( hdr, type ) \
	do{\
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_DEV; \
		hdr.DataId = type;\
	}while(0)
	
#define EVT_HEADER_INIT( hdr, type, msg )\
	do{\
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_EVT; \
		hdr.DataId = type;\
		memcpy( hdr.evtAttr.text, msg, sizeof( hdr.evtAttr.text ));\
	}while(0)
	

#define QR_HEADER_INTI( hdr, type ) \
	do{\
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_QR; \
		hdr.DataId = type;\
	}while(0)

#define LED_HEADER_INIT( hdr, c, p )	\
do {\
	memset(&hdr, 0, sizeof(hdr)); \
	hdr.DataType = DTYP_LED; \
	hdr.ledAttr.code = c; \
	hdr.ledAttr.param = p; \
} while (0)


#define CTRL_HEADER_INIT( hdr, c, p )	\
	do { \
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_CTRL; \
		hdr.ctrlAttr.code = c; \
		hdr.ctrlAttr.param = p; \
	} while (0)
	
#define TEXT_HEADER_INIT( hdr, id, s ) \
	do { \
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_TEXT; \
		hdr.DataId = id; \
		strncpy( hdr.textAttr.text, s, 32 ); \
		hdr.textAttr.text[31] = '\0'; \
	} while (0)
		
#define REPLY_HEADER_INIT( hdr, id ) \
	do { \
		memset( &(hdr), 0, sizeof(hdr) ); \
		(hdr).DataType = DTYP_REPLY; \
		(hdr).DataId = id; \
	} while (0)

#define ACK_HEADER_INIT( hdr ) \
	do { \
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_ACK; \
	} while (0)

#define HBEAT_HEADER_INIT( hdr ) \
	do { \
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_HBEAT; \
	} while (0)

#define VERSION_HEADER_INIT( hdr, algv, s, p1, p2, p3 ) \
	do { \
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_DATA; \
		hdr.DataId = DID_VERSION; \
		strncpy( hdr.verAttr.strVersion, s, 16 ); \
		hdr.verAttr.strVersion[15] = '\0'; \
		hdr.verAttr.algver = algv; \
		hdr.verAttr.param[0] = p1; \
		hdr.verAttr.param[1] = p2; \
		hdr.verAttr.param[2] = p3; \
	} while (0)

#define IO_STAT_HEADER_INIT( hdr, u, l, n ) \
	do{\
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_LANE_STAT; \
		hdr.DataId = 0; \
		hdr.statAttr.ulid = (u);\
		hdr.statAttr.di_last = (l);\
		hdr.statAttr.di_now = (n);\
	}while(0)


#define TIME_HEADER_INIT( hdr, y, m, d, hh, mm, ss, ms ) \
do {\
	memset(&hdr, 0, sizeof(hdr)); \
	hdr.DataType = DTYP_TIME; \
	hdr.timeAttr.year = y; \
	hdr.timeAttr.month = m; \
	hdr.timeAttr.day = d; \
	hdr.timeAttr.hour = hh; \
	hdr.timeAttr.minute = mm; \
	hdr.timeAttr.second = ss; \
	hdr.timeAttr.msec = ms; \
} while (0)

#define CONF_HEADER_INIT( hdr, type, sz ) do { \
		memset(&hdr, 0, sizeof(hdr)); \
		hdr.DataType = DTYP_CONF; \
		hdr.DataId = type; \
		hdr.size = sz;\
} while (0)

#define IMGID_PLATEBIN				0		// 车牌二值图
#define IMGID_PLATECOLOR		    1		// 车牌彩色图
#define IMGID_CAPFULL				2		// 抓拍全图
#define IMGID_CAPMIDDLE			    3		// 抓拍中图 (4/9解像度)


// [Data Id]
// image Id - full frame image (when data type is DTYP_IMAGE) 
#define IID_PLBIN			0
#define IID_PLRGB			1
#define IID_CAP_FULL		2		// plate binarized after de-clutter (level 1)
#define IDD_CAP_MID			3
#define IDD_MAXID			4

#define CID_AP_SYS			1

#endif
