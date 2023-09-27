
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <netdb.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <ifaddrs.h>
// #include <net/if.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <linux/if_arp.h>
#include <linux/route.h>

#include "utils_ptrlist.h"
#include "lc_module.h"
#include "lvp_client.h"
#include "rwdev.h"
#include "typedef.h"
#include "utils_input.h"
#include "utils_mtrace.h"
#include "gpio.h"
#include "cJSON.h"
#include "lc_config.h"
#include "spm_config.h"
#include "imp_http_task.h"

typedef struct tagServerObject
{
	int listen_tcp;
	char listen_ip[16];
	pthread_t thread;
	int bQuit;
	int disable_scan;
	MODULE_CALL_BACK call_back;
	PtrList OutPutList;
} OBJECT, *PHVOBJECT;

#define CONF_FNAME "./conf/ap.dat"
static void Core_led_callback(HANDLE h, int code, void *arg);
static void Core_qr_callback(HANDLE h, int code, void *arg);
static int CORE_load_modules();
static void *log_trans = NULL;
RWDevCallBack DevCallBack = NULL;
static PtrList HanderList = PTRLIST_INITIALIZER;
void LED_Display(const char *text);

extern void spm_send_qrcode(int index, const char *qrcode);
static OBJECT theObject;
AppConfig apConfig = {0};
static void sync_config();
static void read_config();

static const char *config_def =
	"ui_type=1	# UI更新方式 0：UDP 1：http\r\n"
	"ui_udp=192.168.1.132 # UDP更新界面方式，NUC地址\r\n"
	"ui_url=http://192.168.1.111:9110/xj/autorun #ui控制\r\n"
	"talk_ctrl=http://172.16.13.16:9110/xj/getAllows     # 接听控制接口 \r\n"
	"talk_back_up=http://172.16.13.16:9110/xj/Calling?camera=0&phoneId=1001  # 上对讲\r\n"
	"talk_back_dwn=http://172.16.13.16:9110/xj/Calling?camera=1&phoneId=1002  # 下对讲\r\n"
	"talk_third=http://172.16.13.16:9110/xj/sipInit";


SystemConfig g_apconfig = {0};

static HANDLE h_qrcode[2] = {NULL, NULL};

void ltrace(const char *fmt, ...)
{
	va_list va;
	int rc;
	if (log_trans == NULL)
		return;
	va_start(va, fmt);
	rc = mtrace_vlog(log_trans, fmt, va);
	va_end(va);
}

void set_enabel_scan(int en)
{
	theObject.disable_scan = en ? 0 : 1;
}

int is_enable_scan()
{
	return theObject.disable_scan == 0;
}

static void QR_SendToClient(int device, const char *qr_data)
{
	DataHeader header;
	if (!qr_data || *qr_data == '\0')
		return -1;
	QR_HEADER_INTI(header, device);
	header.size = strlen(qr_data) + 1;
	ltrace("real send to all clients..\r\n");
	Client_send(NULL, &header, qr_data);
	return 0;
}

static void LED_SoundDu()
{
	int type = get_module_type("德亚费显");
	cIOModule *ioModule = NULL;
	ioModule = (cIOModule *)CORE_find_module(type, NULL);
	if (ioModule)
	{
		ioModule->ioctl(NULL, 1, 0, NULL);
		ltrace("Play Du success！\r\n");
	}
	else
	{
		ltrace("不存在德亚费显模块！\r\n");
	}
}

static void qr_code_trimed(char *code, int size)
{
	int i = 0;
	for (i = 0; i < size; i++)
	{
		if (code[i] == '\r' || code[i] == '\n' || code[i] == '\0')
		{
			code[i] = '\0';
			return;
		}
	}
}

#include "utils_netcfg.h"

