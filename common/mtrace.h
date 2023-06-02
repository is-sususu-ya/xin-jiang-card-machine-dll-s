#include <stdarg.h>
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
#if defined ENABLE_LOG

#define MLOG_INIT					mlog_init
#define MTRACE_LOG				mtrace_log
#define MTRACE_VLOG				mtrace_vlog
#define MLOG_CLOSE				mlog_close
#define MLOG_SETPATH			mlog_setpath
#define MLOG_SETBASENAME(h,base)	mlog_setbasename(h,base)
#define MLOG_ENABLE				mlog_enable
#define MLOG_KEEPOPEN		mlog_keepopen
#define MLOG_SETLIMITCNT		mlog_setlimitcnt

#else		// if not defined __DEBUG and nit defined ENABLE_LOG

#if defined _WIN32 || defined _WIN64
#define MTRACE_LOG(h,fmt,...)		(0)
#else		// linux
#define MTRACE_LOG(h, fmt...)		(0)
#endif
#define MLOG_INIT(dir,bname)		(NULL)
#define MTRACE_VLOG	(h,fmt,va)	(0)
#define MLOG_CLOSE(h)
#define MLOG_SETPATH(h,path)	
#define MLOG_SETBASENAME(h,base)
#define MLOG_ENABLE(h,bEn)
#define MLOG_KEEPOPEN(h,bKeep)
#define MLOG_SETLIMITCNT(h,limit,cnt)

#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef linux
typedef const char *  LPCTSTR;
#endif

extern void *mlog_init( LPCTSTR LogDir,LPCTSTR BaseName );
extern void mtrace_log( void *h, LPCTSTR fmt, ...);
// д��־���ɱ䳤�Ȳ����ȴ���һ���������Ǹ������ٵ���mtrace_vlog
extern int mtrace_vlog( void *h, LPCTSTR fmt, va_list argp);
extern void mlog_close( void *h );
extern void mlog_setpath( void *h, LPCTSTR path);
extern void mlog_setbasename(void *h, LPCTSTR base);
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

