 
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
 
#ifndef LC_CONFIG_HEADER_
#define LC_CONFIG_HEADER_

#define EN_HTTP_INTF        0x01
#define EN_TCP_INTF         0x02
#define EN_MQTT_INTF        0x04
 
typedef struct tagAppConfig
{
	HI_S32 flag;
	char http_server[128];      // http��������ַ
	char tcp_server[128];       // tcp ��������ַ
	char device_id[64];         // �豸ID
	char app_key[64];           // �豸��Կ
    char mqtt_server[16];       // mqtt ��ַ
    HI_S32 mqtt_port;           // mqtt �˿�
    char mqtt_project_name[64]; // mqtt ��Ŀ 
    char mqtt_device_id[64];    // mqtt �豸ID
    char mqtt_username[32];     // ��¼�û� 
    char mqtt_password[32];     // ��¼����
	HI_S32 reserv[32];          // ��������
}AppConfig;

typedef struct tagApConfig
{
	int en_udp;
	char ui_udp[32];
	int machine_type;
	char ui_url[256];
	char talk_back_up[256];
	char talk_back_dwn[256];
	char talk_third[256];
} SystemConfig;

extern AppConfig apConfig;

#define EN_FUNCTION(n)  (n&apConfig.flag)





#endif
