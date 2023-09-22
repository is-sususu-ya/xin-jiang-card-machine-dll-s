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

#include "cJSON.h"
#include "utils_ptrlist.h"
#include "dbg_printf.h"
#include "MQTTClient.h"
#include "MQTTLinux.h"
#include "nromal_task.h"

#include "lc_config.h"

#define MQ_TOPIC_QOS 1

typedef struct tagTopicHandle
{
    int qos; // ������׼
    const char *topic;
    const char *reply_topic;
    void (*messagefxc)(MessageData *md);
} TopicHandle;

typedef struct tagAppObject
{
    uint32_t magic;
    int hasInit;
    int32_t bQuit;
    int32_t tty_fd;
    int32_t use_net;
    char *server_ip;
    int server_port;
    char *will_msg;
    char *will_topic;
    char *client_id;
    char *client_name;
    char *client_pwd;
    char *topic;
    int interval;
    int en_scan;
    int mem_total;
    int mem_free;
    int disk_total;
    int disk_free;
    int cpu;
    int fd;
    int update_flag;
    Network n;
    MQTTClient c;
    PtrList TopicList;
    PtrList MessageQueue;
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_mutex_t lock;
    unsigned char sndbuf[4096];
    unsigned char readbuf[4096];
} MqTTObject;

static MqTTObject _mqttApp;
#define MQLock() pthread_mutex_lock(&_mqttApp.lock)
#define MQUnLock() pthread_mutex_unlock(&_mqttApp.lock)

static void message_handle_sysctr(MessageData *md);
static void message_handle_soundplay(MessageData *md);
static void message_handle_ledshow(MessageData *md);
static int is_mq_online();

extern void ltrace(const char *fmt, ...);

#define MSGDATA_INIT(md)                                                                 \
    char topic[128] = {0};                                                               \
    char gbk_text[4096] = {0};                                                           \
    cJSON *root = NULL;                                                                  \
    unsigned char *payload = md->message->payload;                                       \
    int len = md->message->payloadlen;                                                   \
    int qos = md->message->qos;                                                          \
    int id = md->message->id;                                                            \
    sprintf(topic, "%.*s", md->topicName->lenstring.len, md->topicName->lenstring.data); \
    if (payload)                                                                         \
    {                                                                                    \
        UTF8ToGBK(payload, len, gbk_text);                                               \
        root = cJSON_Parse(gbk_text);                                                    \
    }

static const char *find_reply_topic(const char *topic)
{
    POSITION pos = _mqttApp.TopicList.head;
    for (; pos != NULL; pos = pos->next)
    {
        TopicHandle *h = (TopicHandle *)pos->ptr;
        if (strcmp(h->topic, topic) == 0)
            return h->reply_topic;
    }
    return NULL;
}

static char *time_stamp()
{
    static char timestr[30];
    char *p = timestr;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    strftime(p, sizeof(timestr), "%F %H:%M:%S", localtime(&tv.tv_sec));
    return timestr;
}

static const char *get_device_id()
{
    static char buffer[128] = {0};
    sprintf(buffer, "%s/%s", apConfig.mqtt_project_name, apConfig.mqtt_device_id);
    return buffer;
}

static const char *get_online_text(int en)
{
    static char text[128] = {0};
    sprintf(text, "{\"type\":\"%s\",\"id\":\"%s\"}", en ? "online" : "offline", get_device_id());
    return text;
}
// 47.116.13.41:1883
static const char *get_topic_prefix()
{
    static char text[256] = {0};
    sprintf(text, "ts/spm/x5/%s/%s", apConfig.mqtt_project_name, apConfig.mqtt_device_id);
    return text;
}

