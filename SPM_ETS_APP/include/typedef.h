#include <longtime.h>
#ifndef TYPEDEF_HEADER_H
#define TYPEDEF_HEADER_H

#define TRUE 1
#define FALSE 0

#define SOCKET_ERROR (-1)
#define DLLAPI
#define CALLTYPE

#ifndef _TYPEDEF_BOOL_
#define _TYPEDEF_BOOL_
typedef int BOOL;
#endif

#ifndef _TYPEDEF_SOCKET_
#define _TYPEDEF_SOCKET_
typedef int SOCKET;
#endif
// typedef unsigned char BYTE;
#ifndef _TYPEDEF_BYTE_
#define _TYPEDEF_BYTE_
typedef unsigned char BYTE;
#endif
// typedef unsigned long	DWORD;
#ifndef _TYPEDEF_DWORD_
#define _TYPEDEF_DWORD_
typedef unsigned long DWORD;
#endif
// typedef void *				PVOID;
#ifndef _TYPEDEF_PVOID_
#define _TYPEDEF_PVOID_
typedef void *PVOID;
#endif

#ifndef _TYPEDEF_UINT8_
#define _TYPEDEF_UINT8_
typedef unsigned char UINT8;
#endif

#ifndef _TYPEDEF_UINT16_
#define _TYPEDEF_UINT16_
typedef unsigned short UINT16;
#endif

#ifndef _TYPEDEF_UINT32_
#define _TYPEDEF_UINT32_
typedef unsigned int UINT32;
#endif

#ifndef _TYPEDEF_HANDLE_
#define _TYPEDEF_HANDLE_
typedef void *HANDLE;
#endif

#ifndef _TYPEDEF_LPCTSTR_
#define _TYPEDEF_LPCTSTR_
typedef const char *LPCTSTR;
#endif

#ifndef _TYPEDEF_LPSTR_
#define _TYPEDEF_LPSTR_
typedef char *LPSTR;
#endif
#define IN
#define OUT

#define true 1
#define false 0

#define FMVER_YMD(y, m, d) ((y << 16) | (m << 8) | (d))
#define Y_FMVER(fmver) (fmver >> 16)
#define M_FMVER(fmver) ((fmver >> 8) & 0xFF)
#define D_FMVER(fmver) (fmver & 0xFF)

#define BASE_MODULE_STRUCT       \
	struct                       \
	{                            \
		int check_size;          \
		int version;             \
		int type;                \
		char *name;              \
		char *description;       \
		int (*initialize)(void); \
		int (*terminate)(void);  \
		BOOL(*is_type)           \
		(int type);              \
	}

typedef BASE_MODULE_STRUCT cBaseModule;

typedef void (*MODULE_CALL_BACK)(void *h, int type, void *arg);

typedef struct tagIOModule
{
	BASE_MODULE_STRUCT; // 共同部分，相当于base class
	HANDLE(*open)
	(const char *args);
	int (*close)(HANDLE h);
	int (*enable)(HANDLE h, int bEnable);
	int (*read)(HANDLE h, void *buf, int size);
	int (*write)(HANDLE h, void *buf, int size);
	int (*ioctl)(HANDLE h, int cmd, int param, void *args);
	void (*register_callback)(MODULE_CALL_BACK cb);
	int (*get_status)(HANDLE h, int *status);
	void (*qr_ctrl)(int opt);
} cIOModule;

typedef struct tagModuleInfo
{
	char *path;
	void *dlh; // handle of dynamic library file
	cBaseModule *mod;
} MODULE_INFO_S;

#define HANDLE_MODULE(h) ((cIOModule *)(*(int *)h))
cIOModule *module_find(int type, int vendor_model);
void module_terminate();

#define VER_MM(major, minor) (((major) << 16) | (minor))
#define MAJOR_VER(fmver) ((fmver) >> 16)
#define MINOR_VER(fmver) ((fmver) & 0xFFFF)
#define MOD_TYPE(mt, vm) ((vm) << 16 | (mt))

#define MTYP_LPNR 100 // 车牌识别
#define MTYP_LED 101  // 显示屏
#define MTYP_SCAN 102 // 显示屏
#define MTYP_PAY 103  // 支付服务
#define MTYP_QR 104	  // QR代码
#define MTYP_WEB 105  // WEB请求模块

#define MOD_IS_ONLY 0
#define LED_IS_TRANSPEED 1 // 德亚费显屏幕
#define PAY_IS_XINJIANG 1  // 新疆支付
#define PAY_IS_TRANSPEED 2 // 德亚缴费机
#define QR_IS_TRANSPEED 1
#define QR_IS_TRANSPEED_TTY 2