static int do_system_command(const char *qrg)
{
	char led_text[256] = {0};
	char ip1[32] = {0};
	char ip2[32] = {0};
	char ip3[32] = {0};
	if (strstr(qrg, "reboot"))
	{
		ltrace("读取到重启二维码！重启系统!\r\n");
		sprintf(led_text, "LINE1=\"正在进行\" LINE2=\"系统重启\" LINE3=\"请稍后\" LINE4=\" \" COLOR=RED ");
		LED_Display(led_text);
		sleep(1);
		system("reboot");
		return 1;
	}
	if (strstr(qrg, "rwnetinfo"))
	{
		ltrace("读取网络配置!\r\n");
		NET_CONFIG_S cfg;
		get_network_config(&cfg);
		strcpy(ip1, INET_NTOA(cfg.ip));
		strcpy(ip2, INET_NTOA(cfg.netmask));
		strcpy(ip3, INET_NTOA(cfg.gw));
		sprintf(led_text, "VOL=3 SOUND=\"网络配置,IP地址, %s, 子网掩码, %s, 网关, %s \"", ip1, ip2, ip3);
		LED_Display(led_text);
		return 1;
	}
	if (strstr(qrg, "rwversion"))
	{
		sprintf(led_text, "VOL=3 SOUND=\"软件版本：1.1.0 \"");
		LED_Display(led_text);
		return 1;
	}
	return 0;
}

static void on_qrcode_ready(int index, const char *arg)
{
	int ret;
	char buffer[256] = {0};
	strncpy(buffer, arg, sizeof(buffer));
	qr_code_trimed(buffer, strlen(buffer));
	ltrace("有二维码【%d】：%s \r\n", index, arg);
	QR_SendToClient(0, arg);
	if (DevCallBack)
		DevCallBack(EVT_QR_READY, 1, arg);
	if (do_system_command(arg))
		return;
#ifdef ENABLE_HTTP_INTERFACE
	// http协议
	ret = SendQrCodeToHttpServer(arg);
	if (ret == -1 || ret == -2)
		LED_Display("SOUND=\"请重新扫码\"");
	// tcp协议
	SendQrCodeToTcpServer(arg);
#endif
	// 串口协议
	spm_send_qrcode(index, arg);
	// TODO mqtt协议
#ifdef ENABLE_MQTT_INTERFACE
	mqtt_send_qrcode(index, arg);
#endif
}

static void Core_led_callback(HANDLE h, int code, void *arg)
{
	int ret = 0;
	switch (code)
	{
	case QR_EVT_READY:
		ltrace("收到来自费显转发的二维码：%s \r\n", arg);
		on_qrcode_ready(code, arg);
		break;
	default:
		break;
	}
}

static void Core_qr_callback(HANDLE h, int code, void *arg)
{
	int index = 0;
	index = h_qrcode[0] == h ? 0 : 1;
	switch (code)
	{
	case QR_EVT_READY:
#if defined(TSP86)
		sound_play_du();
#elif defined(TMPE42)
		LED_SoundDu();
#else
#endif
		ltrace("收到来自扫码模块转发的二维码，编号[%d]，二维码：%s \r\n", index, arg);
		on_qrcode_ready(index, arg);
		break;
	default:
		break;
	}
}

void on_qr_ready(const char *msg)
{
	Core_led_callback(NULL, 0x11, msg);
}

void LED_Display(const char *text)
{
	if (!text || *text == '\0')
	{
		ltrace("LED显示内容为空！\r\n");
		return;
	}
	cIOModule *ioModule = NULL;
	int type = get_module_type("德亚费显");
	POSITION pos = HanderList.head;
	for (pos; pos != NULL; pos = pos->next)
	{
		ioModule = (cIOModule *)(*((int *)pos->ptr));
		if (ioModule->is_type(type))
		{
			ioModule->write(NULL, (char *)text, strlen(text));
		}
	}
}

static int load_module_by_name(const char *name)
{
	int rc = -1;
	rc = core_load_module(name);
	if (rc != 0)
		ltrace("load module  %s failed..\n", name);
	else
		ltrace("load module  %s success..\n", name);
	return rc;
}

static _longtime GetTickCount()
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

static void keyboard_event(char *key)
{
	DataHeader header;
	char text[30] = {0};
	strncpy(text, key, sizeof(text));
	text[27] = '\0';
	ltrace("键盘扫码信息：%s \r\n", key);
	on_qr_ready(key);
	EVT_HEADER_INIT(header, EVT_KEYBOARD, text);
	Client_send(NULL, &header, NULL);
}

