#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "gpio.h"
#include "rwdev.h"
#include "utils_net.h"
#include "utils_web.h"
#include "dbg_printf.h"
#include "utils_tty.h"
#include "imp_http_task.h"
#include "lc_config.h"
#include "spm_config.h"
#include "cJSON.h"

typedef unsigned long long UInt64;
#define max(a, b) ((a) > (b) ? (a) : (b))

#define DATA_TCP 1
#define DATA_TTY 2

#define CLIENT_IDLE_TOUT 30000 // 心跳超时
#define LISTEN_PORT 6018       // 本地监听端口

typedef struct tagAppObject
{
    uint32_t magic;
    int has_init;
    int32_t bQuit;
    int32_t listen_tcp;
    int32_t peer_fd;
    int32_t tty_fd;
    int32_t use_net;
    int enableQrCode;
    pthread_t thread;
    pthread_mutex_t mutex;
} APP_OBJECT_S;

static APP_OBJECT_S theApp;
static int g_recv_response = 0;
#define SendLock() pthread_mutex_lock(&theApp.mutex)
#define SendUnLock() pthread_mutex_unlock(&theApp.mutex)
 
 extern void trace_log(const char *fmt,...);

static UInt64 GetTickCount()
{
    static UInt64 begin_time = 0;
    static UInt64 now_time;
    struct timespec tp;
    UInt64 tmsec = 0;
    if (clock_gettime(CLOCK_MONOTONIC, &tp) != -1)
    {
        now_time = tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
    }
    if (begin_time == 0)
        begin_time = now_time;
    tmsec = (UInt64)(now_time - begin_time);
    return tmsec;
}

static const char *show_hex(uint8_t *buf, int32_t size)
{
    int32_t i;
    static char text[4096] = {0};
    char *p = (char *)text;
    uint8_t tmp;
    for (i = 0; i < size; i++)
    {
        tmp = buf[i];
        sprintf((char *)p, "%02X ", tmp);
        p += 3;
    }
    return text;
}

static void len2bcd(int32_t value, uint8_t *bcd)
{
    uint8_t *p = bcd;
    uint8_t nibble;
    nibble = (uint8_t)(value / 100);
    bcd[0] = ((nibble / 10) << 4) + (nibble % 10);
    value %= 100;
    nibble = (uint8_t)value;
    bcd[1] = ((nibble / 10) << 4) + (nibble % 10);
}

static int32_t bcd2len(uint8_t *bcd)
{
    int i = 2;
    int32_t value = 0;
    for (; i--;)
    {
        value *= 100;
        value += (((*bcd) & 0xf0) >> 4) * 10 + ((*bcd) & 0x0f);
        bcd++;
    }
    return value;
}

/**
 * @brief Create a package object
 *
 * @param type 0请求帧，1响应帧
 * @param cmd
 * @param buf
 * @param len
 * @param dst
 * @param max_len
 * @return int32_t
 */
static int32_t create_package(int type, uint32_t cmd, const uint8_t *buf, uint32_t len, uint8_t *dst, uint32_t max_len)
{
    int32_t i = 0;
    uint8_t crc = 0;
    uint8_t *p = dst;
    if (type == 0)
    {
        p[0] = 0xa0;
        p[1] = 0xb0;
        p[2] = 0xc0;
        p[3] = 0xd0;
        p[4] = cmd & 0xff;
    }
    else
    {
        p[0] = 0xa1;
        p[1] = 0xb1;
        p[2] = 0xc1;
        p[3] = 0xd1;
        p[4] = cmd & 0xff; // ack 代码需要设置最高位8
    }
    len2bcd(len, p + 5);
    if (buf && len > 0)
        memcpy(p + 7, buf, len);
    p += 7;
    p += len;
    for (i = 0; i < p - dst; i++)
    {
        crc ^= dst[i];
    }
    p[0] = crc;
    p++;
    return p - dst;
}

void led_display_text(int nLine, int format, int color, const char *text);
void led_clear_all();
void led_voice_send(int vol, const char *text);

static const char *time_stamp()
{
    static char timestr[64];
#ifdef linux
    char *p = timestr;
    struct timeval tv;
    gettimeofday(&tv, NULL);

    strftime(p, sizeof(timestr), "%y/%m/%d %H:%M:%S", localtime(&tv.tv_sec));
    sprintf(p + strlen(p), ".%03lu", tv.tv_usec / 1000);

#else
    SYSTEMTIME tnow;
    GetLocalTime(&tnow);
    sprintf(timestr, "%04d/%02d/%02d %02d:%02d:%02d.%03d",
            tnow.wYear, tnow.wMonth, tnow.wDay,
            tnow.wHour, tnow.wMinute, tnow.wSecond, tnow.wMilliseconds);
#endif
    return timestr;
}

