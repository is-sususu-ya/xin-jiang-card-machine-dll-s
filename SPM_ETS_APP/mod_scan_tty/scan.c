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
#include <typedef.h>
#include <utils_ptrlist.h>
#include <utils_msq.h>
#include <utils_conf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utils_mtrace.h>
#include <utils_net.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define INVALID_SOCKET -1
    
MODULE_CALL_BACK module_callback;
static cIOModule m_cQRModule;

typedef struct{
	cIOModule *module;
    int fd;
    char dev[20];
    int Run;
    pthread_t thread;
}QR_OBJECT_S,*PHVOBJ;

static PtrList m_HandleList = PTRLIST_INITIALIZER;
static HANDLE h_qr = NULL;

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
 
static void qr_work_thread(void *arg)
{
	QR_OBJECT_S *obj = (QR_OBJECT_S*)arg;
    char qr_data[256] = {0};
	int i = 0; 
	int32_t len; 
	int m_tty = open( obj->dev, O_RDWR|O_NOCTTY|O_NDELAY|O_NONBLOCK); 
	if (m_tty < 0)
	{
		trace_log("open port %s failed:%s  \r\n", obj->dev, strerror( errno ) );
		return NULL;
	}
	trace_log("open tty port success!\r\n", obj->dev );
	tty_raw(m_tty, 9600, 8, 0);
	obj->Run = 1;
	while (obj->Run)
	{
		if (tty_ready(m_tty, 500) > 0)
		{
			memset(qr_data, 0, sizeof(qr_data));
			tty_gets(m_tty, qr_data, sizeof(qr_data), 50, 0, NULL);
			for (i = 0; i < sizeof(qr_data); i++)
				if (qr_data[i] == '\r' || qr_data[i] == '\n' || qr_data[i] == '\0')
					break;
			qr_data[i] = '\0';
			if( qr_data[0] == '\0' )
			{
				usleep( 500000 );
				continue;	
			} 
			if ( strlen(qr_data) > 5)
			{ 
			    if( module_callback )
					module_callback( obj, QR_EVT_READY, qr_data );
			}
		}else{
			usleep( 100000 );
		}
	}
}

static int QR_initialize()
{
	char log_name[100] = "./log";
	log_handle = mlog_init(log_name, "mod_qr");
	mlog_setlimitcnt(log_handle, 200, 2);
    return 0;
}

static int QR_terminate()
{
    return 0;
}

static int QR_is_type(int type)
{
    return m_cQRModule.type == type;
}

static intQR_enable(HANDLE h, BOOL bEnable)
{
	return -1;
}

static intQR_ioctl(HANDLE h, int cmd, int param, void *arg)
{
	return -1;
}

static void QR_register_callback(MODULE_CALL_BACK fxc)
{
	module_callback = fxc;
}

static HANDLE QR_open(const char *args)
{
    char device[32];
	char operand[32];
	char *ptr = (char *)args;
	char keyword[32];
	char value[32];
	QR_OBJECT_S *obj = malloc( sizeof( QR_OBJECT_S ));	
	memset( obj, 0, sizeof(QR_OBJECT_S));
	obj->module = &m_cQRModule;
	obj->Run = 1;
	trace_log("<mod_qr open args=<%s>><%s><%s><%d>\r\n", args, __FILE__, __func__, __LINE__);
	while (strgettokenpair(ptr, keyword, value, &ptr) > 0)
	{
		int keyidx = stridxinargs(keyword, 0, "dev", (char *)0);
		switch (keyidx)
		{
		case 0:
			strcpy(obj->dev, value);
			trace_log("open device=%s,<%s>,<%s>,<%d>\n", obj->dev, __FILE__, __func__, __LINE__);
			break;
		default:
			break;
		}
	}
    pthread_create( &obj->thread, NULL, qr_work_thread, obj );
	PtrList_append(&m_HandleList, obj);
	return obj;
}

static int QR_close(HANDLE h)
{
	QR_OBJECT_S *obj =(	QR_OBJECT_S *)h;
	obj->Run = 0;
	pthread_join( obj->thread, NULL );
}

static int QR_enable(HANDLE h, BOOL bEnable)
{

}

static int QR_read(HANDLE h, void *buf, int size)
{

}

static cIOModule m_cQRModule = {
	{sizeof(cBaseModule),
	 FMVER_YMD(2019,1, 26),
	 MOD_TYPE( MTYP_QR, QR_IS_TRANSPEED_TTY ),
	 "mod_LED",
	 "WR LED module",
	QR_initialize,
	QR_terminate,
	QR_is_type
    },
	QR_open,
	QR_close,
	QR_enable,
	QR_read,
	NULL,
	NULL,
	QR_register_callback,
	NULL
};

cIOModule *get_module(void)
{
	return &m_cQRModule;
}