static int get_module_type(const char *mode_name)
{
	if (strcmp(mode_name, "朗为缴费机") == 0)
		return MOD_TYPE(MTYP_PAY, PAY_IS_TRANSPEED);
	if (strcmp(mode_name, "HTTP") == 0)
		return MOD_TYPE(MTYP_WEB, MOD_IS_ONLY);
	if (strcmp(mode_name, "德亚费显") == 0)
		return MOD_TYPE(MTYP_LED, LED_IS_TRANSPEED);
	if (strcmp(mode_name, "新疆缴费机") == 0)
		return MOD_TYPE(MTYP_PAY, PAY_IS_XINJIANG);
	if (strcmp(mode_name, "德亚扫码枪") == 0)
		return MOD_TYPE(MTYP_QR, QR_IS_TRANSPEED);
	if (strcmp(mode_name, "德亚串口扫码枪") == 0)
		return MOD_TYPE(MTYP_QR, QR_IS_TRANSPEED_TTY);
	return MOD_TYPE(0, 0);
}

// 支付类型
typedef enum
{
	PAYCHN_UNKNOW = 0,
	PAYCHN_WECHAT,	// 微信支付
	PAYCHN_ALI,		// 阿里支付宝密码
	PAYCHN_UMS_PAY, // 银联支付
	PAYCHN_CLOUD,	// 云闪付
	PAYCHN_ETC		// ETC卡支付
} PAY_CHN_E;

typedef enum
{
	PAYTYP_UNKNOW = 0, // 未知支付类型
	PAYTYP_APP,		   //  App 支付,
	PAYTYP_LITE,	   // 小程序支付,
	PAYTYP_H5,		   // h5 支付,
	PAYTYP_PUB,		   // 公众号支付,
	PAYTYP_QRCODE,	   // 条码支付
	PAYTYP_CONTRACT	   // 免密支付
} PAY_TYPE_E;

// 新疆缴费机字段
static const char *get_strPayChn(PAY_CHN_E type)
{
	switch ((int)type)
	{
	case PAYCHN_UNKNOW:
		return "PAY_UNKNOW";
		break;
	case PAYCHN_ALI:
		return "ALI_PAY";
		break;
	case PAYCHN_WECHAT:
		return "TENCENT_PAY";
		break;
	case PAYCHN_UMS_PAY:
		return "UNION_PAY";
		break;
	default:
		return "PAY_UNKNOW";
		break;
	}
}

static const char *get_strPayType(PAY_TYPE_E type)
{
	switch ((int)type)
	{
	case PAYTYP_UNKNOW:
		return "PAY_UNKNOW";
		break;
	case PAYTYP_APP:
		return "PAY_APP";
		break;
	case PAYTYP_LITE:
		return "PAY_LITE";
		break;
	case PAYTYP_H5:
		return "PAY_H5";
		break;
	case PAYTYP_PUB:
		return "PAY_PUB";
		break;
	case PAYTYP_QRCODE:
		return "PAY_QRCODE";
		break;
	case PAYTYP_CONTRACT:
		return "PAY_CONTRACT";
		break;
	default:
		return "PAY_UNKNOW";
		break;
	}
}

typedef enum
{
	TFSM_CREATE,	// 创建交易
	TFSM_WAIT_QR,	// 等待QR二维码
	TFSM_PAING,		// 支付中
	TFSM_QUERYING,	// 查询中
	TFSM_REFUNDING, // 退款中
	TFSM_DONE,		// 交易结束
} TRASN_FSM_E;

static const char *getStrFsm(TRASN_FSM_E type)
{
	switch (type)
	{
	case TFSM_CREATE:
		return "TFSM_CREATE";
		break;
	case TFSM_WAIT_QR:
		return "TFSM_WAIT_QR";
		break;
	case TFSM_PAING:
		return "TFSM_PAING";
		break;
	case TFSM_QUERYING:
		return "TFSM_QUERYING";
		break;
	case TFSM_REFUNDING:
		return "TFSM_REFUNDING";
		break;
	case TFSM_DONE:
		return "TFSM_DONE";
		break;
	default:
		return "TFSM_UNKNOW";
		break;
	}
}

typedef enum
{
	PAY_UNPAID = 0,
	PAY_HAS_PAID,
} PayStatE;

// 当对接模块的ioctl调用相关请求时，需要修改交易结构里的reply字段
typedef enum
{
	Reply_Web_Failed,	   // 网络失败
	Reply_Pay_Success,	   // 支付请求发送成功
	Reply_Pay_Failed,	   // 支付请求发送,缺钱？
	Reply_Query_HasPaied,  // 扣费成功
	Reply_Query_NoPaied,   // 未支付成功
	Reply_Query_Exception, // 支付中发现其他错误，需要重新开启交易
	Reply_ReFund_Success,  // 退款成功
	Reply_ReFund_Failed,   // 退款失败
} WebReplyE;

