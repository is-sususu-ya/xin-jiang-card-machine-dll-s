#define _GNU_SOURCE
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utils_mtrace.h>
#include <typedef.h>
#include <utils_ptrlist.h>
#include <utils_str.h>
#include <dbg_printf.h>
#include <longtime.h>
#include "led.h"

typedef struct tagLEDHandle
{
	cIOModule *module;
	HANDLE h_led;
	char chDevLED[32];
	BOOL bEnabled;
	BOOL led_online;
	int color1, color2, color3, color4;
} cLEDHandle;

static int has_init = 0;
static cIOModule m_cLEDModule;
static PtrList m_HandleList = PTRLIST_INITIALIZER;
static MODULE_CALL_BACK module_callback = NULL;
static HANDLE h_led = NULL;

static void *log_handle = NULL;

void trace_log(const char *fmt, ...)
{
	va_list va;
	int rc;
	if (log_handle == NULL)
		return;
	va_start(va, fmt);
	rc = mtrace_vlog(log_handle, fmt, va);
	va_end(va);
}

static int LEDShow_initialize()
{
	if (has_init)
		return 0;
	has_init = 0;
	char log_name[100] = "./log";
	log_handle = mlog_init(log_name, "mod_led");
	mlog_setlimitcnt(log_handle, 200, 2);
	memset(&m_HandleList, 0, sizeof(m_HandleList));
	return 0;
}

static int LEDShow_terminate()
{
	has_init = 0;
	cLEDHandle *ptr;
	while ((ptr = PtrList_remove_head(&m_HandleList)) != NULL)
	{
		cLEDHandle *h = (cLEDHandle *)ptr;
		led_exit(h->h_led);
	}
}

static void led_event(int code, void *arg)
{
	const char *qr_code = (const char *)arg;
	if (code == EVT_QR_READY)
	{
		if (module_callback)
			module_callback(NULL, QR_EVT_READY, qr_code);
	}
	else if (code == EVT_IOCHANGE)
	{
		if (module_callback)
			module_callback(NULL, QR_EVT_KEYPRESS, NULL);
	}
}

static HANDLE LEDShow_open(const char *args)
{
	char device[32];
	char operand[32];
	char *ptr = (char *)args;
	char keyword[32];
	char value[32];
	trace_log("<mod_led open args=<%s>><%s><%s><%d>\r\n", args, __FILE__, __func__, __LINE__);
	while (strgettokenpair(ptr, keyword, value, &ptr) > 0)
	{
		int keyidx = stridxinargs(keyword, 0, "dev", (char *)0);
		switch (keyidx)
		{
		case 0:
		case 4:
			memset(device, 0, sizeof(device));
			strcpy(device, value);
			trace_log("open device=%s,<%s>,<%s>,<%d>\n", device, __FILE__, __func__, __LINE__);
			break;
		default:
			break;
		}
	}

	POSITION pos;
	cLEDHandle *hLED = NULL;
	for (pos = m_HandleList.head; pos != NULL; pos = pos->next)
	{
		hLED = (cLEDHandle *)pos->ptr;
		if (strcmp(hLED->chDevLED, device) == 0 && hLED->h_led != NULL)
			return (HANDLE)hLED;
	}
	hLED = (cLEDHandle *)malloc(sizeof(cLEDHandle));
	memset(hLED, 0, sizeof(cLEDHandle));
	hLED->module = &m_cLEDModule;
	strcpy(hLED->chDevLED, device);
	hLED->h_led = led_init(device);
	if (hLED->h_led == NULL)
	{
		free(hLED);
		trace_log("LED打开失败！！\r\n");
		return NULL;
	}
	led_set_callback(hLED->h_led, led_event);
	PtrList_append(&m_HandleList, hLED);
	return (HANDLE)hLED;
}

