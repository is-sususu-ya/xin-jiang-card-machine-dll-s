#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <dirent.h>
#define TRUE  1
#define FALSE 0

typedef struct tagKVStruct
{
    const char *key;
    const char *value;
    struct tagKVStruct *next;
} KVS;
KVS *root = NULL;
 
static void add_key_value(const char *key, const char *value)
{
    if (key == NULL || value == NULL || key == '\0' || value == '\0')
        return;
    KVS *next, *tail;
    next = root;
    while (next != NULL)
    {
        if (strcmp(next->key, key) == 0)
        {
            free((void *)next->value);
            next->value = strdup(value);
            return;
        }
        next = next->next;
    }
    KVS *node = malloc(sizeof(KVS));
    memset(node, 0, sizeof(KVS));
    node->key = strdup(key);
    node->value = strdup(value);
    node->next = NULL;
    next = root;
    tail = root;
    while (next != NULL)
    {
        tail = next;
        next = next->next;
    }
    if (tail)
        tail->next = node;
    else
        root = node;
}

static char *trim(char *str)
{
    int len;
    char *ptr, *q;
    for (ptr = str; *ptr == ' ' || *ptr == '\t'; ptr++)
        ;
    for (q = str + strlen(str) - 1; *q == ' ' || *q == '\t' || *q == '\r' || *q == '\n'; q--)
        ;
    len = q + 1 - ptr;
    memmove(str, ptr, len);
    str[len] = '\0';
    return str;
}

static void str_trimed(char *str)
{
    char buffer[256] = {0};
    char *ptr = NULL;
    if (strlen(str) > sizeof(buffer))
        return;
    strcpy(buffer, str);
    ptr = trim(buffer);
    strcpy(str, buffer);
}

static void parse_line(const char *arg)
{
    // ini格式的字符是以key=value格式的，我们这里不支持多entry的格式，只支持单独的key和value格式，如�?
    // 是注释，那么必然�?#开头的
    const char *ptr = arg;
    char line[256];
    char key[256] = {0};
    char value[256] = {0};
    if (arg == NULL || *arg == '\0')
        return;
    if (strlen(arg) > sizeof(line))
        return;

    if (strstr(arg, "=") == NULL)
        return;
    for (; *ptr == '\r' || *ptr == '\n' || *ptr == ' ' || *ptr == '\t'; ptr++)
        ;
    if (*ptr == '#' || (*ptr == '/' && *(ptr + 1) == '/'))
    {
        printf("这是一个注解内容，忽略..\r\n");
        return;
    }
    if (*ptr == '\0')
        return;
    // 注释的部分需要全部去�?
    strcpy(line, arg);
    if (NULL != (ptr = strstr(line, "#")))
    {
        line[ptr - line] = '\0';
    }
    // 差不多这个时候只剩下key 和value�?
    memset(key, 0, sizeof(key));
    memset(value, 0, sizeof(value));
    ptr = strstr(line, "=");
    if (ptr)
    {
        strncpy(key, line, ptr - line);
        strncpy(value, ptr + 1, strlen(ptr + 1));
    }
    str_trimed(key);
    str_trimed(value);
    add_key_value(key, value);
}

int ConfigLoad(const char *path)
{
    FILE *fp;
    char *line = NULL;
    int len = 0;
    int read;

#ifdef __WIN32
    if (access(path, 0) != 0)
#else
    if (access(path, F_OK) != 0)
#endif
    {
        printf("config file not exist..\r\n");
        return FALSE;
    }
    fp = fopen(path, "r");
    if (fp == NULL)
        return FALSE;

    while ((read = getline(&line, (size_t *)&len, fp)) != -1)
        parse_line(line);

    if (line)
        free(line);
    return TRUE;
}

void ConfigClean()
{
    KVS *next = root;
    KVS *tmp;
    while (next != NULL)
    {
        free((void *)next->key);
        free((void *)next->value);
        tmp = next->next;
        next = tmp;
    }
    root = NULL;
}

const char *ConfigGetKey(const char *key)
{
    KVS *next = root;
    while (next != NULL)
    {
        if (strcmp(next->key, key) == 0)
            return next->value;
        next = next->next;
    }
    return NULL;
}

int ConfigSaveAs(const char *path)
{
    FILE *fp = fopen(path, "w");
    if (fp == NULL)
        return -1;
    KVS *next = root;
    char line[512];
    while (next != NULL)
    {
        sprintf(line, "%s=%s \r\n", next->key, next->value);
        fwrite(line, 1, strlen(line), fp);
        next = next->next;
    }
    fclose(fp);
    return 0;
}

void AddKeyValue(const char *key, const char *value)
{
    add_key_value(key, value);
}