/** 科大讯飞语音播报板 **/
static int send_kd_data(const char *stVoice)
{
    // 科大讯飞语音帧
    char dat[1024] = {0};
    int tty_port = -1;
    int len = strlen(stVoice) + 2;
    int total = len + 5;
    dat[0] = 0xfd;
    dat[1] = (len >> 8) & 0xff;
    dat[2] = len & 0xff;
    dat[3] = 0x01;
    dat[4] = 0x00;
#if defined(TSP86)
    tty_port = open(DEV_SOUND_PORT, O_RDWR);
    if (tty_port < 0)
    {
        trace_log("串口打开失败[%s]！\r\n", DEV_SOUND_PORT);
        return -1;
    }
    tty_raw(tty_port, 9600, 8, 0);
    strcpy(dat + 5, stVoice);
    tty_write(tty_port, dat, total);
    tty_close(tty_port);
#endif
    return 0;
}

void sound_play_du()
{
    char snd_text[32] = {"[v2][x1]sound123"};
    send_kd_data(snd_text);
}

void sound_play_text(int vol, const char *text)
{
    char snd_text[512] = {0};
    int v = 3; // 音量
    int s = 5;
    int t = 5;
    int m = 3;
    v = vol;
    v = (v <= 0 || v > 8) ? 4 : v;
    sprintf(snd_text, "[v%d][s%d][t%d][m%d]%s", v, s, t, m, text);
    send_kd_data(snd_text);
    trace_log("向语音板发送语音：%s 音量：%d \r\n", text, vol);
}

extern SystemConfig g_apconfig;

static int write_file(const char *name, char *da, int len)
{
    FILE *file = fopen(name, "w");
    if (file)
    {
        fwrite(da, 1, len, file);
        fclose(file);
    }
}

static const char *int32toString(int code)
{
    static char buffer[128] = {0};
    sprintf(buffer, "%d", code);
    return buffer;
}

int init_calling_data(char *input)
{
    cJSON *root;
    cJSON *node = NULL;
    char *json_str = NULL;
    int index = 0, ret = 0;
    char response[1024] = {0};
    char response_gbk[1024] = {0};
    char server[128] = {0};
    char utf_buffer[1024];
    int port;
    char phoneId[128] = {0};
    char password[128] = {0};
    // 解析字符串 XX;XX
    sscanf(input, "%[^;];%d;%[^;];%[^;];", server, &port, phoneId, password);
    if(port == 0)
    {
        trace_log("sip初始化信息为空!\r\n");
        return -1;
    }
    trace_log("init sip, server:[%s] port:[%d] user:[%s] password:[%s]\r\n", server, port, phoneId, password);

#define MAX_TRY_COUNT 1

    // 创建 JSON 对象并设置对应的键值
    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "server", server);
    cJSON_AddStringToObject(root, "port", int32toString(port));
    cJSON_AddStringToObject(root, "phoneId", phoneId);
    cJSON_AddStringToObject(root, "password", password);

    // 将 JSON 对象转换为字符串
    json_str = cJSON_Print(root);
    cJSON_Delete(root);
    GBKToUTF8(json_str, strlen(json_str), utf_buffer);
    write_file("call.json", json_str, strlen(json_str));
    free(json_str);
init_request:
    http_set_timeout(2, 3);
    ret = http_post(g_apconfig.talk_third, 0, utf_buffer, response, sizeof(response));
    if (NET_ERROR_NONE == ret)
    {
        UTF8ToGBK(response, strlen(response), response_gbk);
        trace_log("RESPONSE:%s \r\n", response_gbk);
        root = cJSON_Parse(response_gbk);
        if (!root)
        {
            if (index++ < MAX_TRY_COUNT)
            {
                trace_log("后台响应非JSON\r\n");
                goto init_request;
            }
        }
        node = cJSON_GetObjectItem(root, "msg");
        if (!node || !node->valuestring)
        {
            if (index++ < MAX_TRY_COUNT)
            {
                trace_log("请求后台响应异常！\r\n");
                goto init_request;
            }
        }
        if (strcmp(node->valuestring, "success") == 0)
        {
            trace_log("上报对讲信息成功！\r\n");
            return 0;
        }
    }
    else
    {
        if (index++ < MAX_TRY_COUNT)
        {
            trace_log("请求后台没有合法返回，重新请求一次,第【%d】次！\r\n", index + 1);
            goto init_request;
        }
        trace_log("请求后台没有正确的返回结果!！\r\n");
    }
    return 1;
}