static int LEDShow_close(HANDLE h)
{
	cLEDHandle *hLED = (cLEDHandle *)h;
	POSITION node = PtrList_find(&m_HandleList, hLED);
	if (!node)
		return -1;
	led_exit(hLED->h_led);
	PtrList_remove(&m_HandleList, node);
	free(hLED);
	return 0;
}

static int LEDShow_read(HANDLE h, void *buf, int size)
{
	return -1;
}

typedef struct tagColorInof
{
	char color[20];
	int value;
} Color_S;

static Color_S colorlist[] = {
	{"Snow", 0xFFFAFA},
	{"GhostWhite", 0xF8F8FF},
	{"WhiteSmoke", 0xF5F5F5},
	{"Gainsboro", 0xDCDCDC},
	{"FloralWhite", 0xFFFAF0},
	{"OldLace", 0xFDF5E6},
	{"Linen", 0xFAF0E6},
	{"AntiqueWhite", 0xFAEBD7},
	{"PapayaWhip", 0xFFEFD5},
	{"BlanchedAlmond", 0xFFEBCD},
	{"Bisque", 0xFFE4C4},
	{"PeachPuff", 0xFFDAB9},
	{"NavajoWhite", 0xFFDEAD},
	{"Moccasin", 0xFFE4B5},
	{"Cornsilk", 0xFFF8DC},
	{"Ivory", 0xFFFFF0},
	{"LemonChiffon", 0xFFFACD},
	{"Seashell", 0xFFF5EE},
	{"Honeydew", 0xF0FFF0},
	{"MintCream", 0xF5FFFA},
	{"Azure", 0xF0FFFF},
	{"AliceBlue", 0xF0F8FF},
	{"lavender", 0xE6E6FA},
	{"LavenderBlush", 0xFFF0F5},
	{"MistyRose", 0xFFE4E1},
	{"White", 0xFFFFFF},
	{"Black", 0x000000},
	{"DarkSlateGray", 0x2F4F4F},
	{"DimGrey", 0x696969},
	{"SlateGrey", 0x708090},
	{"LightSlateGray", 0x778899},
	{"Grey", 0xBEBEBE},
	{"LightGray", 0xD3D3D3},
	{"MidnightBlue", 0x191970},
	{"NavyBlue", 0x000080},
	{"CornflowerBlue", 0x6495ED},
	{"DarkSlateBlue", 0x483D8B},
	{"SlateBlue", 0x6A5ACD},
	{"MediumSlateBlue", 0x7B68EE},
	{"LightSlateBlue", 0x8470FF},
	{"MediumBlue", 0x0000CD},
	{"RoyalBlue", 0x4169E1},
	{"Blue", 0x0000FF},
	{"DodgerBlue", 0x1E90FF},
	{"DeepSkyBlue", 0x00BFFF},
	{"SkyBlue", 0x87CEEB},
	{"LightSkyBlue", 0x87CEFA},
	{"SteelBlue", 0x4682B4},
	{"LightSteelBlue", 0xB0C4DE},
	{"LightBlue", 0xADD8E6},
	{"PowderBlue", 0xB0E0E6},
	{"PaleTurquoise", 0xAFEEEE},
	{"DarkTurquoise", 0x00CED1},
	{"MediumTurquoise", 0x48D1CC},
	{"Turquoise", 0x40E0D0},
	{"Cyan", 0x00FFFF},
	{"LightCyan", 0xE0FFFF},
	{"CadetBlue", 0x5F9EA0},
	{"MediumAquamarine", 0x66CDAA},
	{"Aquamarine", 0x7FFFD4},
	{"DarkGreen", 0x006400},
	{"DarkOliveGreen", 0x556B2F},
	{"DarkSeaGreen", 0x8FBC8F},
	{"SeaGreen", 0x2E8B57},
	{"MediumSeaGreen", 0x3CB371},
	{"LightSeaGreen", 0x20B2AA},
	{"PaleGreen", 0x98FB98},
	{"SpringGreen", 0x00FF7F},
	{"LawnGreen", 0x7CFC00},
	{"Green", 0x00FF00},
	{"Chartreuse", 0x7FFF00},
	{"MedSpringGreen", 0x00FA9A},
	{"GreenYellow", 0xADFF2F},
	{"LimeGreen", 0x32CD32},
	{"YellowGreen", 0x9ACD32},
	{"ForestGreen", 0x228B22},
	{"OliveDrab", 0x6B8E23},
	{"DarkKhaki", 0xBDB76B},
	{"PaleGoldenrod", 0xEEE8AA},
	{"LtGoldenrodYello", 0xFAFAD2},
	{"LightYellow", 0xFFFFE0},
	{"Yellow", 0xFFFF00},
	{"Gold", 0xFFD700},
	{"LightGoldenrod", 0xEEDD82},
	{"goldenrod", 0xDAA520},
	{"DarkGoldenrod", 0xB8860B},
	{"RosyBrown", 0xBC8F8F},
	{"IndianRed", 0xCD5C5C},
	{"SaddleBrown", 0x8B4513},
	{"Sienna", 0xA0522D},
	{"Peru", 0xCD853F},
	{"Burlywood", 0xDEB887},
	{"Beige", 0xF5F5DC},
	{"Wheat", 0xF5DEB3},
	{"SandyBrown", 0xF4A460},
	{"Tan", 0xD2B48C},
	{"Chocolate", 0xD2691E},
	{"Firebrick", 0xB22222},
	{"Brown", 0xA52A2A},
	{"DarkSalmon", 0xE9967A},
	{"Salmon", 0xFA8072},
	{"LightSalmon", 0xFFA07A},
	{"Orange", 0xFFA500},
	{"DarkOrange", 0xFF8C00},
	{"Coral", 0xFF7F50},
	{"LightCoral", 0xF08080},
	{"Tomato", 0xFF6347},
	{"OrangeRed", 0xFF4500},
	{"Red", 0xFF0000},
	{"HotPink", 0xFF69B4},
	{"DeepPink", 0xFF1493},
	{"Pink", 0xFFC0CB},
	{"LightPink", 0xFFB6C1},
	{"PaleVioletRed", 0xDB7093},
	{"Maroon", 0xB03060},
	{"MediumVioletRed", 0xC71585},
	{"VioletRed", 0xD02090},
	{"Magenta", 0xFF00FF},
	{"Violet", 0xEE82EE},
	{"Plum", 0xDDA0DD},
	{"Orchid", 0xDA70D6},
	{"MediumOrchid", 0xBA55D3},
	{"DarkOrchid", 0x9932CC},
	{"DarkViolet", 0x9400D3},
	{"BlueViolet", 0x8A2BE2},
	{"Purple", 0xA020F0},
	{"MediumPurple", 0x9370DB},
	{"Thistle", 0xD8BFD8}};