static int CORE_load_modules()
{
	int type = 0;
	char buf[129] = {0};
	HANDLE handle;
	cIOModule *ioModule = NULL;
#ifdef ENBALE_TFI_LED
	if (load_module_by_name("./mod_led.so") == 0)
	{
		type = get_module_type("德亚费显");
		ioModule = (cIOModule *)CORE_find_module(type, NULL);
		if (ioModule)
		{
			ltrace("德亚费显对接模块成功，开启该模块设备!\r\n");
			sprintf(buf, "dev=\"%s\"", DEV_LED_MASTER);
			handle = ioModule->open(buf);
			if (handle)
			{
				ioModule->register_callback(Core_led_callback);
				PtrList_append(&HanderList, handle);
			}
#ifdef ENABLE_SLAVE_LED
			sprintf(buf, "dev=\"%s\"", DEV_LED_SLAVE);
			handle = ioModule->open(buf);
			if (handle)
			{
				ioModule->register_callback(Core_led_callback);
				PtrList_append(&HanderList, handle);
			}
#endif
		}
		else
		{
			ltrace("费显对接模块失败!\r\n");
		}
	}
#endif

#ifdef ENABLE_SCAL_NET
	if (load_module_by_name("./mod_qr.so") == 0)
	{
		type = get_module_type("德亚扫码枪");
		ioModule = (cIOModule *)CORE_find_module(type, NULL);
		if (ioModule)
		{
			strcpy(buf, "dev=\"127.0.0.1\"");
			handle = ioModule->open(buf);
			h_qrcode[1] = handle;
			ioModule->register_callback(Core_qr_callback);
			PtrList_append(&HanderList, handle);
		}
	}
#endif

#ifdef ENABLE_SCAN_TTY
	if (load_module_by_name("./mod_qr_tty.so") == 0)
	{
		type = get_module_type("德亚串口扫码枪");
		ioModule = (cIOModule *)CORE_find_module(type, NULL);
		ltrace("找到串口扫码枪 %p  ..\r\n", ioModule);
		if (ioModule)
		{
			sprintf(buf, "dev=\"%s\"", DEV_SCAN_UP);
			ltrace("扫码模块串口模式，主机；%s !\r\n", buf);
			handle = ioModule->open(buf);
			h_qrcode[0] = handle;
			ioModule->register_callback(Core_qr_callback);
			PtrList_append(&HanderList, handle);
		}
#ifdef ENABLE_SCAN_SLAVE
		if (ioModule)
		{
			sprintf(buf, "dev=\"%s\"", DEV_SCAN_DWN);
			ltrace("扫码模块串口模式，辅机；%s !\r\n", buf);
			handle = ioModule->open(buf);
			h_qrcode[1] = handle;
			ioModule->register_callback(Core_qr_callback);
			PtrList_append(&HanderList, handle);
		}
#endif
	}
#endif
}

void led_voice_send(int vol, const char *text)
{
	cIOModule *ioModule = NULL;
	int type = get_module_type("德亚费显");
	POSITION pos = HanderList.head;
	for (pos; pos != NULL; pos = pos->next)
	{
		ioModule = (cIOModule *)(*((int *)pos->ptr));
		if (ioModule->is_type(type))
		{
			struct SoundAttr
			{
				int vol;
				const char *text;
			} attr;
			attr.vol = vol;
			attr.text = text;
			ioModule->ioctl((HANDLE)pos->ptr, IOC_LEDMODULE_SOUNDPLAY, 0, &attr);
		}
	}
}

void led_clear_all()
{
	cIOModule *ioModule = NULL;
	int type = get_module_type("德亚费显");
	POSITION pos = HanderList.head;
	for (pos; pos != NULL; pos = pos->next)
	{
		ioModule = (cIOModule *)(*((int *)pos->ptr));
		if (ioModule->is_type(type))
		{
			ioModule->ioctl((HANDLE)pos->ptr, IOC_LEDMODULE_CLEAR, 0, NULL);
		}
	}
}

void led_display_text(int nLine, int format, int color, const char *text)
{
	cIOModule *ioModule = NULL;
	int type = get_module_type("德亚费显");
	POSITION pos = HanderList.head;
	for (pos; pos != NULL; pos = pos->next)
	{
		ioModule = (cIOModule *)(*((int *)pos->ptr));
		if (ioModule && ioModule->is_type(type))
		{
			struct DisplayAttr
			{
				int nLine;
				int format;
				int color;
				const char *text;
			} attr;
			attr.nLine = nLine;
			attr.format = format;
			attr.color = color;
			attr.text = text;
			ioModule->ioctl((HANDLE)pos->ptr, IOC_LEDMODULE_DISPLAY, 0, &attr);
		}
	}
}

