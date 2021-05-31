#include <stdarg.h>
#ifndef __MTRACE_INCLUDED__
#define __MTRACE_INCLUDED__

/* NOTE: 
	程序在 inlcude "mtrace.h" 之前就要定义AP是否要使能日志功能。最好定义在Makefile里面
	一般在Release的版本会关闭log。重要的log（流程，事件等）不是debug程序本身需要的，
	应该另外单独记录，别和DEBUG用途的输出日志混在一起。

	原来使用trace,c/Trace.h的程序，要改为mtrace.c/mtrace.h，简单的方式是：
	1. include "Trace.h" 改为 include "mtrace.h"
	2. 在程序模块里面定义一个file scope的变量
		static void *hModuleLog=NULL;

	3. 将原来TRACE_xxxx 的函数定义为 MTRACE_xxxx
		#define LOG_INIT(dir,base)		((hModuleLog=MLOG_INIT(dir,base)) != NULL)
		#define TRACE_LOG(fmt,...)		MTRACE_LOG(hModuleLog,fmt)
		#define LOG_CLOSE()				MLOG_CLOSE(hModuleLog)

    2020/05/16
	   - MLOG_INIT(log_root, base_name)
	   - MLOG_SETPATH(h, log_root)
	   - MLOG_SETBASENAME(h, base_name)
	   以上几个参数里, log_root 和 base_name 里面可以使用 $DATE 宏. base_name里面可以使用 $DATE和$TIME
	   MLOG_INIT("./Log/$DATE", "RSU-$TIME")
	   产生的日志 每天都会创建一个子文件夹, 因为log_root里面有$DATE, 每天过午夜第一次打印日志时,程序发现日期已经改变. 会根据
	   ./Log/$DATE 的宏展开创建像 "./Log/20200517" 的文件夹(2020年5月17日日志文件夹)并在此文件夹里面创建新的日志文件
	   每次生成的日志文件名根据 RSU-$TIME的宏定义, 将生成例如 RSU-000013.log (凌晨0点0分13秒)
	   当该文件大小超过限制的日志文件大小时, 会关闭重新产生一个新的. 不像之前版本会将当前日志滚动备份( 加后缀的数字), 而是根据要重新生成时的
	   系统时间生成,例如在早上8点23分52秒发现当前日志文件大小已经超过限制,就会关闭当前文件,重新创建一个日志 RSU-082352.log 开始记录新日志.
	   当然, 使用宏$TIME在base_name里面后,滚动备份的数量不再有效, 但文件大小限制还是在的.
	   如果设置是 MLOG_INIT("./Log", "RSU-$DATE") 时, 那所有日志都在一个文件夹里面(和之前一样). 因为没有$TIME,所以会有和以前一样的滚动备份
	   (带序号的后缀的日志). 这样的作法好处是可以快速找到某天甚至某个时间段日志所在的文件. 只要时滚动备份,最大备份文件数(同一个基础文件名)还是
	   有作用的. 基础文件名是带日期,但滚动备份数上限设置为10, 当天就只能保存最后10个备份文件.
	   配合$DATE宏, 新增一个控制历史备份文件数量的方式.
	   MLOG_SETLIFEDAYS(h, nday)
	   这是指定日志保存'nday'天. 至于每天能保存的文件数量限制,如果base_name里面没有$TIME, 那每天的文件数是受
	   MLOG_SETLIMITCNT(h, kb_limit, cnt_limit)
	   这个函数设置的cnt_limit限制的, 这个默认值是30. 上限可以到999
	   kb_limit是设置每个日志文件的大小(单位KB). 默认是1024K (1MB), 上限是32768K (32MB)
	   超过这个文件大小限制就会关闭文件(并且进行滚动备份, 如果base_name没有用到$TIME).
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
// 写日志，可变长度参数先传入一个函数，那个函数再调用mtrace_vlog
extern int mtrace_vlog( void *h, LPCTSTR fmt, va_list argp);
extern void mlog_close( void *h );
extern void mlog_setpath( void *h, LPCTSTR path);
extern void mlog_setbasename(void *h, LPCTSTR base);
// 临时允许/禁止log。Log被禁止的时候，调用 mtrace_log不起作用。默认是enable
extern void mlog_enable( void  *h, BOOL bEnable );
// 是否保持日志文件打开的状态，避免频繁打开，关闭文件。默认是KEEP-OPEN
extern void mlog_keepopen( void  *h, BOOL bKeep );
// 设置每个log文件大小为 'limit' KB, 一共有'cnt'个滚动备份旧文件
extern void mlog_setlimitcnt( void *h, int liimit, int cnt );
// 设置保存几天的文件(当mlog_setbasename的base参数或是mlog_init的BaseName参数带有$DATE宏时才有效果, 默认10天
extern void mlog_setlifedays( void *h, int nday);
// 查询当前日志是否使能
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