int getColor(const char *color, int light)
{
	int i = 0;
	int icolor = 0xff0000;
	int tmp = 0;
	for (i; sizeof(colorlist) / sizeof(colorlist[0]); i++)
	{
		if (strcasecmp(colorlist[i].color, color) == 0)
		{
			icolor = colorlist[i].value;
			break;
		}
	}
	int b = (icolor & 0xff);
	int g = (icolor & 0xff00) >> 8;
	int r = (icolor & 0xff0000) >> 16;
	r = r >> light;
	g = g >> light;
	b = b >> light;
	tmp = (r | (g << 8) | (b << 16)) & 0xffffff;
	return tmp;
}

static int LEDShow_append_data(HANDLE h, void *buf, int size) // 颜色；行；内容
{
	if (m_HandleList.count == 0)
		return;
	cLEDHandle *tmp_h = m_HandleList.head->ptr;
	cLEDHandle *hLED = (cLEDHandle *)h;
	HANDLE h_led = hLED == NULL ? tmp_h->h_led : hLED->h_led;
	trace_log("led_write<%s>\r\n", buf);
	if (h_led == NULL)
		return;
	char *ops = (char *)buf;
	int count = 2;
	char voice_text[256] = {0};
	char key[20];
	char value[128];
	char line1[128] = {0};
	char line2[128] = {0};
	char line3[128] = {0};
	char line4[128] = {0};
	int en_voice = 0;
	int vul_voice = 8;
	int fmt1 = 1, fmt2 = 1, color1, color2;
	int fmt3 = 1, fmt4 = 1, color3, color4;
	int light = 0;
	int speed = 4;
	int i = 0;
	color1 = color2 = color3 = color4 = 0x0000ff;
	if (strstr(ops, "=") == NULL)
		return 0;
	while (strgettokenpair(ops, key, value, &ops) != 0)
	{
		int index = stridxinargs(key, 0, "LIGHT", NULL);
		switch (index)
		{
		case 0:
			light = atoi(value);
			light = (light >= 0 && light < 3) ? light : 0;
			break;
		}
	}
	ops = buf;

	while (strgettokenpair(ops, key, value, &ops) != 0)
	{
		int index = stridxinargs(key, 0, "LINE1", "LINE2", "FORMAT1", "FORMAT2", "COLOR1", "COLOR2", "COLOR",
								 "LINE3", "LINE4", "FORMAT3", "FORMAT4", "LIGHT", "SOUND", "FORMAT", "SPEED", "VOL", "COLOR3", "COLOR4", NULL);
		switch (index)
		{
		case 0:
			strcpy(line1, value);
			break;
		case 1:
			strcpy(line2, value);
			count = 2;
			break;
		case 2:
			fmt1 = atoi(value);
			break;
		case 3:
			fmt2 = atoi(value);
			break;
		case 4:
			color1 = getColor(value, light);
			break;
		case 5:
			color2 = getColor(value, light);
			break;
		case 6:
			color2 = color1 = color3 = color4 = getColor(value, light);
			break;
		case 7:
			strcpy(line3, value);
			break;
		case 8:
			strcpy(line4, value);
			count = 4;
			break;
		case 9:
			fmt3 = atoi(value);
			break;
		case 10:
			fmt4 = atoi(value);
			break;
		case 11:
			break;
		case 12:
			en_voice = 1;
			strcpy(voice_text, value);
			break;
		case 13:
			fmt1 = fmt2 = fmt3 = fmt4 = atoi(value);
			break;
		case 14:
			speed = atoi(value);
			break;
		case 15:
			vul_voice = atoi(value);
			break;
		case 16:
			color3 = getColor(value, light);
			break;
		case 17:
			color4 = getColor(value, light);
			break;
		default:
			trace_log("unknow key: %s ==> args: %s\r\n", key, value);
			break;
		}
	}
	if (en_voice)
	{
		led_send_voice(h_led, voice_text, vul_voice);
		//  led_send_voice(h_led, voice_text, 1);
		trace_log("语音信息：%s 音量：%d \r\n", voice_text, vul_voice);
	}
	for (i = 0; i < count; i++)
	{
		switch (i)
		{
		case 0:
			if (line1[0] != '\0')
				led_send_line(h_led, i + 1, color1, fmt1, speed, line1);
			break;
		case 1:
			if (line2[0] != '\0')
				led_send_line(h_led, i + 1, color2, fmt2, speed, line2);
			break;
		case 2:
			if (line3[0] != '\0')
				led_send_line(h_led, i + 1, color3, fmt3, speed, line3);
			break;
		case 3:
			if (line4[0] != '\0')
				led_send_line(h_led, i + 1, color4, fmt4, speed, line4);
			break;
		default:
			break;
		}
	}
	return -1;
}

