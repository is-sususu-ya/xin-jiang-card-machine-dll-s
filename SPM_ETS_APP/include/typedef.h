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
	BASE_MODULE_STRUCT; // ��ͬ���֣��൱��base class
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

#define MTYP_LPNR 100 // ����ʶ��
#define MTYP_LED 101  // ��ʾ��
#define MTYP_SCAN 102 // ��ʾ��
#define MTYP_PAY 103  // ֧������
#define MTYP_QR 104	  // QR����
#define MTYP_WEB 105  // WEB����ģ��

#define MOD_IS_ONLY 0
#define LED_IS_TRANSPEED 1 // ���Ƿ�����Ļ
#define PAY_IS_XINJIANG 1  // �½�֧��
#define PAY_IS_TRANSPEED 2 // ���ǽɷѻ�
#define QR_IS_TRANSPEED 1
#define QR_IS_TRANSPEED_TTY 2

static int get_module_type(const char *mode_name)
{
	if (strcmp(mode_name, "��Ϊ�ɷѻ�") == 0)
		return MOD_TYPE(MTYP_PAY, PAY_IS_TRANSPEED);
	if (strcmp(mode_name, "HTTP") == 0)
		return MOD_TYPE(MTYP_WEB, MOD_IS_ONLY);
	if (strcmp(mode_name, "���Ƿ���") == 0)
		return MOD_TYPE(MTYP_LED, LED_IS_TRANSPEED);
	if (strcmp(mode_name, "�½��ɷѻ�") == 0)
		return MOD_TYPE(MTYP_PAY, PAY_IS_XINJIANG);
	if (strcmp(mode_name, "����ɨ��ǹ") == 0)
		return MOD_TYPE(MTYP_QR, QR_IS_TRANSPEED);
	if (strcmp(mode_name, "���Ǵ���ɨ��ǹ") == 0)
		return MOD_TYPE(MTYP_QR, QR_IS_TRANSPEED_TTY);
	return MOD_TYPE(0, 0);
}

// ֧������
typedef enum
{
	PAYCHN_UNKNOW = 0,
	PAYCHN_WECHAT,	// ΢��֧��
	PAYCHN_ALI,		// ����֧��������
	PAYCHN_UMS_PAY, // ����֧��
	PAYCHN_CLOUD,	// ������
	PAYCHN_ETC		// ETC��֧��
} PAY_CHN_E;

typedef enum
{
	PAYTYP_UNKNOW = 0, // δ֪֧������
	PAYTYP_APP,		   //  App ֧��,
	PAYTYP_LITE,	   // С����֧��,
	PAYTYP_H5,		   // h5 ֧��,
	PAYTYP_PUB,		   // ���ں�֧��,
	PAYTYP_QRCODE,	   // ����֧��
	PAYTYP_CONTRACT	   // ����֧��
} PAY_TYPE_E;

// �½��ɷѻ��ֶ�
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
	TFSM_CREATE,	// ��������
	TFSM_WAIT_QR,	// �ȴ�QR��ά��
	TFSM_PAING,		// ֧����
	TFSM_QUERYING,	// ��ѯ��
	TFSM_REFUNDING, // �˿���
	TFSM_DONE,		// ���׽���
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

// ���Խ�ģ���ioctl�����������ʱ����Ҫ�޸Ľ��׽ṹ���reply�ֶ�
typedef enum
{
	Reply_Web_Failed,	   // ����ʧ��
	Reply_Pay_Success,	   // ֧�������ͳɹ�
	Reply_Pay_Failed,	   // ֧��������,ȱǮ��
	Reply_Query_HasPaied,  // �۷ѳɹ�
	Reply_Query_NoPaied,   // δ֧���ɹ�
	Reply_Query_Exception, // ֧���з�������������Ҫ���¿�������
	Reply_ReFund_Success,  // �˿�ɹ�
	Reply_ReFund_Failed,   // �˿�ʧ��
} WebReplyE;

typedef enum
{
	SERVER_NONE_ERROR = 0,
	SERVER_ERROR_HAPPEN
} SERVER_STATUS;

typedef struct tagTarnsUnit
{
	int nTransNum;
	UINT8 trans_id[64];			 // ����ID
	char order_num[128];		 // ������
	char refund_num[128];		 // �˿����
	SERVER_STATUS server_status; // ������״̬
	char plate[20];
	_longtime ltCreate; // ����ʱ��
	_longtime ltScan;	// ���յ���ά��ʱ�䣬Ҳ��֧��ʱ��
	_longtime ltDone;	// �������ʱ��
	int pay_cnt;
	int query_cnt;
	int refund_error_code; // �˿�ʧ����
	PayStatE has_pay;
	TRASN_FSM_E last_fsm;
	TRASN_FSM_E now_fsm; // ����״̬
	int total_money;	 // ���׽��
	WebReplyE reply;
	PAY_CHN_E pay_chn;		 // ֧��ͨ��
	PAY_TYPE_E pay_type;	 // ����֧������
	char str_trans_time[30]; // ����ʱ��ֵ
	int utc_time;			 // UTCʱ��ֵ��ʾ
	char qr_code[100];
	char reserv[256];
} TransUnitS;

