#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/mman.h>
#include <pthread.h>
#include "utils_daemon.h"

typedef int bool;
#define false 0
#define true 1

static int service_run();
static void service_stop();
extern void sim_loop();
extern void sim_exit();

static void *log_sys = NULL;

void sys_log(const char *fmt, ...)
{
	va_list va;
	int rc;
	char str[512] = {0};
	if (log_sys == NULL)
		return;
	va_start(va, fmt);
	rc = mtrace_vlog(log_sys, fmt, va);
	va_end(va);
}

static void write_default(const char *path, const char *buffer, int size)
{
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC);
	if (fd < 0)
		return;
	write(fd, buffer, size);
	close(fd);
}

extern int core_start( );

static int service_run()
{
	core_start();
}

static void service_stop()
{
	core_stop();
}

static void print_help()
{
	printf("Build on %s - %s \r\n"
			"-n enable http update ui\r\n" 
			"-g [address] remote addr, if using udp is ip, if use http it is url\r\n",
			__DATE__, __TIME__ );
}

int reset_interval = 0;
const char *dst_ip = "127.0.0.1";
int g_en_http_ui = 0;
int g_enable_lcd_page_remap = 0;

int main(int argc, char *const argv[])
{
	int opt;
	char *ptr;
	bool b_stop = false;
	bool b_daemon = false;
	bool b_killchild = false;
	char name[128] = {0};
	char input[128] = {0};
    char log_name[128] = {0};
	strcpy( input, argv[0] );
	if( NULL == ( ptr = strrchr( argv[0], '/' ) ))
		sprintf( name, "/var/tmp/.pid_%s.dat", argv[0] );
	else
		sprintf( name, "/var/tmp/.pid_%s.dat", ptr +1 );
	print_help();
	while ((opt = getopt(argc, argv, ":dxnkrhmi:g:")) != -1)
	{
		switch (opt)
		{
		case ':':
			printf("missing required parameter for command option %c.\n", opt);
			return -1;
		case '?':
			printf("%s -g ip gui port\n", argv[0] );
			break;
		case 'd':
		case 'r':
			b_daemon = true;
			break;
		case 'k':
			b_killchild = true;
			break;
		case 'g':
			dst_ip = strdup( optarg );
			printf("remote - %s \r\n", dst_ip );
			break;
		case 'n':
			g_en_http_ui = 1;
			break;
		case 'm':
			g_enable_lcd_page_remap = 1;
			break;
		case 'i':
			reset_interval = atoi( optarg );
			reset_interval = reset_interval < 0 ? 60*60 : reset_interval;
			printf("reset after:%d s\r\n", reset_interval );
		case 'x':
			b_stop = true;
			break;
		default:
			break;
		}
	}
    strcpy(log_name, "/home/pay/log" );
	log_sys = mlog_init(log_name, "sys_run");
	mlog_setlimitcnt(log_sys, 200, 2 );

	sys_log("%sLCD屏页面重映射\r\n", g_enable_lcd_page_remap ? "开启" : "关闭" );
	
	daemon_config( argv[0], name, sys_log );
	daemon_service(service_run, service_stop);
	if (b_stop)
	{
		printf("Stop daemon and it's child process...\n");
		daemon_stop();
		return 0;
	}
	else if (b_killchild)
	{
		printf("Stop child process (the working process)...\n");
		daemon_killchild();
		return 0;
	}
	else if (daemon_probe() == 0)
	{
		printf("another copy is running, please stop it first by \"%s -x\"\n", argv[0]);
		return 0;
	}
	daemon_main(b_daemon);
	return 0;
}