static int mqtt_client_init(MqTTObject *pHvObj)
{
    int rc, i, fd;
    int command_timeout = 2000; // �ȴ���Ӧ��ʱ��
    char topic[128] = {0};
    char will_text[128] = {0};
    POSITION pos = pHvObj->TopicList.head;
    NetworkInit(&pHvObj->n);
    fd = NetworkConnect(&pHvObj->n, pHvObj->server_ip, pHvObj->server_port);
    if (fd < 0)
    {
        ltrace("connect to server error[%s:%d]\r\n", pHvObj->server_ip, pHvObj->server_port);
        return -1;
    }
    ltrace("connect to server %s port %d success \r\n", pHvObj->server_ip, pHvObj->server_port);
    pHvObj->fd = fd;
    MQTTClientInit(&pHvObj->c, &pHvObj->n, command_timeout, pHvObj->sndbuf, sizeof(pHvObj->sndbuf), pHvObj->readbuf, sizeof(pHvObj->readbuf));
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    MQTTPacket_willOptions will = MQTTPacket_willOptions_initializer;
    sprintf(topic, "%s/link", get_topic_prefix());
    will.topicName.cstring = pHvObj->will_topic;
    will.qos = 1;
    will.retained = 1;
    will.message.cstring = pHvObj->will_msg;
    data.willFlag = 1;
    memcpy(&data.will, &will, sizeof(will));
    data.MQTTVersion = 3;
    data.clientID.cstring = pHvObj->client_id;
    data.username.cstring = pHvObj->client_name;
    data.password.cstring = pHvObj->client_pwd;
    data.keepAliveInterval = pHvObj->interval; // �������
    data.cleansession = 1;
    rc = MQTTConnect(&pHvObj->c, &data);
    if (rc != SUCCESS)
    {
        MQTTDisconnect(&pHvObj->c);
        NetworkDisconnect(pHvObj);
        pHvObj->fd = -1;
        ltrace("=== ERROR ==== \r\nconnect error\r\n");
        return -1;
    }
    while (pos != NULL)
    {
        TopicHandle *t = (TopicHandle *)pos->ptr;
        rc = MQTTSubscribe(&pHvObj->c, t->topic, t->qos, t->messagefxc);
        if (rc == -1)
        {
            ltrace("=== ERROR ==== \r\n����ʧ�ܣ��Ͽ����ӣ�\r\n");
            MQTTDisconnect(&pHvObj->c);
            NetworkDisconnect(pHvObj);
            return -1;
        }
        else
        {
            ltrace("���ġ�%s���ɹ���\r\n", t->topic);
        }
        pos = pos->next;
    }
    ltrace("MQTT Init Success!\r\n");
    return rc;
}

static int init_message_handle(MqTTObject *pHvObj, int qos, const char *tpic, const char *reply_topic, void (*msgcb)(MessageData *md))
{
    TopicHandle *topic = malloc(sizeof(TopicHandle));
    memset(topic, 0, sizeof(TopicHandle));
    topic->qos = qos;
    topic->topic = strdup(tpic);
    if (reply_topic)
        topic->reply_topic = strdup(reply_topic);
    topic->messagefxc = msgcb;
    ltrace("��ʼ���������⣺[%s]���ȼ�[%d]\r\n", tpic, qos);
    PtrList_append(&pHvObj->TopicList, topic);
}