static void device_staus_examinaiton()
{
	DataHeader header;
	DEV_INFO_INTI(header, DEV_STATUS);
	header.devAttr.option[0] = access("/dev/ttyUSB1", F_OK) == 0 ? 1 : 0;
	Client_send(NULL, &header, NULL);
}

static void LED_SetPicture(unsigned color, int x, int open)
{
	int type = get_module_type("德亚费显");
	cIOModule *ioModule = NULL;
	int option[10] = {0};
	option[0] = color;
	option[1] = x;
	option[2] = open;
	ioModule = (cIOModule *)CORE_find_module(type, NULL);
	if (ioModule)
	{
		ioModule->ioctl(NULL, IOC_LEDMODULE_SETPIC, 0, option);
		ltrace("费显控制图片输出！\r\n");
	}
	else
	{
		ltrace("不存在德亚费显模块！\r\n");
	}
}

static void handle_set_systime(const char *peer_name, TimeAttr *ptmAttr)
{
	time_t t_host;
	struct tm tm_host;
	struct timeval tv;
	_longtime lt_old, lt_new;
	_longtime t_delta;

	if (ptmAttr->year < 2015)
	{
		ltrace("set system time: %04d/%02d/%02d/ %02d:%02d:%02d - wrong date, ignored\n",
			   ptmAttr->year, ptmAttr->month, ptmAttr->day,
			   ptmAttr->hour, ptmAttr->minute, ptmAttr->second);
		return;
	}
	ltrace("client %s set time as %04d/%02d/%02d %02d:%02d:%02d.%03d\n",
		   peer_name,
		   ptmAttr->year, ptmAttr->month, ptmAttr->day,
		   ptmAttr->hour, ptmAttr->minute, ptmAttr->second, ptmAttr->msec);
	lt_old = timeGetLongTime();
	tm_host.tm_year = ptmAttr->year - 1900;
	tm_host.tm_mon = ptmAttr->month - 1;
	tm_host.tm_mday = ptmAttr->day;
	tm_host.tm_hour = ptmAttr->hour;
	tm_host.tm_min = ptmAttr->minute;
	tm_host.tm_sec = ptmAttr->second;
	t_host = mktime(&tm_host);
	tv.tv_sec = t_host;
	tv.tv_usec = ptmAttr->msec * 1000;
	settimeofday(&tv, NULL);
	usleep(1000);
	lt_new = timeGetLongTime();
	t_delta = (lt_new - lt_old);
	ltrace("--> delta time (new - old) = %lld msec\n", t_delta);
	system("hwclock -w");
	Client_reset_hbeat(NULL);
}