typedef struct tagDevStat
{
	char serial_num[64];	// �豸����
	int is_computer_online; // ��λ���Ƿ�����
	int is_server_crupted;	// �Ƿ����������
	char mobile_ip[20];		// 4Gģ��IP
	char local_ip[20];		// ����IP
	char app_version[20];	// app�汾
} DevStatusS;

#define UPDATE_FSM(h, f)              \
	do                                \
	{                                 \
		(h)->last_fsm = (h)->now_fsm; \
		(h)->now_fsm = f;             \
	} while (0)

// �Խ�ģ��Ŀ�����Ҫͨ��IOC��ʽ����Ϊ����read
// ��write����Ҳûʲô������
// �Խ�ģ�飬����ʵ������IOC
// �ֱ�Ϊ�����ɷѽ��ף������׽������緵��
// ������ѯ���ף������ѯ���׷���
// �����˿��, �����˿�׷���
// IOC�ӿ����ж���cmd�룬param��Ϊ0��ʾ���󣬷�0����ʾ�������
#define IOC_INIT_TRANS 0x00	   // ��ȡ�����
#define IOC_PAY_START 0x01	   // �ɷ�����
#define IOC_PAY_QUERY 0x02	   // �ɷѺ��ѯ����
#define IOC_PAY_REFUND 0x03	   // �˷�����
#define IOC_PAY_UPLOAD 0x04	   // �ɷѳɹ��󣬽ɷѼ�¼�ϴ�
#define IOC_PAY_CANCEL 0x05	   // ȡ���ɷ�
#define IOC_REPPAY_START 0xf1  // �ɷ�����
#define IOC_REPPAY_QUERY 0xf2  // �ɷѺ��ѯ����
#define IOC_REPPAY_REFUND 0xf3 // �˷�����
#define IOC_REPPAY_UPLOAD 0xf4 // �ɷѳɹ��󣬽ɷѼ�¼�ϴ�
#define IOC_REPPAY_CANCEL 0xf5 // ȡ���ɷ�

#define IOC_UPLOAD_DVE_STAT 0x0f6

// �ɷѳɹ���״̬�ϴ�?

// LEDģ������LED��Ϣ
#define IOC_SET_LED_INFO 0x01 // ���÷�����Ϣ
// WEBģ����ӽ������ȡ���
#define IOC_WEB_ADD_TASK 0x01
#define IOC_WEB_GET_REPONSE 0x02

// �κ�һ��ʵ�ֳ���ģ������Ϣ֪ͨ������ʱֻ������֧�ֵ���Ϣ���ͣ�
// �����������Ϣ�����������
#define PAY_EVT_START 0x01	// ��ʼһ������
#define PAY_EVT_QUERY 0x02	// ��ѯ����״̬
#define PAY_EVT_CANCEL 0x03 // ȡ��������ɾ�����׶�����ĸö�������
#define PAY_EVT_REFUND 0x04 // �˿�
#define PAY_EVT_ONLINE 0x05 //
#define PAY_EVT_OFFLINE 0x06
// ����Խ�ģ����Ҫ�������裬���ڻص��¼��з�����Ϣ�����¼�
// �ص���arg����Ϊʵ��LED��ʾ���ݣ���ʾ���ݸ�ʽ���յ���������ʾ�ִ���׼���Խ�ģ����Ҫ�����˼���
// ��ʱ�Խ�ģ��ֻ�ṩ���Կ��ƽӿ�
#define PAY_EVT_CTR_LED 0x07
#define PAY_EVT_CTR_VOICE 0x08

#define PAY_EVT_CSB_READY 0x09	// CSBģʽ���Ѿ���ȡ��֧����ά��
#define PAY_EVT_CSB_FAILED 0x0a // CSBģʽ����ȡ֧����ά��ʧ��

static const char *getStrPayEvent(int code)
{
	switch (code)
	{
	case PAY_EVT_START:
		return "֧��ģ�鷢��֧��ָ��";
		break;
	case PAY_EVT_QUERY:
		return "֧��ģ�鷢���ѯָ��";
		break;
	case PAY_EVT_CANCEL:
		return "֧��ģ�鷢��ȡ��ָ��";
		break;
	case PAY_EVT_REFUND:
		return "֧��ģ�鷢���˿�ָ��";
		break;
	case PAY_EVT_ONLINE:
		return "֧��ģ�鷢���豸����ָ��";
		break;
	case PAY_EVT_OFFLINE:
		return "֧��ģ�鷢���豸������ָ��";
		break;
	case PAY_EVT_CTR_LED:
		return "֧��ģ�鷢����Կ���ָ��";
		break;
	case PAY_EVT_CTR_VOICE:
		return "֧��ģ�鷢�����������ָ��";
		break;
	default:
		return "֧��ģ�鷢��δָ֪��..";
		break;
	}
}

// Qrģ����߷���ģ���ж�ά��ص�������
#define QR_EVT_READY 0x11
#define QR_EVT_KEYPRESS 0x12

// WEBģ��ص��¼�
#define WEB_EVT_RESPONSE 0x21

typedef struct
{
	int pay_type;			// �Խ��ļ�֧������
	char str_pay_type[128]; // �Խӳ�������
	BOOL has_led;
	char dev_led[40];
	BOOL has_qr;
	char dev_qr[40];
	int pay_timeout;
} SysTemConfig;

typedef struct
{
	int type;			// ��������
	int stat_code;		// ״̬��
	char trans_id[128]; // ����ID
	char url[256];		// ����URL
	char body[4096];	// �����ģ����߽������
} WEBTrans_Contex;

// �����룬���ڴ������׷���ʱ������������λ����״̬����
// �ò������ݽ�����ͨ�������ϱ���̨
// ���ڶԽ�Э��,��λ������֧��,���׽���ϴ�ʱ����
#define ERROR_NONE 0x00			// ���óɹ�
#define ERROR_SYSTEM_ERROR 0x01 // �ն��쳣
#define ERROR_SERVER_ERROR 0x02 // �������쳣
#define ERROR_TRANS_EXIST 0x03	// ֧���Ѵ���
#define ERROR_NET_ERROR 0x04	// �������
#define ERROR_QRCODE_ERR 0x05	// ��ά�����
#define ERROR_LACK_MONEY 0x06	// ����
#define ERROR_INPUT_PWD 0x07	// �����������֧��
#define ERROR_NOT_PAY 0x08		// ��δ֧��
// ��ѯ�������󷵻�ֵ
#define REP_QUERY_TRANS_SUCCESS 0x00   // �ɹ�
#define REP_QUERY_SERVER_ERROR 0x01	   // �������쳣
#define REP_QUERY_TRANS_NOT_EXIST 0x02 // ��ѯ��Ӧ���޴˽���
// �˿�ظ�
#define REP_REFUND_SUCCESS 0x00			// �˿�ɹ�
#define REP_REFUND_TRANS_NOT_EXIST 0x01 // ����ID������
#define REP_REFUND_MONEY_ERROR 0x02		// ������˿�������
#define REP_REFUND_PAY_FAILED 0x03		// �˿�ʧ��
#define REP_REFUND_NOT_PAIED 0x04		// δ����
// ȡ��������
#define REP_CANCEL_TRANS_SUCCESS 0x00	// �ɹ�
#define REP_CANCEL_SERVER_ERROR 0x01	// ������δ����
#define REP_CANCEL_TRANS_NOT_EXIST 0x02 // ��ǰ�޶���

#define PAYTYP_WECHAT 0x01 // ΢��֧��
#define PAYTYP_ALIPAY 0x02 // ����֧����֧��
#define PAYTYP_USM 0x03	   // ����֧��
#define PAYTYP_CLOUD 0x04  // ������
#define PAYTYP_ETC 0x05	   // ETC��

// �½��ɷѻ��ն˶ԽӺ�̨�����룬�ĵ���֧��ƽ̨����ӿ�
#define SUBCODE_SUCCESS 0x00			 // ���óɹ�
#define SUBCODE_SYSTEM_BUSY -1			 // ϵͳ��æ
#define SUBCODE_ERROR_OTHER 10000		 // ��������
#define SUBCODE_ERROR_INPUT_ARGS 10001	 // �����������
#define SUBCODE_ERROR_DB 10002			 // ���ݿ����ʧ��
#define SUBCODE_ERROR_CACHE 10003		 // ���ݿ⻺��ʧ��
#define SUBCODE_ERROR_QRCODE 10004		 // ������ʧЧ
#define SUBCODE_ERROR_LACK_MONEY 10005	 // ����
#define SUBCODE_ERROR_CALL_TIMEOUT 20000 // ͨ�����ó�ʱ
#define SUBCODE_ERROR_CALL_FAULT 20001	 // ͨ�����ô���
#define SUBCODE_ERROR_PAYING 20002		 // ֧����

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