typedef enum
{
	SERVER_NONE_ERROR = 0,
	SERVER_ERROR_HAPPEN
} SERVER_STATUS;

typedef struct tagTarnsUnit
{
	int nTransNum;
	UINT8 trans_id[64];			 // 交易ID
	char order_num[128];		 // 订单号
	char refund_num[128];		 // 退款订单号
	SERVER_STATUS server_status; // 服务器状态
	char plate[20];
	_longtime ltCreate; // 创建时间
	_longtime ltScan;	// 接收到二维码时间，也既支付时间
	_longtime ltDone;	// 交易完成时间
	int pay_cnt;
	int query_cnt;
	int refund_error_code; // 退款失败码
	PayStatE has_pay;
	TRASN_FSM_E last_fsm;
	TRASN_FSM_E now_fsm; // 交易状态
	int total_money;	 // 交易金额
	WebReplyE reply;
	PAY_CHN_E pay_chn;		 // 支付通道
	PAY_TYPE_E pay_type;	 // 交易支付类型
	char str_trans_time[30]; // 交易时间值
	int utc_time;			 // UTC时间值表示
	char qr_code[100];
	char reserv[256];
} TransUnitS;

typedef struct tagDevStat
{
	char serial_num[64];	// 设备串号
	int is_computer_online; // 上位机是否离线
	int is_server_crupted;	// 是否服务器挂了
	char mobile_ip[20];		// 4G模块IP
	char local_ip[20];		// 本地IP
	char app_version[20];	// app版本
} DevStatusS;

#define UPDATE_FSM(h, f)              \
	do                                \
	{                                 \
		(h)->last_fsm = (h)->now_fsm; \
		(h)->now_fsm = f;             \
	} while (0)

// 对接模块的控制主要通过IOC方式，因为好像read
// 和write好像也没什么好做的
// 对接模块，必须实现如下IOC
// 分别为创建缴费交易，处理交易交易网络返回
// 创建查询交易，处理查询交易返回
// 创建退款交易, 处理退款交易返回
// IOC接口下列定义cmd码，param码为0表示请求，非0，表示请求错误
#define IOC_INIT_TRANS 0x00	   // 获取金额数
#define IOC_PAY_START 0x01	   // 缴费请求，
#define IOC_PAY_QUERY 0x02	   // 缴费后查询请求
#define IOC_PAY_REFUND 0x03	   // 退费请求
#define IOC_PAY_UPLOAD 0x04	   // 缴费成功后，缴费记录上传
#define IOC_PAY_CANCEL 0x05	   // 取消缴费
#define IOC_REPPAY_START 0xf1  // 缴费请求，
#define IOC_REPPAY_QUERY 0xf2  // 缴费后查询请求
#define IOC_REPPAY_REFUND 0xf3 // 退费请求
#define IOC_REPPAY_UPLOAD 0xf4 // 缴费成功后，缴费记录上传
#define IOC_REPPAY_CANCEL 0xf5 // 取消缴费

#define IOC_UPLOAD_DVE_STAT 0x0f6

// 缴费成功后状态上传?

// LED模块设置LED信息
#define IOC_SET_LED_INFO 0x01 // 设置费显信息
// WEB模块添加结果，获取结果
#define IOC_WEB_ADD_TASK 0x01
#define IOC_WEB_GET_REPONSE 0x02

// 任何一个实现厂家模块有消息通知主进程时只能如下支持的消息类型，
// 如果有其他消息，再另外添加
#define PAY_EVT_START 0x01	// 开始一个交易
#define PAY_EVT_QUERY 0x02	// 查询订单状态
#define PAY_EVT_CANCEL 0x03 // 取消订单（删除交易队列里的该订单？）
#define PAY_EVT_REFUND 0x04 // 退款
#define PAY_EVT_ONLINE 0x05 //
#define PAY_EVT_OFFLINE 0x06
// 如果对接模块需要控制外设，则在回调事件中发送消息如下事件
// 回调的arg参数为实际LED显示内容，显示内容格式按照德亚文字显示字串标准，对接模块需要做到此兼容
// 暂时对接模块只提供费显控制接口
#define PAY_EVT_CTR_LED 0x07
#define PAY_EVT_CTR_VOICE 0x08

#define PAY_EVT_CSB_READY 0x09	// CSB模式，已经获取到支付二维码
#define PAY_EVT_CSB_FAILED 0x0a // CSB模式，获取支付二维码失败

