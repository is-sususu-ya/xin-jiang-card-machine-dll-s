 
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
	char http_server[128];      // http服务器地址
	char tcp_server[128];       // tcp 服务器地址
	char device_id[64];         // 设备ID
	char app_key[64];           // 设备秘钥
    char mqtt_server[16];       // mqtt 地址
    HI_S32 mqtt_port;           // mqtt 端口
    char mqtt_project_name[64]; // mqtt 项目名
    char mqtt_device_id[64];    // mqtt 设备ID
    char mqtt_username[32];     // 登录用户名
    char mqtt_password[32];     // 登录密码
	HI_S32 reserv[32];          // 保留参数
}AppConfig;

extern AppConfig apConfig;

#define EN_FUNCTION(n)  (n&apConfig.flag)





#endif