static int strendwith(const char *input, const char *endstr)
{
    if (strlen(input) < strlen(endstr))
        return 0;
    return strcmp(input + strlen(input) - strlen(endstr), endstr) == 0;
}

static int update_hang_ctrl(char *url, const char *phoneId, int enable)
{
    char buffer[256] = {0};
    sprintf(buffer, "%s?phoneId=%s&hang=%s", g_apconfig.talk_ctrl, phoneId, enable ? "yes" : "no");
    strcpy(url, buffer);
    trace_log("hang ctrl url:%s\r\n", url);
}

static int update_phone_id(char *url, const char *phoneId)
{
    char *next = strstr(url, "phoneId=");
    trace_log("phoneId:[%s]\r\n", phoneId);
    if (next)
        next += strlen("phoneId=");
    if (strendwith(url, "phoneId="))
        strcat(url, phoneId);
    else
    {
        if (next)
            strcpy(next, phoneId);
    }
    trace_log("update url:%s\r\n", url);
}

/**
 * @brief 处理动态库或者PC发来的消息
 *
 * @param pHvObj
 * @param type
 * @param cmd
 * @param param
 * @param len
 */
static void process_command_data(APP_OBJECT_S *pHvObj, int32_t type, uint8_t cmd, uint8_t *param, int32_t plen)
{
    uint8_t buffer[512] = {0};
    char request[1024] = {0};
    char url[256] = {0};
    int32_t len = 0;
    int32_t id = 0;
    int ret = 0;
    int32_t index, timeout, ptout;
    int32_t v1, v2, v3;
    int32_t ctype = 0;
    char phoneId[128] = {0};
    int code;
    int nNumber, nColor, nFormat;
    struct timeval m_tv;
    struct tm m_tm;
    time_t now;
    uint32_t tmp_buf[20] = {0};
    int reboot = 0;
    char input[1024] = {0};
    trace_log("type:%d cmd:[%#x]:[%s] \r\n", type, cmd, show_hex(param, len));
    switch (cmd)
    {
    case 0x00:
        trace_log("%s 心跳！\r\n", time_stamp());
        len = create_package(1, 0x00, NULL, 0, buffer, sizeof(buffer));
        break;
    case 0x02:
        index = param[0];
        strncpy((char *)url, (char *)param + 1, plen - 1);
        trace_log("登记URL地址[%d][%s]！\r\n", index, url);
        register_http_url(index, url);
        break;
    case 0x03:
        timeout = param[0];
        ptout = param[1];
        http_set_timeout(timeout, ptout);
        trace_log("设置http超时参数[%d][%d]\r\n", timeout, ptout);
        break;
    case 0x90:
        index = param[0];
        id = param[1];
        ctype = param[2];
        strncpy(request, param + 3, plen - 3);
        g_recv_response = 1;
        add_http_post_task(index, id, ctype, request);
        trace_log("POST请求地址【%d】，id【%d】, Type:【%d】, 请求参数【%s】\r\n", index, id, ctype, request);
        break;
    case 0x91:
        id = param[0];
        g_recv_response = 1;
        printf("Recv:%s\r\n", show_hex(param, plen));
        trace_log("GET请求id【%d】,地址:【%s】\r\n", id, request);
        strncpy(request, param + 1, plen - 1);
        add_http_get_task(id, request);
        break;
    case 0x10:
        trace_log("系统控制, 控制码：%d ！\r\n", param[0]);
        if (param[0] == 0x01)
            pHvObj->enableQrCode = 0;
        else if (param[0] == 0x02)
            pHvObj->enableQrCode = 1;
        else
            reboot = 1;
        len = create_package(1, 0x10, NULL, 0, buffer, sizeof(buffer));
        break;
    case 0x01:
        trace_log("设置系统时间！\r\n");
        if (len > 14)
        {
            memcpy(tmp_buf, param, 17);
            tmp_buf[17] = 0;
            if (strptime((const char *)tmp_buf, "%Y%m%d%H%M%S", &m_tm))
            {
                now = mktime(&m_tm);
                m_tv.tv_sec = now;
                m_tv.tv_usec = 0;
                if (time(NULL) - now > 5)
                {
                    trace_log("time is different, reset time..\n");
                    settimeofday(&m_tv, NULL);
                    // rtc_sync();
                }
            }
        }
        len = create_package(1, 0x01, NULL, 0, buffer, sizeof(buffer));
        break;
    case 0x20:
        trace_log("LED显示控制！\r\n");
        nNumber = param[0] & 0x0f;
        nFormat = param[1] & 0x0f;
        nColor = param[2] | param[3] << 8 | param[4] << 16;
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, param + 5, plen - 5);
        led_display_text(nNumber, nFormat, nColor, buffer);
        trace_log("显示内容: %s - 行：%d \r\n", buffer, nNumber);
        len = create_package(1, 0x20, NULL, 0, buffer, sizeof(buffer));
        break;
    case 0x04:
        trace_log("清屏！\r\n");
        led_clear_all();
        len = create_package(1, 0x04, NULL, 0, buffer, sizeof(buffer));
        break;
    case 0x50:
        trace_log("语音播报!\r\n");
        nNumber = param[0];
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, param + 1, plen - 1);
#if defined(TSP86)
        trace_log("向科大语音板发送语音控制！\r\n");
        sound_play_text(nNumber, buffer);