static int LEDShow_enable(HANDLE h, BOOL bEnable)
{
	return -1;
}

extern void led_set_picture(HANDLE h, unsigned int nColor, int x, int bOpen);
static int LEDShow_ioctl(HANDLE h, int cmd, int param, void *arg)
{
	cLEDHandle *tmp_h = m_HandleList.head->ptr;
	cLEDHandle *hLED = (cLEDHandle *)h;
	int *option = (int *)arg;
	HANDLE h_led = hLED == NULL ? tmp_h->h_led : hLED->h_led;
	switch (cmd)
	{
	case IOC_LEDMODULE_PLAYDU:
		trace_log("播放嘟一声..\r\n");
		led_play_du(h_led, 3);
		break;
	case IOC_LEDMODULE_SETPIC:
		trace_log("设置图片[%#x][%d][%d]..\r\n", option[0], option[1], option[2]);
		led_set_picture(h_led, option[0], option[1], option[2]);
		break;
	case IOC_LEDMODULE_DISPLAY:
	{
		struct DisplayAttr
		{
			int nLine;
			int format;
			int color;
			const char *text;
		} *display_attr = (struct DisplayAttr *)arg;
		led_send_line(h_led, display_attr->nLine, display_attr->color, display_attr->format, 1, display_attr->text);
	}
	break;
	case IOC_LEDMODULE_SOUNDPLAY:
	{
		struct SoundAttr
		{
			int vol;
			const char *text;
		} *sound_attr = (struct SoundAttr *)arg;
		led_send_voice(h_led, sound_attr->text, sound_attr->vol);
	}
	break;
	case IOC_LEDMODULE_CLEAR:
		led_clear(h_led);
		break;
	default:
		break;
	}
	return 0;
}

