#ifndef __MTRACE_INCLUDED__
#define __MTRACE_INCLUDED__

/* NOTE: 
	������ inlcude "mtrace.h" ֮ǰ��Ҫ����AP�Ƿ�Ҫʹ����־���ܡ���ö�����Makefile����
	һ����Release�İ汾��ر�log����Ҫ��log�����̣��¼��ȣ�����debug��������Ҫ�ģ�
	Ӧ�����ⵥ����¼�����DEBUG��;�������־����һ��

	ԭ��ʹ��trace,c/Trace.h�ĳ���Ҫ��Ϊmtrace.c/mtrace.h���򵥵ķ�ʽ�ǣ�
	1. include "Trace.h" ��Ϊ include "mtrace.h"
	2. �ڳ���ģ�����涨��һ��file scope�ı���
		static void *hModuleLog=NULL;

	3. ��ԭ��TRACE_xxxx �ĺ�������Ϊ MTRACE_xxxx
		#define LOG_INIT(dir,base)		((hModuleLog=MLOG_INIT(dir,base)) != NULL)
		#define TRACE_LOG(fmt,...)		MTRACE_LOG(hModuleLog,fmt)
		#define LOG_CLOSE()				MLOG_CLOSE(hModuleLog)
*/
#define ENABLE_LOG

#if defined __DEBUG || defined ENABLE_LOG

#define MLOG_INIT				mlog_init
#define MTRACE_LOG				mtrace_log
#define MLOG_CLOSE				mlog_close
#define MLOG_SETPATH			mlog_setpath
#define MLOG_ENABLE				mlog_enable
#define MLOG_KEEPOPEN			mlog_keepopen
#define MLOG_SETLIMITCNT		mlog_setlimitcnt

#else		// if not defined __DEBUG

#if defined _WIN32 || defined _WIN64
#define MLOG_INIT				(1 ? (void*)0 : mlog_init)
#define MTRACE_LOG				(1 ? (void*)0 : mtrace_log)
#define MLOG_CLOSE				(1 ? (void*)0 : mlog_close)
#define MLOG_SETPATH			(1 ? (void*)0 : mlog_setpath)
#define MLOG_ENABLE				(1 ? (void*)0 : mlog_enable)
#define MLOG_KEEPOPEN			(1 ? (void*)0 : mlog_keepopen)
#define MLOG_SETLIMITCNT		(1 ? (void*)0 : mlog_setlimitcnt)
#else		// linux
#define MLOG_INIT(dir,name)
#define MTRACE_LOG(h, fmt...)
#define MLOG_CLOSE(h)
#define MLOG_ENABLE(h,b)
#define MLOG_KEEPOPEN(h,b)
#define MLOG_SETLIMITCNT(h,cnt)
//#define 
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

extern void *mlog_init( const char *LogDir, const char *BaseName );
extern void mtrace_log( void *h, const char *fmt, ...);
extern void mlog_close( void *h );
extern void mlog_setpath( void *h, const char *path);
// ��ʱ����/��ֹlog��Log����ֹ��ʱ�򣬵��� mtrace_log�������á�Ĭ����enable
extern void mlog_enable( void  *h, BOOL bEnable );
// �Ƿ񱣳���־�ļ��򿪵�״̬������Ƶ���򿪣��ر��ļ���Ĭ����KEEP-OPEN
extern void mlog_keepopen( void  *h, BOOL bKeep );
// ����ÿ��log�ļ���СΪ 'limit' KB, һ����'cnt'���������ݾ��ļ�
extern void mlog_setlimitcnt( void *h, int liimit, int cnt );

#ifdef __cplusplus
}
#endif

#endif