int config_init(MqTTObject *pHvObj)
{
    char topic[256] = {0};
    char reply[256] = {0};
    char strip[26] = {0};
    uint8_t MQTTPort[2];
    pHvObj->server_ip = strdup(apConfig.mqtt_server);
    pHvObj->server_port = apConfig.mqtt_port;
    pHvObj->client_id = strdup(apConfig.mqtt_device_id);
    pHvObj->client_name = strdup(apConfig.mqtt_username);
    pHvObj->client_pwd = strdup(apConfig.mqtt_password);
    pHvObj->will_msg = strdup(get_online_text(0));
    pHvObj->interval = 30;
    sprintf(topic, "%s/link", get_topic_prefix());
    pHvObj->will_topic = strdup(topic);
    ltrace("MQTT:%s - %d \r\n", pHvObj->server_ip, pHvObj->server_port);
    ltrace("MQTT:[%s][%s][%s][%s]\r\n", apConfig.mqtt_project_name, apConfig.mqtt_device_id, apConfig.mqtt_username, apConfig.mqtt_password);
    // ����
    sprintf(topic, "%s/ledshow", get_topic_prefix());
    init_message_handle(pHvObj, MQ_TOPIC_QOS, topic, NULL, message_handle_ledshow);
    sprintf(topic, "%s/soundplay", get_topic_prefix());
    init_message_handle(pHvObj, MQ_TOPIC_QOS, topic, NULL, message_handle_soundplay);
    sprintf(topic, "%s/sysctr", get_topic_prefix());
    init_message_handle(pHvObj, MQ_TOPIC_QOS, topic, NULL, message_handle_sysctr);
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

static int led_json_format(cJSON *root, char *text)
{
    char line[128] = {0};
    const char *color;
    char *lines[4] = {0};
    const char *sound = NULL;
    cJSON_get_item_safe(root, "line1", " ");
    color = cJSON_get_item_safe(root, "color", "RED");
    lines[0] = cJSON_get_item_safe(root, "line1", " ");
    lines[1] = cJSON_get_item_safe(root, "line2", " ");
    lines[2] = cJSON_get_item_safe(root, "line3", " ");
    lines[3] = cJSON_get_item_safe(root, "line4", " ");
    if (strcmp(lines[0], " ") == 0 && strcmp(lines[1], " ") != 0)
    {
        ltrace("LED���ݷǷ���\r\n");
        return -1;
    }
    sprintf(line, "COLOR=%s ", color);
    strcpy(text, line);
    sprintf(line, "LINE1=\"%s\" FORMAT1=%d ", lines[0], strlen(lines[0]) > 12 ? 5 : 1);
    strcat(text, line);
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
    return 0;
}

static int sound_json_format(cJSON *child, char *text)
{
    const char *tt = cJSON_get_item_safe(child, "text", " ");
    if (strcmp(tt, " ") == 0)
    {
        ltrace("�������ݸ�ʽ�Ƿ���\r\n");
        return -1;
    }
    sprintf(text, "SOUND=\"%s\" VOL=%s ", cJSON_get_item_safe(child, "text", " "), cJSON_get_item_safe(child, "vol", " "));
    return 0;
}

static void message_handle_ledshow(MessageData *md)
{
    char notifyID[21] = {0};
    int DeviceStatus = 0; // �����ȼ�
    int text_len = 0;
    char text[1024] = {0};
    const char *topic_reply = NULL;
    MQTTMessage reply_message;
    struct task_content cot = {0};
    MSGDATA_INIT(md)
    ltrace("Topic: %s \r\n", topic);
    ltrace("Payload: len: %d\r\n %s \r\n", len, gbk_text);
    if (!root)
    {
        ltrace("���ݷ�JSON��ʽ�����ԣ�\r\n");
        strcpy(cot.text, "LED��ʾ���ݷǺϷ�Json����");
        task_add(TASK_SYSTEM_ERROR, 0, &cot, sizeof(cot));
        return;
    }
    if (-1 == led_json_format(root, text))
    {
        ltrace("���ݷǷ���\r\n");
        strcpy(cot.text, "LED����Json��ʽ�Ƿ���");
        task_add(TASK_SYSTEM_ERROR, 0, &cot, sizeof(cot));
        cJSON_Delete(root);
        return;
    }
    strcpy(cot.text, text);
    task_add(TASK_LED_SHOW, 0, &cot, sizeof(cot));
    cJSON_Delete(root);
}

static void message_handle_soundplay(MessageData *md)
{
    char notifyID[21] = {0};
    int DeviceStatus = 0; // �����ȼ�
    int text_len = 0;
    char text[1024] = {0};
    const char *topic_reply = NULL;
    MQTTMessage reply_message;
    struct task_content cot = {0};
    MSGDATA_INIT(md)
    ltrace("Topic: %s \r\n", topic);
    ltrace("Payload: len: %d\r\n %s \r\n", len, gbk_text);
    if (!root)
    {
        ltrace("���ݷ�JSON��ʽ�����ԣ�\r\n");
        strcpy(cot.text, "�����������ݷǺϷ�Json����");
        task_add(TASK_SYSTEM_ERROR, 0, &cot, sizeof(cot));
        return;
    }
    if ( -1 == sound_json_format(root, text) )
    {
        ltrace("�������ݷǷ�����ʽ����ȷ\r\n");
        strcpy(cot.text, "��������Json��ʽ�Ƿ���");
        task_add(TASK_SYSTEM_ERROR, 0, &cot, sizeof(cot));
        cJSON_Delete(root);
        return;
    }
    strcpy(cot.text, text);
    task_add(TASK_LED_SHOW, 0, &cot, sizeof(cot));
    cJSON_Delete(root);
}

static void message_handle_sysctr(MessageData *md)
{
    char notifyID[21] = {0};
    int DeviceStatus = 0; // �����ȼ�
    int text_len = 0;
    char text[1024] = {0};
    char reply_data[256] = {0};
    const char *type, *param;
    const char *topic_reply = NULL;
    MQTTMessage reply_message;
    struct task_content cot = {0};
    MSGDATA_INIT(md)
    ltrace("Topic: %s \r\n", topic);
    ltrace("Payload: len: %d\r\n %s \r\n", len, gbk_text);
    if (!root)
    {
        ltrace("���ݷ�JSON��ʽ�����ԣ�\r\n");
        strcpy(cot.text, "���ݷ�JSON��ʽ�����ԣ�");
        task_add(TASK_SYSTEM_ERROR, 0, &cot, sizeof(cot));
        return;
    }
    type = cJSON_get_item_safe(root, "type", NULL);
    param = cJSON_get_item_safe(root, "param", NULL);
    if (!type)
    {
        ltrace("������type�����ֶΣ�\r\n");
        strcpy(cot.text, "������type�����ֶΣ�");
        task_add(TASK_SYSTEM_ERROR, 0, &cot, sizeof(cot));
        goto error_ret;
    }
    if (strcmp("sync_time", type) == 0 && param)
    {
        struct tm tms;
        time_t tt;
        struct timeval tval;
        strptime(param, "%F %H:%M:%S", &tms);
        tt = mktime(&tms);
        ltrace("��ϵͳʱ������10s������ϵͳʱ��");
        tval.tv_sec = tt;
        tval.tv_usec = 0;
        settimeofday(&tval, NULL);
    }
    else if (strcmp("enable_scan", type) == 0)
    {
        if (!param)
        {
            ltrace("ʹ��ɨ�뵫��û�в�����\r\n");
            goto error_ret;
        }
        ltrace("ʹ��ɨ�룺%s \r\n", param);
        if (strcmp(param, "yes") == 0)
        {
            set_enabel_scan(1);
        }
        else
        {
            set_enabel_scan(0);
        }
    }
    else if (strcmp("reboot", type) == 0)
    {
        ltrace("ϵͳ����\r\n");
        system("reboot");
    }
    else if (strcmp("link_check", type) == 0)
    {
        ltrace("�������״̬��\r\n");
        _mqttApp.update_flag = 1;
    }
error_ret:
    cJSON_Delete(root);
}

static int mqtt_publish_all(const char *topic, int qos, int retain, void *payload, int size)
{
    MQTTMessage message;
    if (!is_mq_online())
        return -1;
    int success;
    memset(&message, 0, sizeof(message));
    message.qos = qos;
    message.retained = retain;
    message.payload = payload;
    message.payloadlen = size;
    if (payload)
        ltrace("������Ϣ��%s \r\n%s \r\n", topic, payload);
    else
        ltrace("��������Ϣ: %s \r\n", topic);
    MQLock();
    success = MQTTPublish(&_mqttApp.c, topic, &message);
    MQUnLock();
    if (0 != success)
        ltrace("����ʧ�ܡ�%s��\r\n", topic);
    return success;
}

#define mqtt_publish(a, b, c, d) mqtt_publish_all(a, b, 0, c, d)

static void dev_status()
{
    char topic[128] = {0};
    const char *ptr = NULL;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "id", get_device_id());
    cJSON_AddBoolToObject(root, "en_scan", _mqttApp.en_scan ? 1 : 0);
    cJSON_AddNumberToObject(root, "cpu", _mqttApp.cpu);
    cJSON_AddNumberToObject(root, "mem_total", _mqttApp.mem_total);
    cJSON_AddNumberToObject(root, "mem_free", _mqttApp.mem_free);
    cJSON_AddNumberToObject(root, "disk_total", _mqttApp.disk_total);
    cJSON_AddNumberToObject(root, "disk_free", _mqttApp.disk_free);
    ptr = cJSON_Print(root);
    cJSON_Delete(root);
    sprintf(topic, "%s/status", get_topic_prefix());
    mqtt_publish(topic, 0, ptr, strlen(ptr));
    free(ptr);
}