static int park_handle_tcp(LVPClient *cl)
{
	DataHeader *pheader = &cl->inHeader;
	int code[10];
	static int di_last = 0;
	ltrace("handle client %s tcp data type=0x%x\n", CLIENT_GET_NAME(cl), pheader->DataType);
	switch (pheader->DataType)
	{
	case DTYP_TIME: // time sync.
		handle_set_systime(cl->peer_name, &pheader->timeAttr);
		break;
	case DTYP_LED:
	{
		if (cl->inHeader.ctrlAttr.code == LED_SETPICTURE)
		{
			ltrace("收到来自客户的控制费显显示图片请求 %#x %d %d \r\n",
				   cl->inHeader.ledAttr.option[0], cl->inHeader.ledAttr.option[1], cl->inHeader.ledAttr.option[2]);
			LED_SetPicture(cl->inHeader.ledAttr.option[0], cl->inHeader.ledAttr.option[1], cl->inHeader.ledAttr.option[2]);
		}
	}
	break;
	case DTYP_CTRL:
	{
		if (cl->inHeader.ctrlAttr.code == CTRL_LED_DIS)
		{
			if (cl->inPayload)
			{
				ltrace("收到来自客户的led显示请求:%s \r\n", (char *)cl->inPayload);
				LED_Display(cl->inPayload);
				free(cl->inPayload);
				cl->inPayload = NULL;
			}
			else
			{
				ltrace("费显负载内容为空，忽略..\r\n");
			}
		}
		else if (cl->inHeader.ctrlAttr.code == CTRL_DEVICE_INFO)
		{
			ltrace("接收来自客户的设备检测指令..\r\n");
			device_staus_examinaiton();
		}
		else if (cl->inHeader.ctrlAttr.code == CTRL_IO_PULSE)
		{
			ltrace("收到来自客户端的脉冲输出请求..\r\n");
			int pin = cl->inHeader.ctrlAttr.option[0];
			int count = cl->inHeader.ctrlAttr.option[1];
			int period = cl->inHeader.ctrlAttr.param;
			ltrace("gpio_pulse:%d %d %d\r\n", pin, count, period);
			GPIO_pulse(pin, count, period);
		}
		else if (cl->inHeader.ctrlAttr.code == CTRL_IO_SET)
		{
			ltrace("输出io:%d\r\n");
			int val = cl->inHeader.ctrlAttr.param;
			int i = 0;
			int di_mask = di_last ^ val;
			for (i = 0; i < 4; i++)
			{
				if (di_mask & (0x01 << i))
					GPIO_output(i, val & (0x01 << i));
			}
			di_last = val;
		}
		else if (cl->inHeader.ctrlAttr.code == CTRL_SCROLL_PAGE)
		{
			ltrace("收到页面切换指令:page:%d \r\n", cl->inHeader.ctrlAttr.param);
			code[0] = cl->inHeader.ctrlAttr.param;
			if (cl->inPayload)
			{
				ltrace("切换页面还有文本信息需要传递：%s \r\n", cl->inPayload);
				if (DevCallBack)
					DevCallBack(EVT_CHANGE_PAGE, code[0], cl->inPayload);
				free(cl->inPayload);
				cl->inPayload = NULL;
			}
			else
			{
				ltrace("切换页面无需附加信息..\r\n");
				if (DevCallBack)
					DevCallBack(EVT_CHANGE_PAGE, code[0], NULL);
			}
		}
		else
		{
		}
	}
	case DTYP_CONF:
	{
		if (cl->inHeader.DataId == CID_AP_SYS && cl->inHeader.size == sizeof(AppConfig))
		{
			memcpy(&apConfig, cl->inPayload, sizeof(AppConfig));
			sync_config();
		}
	}
	break;
		break;
	case DTYP_HBEAT:
		break;
	default:
		ltrace("不接受的tcp数据%#x ..\r\n", pheader->DataType);
		break;
	}
}

static void SendConfig(LVPClient *cl)
{
	DataHeader header;
	CONF_HEADER_INIT(header, CID_AP_SYS, sizeof(apConfig));
	Client_send(cl, &header, &apConfig);
}

static int input_fd = -1;
static _longtime next_input = 0;

#define max(a, b) ((a) > (b) ? (a) : (b))
static void hnlp_poll_client_data(PHVOBJECT pHvObj, int tout)
{
	struct timeval tv;
	fd_set read_fds;
	int nsel, fdmax;
	char text_buffer[80] = {0};
	int rlen = 0;
	if (input_fd == -1 && GetTickCount() > next_input)
	{
		input_fd = open_input_dev("/dev/input/event0");
		if (input_fd < 0)
		{
			next_input = GetTickCount() + 2000;
			input_fd = -1;
		}
		else
		{
			ltrace("打开键盘成功!\r\n");
		}
	}
	fdmax = pHvObj->listen_tcp;
	FD_ZERO(&read_fds);
	FD_SET(pHvObj->listen_tcp, &read_fds);
	nsel = Client_fdset(&read_fds);
	fdmax = max(fdmax, nsel);
	if (input_fd > 0)
	{
		FD_SET(input_fd, &read_fds);
		fdmax = max(fdmax, input_fd);
	}
	tv.tv_sec = 0;
	tv.tv_usec = tout * 1000;
	nsel = select(fdmax + 1, &read_fds, NULL, NULL, &tv);
	if (nsel > 0)
	{
		POSITION pos;
		LVPClient *cl;
		if (input_fd > 0 && FD_ISSET(input_fd, &read_fds))
		{
			if (0 < (rlen = input_read(input_fd, text_buffer, sizeof(text_buffer))))
			{
				text_buffer[rlen] = '\0';
				keyboard_event(text_buffer);
			}
			else
			{
				close(input_fd);
				input_fd = -1;
			}
		}
		if (FD_ISSET(pHvObj->listen_tcp, &read_fds))
		{
			int fd = sock_accept(pHvObj->listen_tcp);
			if (fd >= 0 && (cl = Client_create(fd)) != NULL)
			{
				SendConfig(cl);
				char buf[256];
				sprintf(buf, "[ parklc engine] - client %s connected (fd=%d). # of clients=%d\n", CLIENT_GET_NAME(cl), fd, Client_count());
				ltrace(buf);
			}
		}
		if (Client_markset(&read_fds) > 0)
		{
			int rc;
			for (pos = Client_head(); pos != NULL;)
			{
				cl = Client_get(pos);
				pos = Client_next(pos);
				rc = Client_read(cl);
				if (rc > 0)
					park_handle_tcp(cl);
				else if (rc == 0)
				{
				}
				else
				{
					ltrace("client FORM %s is disconnect, close it..\r\n", CLIENT_GET_NAME(cl));
					Client_destroy(cl);
				}
			}
		}
	}
}

