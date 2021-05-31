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

    2020/05/16
	   - MLOG_INIT(log_root, base_name)
	   - MLOG_SETPATH(h, log_root)
	   - MLOG_SETBASENAME(h, base_name)
	   ���ϼ���������, log_root �� base_name �������ʹ�� $DATE ��. base_name�������ʹ�� $DATE��$TIME
	   MLOG_INIT("./Log/$DATE", "RSU-$TIME")
	   ��������־ ÿ�춼�ᴴ��һ�����ļ���, ��Ϊlog_root������$DATE, ÿ�����ҹ��һ�δ�ӡ��־ʱ,�����������Ѿ��ı�. �����
	   ./Log/$DATE �ĺ�չ�������� "./Log/20200517" ���ļ���(2020��5��17����־�ļ���)���ڴ��ļ������洴���µ���־�ļ�
	   ÿ�����ɵ���־�ļ������� RSU-$TIME�ĺ궨��, ���������� RSU-000013.log (�賿0��0��13��)
	   �����ļ���С�������Ƶ���־�ļ���Сʱ, ��ر����²���һ���µ�. ����֮ǰ�汾�Ὣ��ǰ��־��������( �Ӻ�׺������), ���Ǹ���Ҫ��������ʱ��
	   ϵͳʱ������,����������8��23��52�뷢�ֵ�ǰ��־�ļ���С�Ѿ���������,�ͻ�رյ�ǰ�ļ�,���´���һ����־ RSU-082352.log ��ʼ��¼����־.
	   ��Ȼ, ʹ�ú�$TIME��base_name�����,�������ݵ�����������Ч, ���ļ���С���ƻ����ڵ�.
	   ��������� MLOG_INIT("./Log", "RSU-$DATE") ʱ, ��������־����һ���ļ�������(��֮ǰһ��). ��Ϊû��$TIME,���Ի��к���ǰһ���Ĺ�������
	   (����ŵĺ�׺����־). �����������ô��ǿ��Կ����ҵ�ĳ������ĳ��ʱ�����־���ڵ��ļ�. ֻҪʱ��������,��󱸷��ļ���(ͬһ�������ļ���)����
	   �����õ�. �����ļ����Ǵ�����,��������������������Ϊ10, �����ֻ�ܱ������10�������ļ�.
	   ���$DATE��, ����һ��������ʷ�����ļ������ķ�ʽ.
	   MLOG_SETLIFEDAYS(h, nday)
	   ����ָ����־����'nday'��. ����ÿ���ܱ�����ļ���������,���base_name����û��$TIME, ��ÿ����ļ�������
	   MLOG_SETLIMITCNT(h, kb_limit, cnt_limit)
	   ����������õ�cnt_limit���Ƶ�, ���Ĭ��ֵ��30. ���޿��Ե�999
	   kb_limit������ÿ����־�ļ��Ĵ�С(��λKB). Ĭ����1024K (1MB), ������32768K (32MB)
	   ��������ļ���С���ƾͻ�ر��ļ�(���ҽ��й�������, ���base_nameû���õ�$TIME).
*/
#ifdef ENABLE_LOG

#define MLOG_INIT						mlog_init
#define MTRACE_LOG					mtrace_log
#define MTRACE_VLOG				mtrace_vlog
#define MLOG_CLOSE					mlog_close
#define MLOG_SETPATH				mlog_setpath
#define MLOG_SETBASENAME	mlog_setbasename
#define MLOG_ENABLE					mlog_enable
#define MLOG_KEEPOPEN			mlog_keepopen
#define MLOG_SETLIMITCNT		mlog_setlimitcnt
#define MLOG_SETLIFEDAYS		mlog_setlifedays
#define MLOG_IS_ENABLED		mlog_is_enabled


#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#include <Windows.h>
#else
#include "wintype.h"
#endif

extern void *mlog_init( LPCTSTR LogDir, LPCTSTR BaseName );
extern int mtrace_log( void *h, LPCTSTR fmt, ...);
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
// ���ñ��漸����ļ�(��mlog_setbasename��base��������mlog_init��BaseName��������$DATE��ʱ����Ч��, Ĭ��10��
extern void mlog_setlifedays( void *h, int nday);
// ��ѯ��ǰ��־�Ƿ�ʹ��
extern int mlog_is_enabled( void *h);

#ifdef __cplusplus
}
#endif

#else		

#ifdef WIN32
#define MTRACE_LOG(h,fmt,...)		(0)
#else		// linux
#define MTRACE_LOG(h, fmt...)		(0)
#endif
#define MLOG_INIT(dir,bname)		(NULL)
#define MTRACE_VLOG(h,fmt,va)		(0)
#define MLOG_CLOSE(h)
#define MLOG_SETPATH(h,path)	
#define MLOG_SETBASENAME(h,base)
#define MLOG_ENABLE(h,bEn)
#define MLOG_KEEPOPEN(h,bKeep)
#define MLOG_SETLIMITCNT(h,limit,cnt)
#define MLOG_SETLIFEDAYS(h,nday)
#define MLOG_IS_ENABLED(h)		(0)

#endif

#endif