void mqtt_send_qrcode(int index, const char *qrcode)
{
    char topic[128] = {0};
    const char *ptr = NULL;
    cJSON *root = NULL;
    if (!is_mq_online())
    {
        ltrace("MQû�����磬�޷�������ά����Ϣ��\r\n");
        return;
    }
    if (!qrcode || strlen(qrcode) < 4)
    {
        ltrace("�Ƿ��Ķ�ά����Ϣ��\r\n");
        return;
    }
    ltrace("������ά����Ϣ��\r\n");
    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "id", get_device_id());
    cJSON_AddStringToObject(root, "time_stamp", time_stamp());
    cJSON_AddStringToObject(root, "qrcode", qrcode);
    ptr = cJSON_Print(root);
    cJSON_Delete(root);
    sprintf(topic, "%s/qrcode", get_topic_prefix());
    mqtt_publish(topic, 1, ptr, strlen(ptr));
    free(ptr);
}

void mqtt_send_online()
{
    char topic[128] = {0};
    char text[256] = {0};
    const char *ptr = NULL;
    cJSON *root = NULL;
    if (!is_mq_online())
    {
        ltrace("MQû�����磬�޷�������ά����Ϣ��\r\n");
        return;
    }
    sprintf(text, get_online_text(1));
    sprintf(topic, "%s/link", get_topic_prefix());
    mqtt_publish_all(topic, 1, 1, text, strlen(text));
}