#else
        trace_log("向led模组发送语音控制！\r\n");
        led_voice_send(nNumber, buffer);
#endif
        trace_log("播报内容: %s\r\n", buffer);
        len = create_package(1, 0x50, NULL, 0, buffer, sizeof(buffer));
        break;
    case 0x60:
        trace_log("LCD控制!\r\n");
        index = param[0];
        if (DevCallBack)
        {
            memset(buffer, 0, sizeof(buffer));
            memcpy(buffer, param + 1, plen - 1);
            if (buffer[0] == 0)
            {
                trace_log("参数：【%d】页，负载为空！\r\n", index);
                DevCallBack(EVT_CHANGE_PAGE, index, NULL);
            }
            else
            {
                trace_log("参数：【%d】页，负载[%s]\r\n", index, buffer);
                DevCallBack(EVT_CHANGE_PAGE, index, buffer);
            }
        }
        else
        {
            trace_log("无应用注册回调，不做处理！！\r\n");
        }
        len = create_package(1, 0x60, NULL, 0, buffer, sizeof(buffer));
        break;
    case 0x70:
        v1 = param[0];
        v2 = param[1];
        v3 = param[2] | param[3] << 8 | param[4] << 16 | param[5] << 24;
        if (v1 == 0)
        {
            trace_log("gpio输出[%d][%d]!\r\n", v2, v3);
            GPIO_output(v2, v3);
        }
        else if (v1 == 1)
        {
            trace_log("gpio正向脉冲[%d][%d]!\r\n", v2, v3);
            GPIO_pulse(v2, 1, v3);
        }
        else if (v1 == 2)
        {
            trace_log("gpio反向脉冲[%d][%d]!\r\n", v2, v3);
            GPIO_pulse_negative(v2, 1, v3);
        }
        else
        {
            trace_log("未定义功能\r\n");
        }
        break;
    case 0x80:
        trace_log("初始化语音对讲参数..\r\n");
        memcpy(input, param + 4, plen - 5);
        ret = init_calling_data(input);
        spm_call_init_success(ret);
        break;
    case 0x81:
    {
        memcpy(input, param + 4, plen - 5);
        sscanf((char *)input, "%d;%[^;];", &index, phoneId);
        trace_log("spm call phone:[%d][%s]\r\n", index, input);
        if (index == 0)
        {
            trace_log("触发上工位语音对讲..\r\n");
            strcpy(url, g_apconfig.talk_back_up);
            update_phone_id(url, phoneId);
            add_http_get_task(0, url);
        }
        else if (index == 1)
        {
            trace_log("触发下工位语音对讲..\r\n");
            strcpy(url, g_apconfig.talk_back_dwn);
            update_phone_id(url, phoneId);
            add_http_get_task(0, url);
        }
    }
    break;
    case 0x82:        
        trace_log("接听控制.");
        memcpy(input, param + 4, plen - 5);
        sscanf((char *)input, "%[^;];%d", phoneId, &code);
        trace_log("phoneId:[%s] code:[%d]\r\n", phoneId, code);
        // 如果是默认接听的话，就无所谓了
        if (code == 100)
        {
            trace_log("接听.\r\n");
            update_hang_ctrl(url, phoneId, 1);
            add_http_get_task(id, url);
        }
        else if (code == 101)
        {
            trace_log("挂机\r\n");
            update_hang_ctrl(url, phoneId, 0);
            add_http_get_task(id, url);
        }
        break;
    default:
        break;
    }
    SendLock();
    if (type == DATA_TCP)
    {
        sock_write_n_bytes(pHvObj->peer_fd, buffer, len);
    }
    else if (type == DATA_TTY)
    {
#ifndef ENABLE_TTY_ONLY_QRCODE
        tty_write(pHvObj->tty_fd, buffer, len);
#endif
    }
    SendUnLock();
    if (reboot)
        system("reboot");
}