void onLCDEvent(int index, const char *qrcode, const char *text, const char *sound, int vol, int tout)
{
	int param = index | tout << 16;
	const char *ptr = NULL;
	char buffer[256];
	char ches[256];
	char led_text[256];
	cJSON *root = cJSON_CreateObject();
	if (!root)
	{
		system("reboot");
		return;
	}
	cJSON_AddStringToObject(root, "qrcode", qrcode);
	cJSON_AddStringToObject(root, "html", text);
	if (sound[0] != 0)
	{
		str_b64dec(sound, ches, sizeof(ches));
		sprintf(led_text, "SOUND=\"%s\" VOL=%d ", ches, vol);
		LED_Display(led_text);
	}
	ptr = cJSON_Print(root);
	if (DevCallBack)
		DevCallBack(EVT_CHANGE_PAGE, param, ptr);
	ltrace("上报消息：%s \r\n", ptr);
	free(ptr);
	cJSON_Delete(root);
}

void onLEDEvent(const char *led)
{
	ltrace("收到费显信息：%s \r\n", led);
	LED_Display(led);
}

static void Core_server_callback(HANDLE h, int code, void *arg)
{
}

static void *local_server(void *arg)
{
	PHVOBJECT pHvObj = &theObject;
	int interval = 0;
	const char *listen_ip = (pHvObj->listen_ip[0] == '\0' || inet_addr(pHvObj->listen_ip) == -1) ? "0.0.0.0" : pHvObj->listen_ip;
	ltrace("listen on port %d %s \r\n", 90, listen_ip);
	pHvObj->listen_tcp = sock_listen(90, listen_ip, 5);
	if (pHvObj->listen_tcp <= 0)
	{
		ltrace("listen local error..\r\n");
		return;
	}
	ltrace("work thread start success..\r\n");
	while (!pHvObj->bQuit)
	{
		hnlp_poll_client_data(pHvObj, 500);
	}
	return NULL;
}

#include "gpio.h"

#define PIN_UP_HELP 0x01
#define PIN_DOWN_HELP 0x02

static int has_init = 0;
static int first_calling = 1;
static void io_cb(int di_last, int di_this)
{
	ltrace("io change:%#x - %#x \r\n", di_last, di_this);
	DataHeader header;
	IO_STAT_HEADER_INIT(header, 0, di_last, di_this);
	Client_send(NULL, &header, NULL);
	spm_gpio_change(di_last, di_this); 
	if (!(di_last & PIN_UP_HELP) && (di_this & PIN_UP_HELP))
	{
		ltrace("上工位求助，触发对讲.\r\n");
		add_http_get_task(0, g_apconfig.talk_back_up);
	}
	if (!(di_last & PIN_DOWN_HELP) && (di_this & PIN_DOWN_HELP))
	{
		ltrace("下工位求助，触发对讲.\r\n");
		add_http_get_task(0, g_apconfig.talk_back_dwn);
	}
}

