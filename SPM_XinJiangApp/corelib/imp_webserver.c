/**  完成对接后台接口 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <pthread.h>
#include "utils_web.h"
#include "utils_str.h"
#include "cJSON.h"
#include "md5.h"
#include "utils_net.h"
#include "utils_sys.h"
#include "vlprotocol.h"
#include "lc_config.h"
#include "utils_ptrlist.h"

static char message_url[128] = {"http://192.168.0.123:8080/api/message"};
static char request_url[128] = {"http://192.168.0.123:8080/api/qrcode"};
static char tcp_server[128] = {"192.168.0.123:8080"};
static PtrList qrcode_list = PTRLIST_INITIALIZER;
static char device_id[64] = {"123456"};
static char ap_key[64] = {"123456"};
static int bQuit = 0;
static int sock = INVALID_SOCKET;
extern AppConfig apConfig;
static pthread_mutex_t lock;
static pthread_t tcp_thread;
static pthread_t http_thread;
static pthread_t send_qrcode;
extern void ltrace(const char *fmt, ...);
#define trace_log ltrace

extern void onLCDEvent(int index, const char *qrcode, const char *text, const char *sound, int vol, int tout);
extern void onLEDEvent(const char *led);

#define DataLock() pthread_mutex_lock(&lock)
#define DataUnLock() pthread_mutex_unlock(&lock)

static char *time_stamp()
{
    static char timestr[30];
    char *p = timestr;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    strftime(p, sizeof(timestr), "%F %H:%M:%S", localtime(&tv.tv_sec));
    return timestr;
}

static const char *byte2hexBig(BYTE *byte, int nByte, BYTE *dst, int big)
{
    int i = 0;
    BYTE v = 0;
    const char *start = (const char *)dst;
    for (i = 0; i < nByte; i++)
    {
        BYTE val = *byte++;
        v = 0x0f & ((val & 0xf0) >> 4);
        if (big)
            v = v > 9 ? (v - 10 + 'A') : (v + '0');
        else
            v = v > 9 ? (v - 10 + 'a') : (v + '0');
        *dst++ = v;
        v = 0x0f & val;
        if (big)
            v = v > 9 ? (v - 10 + 'A') : (v + '0');
        else
            v = v > 9 ? (v - 10 + 'a') : (v + '0');
        *dst++ = v;
    }
    return start;
}

static void create_message_request(char *msg)
{
    MD5_CTX md5;
    char md[16];
    char tmstr[64] = {0};
    char data[128] = {0};
    char md_val[64] = {0};
    strcpy(tmstr, time_stamp());
    MD5Init(&md5);
    sprintf(data, "%s&%s", ap_key, tmstr);
    MD5Update(&md5, (unsigned char *)data, (unsigned int)strlen(data));
    MD5Final(&md5, (unsigned char *)md);
    byte2hexBig((BYTE *)md, 16, (BYTE *)md_val, 0);
    sprintf(msg, "{\"device_id\":\"%s\",\"time_stamp\":\"%s\",\"sign\":\"%s\"}", device_id, tmstr, md_val);
}

static const char *cJSON_get_item_safe(cJSON *cf, const char *key, const char *defval)
{
    cJSON *ele;
    const char *strval;
    ele = cJSON_GetObjectItem(cf, key);
    if (!ele)
        return defval;
    strval = ele->valuestring;
    if (!strval)
        return defval;
    return strval;
}

static void parse_lcd_event(cJSON *root)
{
    int tout, index;
    char data[4096] = {0};
    char qrcode[256] = {0};
    char sound[256] = {0};
    int vol;
    const char *ptr;
    ptr = cJSON_get_item_safe(root, "timeout", "10000");
    tout = atoi(ptr);
    ptr = cJSON_get_item_safe(root, "index", "1");
    index = atoi(ptr);
    ptr = cJSON_get_item_safe(root, "qrcode", NULL);
    if (ptr && strlen(ptr) > 3)
    {
        strncpy(qrcode, ptr, 256);
    }
    ptr = cJSON_get_item_safe(root, "data", NULL);
    if (ptr && strlen(ptr) > 3)
    {
        strncpy(data, ptr, 4096);
    }
    ptr = cJSON_get_item_safe(root, "sound", NULL);
    if (ptr && strlen(ptr) > 3)
    {
        strncpy(sound, ptr, 256);
    }
    vol = atoi(cJSON_get_item_safe(root, "vol", "5"));
    onLCDEvent(index, qrcode, data, sound, vol, tout);
}

static void parse_led_event(cJSON *child)
{
    char line[128] = {0};
    const char *color;
    const char *tmp;
    char *lines[4] = {0};
    const char *sound = NULL;
    int vol = 5;
    char text[512] = {0};
    cJSON_get_item_safe(child, "line1", " ");
    color = cJSON_get_item_safe(child, "color", "RED");
    lines[0] = cJSON_get_item_safe(child, "line1", " ");
    lines[1] = cJSON_get_item_safe(child, "line2", " ");
    lines[2] = cJSON_get_item_safe(child, "line3", " ");
    lines[3] = cJSON_get_item_safe(child, "line4", " ");
    sound = cJSON_get_item_safe(child, "sound", " ");
    vol = atoi(cJSON_get_item_safe(child, "vol", "5"));
    sprintf(line, "LINE1=\"%s\" FORMAT1=%d ", lines[0], strlen(lines[0]) > 12 ? 5 : 1);
    strcpy(text, line);
    sprintf(line, "LINE2=\"%s\" FORMAT2=%d ", lines[1], strlen(lines[1]) > 12 ? 5 : 1);
    strcat(text, line);
    if (strcmp(lines[2], " ") != 0 && strlen(lines[2]) > 4)
    {
        sprintf(line, "LINE3=\"%s\" FORMAT3=%d ", lines[2], strlen(lines[2]) > 12 ? 5 : 1);
        strcat(text, line);
    }
    if (strcmp(lines[3], " ") != 0 && strlen(lines[3]) > 4)
    {
        sprintf(line, "LINE4=\"%s\" FORMAT4=%d ", lines[3], strlen(lines[3]) > 12 ? 5 : 1);
        strcat(text, line);
    }
    if (strcmp(sound, " ") != 0 && strlen(sound) > 4)
    {
        sprintf(line, "SOUND=\"%s\" VOL=%d ", sound, vol);
        strcat(text, line);
    }
    onLEDEvent(text);
}

static void *workthread_http_notify(void *arg)
{
    char request[1024] = {0};
    char response[2048] = {0};
    char response_gbk[2048] = {0};
    int ret;
    int count = 0;
    int has_online = 0;
    const char *value;
    cJSON *root;
    cJSON *node;
    http_set_timeout(5, 30);
    while (!bQuit)
    {
        if (!has_online && count > 100)
        {
            sleep(10);
            trace_log("请求消息接口一直没有响应，认定后台不支持消息接口..\r\n");
            return NULL;
        }
        create_message_request(request);
        ret = http_post(message_url, 0, request, response, sizeof(response));
        UTF8ToGBK( response, strlen(response), response_gbk);
        if (NET_ERROR_NONE == ret)
        {
            count = 0;
            has_online = 1;
            root = cJSON_Parse(response_gbk);
            if (!root)
            {
                sleep(2);
                continue;
            }
            node = cJSON_GetObjectItem(root, "type");
            if (!node || !node->valuestring)
            {
                cJSON_Delete(root);
                continue;
            }
            if (strcmp(node->valuestring, "idle") == 0)
            {
                value = cJSON_get_item_safe(root, "time_stamp", NULL);
                if (value)
                {
                    struct tm tms;
                    time_t tt;
                    struct timeval tval;
                    strptime(value, "%F %H:%M:%S", &tms);
                    tt = mktime(&tms);
                    if (abs(time(NULL) - tt) > 10)
                    {
                        tval.tv_sec = tt;
                        tval.tv_usec = 0;
                        settimeofday(&tval, NULL);
                    }
                }
                cJSON_Delete(root);
                continue;
            }
            if (strcmp(node->valuestring, "led_display") == 0)
            {
                parse_led_event(root);
                cJSON_Delete(root);
                continue;
            }
            if (strcmp(node->valuestring, "lcd_change") == 0)
            {
                parse_lcd_event(root);
                cJSON_Delete(root);
                continue;
            }
        }
        else
        {
            count++;
            sleep(2);
            continue;
        }
    }
    return NULL;
}

static void get_address(const char *addr, char *ip, int *iport)
{
    char *ptr = strchr(addr, ':');
    char strIP[128] = {0};
    int port = 23;
    if (ptr)
        strncpy(strIP, addr, ptr - addr);
    else
    {
        strcpy(strIP, addr);
    }
    if (ptr)
    {
        ptr++;
        port = atoi(ptr);
    }
    *iport = port;
    strcpy(ip, strIP);
}

static unsigned char getCrc(unsigned char *buf, int len)
{
    unsigned char tmp;
    int i = 0;
    for (i = 0; i < len; i++)
    {
        tmp ^= buf[i];
    }
    return tmp;
}

static int send_packet(int sock, const char *data)
{
    char buffer[4096] = {0};
    int len = strlen(data);
    int index = 0;
    buffer[index++] = 0xFF;
    buffer[index++] = (len >> 24) & 0xff;
    buffer[index++] = (len >> 16) & 0xff;
    buffer[index++] = (len >> 8) & 0xff;
    buffer[index++] = (len >> 0) & 0xff;
    memcpy(buffer + index, data, len);
    index += len;
    buffer[index++] = getCrc(data, len);
    sock_write_n_bytes(sock, buffer, len);
}

static int send_reply(int sock, const char *type, const char *code, const char *msg)
{
    char buffer[256];
    sprintf(buffer, "{\"type\":\"%s\",\"code\":\"%s\",\"msg\":\"%s\"}", type, code, msg);
    send_packet(sock, buffer);
}

int tcp_send_msg(const char *qr)
{
    char buffer[1024];
    char base[1024];
    str_b64enc(qr, base, sizeof(base));
    sprintf(buffer, "{\"type\":\"qrcode\",\"data\":\"%s\"}", base);
    if (sock != INVALID_SOCKET)
    {
        send_packet(sock, buffer);
    }
    else
    {
        printf("tcp 未链接！\r\n");
    }
}

static void *workthread_tcp_notify(void *arg)
{
    char strip[64];
    int port;
    char soh[5];
    int len, rlen;
    char buffer[4096];
    const char *value;
    const char *heart_beat = "{\"type\":\"heartbeat\"}";
    char login[256];
    sprintf(login, "{\"type\":\"login\",\"device_id\":\"%s\",\"key\":\"%s\"}", device_id, ap_key);
    get_address(tcp_server, strip, &port);
    int loginStatus;
    cJSON *root;
    cJSON *node;
    while (!bQuit)
    {
        if (sock == INVALID_SOCKET)
        {
            loginStatus = 0;
            sock = sock_connect(strip, port);
            if (sock == INVALID_SOCKET)
            {
                sleep(5);
                continue;
            }
            send_packet(sock, login);
        }
        if (sock_dataready(sock, 200) > 0)
        {
            len = sock_read_n_bytes(sock, soh, 5);
            if (len <= 0)
            {
                sock_close(sock);
                sock = INVALID_SOCKET;
                continue;
            }
            if (len != 6 || soh[0] != 0xff)
            {
                sock_drain(sock);
                continue;
            }
            len = soh[1] << 24 | soh[2] << 16 | soh[3] << 8 | soh[4];
            rlen = sock_read_n_bytes(sock, buffer, len + 1);
            if (rlen <= 0)
            {
                sock_close(sock);
                sock = INVALID_SOCKET;
                continue;
            }
            if (rlen != (len + 1))
            {
                sock_drain(sock);
                continue;
            }
            buffer[len] = 0;
            root = cJSON_Parse(buffer);
            if (!root)
            {
                sleep(2);
                continue;
            }
            node = cJSON_GetObjectItem(root, "type");
            if (!node || !node->valuestring)
            {
                cJSON_Delete(root);
                continue;
            }
            if (strcmp(node->valuestring, "heartbeat") == 0)
            {
                value = cJSON_get_item_safe(root, "time_stamp", NULL);
                if (value)
                {
                    struct tm tms;
                    time_t tt;
                    struct timeval tval;
                    strptime(value, "%F %H:%M:%S", &tms);
                    tt = mktime(&tms);
                    if (abs(time(NULL) - tt) > 10)
                    {
                        tval.tv_sec = tt;
                        tval.tv_usec = 0;
                        settimeofday(&tval, NULL);
                    }
                }
                cJSON_Delete(root);
                continue;
            }
            if (strcmp(node->valuestring, "led_display") == 0)
            {
                parse_led_event(root);
                cJSON_Delete(root);
                send_reply(sock, "led_display", "0", "success");
                continue;
            }
            if (strcmp(node->valuestring, "lcd_change") == 0)
            {
                parse_lcd_event(root);
                cJSON_Delete(root);
                send_reply(sock, "lcd_change", "0", "success");
                continue;
            }
            cJSON_Delete(root);
            continue;
        }
    }
}

static int http_report_qrcode(const char *arg)
{
    char qrcode[128] = {0};
    char msg[512] = {0};
    char response[1024] = {0};
    char response_gbk[1024] = {0};
    MD5_CTX md5;
    int ret;
    int index = 0;
    cJSON *root, *node;
    char md[16];
    char tmstr[64] = {0};
    char data[128] = {0};
    char md_val[64] = {0};
    char base[512] = {0};
    if (!(apConfig.flag & 0x01))
    {
        printf("未启用http..\r\n");
        return 0;
    }
    strcpy(qrcode, arg);
    trim(qrcode);
    str_b64enc(qrcode, base, sizeof(base));
    strcpy(tmstr, time_stamp());
    MD5Init(&md5);
    sprintf(data, "%s&%s", ap_key, tmstr);
    MD5Update(&md5, (unsigned char *)data, (unsigned int)strlen(data));
    MD5Final(&md5, (unsigned char *)md);
    byte2hexBig((BYTE *)md, 16, (BYTE *)md_val, 0);
    sprintf(msg, "{\"device_id\":\"%s\",\"time_stamp\":\"%s\",\"qrcode\":\"%s\",\"sign\":\"%s\"}", device_id, tmstr, base, md_val);
    trace_log("请求URl:%s \r\n", request_url);
    trace_log("BODY:%s \r\n", msg);
init_request: 
    ret = http_post(request_url, 0, msg, response, sizeof(response));
    UTF8ToGBK( response, strlen(response), response_gbk);
    if (NET_ERROR_NONE == ret)
    {
        trace_log("RESPONSE:%s \r\n", response);
        root = cJSON_Parse(response);
        if (!root)
        {
            trace_log("后台响应非JSON\r\n");
            return -1;
        }
        node = cJSON_GetObjectItem(root, "type");
        if (!node || !node->valuestring)
        {
            cJSON_Delete(root);
            trace_log("请求后台响应异常！\r\n");
            return -1;
        }
        if (strcmp(node->valuestring, "led_display") == 0)
        {
            parse_led_event(root);
            cJSON_Delete(root);
            return 0;
        }
        if (strcmp(node->valuestring, "lcd_change") == 0)
        {
            parse_lcd_event(root);
            cJSON_Delete(root);
            return 0;
        }
        trace_log("不支持的操作类型！%s \r\n", node->valuestring);
        return -1;
    }
    else
    {
        if (index++ < 3)
        {
            trace_log("请求后台没有合法返回，重新请求一次,第【%d】次！\r\n", index + 1);
            goto init_request;
        }
        trace_log("请求后台没有正确的返回结果!！\r\n");
        return -2;
    }
    return -2;
}

static void *workthread_qrcode(void *arg)
{
    const char *qrcode;
    while (!bQuit)
    {
        if (qrcode_list.count == 0)
        {
            usleep(100000);
            continue;
        }
        qrcode = qrcode_list.head->ptr;
        http_report_qrcode(qrcode);
        DataLock();
        PtrList_delete_head(&qrcode_list);
        DataUnLock();
    }
}

int SendQrCodeToHttpServer(const char *arg)
{
    if (EN_FUNCTION(EN_HTTP_INTF))
    { 
        if (StrList_find(&qrcode_list, arg))
        {
            ltrace("相同的二维码，不做处理！\r\n");
            return 0;
        }
        DataLock();
        PtrList_append(&qrcode_list, strdup(arg));
        DataUnLock();
    }
    return 0;
}

void SendQrCodeToTcpServer(const char *arg)
{
    char qrcode[256] = {0};
    if (EN_FUNCTION(EN_TCP_INTF))
    { 
        strncpy(qrcode, arg, sizeof(qrcode)); 
        trim(qrcode);
        tcp_send_msg(qrcode); 
    }
}

void ServicesStart()
{
    strcpy(device_id, apConfig.device_id);
    strcpy(ap_key, apConfig.app_key);
    if (EN_FUNCTION(EN_HTTP_INTF))
    {
        sprintf(message_url, "%s/api/message", apConfig.http_server);
        sprintf(request_url, "%s/api/qrcode", apConfig.http_server);
        THREAD_create(&http_thread, NULL, workthread_http_notify, NULL);
        THREAD_create(&send_qrcode, NULL, workthread_qrcode, NULL);
    }
    if (EN_FUNCTION(EN_TCP_INTF))
    {
        strcpy(tcp_server, apConfig.tcp_server);
        THREAD_create(&tcp_thread, NULL, workthread_tcp_notify, NULL);
    }
}