static const char *getStrPayEvent(int code)
{
	switch (code)
	{
	case PAY_EVT_START:
		return "支付模块发起支付指令";
		break;
	case PAY_EVT_QUERY:
		return "支付模块发起查询指令";
		break;
	case PAY_EVT_CANCEL:
		return "支付模块发起取消指令";
		break;
	case PAY_EVT_REFUND:
		return "支付模块发起退款指令";
		break;
	case PAY_EVT_ONLINE:
		return "支付模块发起设备上线指令";
		break;
	case PAY_EVT_OFFLINE:
		return "支付模块发起设备离线线指令";
		break;
	case PAY_EVT_CTR_LED:
		return "支付模块发起费显控制指令";
		break;
	case PAY_EVT_CTR_VOICE:
		return "支付模块发起的语音播放指令";
		break;
	default:
		return "支付模块发起未知指令..";
		break;
	}
}

// Qr模块或者费显模块有二维码回调此内容
#define QR_EVT_READY 0x11
#define QR_EVT_KEYPRESS 0x12

// WEB模块回调事件
#define WEB_EVT_RESPONSE 0x21

typedef struct
{
	int pay_type;			// 对接哪家支付类型
	char str_pay_type[128]; // 对接厂家名称
	BOOL has_led;
	char dev_led[40];
	BOOL has_qr;
	char dev_qr[40];
	int pay_timeout;
} SysTemConfig;

typedef struct
{
	int type;			// 请求类型
	int stat_code;		// 状态码
	char trans_id[128]; // 交易ID
	char url[256];		// 请求URL
	char body[4096];	// 请求报文，或者结果报文
} WEBTrans_Contex;

// 错误码，用在创建交易发起时，程序反馈给上位机的状态。。
// 该部分内容仅用于通过串口上报后台
// 串口对接协议,上位机发起支付,交易结果上传时返回
#define ERROR_NONE 0x00			// 调用成功
#define ERROR_SYSTEM_ERROR 0x01 // 终端异常
#define ERROR_SERVER_ERROR 0x02 // 服务器异常
#define ERROR_TRANS_EXIST 0x03	// 支付已存在
#define ERROR_NET_ERROR 0x04	// 网络错误
#define ERROR_QRCODE_ERR 0x05	// 二维码错误
#define ERROR_LACK_MONEY 0x06	// 余额不足
#define ERROR_INPUT_PWD 0x07	// 输入密码完成支付
#define ERROR_NOT_PAY 0x08		// 暂未支付
// 查询订单请求返回值
#define REP_QUERY_TRANS_SUCCESS 0x00   // 成功
#define REP_QUERY_SERVER_ERROR 0x01	   // 服务器异常
#define REP_QUERY_TRANS_NOT_EXIST 0x02 // 查询响应，无此交易
// 退款回复
#define REP_REFUND_SUCCESS 0x00			// 退款成功
#define REP_REFUND_TRANS_NOT_EXIST 0x01 // 交易ID不存在
#define REP_REFUND_MONEY_ERROR 0x02		// 输入的退款金额有误
#define REP_REFUND_PAY_FAILED 0x03		// 退款失败
#define REP_REFUND_NOT_PAIED 0x04		// 未付款
// 取消订单呢
#define REP_CANCEL_TRANS_SUCCESS 0x00	// 成功
#define REP_CANCEL_SERVER_ERROR 0x01	// 服务器未连接
#define REP_CANCEL_TRANS_NOT_EXIST 0x02 // 当前无订单

#define PAYTYP_WECHAT 0x01 // 微信支付
#define PAYTYP_ALIPAY 0x02 // 阿里支付宝支付
#define PAYTYP_USM 0x03	   // 银联支付
#define PAYTYP_CLOUD 0x04  // 云闪付
#define PAYTYP_ETC 0x05	   // ETC卡

// 新疆缴费机终端对接后台返回码，文档：支付平台对外接口
#define SUBCODE_SUCCESS 0x00			 // 调用成功
#define SUBCODE_SYSTEM_BUSY -1			 // 系统繁忙
#define SUBCODE_ERROR_OTHER 10000		 // 其他错误
#define SUBCODE_ERROR_INPUT_ARGS 10001	 // 输入参数错误
#define SUBCODE_ERROR_DB 10002			 // 数据库入库失败
#define SUBCODE_ERROR_CACHE 10003		 // 数据库缓存失败
#define SUBCODE_ERROR_QRCODE 10004		 // 付款码失效
#define SUBCODE_ERROR_LACK_MONEY 10005	 // 余额不足
#define SUBCODE_ERROR_CALL_TIMEOUT 20000 // 通道调用超时
#define SUBCODE_ERROR_CALL_FAULT 20001	 // 通道调用错误
#define SUBCODE_ERROR_PAYING 20002		 // 支付中

#define PAY_NONE 0
#define PAY_XINJIANG 1
#define PAY_RUNWELL 2
#define PAY_SICHUAN 3

#define IOC_LEDMODULE_PLAYDU 1
#define IOC_LEDMODULE_SETPIC 2
#define IOC_LEDMODULE_DISPLAY 10
#define IOC_LEDMODULE_SOUNDPLAY 11
#define IOC_LEDMODULE_CLEAR 12

#endif