static void sync_config()
{
	int fd;
	if (access(CONF_FNAME, F_OK) != 0)
		fd = open(CONF_FNAME, O_RDWR | O_CREAT);
	else
		fd = open(CONF_FNAME, O_RDWR | O_TRUNC);
	if (fd > 0)
	{
		write(fd, &apConfig, sizeof(apConfig));
		close(fd);
		return;
	}
}

static void read_config()
{
	int fd;
	fd = open(CONF_FNAME, O_RDONLY);
	if (fd > 0)
	{
		read(fd, &apConfig, sizeof(apConfig));
		close(fd);
		return;
	}
}

static void read_param()
{
	int fd;
	if (access("conf", F_OK) != 0)
		mkdir("conf", 0644);
	if (access(CONF_FNAME, F_OK) != 0)
		sync_config();
	read_config();
}

static void write_file(const char *file_name, void *arg, int size)
{
	if (access(file_name, F_OK) == 0)
		unlink(file_name);

	int fd = open(file_name, O_CREAT | O_RDWR, 0644);
	if (fd > 0)
	{
		write(fd, (char *)arg, size);
		close(fd);
	}
}

extern const char *default_cert_conf;
static int udp_sock = -1;
extern const char *dst_ip;
extern int g_en_http_ui;

static void send_udp_message(int page, char *buf)
{
	int i = 0;
	int len = 0;
	uint8_t tmp[512] = {0};
	len = strlen(buf) + 7;
	tmp[0] = 0xaa;
	tmp[1] = 0xbb;
	tmp[2] = 0x00;
	tmp[3] = 0x02;
	tmp[4] = page & 0xff;
	strcpy(tmp + 6, buf);
	for (i = 0; i < 2; i++)
	{
		sock_udp_send0(udp_sock, inet_addr(dst_ip), 2000, tmp, len);
		if (sock_dataready(udp_sock, 200))
		{
			ltrace("收到LCD屏响应！\r\n");
			break;
		}
		else
		{
			ltrace("未收到LCD屏响应！\r\n");
		}
	}
}

typedef struct node_page
{
	int src;
	int dst;
} nod_page_s;

static int page_remap(int page, char *buffer)
{
	int i = 0;
	//clang-format off
	nod_page_s p_list[] =
		{
			{1, 1},
			{2, 11},
			{3, 5},
			{4, 2},
			{5, 3},
			{6, 8},
			{7, 10},
			{8, 10},
		};
	//clang-format on
	ltrace("缴费机重新映射画面...\r\n");
	if (page == 7)
	{
		strcpy(buffer, "支付失败;请重新支付");
	}
	for (i = 0; i < sizeof(p_list) / sizeof(p_list[0]); i++)
	{
		if (page == p_list[i].src)
			return p_list[i].dst;
	}
	return 1;
}

int RWDevRegisterCallBack(RWDevCallBack cb)
{
	DevCallBack = cb;
}

extern void add_http_url_post_task(int index, const char *url, int type, char *request);
extern int g_enable_lcd_page_remap;

static void handle_lcd_info(int page, char *buf)
{
	char url[512] = {0};
	char gbk_buffer[1024] = {0};
	char utf_buffer[1024] = {0};
	sprintf(url, "%s", g_apconfig.ui_url);
	// if (!g_enable_lcd_page_remap)
	// 	page = page_remap(page, buf);
	if (strncmp(buf, "{", 1) == 0)
		sprintf(gbk_buffer, "{\"page\":%d,\"msg\":%s}", page, buf);
	else
		sprintf(gbk_buffer, "{\"page\":%d,\"msg\":\"%s\"}", page, buf);

	GBKToUTF8(gbk_buffer, strlen(gbk_buffer), utf_buffer);
	add_http_url_post_task(0, url, 0, utf_buffer);
	ltrace("URL:%s\r\nDATA:%s\r\n", url, gbk_buffer);
}

static void DoDevEventHandle(int code, int param, void *arg)
{
	int page = param;
	char buffer[512] = {0};
	if (EVT_CHANGE_PAGE == code)
	{
		if (arg)
			strcpy(buffer, arg);
		ltrace("更新LCD 页面[%d]，数据[%s] 【%d】\r\n", page, buffer, g_apconfig.en_udp);
		if (g_apconfig.en_udp)
		{
			ltrace("UDP Type!\r\n");
			send_udp_message(page, buffer);
		}
		else
		{
			ltrace("http Type!\r\n");
			handle_lcd_info(page, buffer);
		}
	}
}