static void LEDShow_register_callback(MODULE_CALL_BACK fxc)
{
	module_callback = fxc;
}

static BOOL LEDShow_is_type(int type)
{
	return (m_cLEDModule.type == type);
}

static cIOModule m_cLEDModule = {
	{sizeof(cBaseModule),
	 FMVER_YMD(2019, 1, 26),
	 MOD_TYPE(MTYP_LED, LED_IS_TRANSPEED),
	 "mod_LED",
	 "WR LED module",
	 LEDShow_initialize,
	 LEDShow_terminate,
	 LEDShow_is_type},
	LEDShow_open,
	LEDShow_close,
	LEDShow_enable,
	LEDShow_read,
	LEDShow_append_data,
	LEDShow_ioctl,
	LEDShow_register_callback,
	NULL};

// so文件唯一暴露的API，获取该模块的结构指针，从而可以调用这个模块的API
cIOModule *get_module(void)
{
	return &m_cLEDModule;
}

// #define TEST_CODE_LED
#ifdef ENABLE_TESTCODEGGG
int main(int argc, char *argv)
{
	LEDShow_initialize();
	char txt[256] = {0};
	sprintf(txt, "dev=\"%s\"", argv[1]);
	printf("-> %s \r\n", txt);
	HANDLE h = LEDShow_open(txt);
	char buffer[100];
	int run = 1;
	int index = 0;
	if (h)
	{
		while (1)
		{

			sprintf(buffer, "LINE1=\"C%d\" LINE2=\"TEST\" ", index++);
			LEDShow_append_data(h, buffer, 0);
			sleep(1);
			continue;
		}
		while (run)
		{
			fgets(buffer, sizeof(buffer), stdin);
			switch (buffer[0])
			{
			case 'q':
				run = 0;
				break;
			case 'a':
				printf("play voice..\r\n");
				LEDShow_ioctl(NULL, 1, 0, NULL);
				break;
			case 'b':
			{
				LEDShow_append_data(h, "LINE1=\"你好啊\" LINE2=\"我很好呢\"", 0);
			}
			break;
				cLEDHandle *hLED = (cLEDHandle *)h;
				HANDLE h_led = hLED->h_led;
				// led_send_voice( h_led, "你好",  5 );
				//				led_send_voice( h_led, "[v3][x1]sound123",  5 );
				break;
			default:
				sprintf(buffer, "LINE1=\"GG Smida!\" LINE2=\"XXXXX-> %d\" ", index++);
				LEDShow_append_data(h, buffer, 0);
				break;
			}
		}
	}
}
#endif