static void mqtt_pre_init(MqTTObject *pHvObj)
{
    if (!pHvObj->hasInit)
    {
        pHvObj->hasInit = 1;
    }
}

static int is_mq_online()
{
    return _mqttApp.c.isconnected;
}

/**
 * @brief MQTT���Ĵ���ģ�飬��Ҫ��ά�����ӣ���������豸���ߣ���������������
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int mqtt_start()
{
    MqTTObject *pHvObj = (MqTTObject *)&_mqttApp;
    memset(pHvObj, 0, sizeof(_mqttApp));
    config_init(pHvObj);
    int first = 0;
    int online = 0;
    time_t ltSignTick;
    time_t ltHeartBeatTick;
    while (!_mqttApp.bQuit)
    {
        if (!is_mq_online())
        {
            MQTTDisconnect(&_mqttApp.c);
            NetworkDisconnect(&_mqttApp.n);
            ltrace("�豸�����ߣ��������ߣ�\r\n");
            mqtt_client_init(pHvObj);
            if (!pHvObj->c.isconnected)
            {
                sleep(2);
                continue;
            }
            online = 1;
            ltSignTick = time(NULL);
            ltHeartBeatTick = time(NULL);
        }
        if (online == 1)
        {
            online = 0;
            mqtt_send_online();
        }
        if (time(NULL) > ltSignTick)
        {
            ltSignTick = time(NULL) + 60; // 60���ϱ�һ��ϵͳ״̬
            dev_status();
        }
        MQLock();
        MQTTYield(&_mqttApp.c, 2000);
        // 500ms ����ʱ�����ò���̫�̣�������ܵ�����Ӧ��������̫���Ӷ�������Ϊ����ʱ���϶������쳣��������Ը���2s
        MQUnLock();
        usleep(10000);
    }
    MQTTDisconnect(&pHvObj->c);
    NetworkDisconnect(&pHvObj->n);
}

void mqtt_init()
{
    if (EN_FUNCTION(EN_MQTT_INTF))
    {
        task_init();
        pthread_create(&_mqttApp.thread, NULL, mqtt_start, NULL);
    }
}

void mqtt_deinit()
{
    if (EN_FUNCTION(EN_MQTT_INTF))
    {
        _mqttApp.bQuit = 1;
        pthread_cancel(_mqttApp.thread);
        pthread_join(_mqttApp.thread, NULL);
    }
}