static void load_config()
{
	char buff[32] = {0};
	const char *valstring = NULL;
	if (access("app.cfg", F_OK) != 0)
		write_file("app.cfg", config_def, strlen(config_def));
	ConfigLoad("app.cfg");
	valstring = ConfigGetKey("ui_url");
	if (valstring)
		strcpy(g_apconfig.ui_url, valstring);

	valstring = ConfigGetKey("ui_type");
	if (valstring)
		g_apconfig.en_udp = atoi(valstring) == 0;
	else
		g_apconfig.en_udp = 0;

	valstring = ConfigGetKey("ui_udp");
	if (valstring)
		strcpy(g_apconfig.ui_udp, valstring);

	valstring = ConfigGetKey("talk_back_up");
	if (valstring)
		strcpy(g_apconfig.talk_back_up, valstring);

	valstring = ConfigGetKey("talk_back_dwn");
	if (valstring)
		strcpy(g_apconfig.talk_back_dwn, valstring);

	valstring = ConfigGetKey("talk_third");
	if (valstring)
		strcpy(g_apconfig.talk_third, valstring);

	valstring = ConfigGetKey("talk_ctrl");
	if (valstring)
		strcpy(g_apconfig.talk_ctrl, valstring);
 

	ltrace("g_apconfig.ui_url:%s\r\n", g_apconfig.ui_url);
	ltrace("g_apconfig.talk_back_up:%s\r\n", g_apconfig.talk_back_up);
	ltrace("g_apconfig.talk_back_dwn:%s\r\n", g_apconfig.talk_back_dwn);
	ltrace("g_apconfig.talk_third:%s\r\n", g_apconfig.talk_third);
	ltrace("en udp lcd:%d \r\n", g_apconfig.en_udp);
	ltrace("udp IP:%s \r\n", g_apconfig.ui_udp);
	ltrace("udp Url:%s \r\n", g_apconfig.ui_url); 
}

extern int test_interface();

int RWDevStart()
{
	char log_name[129] = {0};
	if (has_init)
		return;
	LoadCodeTable();
	strcpy(log_name, "./log");
	log_trans = mlog_init(log_name, "trans");
	mlog_setlimitcnt(log_trans, 1024, 4);
	read_param();
	load_config();
	has_init = 1;
	CORE_load_modules();
	RWDevRegisterCallBack(DoDevEventHandle);
#ifdef ENABLE_GPIO
	GPIO_initialize();
	GPIO_register_callback(io_cb);
#endif
	if (access("certs", F_OK) != 0)
		mkdir("certs", 0644);
	if (access("./certs/cacert.pem", F_OK) != 0)
		write_file("./certs/cacert.pem", (char *)default_cert_conf, strlen(default_cert_conf));
	memset(&theObject, 0, sizeof(theObject));
	pthread_create(&theObject.thread, NULL, local_server, NULL);
	theObject.call_back = Core_server_callback;
	if (strcmp(dst_ip, "127.0.0.1") == 0)
		udp_sock = sock_udp_bindhost(1234, "127.0.0.1");
	else
		udp_sock = sock_udp_bindhost(1234, NULL);
	ServicesStart();
	RPC_startup();
	test_interface();
	// LED_Display("LINE1=\"德亚交通技术有限公司\" LINE2=\" \" COLOR=RED");
	local_protocol_init();
	mqtt_init();
#if defined CHIP_3520_ALB
	GPIO_pulse(6, 1, 500);
#elif defined CHIP_3520
	GPIO_pulse(0, 1, 500);
#else
#endif
}

extern void local_protocol_exit();
int RWDevStop()
{
	theObject.bQuit = 1;
	pthread_cancel(theObject.thread);
	pthread_join(theObject.thread, NULL);
	CORE_unload_modules();
#ifdef ENABLE_GPIO
	GPIO_terminate();
#endif
	local_protocol_exit();
}

int RWDevIoctl(int code, int param, void *arg)
{
	switch (code)
	{
	case IOC_LED_DISPLAY:
		LED_Display(arg);
		break;
	case IOC_VOICE_PLAY:
		LED_Display(arg);
		break;
	default:
		break;
	}
}