/**
 * @brief 处理上位机发来的响应消息
 *
 * @param pHvObj
 * @param type
 * @param cmd
 * @param param
 * @param len
 */
static void process_reply_data(APP_OBJECT_S *pHvObj, int32_t type, uint8_t cmd, uint8_t *param, int32_t plen)
{
    uint8_t buffer[256] = {0};
    int32_t len = 0;
    switch (cmd)
    {
    case 0x40:
        trace_log("收到二维码信息帧响应！\r\n");
        break;
    default:
        break;
    }
}

/**
 * @brief 处理上位机发来的语音帧，TFI协议，兼容性考虑
 * A[v3][s5][t5][m3]阿斯顿发
 * @param pHvObj
 * @param raw
 * @param len
 */
static void process_sound_data(APP_OBJECT_S *pHvObj, int8_t *raw, int32_t len)
{
    char text[256] = {0};
    char led_text[256] = {0};
    int vol, d, m, s;
    sscanf(raw, "<A[v%d][s%d][t%d][m%d]%s", &vol, &s, &d, &m, text);
    trace_log("语音内容：%d %d %d %d ; %s \r\n", vol, s, d, m, text);
    led_voice_send(vol, text);
}

static int32_t msock_getc(int fd, int tout)
{
    int32_t ret = 0;
    int32_t rc = 0;
    if (sock_dataready(fd, tout))
    {
        ret = sock_read(fd, &rc, 1);
        return ret == 1 ? rc : -1;
    }
    return 0;
}

static int32_t recv_reply_package_by_tcp(APP_OBJECT_S *pHvObj, uint8_t *buf, uint32_t max_size)
{
    int32_t rc, len;
    int32_t total, i;
    int32_t param_len;
    uint8_t crc = 0;
    uint8_t buffer[1024] = {0};
    uint8_t *p = buffer;
    uint32_t fd = pHvObj->peer_fd;
    int32_t cmd = 0;
    int type = 0;
    do
    {
        rc = msock_getc(fd, 200);
        if (rc < 0)
            goto sock_broken;
        if (rc == 0)
            return 0;
    } while (rc != 0xa1 && rc != 0xa0 && rc != '<');
    if (rc == 0xa0)
    {
        *p++ = 0xa0;
        len = sock_read_n_bytes_tout(fd, p, 6, 100);
        if (len < 0)
            goto sock_broken;
        if (len != 6)
            goto frame_error;
        if (buffer[0] != 0xa0 && buffer[1] != 0xb0 && buffer[2] != 0xc0 && buffer[3] != 0xd0)
            goto frame_error;
        param_len = bcd2len(buffer + 5);
        cmd = buffer[4];
        p += 6;
        rc = param_len + 1;
        total = param_len + 8;
        len = sock_read_n_bytes_tout(fd, p, rc, 100);
        if (len < 0)
            goto sock_broken;
        if (len != rc)
            goto frame_error;
        for (i = 0; i < total - 1; i++)
            crc ^= buffer[i];
        if (crc != buffer[total - 1])
        {
            trace_log("crc错误，无法处理！\r\n");
            return 0;
        }
        process_command_data(pHvObj, DATA_TCP, cmd, buffer + 7, param_len);
    }
    else if (rc == 0xa1)
    {
        *p++ = 0xa1;
        len = sock_read_n_bytes_tout(fd, p, 6, 100);
        if (len < 0)
            goto sock_broken;
        if (len != 6)
            goto frame_error;
        if (buffer[0] != 0xa1 && buffer[1] != 0xb1 && buffer[2] != 0xc1 && buffer[3] != 0xd1)
            goto frame_error;
        param_len = bcd2len(buffer + 5);
        cmd = buffer[4];
        p += 6;
        rc = param_len + 1;
        total = param_len + 8;
        len = sock_read_n_bytes_tout(fd, p, rc, 100);
        if (len < 0)
            goto sock_broken;
        if (len != rc)
            goto frame_error;
        for (i = 0; i < total - 1; i++)
            crc ^= buffer[i];
        if (crc != buffer[total - 1])
        {
            trace_log("crc错误，无法处理！\r\n");
            return 0;
        }
        process_reply_data(pHvObj, DATA_TCP, cmd, buffer + 7, param_len);
    }
    else if (rc == '<')
    {
        *p++ = '<';
        len = sock_read_until(fd, p, sizeof(buffer) - 1, '>');
        trace_log("收到语音帧！ RECV:%s \r\n", show_hex(buffer, len));
        if (len <= 0)
            goto sock_broken;
        if (p[len - 1] != '>')
            goto frame_error;
        total = len + 1;
        process_sound_data(pHvObj, buffer, len + 1);
    }
    else
    {
        return 0;
    }
    return total;
sock_broken:
    trace_log(" 连接已断开，返回-1！\r\n");
    return -1;
frame_error:
    trace_log("帧错误，无法处理，忽略处理！！\r\n");
    trace_log("RECV:%s \r\n", show_hex(buffer, len));
    sock_drain(fd);
    return 0;
success:
    return 0;
}

