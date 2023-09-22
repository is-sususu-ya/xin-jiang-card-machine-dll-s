#ifndef __UTILS_MTRACE_INCLUDED__
#define __UTILS_MTRACE_INCLUDED__

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
#if defined __DEBUG || defined ENABLE_LOG

#define MLOG_INIT					mlog_init
#define MTRACE_LOG				mtrace_log
#define MTRACE_VLOG				mtrace_vlog
#define MLOG_CLOSE				mlog_close
#define MLOG_SETPATH			mlog_setpath
#define MLOG_ENABLE				mlog_enable
#define MLOG_KEEPOPEN			mlog_keepopen
#define MLOG_SETLIMITCNT	mlog_setlimitcnt

#else		// if not defined __DEBUG

#if defined _WIN32 || defined _WIN64
#define MLOG_INIT					(1 ? NULL : mlog_init)
#define MTRACE_LOG				(1 ? (void)0 : mtrace_log)
#define MTRACE_VLOG				(1 ? (void)0 : mtrace_vlog)
#define MLOG_CLOSE				(1 ? (void)0 : mlog_close)
#define MLOG_SETPATH			(1 ? (void)0 : mlog_setpath)
#define MLOG_ENABLE				(1 ? (void)0 : mlog_enable)
#define MLOG_KEEPOPEN		(1 ? (void)0 : mlog_keepopen)
#define MLOG_SETLIMITCNT		(1 ? (void)0 : mlog_setlimitcnt)
#else		// linux
#define MLOG_INIT(dir,name)
#define MTRACE_LOG(h, fmt...)
#define MTRACE_VLOG(h, fmt, argp)
#define MLOG_CLOSE(h)
#define MLOG_ENABLE(h,b)
#define MLOG_KEEPOPEN(h,b)
#define MLOG_SETLIMITCNT(h,cnt)
#define MLOG_SETPATH(h,p)
//#define 
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif
#include <stdarg.h>

// ��ʼ��һ��log. 'LogDir'����־·��(��NULLʱʹ��Ĭ��·��), 'BaseName'���ļ�base name (���ø���׺��
// ������Զ�����'.log'�ĺ�׺��
// ����ֵ��һ����־��¼��������
extern void *mlog_init( const char *LogDir, const char *BaseName );
// д��־��'h'����־��¼����ľ������¼���ĸ���־���棩�����������printfһ����
extern int mtrace_log( void *h, const char *fmt, ...);
// д��־���ɱ䳤�Ȳ����ȴ���һ���������Ǹ������ٵ���mtrace_vlog
extern int mtrace_vlog( void *h, const char *fmt, va_list argp);
// �ر���־�ļ���'h'��Ҫ�رյ���־��¼��������
extern void mlog_close( void *h );
// ����log�ļ���·����Ĭ����/var/log (for linux) or /rwlog (for Windows). 
// ���hΪNULL�����Ƕ������Ժ󴴽�����־��ʹ������µ�Ĭ��·�������mlog_initû��ָ��·���Ļ���
extern void mlog_setpath( void *h, const char *path);
// ��ʱ����/��ֹlog��Log����ֹ��ʱ�򣬵��� mtrace_log�������á�Ĭ����enable
extern void mlog_enable( void  *h, int bEnable );
// �Ƿ񱣳���־�ļ��򿪵�״̬������Ƶ���򿪣��ر��ļ���Ĭ����KEEP-OPEN
extern void mlog_keepopen( void  *h, int bKeep );
// ����ÿ��log�ļ���СΪ 'limit' KB, һ����'cnt'���������ݾ��ļ�
extern void mlog_setlimitcnt( void *h, int limit, int roll_cnt );

#ifdef __cplusplus
}
#endif

#endif

