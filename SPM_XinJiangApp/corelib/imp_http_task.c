/* 完成http请求任务 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <pthread.h>
#include "utils_web.h"
#include "utils_str.h"
#include "cJSON.h"
#include "md5.h"
#include "utils_net.h"
#include "dbg_printf.h"
#include "utils_ptrlist.h"
#include "typedef.h"
#include "imp_http_task.h"

typedef struct tagHttpRequestContentxS
{
    int is_get;
    char url[256];
    int id;
    int index;
    int type; // 设置
    const char *request;
    const char *response;
} HttpRequestContentxS;

typedef unsigned long long UInt64;

#define MAX_URL_SIZE 256
static char url[10][MAX_URL_SIZE] = {0};
#define FILE_NAME "curlcfg.dat"
static UInt64 ltLastUPdate = 0;
static int bQuit = 0;
static pthread_t http_thread;
static pthread_mutex_t http_mutex;
static PtrList http_task_list = PTRLIST_INITIALIZER;
static void *http_work_thread(void *arg);
static pthread_mutex_t thread_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t thread_cond = PTHREAD_COND_INITIALIZER;
static fxc_http_response http_response_fxc = NULL;

#define Lock() pthread_mutex_lock(&http_mutex)
#define UnLock() pthread_mutex_unlock(&http_mutex)

extern void ltrace(const char *fmt, ...);

static void task_wait(int sec)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += sec;
    pthread_mutex_lock(&thread_lock);
    pthread_cond_timedwait(&thread_cond, &thread_lock, &ts);
    pthread_mutex_unlock(&thread_lock);
}

static void task_wakeup()
{
    pthread_mutex_lock(&thread_lock);
    pthread_cond_broadcast(&thread_cond);
    pthread_mutex_unlock(&thread_lock);
}

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

static void load_data()
{
    int fd;
    if (access(FILE_NAME, F_OK) != 0)
        write_file(FILE_NAME, url, sizeof(url));
    fd = open(FILE_NAME, O_RDONLY);
    if (fd > 0)
    {
        read(fd, url, sizeof(url));
        close(fd);
        return;
    }
}

static const char *GbkString(const char *utf8String)
{
    static char buffer[512] = {0};
    memset(buffer, 0, sizeof(buffer));
    UTF8ToGBK(utf8String, strlen(utf8String), buffer);
}

void register_http_url(int id, const char *durl)
{
    if (id < 0 || id >= 10 || !durl || strncmp(durl, "http", 4) != 0)
    {
        ltrace("参数错误【%d】【%s】\r\n", id, durl);
        return;
    }
    strncpy(url[id], durl, MAX_URL_SIZE);
    ltLastUPdate = GetTickCount() + 500;
    ltrace("注册url【%d】【%s】\r\n", id, durl);
}

void http_task_init()
{
    load_data();
    pthread_create(&http_thread, NULL, http_work_thread, NULL);
}

void http_task_exit()
{
    bQuit = 1;
    task_wakeup();
    pthread_cancel(http_thread);
    pthread_join(http_thread, NULL);
}

void add_http_post_task(int index, int id, int type, char *request)
{
    HttpRequestContentxS *cont = malloc(sizeof(HttpRequestContentxS));
    memset(cont, 0, sizeof(HttpRequestContentxS));
    cont->is_get = 0;
    cont->id = id;
    cont->index = index;
    cont->type = type;
    cont->request = strdup(request);
    Lock();
    PtrList_append(&http_task_list, cont);
    UnLock();
    task_wakeup();
}

void add_http_url_post_task(int index, const char *url, int type, char *request)
{
    HttpRequestContentxS *cont = malloc(sizeof(HttpRequestContentxS));
    memset(cont, 0, sizeof(HttpRequestContentxS));
    cont->is_get = 0;
    strcpy(cont->url, url);
    cont->index = index;
    cont->type = type;
    cont->request = strdup(request);
    Lock();
    PtrList_append(&http_task_list, cont);
    UnLock();
    task_wakeup();
}

void add_http_get_task(int id, char *url)
{
    HttpRequestContentxS *cont = malloc(sizeof(HttpRequestContentxS));
    memset(cont, 0, sizeof(HttpRequestContentxS));
    cont->is_get = 1;
    strcpy(cont->url, url);
    Lock();
    PtrList_append(&http_task_list, cont);
    UnLock();
    task_wakeup();
}

void http_task_setcallback(fxc_http_response cbfx)
{
    http_response_fxc = cbfx;
}

static void *http_work_thread(void *arg)
{
    char response_buf[10924];
    int ret;
    while (!bQuit)
    {
        if (ltLastUPdate != 0 && GetTickCount() > ltLastUPdate)
        {
            ltrace("更新配置文件！");
            ltLastUPdate = 0;
            write_file(FILE_NAME, url, sizeof(url));
        }
        if (http_task_list.count == 0)
        {
            task_wait(5);
            continue;
        }
        Lock();
        HttpRequestContentxS *c = PtrList_remove_head(&http_task_list);
        UnLock();
        memset(response_buf, 0, sizeof(response_buf));
        if (c->is_get)
        {
            ltrace("发起GET请求: %s \r\n", c->url);
            ret = http_get(c->url, response_buf, sizeof(response_buf));
            if (NET_ERROR_NONE == ret)
                c->response = strdup(response_buf);
            else
                c->response = strdup(ErrorString(ret)); 
            ltrace("REPONSE：\r\n%s\r\n", GbkString(response_buf));
            if (http_response_fxc)
                http_response_fxc(c->id, abs(ret), c->response, strlen(c->response));
            free(c->response);
            free(c);
        }
        else
        {
            ltrace("参数：%s \r\n", c->request);
            if (c->url[0] == 0)
            {
                ltrace("发起POST请求: %s \r\n", url[c->index]);
                ret = http_post(url[c->index], c->type, c->request, response_buf, sizeof(response_buf));
                ltrace("POST：\r\n%s\r\n", GbkString(c->request));
                ltrace("REPONSE：\r\n%s\r\n", GbkString(response_buf));
            }
            else
            {
                ltrace("发起POST请求: %s \r\n", c->url);
                ret = http_post(c->url, c->type, c->request, response_buf, sizeof(response_buf));
                ltrace("POST：\r\n%s\r\n", GbkString(c->request));
                ltrace("REPONSE：\r\n%s\r\n", GbkString(response_buf));
            }
            if (NET_ERROR_NONE == ret)
                c->response = strdup(response_buf);
            else
                c->response = strdup(ErrorString(ret));
            ltrace("响应码【%d】响应结果：%s \r\n", ret, GbkString(c->response));
            if (http_response_fxc)
                http_response_fxc(c->id, abs(ret), c->response, strlen(c->response));
            free(c->request);
            free(c->response);
            free(c);
        }
    }
    return NULL;
}