static int32_t recv_reply_package_by_tty(APP_OBJECT_S *pHvObj, uint8_t *buf, uint32_t max_size)
{
    int32_t rc, len;
    int32_t total, i;
    int32_t param_len;
    uint8_t crc = 0;
    uint8_t buffer[1024] = {0};
    uint8_t *p = buffer;
    uint32_t fd = pHvObj->tty_fd;
    int32_t cmd = 0;
    int type = 0;
    while ((rc = tty_getc(fd, 50, 0)) > 0 && (rc != 0xa1 && rc != 0xa0 && rc != '<'))
        ;
    if (rc == -1)
        return 0;
    if (rc == 0xa0)
    {
        *p++ = 0xa0;
        len = tty_read(fd, (char *)p, 6, 50);
        if (len != 6)
            goto frame_error;
        if (buffer[0] != 0xa0 && buffer[1] != 0xb0 && buffer[2] != 0xc0 && buffer[3] != 0xd0)
            goto frame_error;
        param_len = bcd2len(buffer + 5);
        cmd = buffer[4];
        p += 6;
        rc = param_len + 1;
        total = param_len + 8;
        len = tty_read(fd, (char *)p, rc, 50);
        if (len != rc)
            goto frame_error;
        for (i = 0; i < total - 1; i++)
            crc ^= buffer[i];
        if (crc != buffer[total - 1])
        {
            trace_log("CRC 错误！[%s]\r\n", show_hex(buffer, total));
            goto frame_error;
        }
        printf("长度%d\r\n", param_len);
        process_command_data(pHvObj, DATA_TTY, cmd, buffer + 7, param_len);
    }
    else if (rc == 0xa1)
    {
        *p++ = 0xa1;
        len = tty_read(fd, p, 6, -1);
        if (len != 6)
            goto frame_error;
        if (buffer[0] != 0xa1 && buffer[1] != 0xb1 && buffer[2] != 0xc1 && buffer[3] != 0xd1)
            goto frame_error;
        param_len = bcd2len(buffer + 5);
        cmd = buffer[4];
        p += 6;
        rc = param_len + 1;
        total = param_len + 8;
        len = tty_read(fd, p, rc, -1);
        if (len != rc)
            goto frame_error;
        for (i = 0; i < total - 1; i++)
            crc ^= buffer[i];
        if (crc != buffer[total - 1])
        {
            trace_log("CRC 错误！[%s]\r\n", show_hex(buffer, total));
            goto frame_error;
        }
        process_reply_data(pHvObj, DATA_TTY, cmd, buffer + 7, param_len);
    }
    else if (rc == '<')
    {
        *p++ = '<';
        len = tty_gets(fd, p, sizeof(buffer) - 1, 50, '>', NULL);
        trace_log("收到语音帧！ RECV:%s -- %c \r\n", show_hex(buffer, len + 1), p[len]);
        if (p[len - 1] != '>')
            goto frame_error;
        process_sound_data(pHvObj, buffer, len + 1);
        total = len + 1;
    }
    else
    {
        return 0;
    }
    return total;
frame_error:
    trace_log("帧错误，无法处理，忽略处理 - %d！！\r\n", len);
    trace_log("RECV:%s \r\n", show_hex(buffer, len));
    sock_drain(fd);
    return 0;
success:
    return 0;
}

void *protocol_thread(void *arg)
{
    APP_OBJECT_S *pHvObj = &theApp;
    uint32_t nsel, fdmax;
    const char *dev = NULL;
    UInt64 ltLastHeard = GetTickCount();
    UInt64 ltLastSnd = GetTickCount();
    struct timeval tv;
    int peer_fd = 0;
    int fd;
    uint8_t recv_buffer[1024];
    uint8_t snd_buffer[1024];
    pHvObj->enableQrCode = 1;
    dev = DEV_PORT_PC;
    pHvObj->tty_fd = open(dev, O_RDWR);
    if (pHvObj->tty_fd < 0)
    {
        trace_log("打开串口失败[%s]！无法处理！\r\n", dev);
        return -1;
    }
    tty_raw(pHvObj->tty_fd, 9600, 8, 0);
    trace_log("本地串口服务打开成功【%s】！\r\n", dev);
    pHvObj->listen_tcp = sock_listen(LISTEN_PORT, NULL, 5);
    if (pHvObj->listen_tcp <= 0)
    {
        trace_log("listen on local port error :%s \r\n", strerror(errno));
        return NULL;
    }
    pHvObj->bQuit = 0;
    pHvObj->has_init = 1;
    while (!pHvObj->bQuit)
    {
        fd_set read_fds;
        fdmax = pHvObj->listen_tcp;
        FD_ZERO(&read_fds);
        FD_SET(pHvObj->listen_tcp, &read_fds);
        if (peer_fd > 0)
        {
            FD_SET(peer_fd, &read_fds);
            fdmax = max(fdmax, peer_fd);
        }
        FD_SET(pHvObj->tty_fd, &read_fds);
        fdmax = max(fdmax, pHvObj->tty_fd);
        tv.tv_sec = 0;
        tv.tv_usec = 10 * 1000;
        nsel = select(fdmax + 1, &read_fds, NULL, NULL, &tv);
        if (nsel > 0)
        {
            if (FD_ISSET(pHvObj->listen_tcp, &read_fds))
            {
                fd = sock_accept(pHvObj->listen_tcp);
                if (peer_fd > 0)
                {
                    trace_log("new client comming, disconnect the former client..\r\n");
                    sock_close(peer_fd);
                }
                trace_log("new client is comming..\r\n");
                peer_fd = fd;
                pHvObj->peer_fd = fd;
                ltLastHeard = GetTickCount();
                continue;
            }
            if (peer_fd > 0 && FD_ISSET(peer_fd, &read_fds))
            {
                if (recv_reply_package_by_tcp(pHvObj, recv_buffer, sizeof(recv_buffer)) < 0)
                {
                    sock_close(peer_fd);
                    pHvObj->peer_fd = peer_fd = INVALID_SOCKET;
                    continue;
                }
                ltLastHeard = GetTickCount();
            }
            //  不管模式时，不处理接收到的串口数据
#ifdef ENABLE_TTY_ONLY_QRCODE
            if (FD_ISSET(pHvObj->tty_fd, &read_fds))
            {
                tty_clear(pHvObj->tty_fd);
            }
#else
            if (FD_ISSET(pHvObj->tty_fd, &read_fds))
            {
                if (recv_reply_package_by_tty(pHvObj, recv_buffer, sizeof(recv_buffer)))
                    ltLastHeard = GetTickCount();
            }
#endif
        }
        if (peer_fd > 0 && ltLastHeard + CLIENT_IDLE_TOUT < GetTickCount())
        {
            trace_log("已经很久没收到设备的心跳信息，认定客户端已经离线！\r\n");
            sock_close(peer_fd);
            pHvObj->peer_fd = peer_fd = INVALID_SOCKET;
            continue;
        }
    }
    return NULL;
}

void spm_gpio_change(int di_last, int di_this)
{
    char buf[32] = {0};
    char buffer[64] = {0};
    int32_t len;
    memcpy(buf, &di_this, 4);
    trace_log("上报io状态：%#x \r\n", di_this);
    len = create_package(0, 0x80, (uint8_t *)buf, 4, buffer, sizeof(buffer));
    SendLock();
    if (theApp.peer_fd > 0)
        sock_write_n_bytes(theApp.peer_fd, buffer, len);
    if (theApp.tty_fd > 0)
        tty_write(theApp.tty_fd, buffer, len);
    SendUnLock();
}

void spm_answer_talk(const char *phoneId)
{
    int len;
    char buf[256] = {0};
    char buffer[256] = {0};
    phoneId = phoneId == NULL ? "" : phoneId;
    len = strlen(phoneId) + 5;
    strcpy(buf + 4, phoneId);
    trace_log("上报语音接入:%s.\r\n", phoneId);
    len = create_package(0, 0x85, (uint8_t *)buf, len, buffer, sizeof(buffer));
    SendLock();
    if (theApp.peer_fd > 0)
        sock_write_n_bytes(theApp.peer_fd, buffer, len);
    if (theApp.tty_fd > 0)
        tty_write(theApp.tty_fd, buffer, len);
    SendUnLock();
}



void spm_call_init_success(int ret)
{
    int len;
    char buf[32] = {0};
    char buffer[64] = {0};
    buf[0] = ret & 0xff;
    trace_log("上报对讲初始化结果..\r\n");
    len = create_package(0, 0x86, (uint8_t *)buf, 4, buffer, sizeof(buffer));
    SendLock();
    if (theApp.peer_fd > 0)
        sock_write_n_bytes(theApp.peer_fd, buffer, len);
    if (theApp.tty_fd > 0)
        tty_write(theApp.tty_fd, buffer, len);
    SendUnLock();
}

void spm_answer_status(const char *status)
{
    int len;
    char buf[256] = {0};
    char buffer[256] = {0};
    int code = atoi(status);
    buf[0] = code & 0xff;
    trace_log("上报语音接入状态:[%s]\r\n", status );
    len = create_package(0, 0x87, (uint8_t *)buf, 4, buffer, sizeof(buffer));
    SendLock();
    if (theApp.peer_fd > 0)
        sock_write_n_bytes(theApp.peer_fd, buffer, len);
    if (theApp.tty_fd > 0)
        tty_write(theApp.tty_fd, buffer, len);
    SendUnLock();
}

void spm_send_qrcode(int index, const char *qrcode)
{
    char buf[256] = {0};
    int32_t len;
    uint8_t buffer[256] = {0};
    if (strlen(qrcode) <= 5)
    {
        trace_log("非法二维码数据！\r\n");
        return;
    }
    buf[0] = index & 0xff;
    strcpy(buf + 1, qrcode);
    len = create_package(0, 0x40, (uint8_t *)buf, strlen(qrcode) + 1, buffer, sizeof(buffer));
    SendLock();
#if defined ENABLE_TTY_ONLY_QRCODE
    // 直接上报二维码
    strcpy(buf, qrcode);
    len = strlen(qrcode);
    tty_write(theApp.tty_fd, buf, len);
#elif defined USE_FF_PROTOCOL
    // 私用F1 qr code FF 格式发送
    buf[0] = 0xf1;
    strcpy(buf + 1, qrcode);
    len = strlen(qrcode);
    buf[len + 1] = 0xff;
    len += 2;
    if (theApp.peer_fd > 0)
        sock_write_n_bytes(theApp.peer_fd, buf, len);
    tty_write(theApp.tty_fd, buf, len);
    trace_log("发送F1开头二维码数据： %s \r\n", show_hex(buf, len));
#elif defined USE_TFI_EXP1_PROTOCOL
    buf[0] = 0x11;
    strcpy(buf + 1, qrcode);
    len = create_package(0, 0x40, (uint8_t *)buf, strlen(qrcode) + 1, buffer, sizeof(buffer));
    if (theApp.peer_fd > 0)
        sock_write_n_bytes(theApp.peer_fd, buffer, len);
    tty_write(theApp.tty_fd, buffer, len);
    trace_log("使用TFI费显协议：%s \r\n", show_hex(buffer, len));
#elif defined USE_TFI_PROTOCOL
    // 使用TFI协议
    // 兼容老款协议，不带ID标识
    strcpy(buf, qrcode);
    len = create_package(1, 0x40, (uint8_t *)buf, strlen(qrcode), buffer, sizeof(buffer));
    if (theApp.peer_fd > 0)
        sock_write_n_bytes(theApp.peer_fd, buffer, len);
    tty_write(theApp.tty_fd, buffer, len);
    trace_log("使用TFI费显协议：%s \r\n", show_hex(buffer, len));
#else
    // 标准定制协议
    if (theApp.peer_fd > 0)
        sock_write_n_bytes(theApp.peer_fd, buffer, len);
    tty_write(theApp.tty_fd, buffer, len);
    trace_log("发送二维码数据： %s \r\n", show_hex(buffer, len));
#endif
    SendUnLock();
}

#define min(a, b) ((a) > (b) ? (b) : (a))
static void serial_protocol_callback(int id, int ret, char *response, int size)
{
    char buf[1024] = {0};
    char buffer[2048] = {0};
    int asize = min(950, size);
    int32_t len;
    if(!g_recv_response)
        return;
    buf[0] = id & 0xff;
    buf[1] = abs(ret) & 0xff;
    strncpy(buf + 2, response, asize);
    len = create_package(0, 0x09, (uint8_t *)buf, asize + 2, buffer, sizeof(buffer));
    SendLock();
    if (theApp.peer_fd > 0)
        sock_write_n_bytes(theApp.peer_fd, buffer, len);
#ifndef ENABLE_TTY_ONLY_QRCODE
    tty_write(theApp.tty_fd, buffer, len);
#endif
    trace_log("发送响应参数：%s \r\n", show_hex(buffer, len));
    SendUnLock();
}

void local_protocol_init()
{
    http_task_init();
    http_task_setcallback(serial_protocol_callback);
    pthread_create(&theApp.thread, NULL, protocol_thread, NULL);
}

void local_protocol_exit()
{
    pthread_cancel(theApp.thread);
    pthread_join(&theApp.thread, NULL);
}
