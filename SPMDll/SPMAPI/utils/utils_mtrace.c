/*
 * mtrace.c/utils_mtrace.c
 *
 * for multiple log files application using this instead of trace.c. It provides many new features
 *
 *  2020/05/14
 *   - add base name macro $DATE, $TIME
 *   - when base name contains $DATE, log across midnight will be close and open again with new date replace $DATE macro
 *   - when base name contains $TIME, log file over size simply create new one according to current time. (usually, $TIME should go after $DATE)
 *   - when base name contains $DATE. number of total backup files is not limit by 'm_rollcnt'. but 'm_rollcnt' x 'm_storedays'
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// [ W I N D O W S ]
#if defined _WIN32 || defined _WIN64
//#define _CRT_SECURE_NO_WARNINGS 		// disable the annoying warning 4996
#include <windows.h>
#include <io.h>
#include <direct.h>
#include "tchar.h"
#define DEF_LOG_DIR			_T("/rwlog")
typedef HANDLE HMUTEX;
#define THREADCALL	DWORD WINAPI
#define THREADRET		0
#define NAME_MAX	256
// 遍历文件需要的函数, 模拟linux
struct   dirent  
{  
    long   d_ino;  
    off_t   d_off;    
    unsigned  short   d_reclen;  
    char   d_name[NAME_MAX];  
};  

typedef   struct  
{  
    long   handle;                              
    short   offset;                            
    short   finished;                        
    struct   _finddata_t   fileinfo;  
    char   *dir;                                      
    struct   dirent   dent;                    
}   DIR;  

static DIR*     opendir(const   char   *);  
static struct   dirent   *   readdir(DIR   *);  
static int   closedir(DIR   *);

#else
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//   [ L I N U X ]
#include <pthread.h>
#include <error.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/time.h> 
#include "wintype.h"
#define DEF_LOG_DIR			"/var/log"
typedef pthread_mutex_t		HMUTEX;
#define MAX_PATH			256
#define THREADCALL	void *
#define THREADRET		NULL
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define ENABLE_MUTEX
#define DEF_LOG_BASEFILE	_T("RunWell-$DATE")
#define MAX_LOGSIZE			32768			// 32 MB
#define DEF_LOGSIZE			1024			// 1MB (单位是KB)
#define MAX_ROLLCOUNT		999			// <BaseName>-01.log ~ <BaseName>-30.log
#define DEF_ROLLCOUNT		30
#define MAGIC_NO				0xcb34ae78
#define ISVALID_HANDLE(h) ( (h)!=NULL && ((HTRACE) h)->nMagicNo == MAGIC_NO )
#define MAX_BASE_NAME	64
#define DEF_STOREDAYS	10		// 默认保存10天日志, 如果BaseName有$DATE宏才有效果

static char __def_log_dir[MAX_PATH] = DEF_LOG_DIR;
static THREADCALL purge_files_fxc(PVOID lpParameter);

typedef struct {
	int		nMagicNo;
	TCHAR m_strRawLogDir[MAX_PATH];
	TCHAR m_strLogDir[MAX_PATH];
	TCHAR m_strLogFile[MAX_PATH];
	TCHAR m_strBaseName[MAX_BASE_NAME];
	TCHAR m_strRawBaseName[MAX_BASE_NAME];		// 原始的BaseName, 有可能带MACRO 
	size_t  sz_limit;
	int		m_rollcnt;
	int		m_storedays;
	FILE	*m_fp;
	BOOL m_bEnable;
	BOOL m_bKeepOpen;
	BOOL m_bHasDateDir;				// 日志文件夹有日期宏
	BOOL m_bHasDateFormat;		// 日志文件名有日期宏
	BOOL m_bHasTimeFormat;		// 日志文件名有时间宏
	int		   m_nCurDate;					// 日志文件名有带日期时(m_bHasDateFormat=TRUE), 这里时当前使用日志文件的日期(DATE部分). 这样日志跨天可以roll_over
	HMUTEX	m_hMutex;
	int				m_nLineCount;			// 当前日志文件行数 (尚未使用)
} TraceStruct, *HTRACE;

#ifdef ENABLE_MUTEX
#ifdef WIN32
#define Mutex_Lock(hMutex)			WaitForSingleObject( hMutex, INFINITE )
#define Mutex_Unlock(hMutex)		ReleaseMutex( hMutex )
#else
#define Mutex_Lock(hMutex)			pthread_mutex_lock( &hMutex )
#define Mutex_Unlock(hMutex)		pthread_mutex_unlock( &hMutex )
#endif

#else
#define Mutex_Lock(hMutex)
#define Mutex_Unlock(hMutex)
#endif

// copy hTrace->m_strRawBaseName to hTrace->m_strBaseName and replace $DATE and/or $TIME macro
// as current date in YYYYMMDD and current time hhmmss respectively.
static void replace_basename_macro(HTRACE hTrace)
{
	TCHAR *ptr1, *ptr2, *dst;
#ifdef linux
	struct timeval tv;
	struct tm *ptm;

	gettimeofday( &tv, NULL );
	ptm = localtime( &tv.tv_sec );
#else
	SYSTEMTIME	tnow;
	GetLocalTime( &tnow );
#endif
	hTrace->m_bHasDateFormat = FALSE;
	hTrace->m_bHasTimeFormat = FALSE;
	dst = hTrace->m_strBaseName;
	ptr1 = hTrace->m_strRawBaseName;
	while ( (ptr2=_tcschr(ptr1, '$')) != NULL )
	{
		if ( _tcsnccmp(ptr2, _T("$DATE"), 5)==0 )
		{
			_tcsncpy(dst, ptr1, ptr2-ptr1);
			dst += ptr2 - ptr1;
#ifdef linux
			sprintf(dst, "%04d%02d%02d", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday);
			hTrace->m_nCurDate = ptm->tm_mday;
#else
			_stprintf(dst, _T("%04d%02d%02d"), tnow.wYear, tnow.wMonth, tnow.wDay);
			hTrace->m_nCurDate = tnow.wDay;
#endif
			dst += 8;
			ptr1 = ptr2 + 5;
			hTrace->m_bHasDateFormat = TRUE;
		}
		else if ( _tcsnccmp(ptr2, _T("$TIME"), 5)==0 )
		{
			_tcsncpy(dst, ptr1, ptr2-ptr1);
			dst += ptr2 - ptr1;
#ifdef linux
			_stprintf(dst, _T("%02d%02d%02d"), ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
#else
			_stprintf(dst, _T("%02d%02d%02d"), tnow.wHour, tnow.wMinute, tnow.wSecond);
#endif
			dst += 6;
			ptr1 = ptr2 + 5;
			hTrace->m_bHasTimeFormat = TRUE;
		}
		else
		{
			// not supported marco simply copy it
			_tcsncpy(dst, ptr1, ptr2-ptr1);
			dst += ptr2 - ptr1;
			*(dst++) = *(ptr1++);		// copy '$'
		}
	}
	// copy reset of character
	_tcscpy(dst, ptr1);
}

static BOOL date_changed( HTRACE hTrace )
{
	BOOL  bResult = FALSE;
#ifdef linux
	struct timeval tv;
	struct tm *ptm;

	gettimeofday( &tv, NULL );
	ptm = localtime( &tv.tv_sec );
	bResult = hTrace->m_nCurDate != ptm->tm_mday;
	hTrace->m_nCurDate = ptm->tm_mday;
#else
	SYSTEMTIME	tnow;
	GetLocalTime( &tnow );
	bResult = hTrace->m_nCurDate != tnow.wDay;
	hTrace->m_nCurDate = tnow.wDay;
#endif
	return bResult;
}

static void make_log_filename(HTRACE hTrace)
{
	_tcscpy(hTrace->m_strLogFile, hTrace->m_strLogDir);
	_tcscat( hTrace->m_strLogFile, _T("/") );
	_tcscat( hTrace->m_strLogFile, hTrace->m_strBaseName );
	_tcscat( hTrace->m_strLogFile, _T(".log") );
}

static void make_rollover_fileformat(HTRACE hTrace, TCHAR *format)
{
	_tcscpy(format, hTrace->m_strLogDir);
	_tcscat( format, _T("/") );
	_tcscat( format, hTrace->m_strBaseName );
	if ( hTrace->m_rollcnt >= 100 )
		_tcscat( format, _T("-%03d") );
	else if ( hTrace->m_rollcnt >= 10 )
		_tcscat( format, _T("-%02d") );
	else
		_tcscat( format, _T("-%d") );
	_tcscat( format, _T(".log") );
}

static void make_full_path( LPCTSTR path)
{
	const TCHAR *ptr1, *ptr2;
	TCHAR	temp_path[MAX_PATH];
	BOOL   bNotEnd = TRUE;
	struct stat st;
	ptr1 = path;
	for(;bNotEnd;)
	{
		ptr2 = _tcschr(ptr1, '/');
		if ( ptr2 == NULL )
			ptr2 = _tcschr(ptr1, '\\');
		if ( ptr2==NULL )
		{
			bNotEnd = FALSE;
			_tcscpy(temp_path, path);
		}
		else
		{
			int len = ptr2 - path;
			_tcsncpy(temp_path, path, len);
			temp_path[len] = '\0';
		}
		if ( stat(temp_path, &st)==-1 )
		{
#ifdef WIN32
			CreateDirectory( temp_path, NULL );
#else
			mkdir(temp_path,0666);
#endif
		}
		ptr1 = ptr2 + 1;
	}
}

static int make_log_dir( HTRACE hTrace)
{
	int len;
	TCHAR *ptr1, *ptr2, *dst;
#ifdef linux
	struct timeval tv;
	struct tm *ptm;

	gettimeofday( &tv, NULL );
	ptm = localtime( &tv.tv_sec );
#else
	SYSTEMTIME	tnow;
	GetLocalTime( &tnow );
#endif
	hTrace->m_bHasDateDir = FALSE;
	dst = hTrace->m_strLogDir;
	ptr1 = hTrace->m_strRawLogDir;
	if ( (ptr2=_tcsstr(ptr1, _T("$DATE"))) != NULL )
	{
		_tcsncpy(dst, ptr1, ptr2-ptr1);
		dst += ptr2 - ptr1;
#ifdef linux
		sprintf(dst, "%04d%02d%02d", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday);
		hTrace->m_nCurDate = ptm->tm_mday;
#else
		_stprintf(dst, _T("%04d%02d%02d"), tnow.wYear, tnow.wMonth, tnow.wDay);
		hTrace->m_nCurDate = tnow.wDay;
#endif
		dst += 8;
		ptr1 = ptr2 + 5;
		hTrace->m_bHasDateDir = TRUE;
	}
	// copy reset of character
	_tcscpy(dst, ptr1);
	// remove trailing '/'or '\\'
	len = (int)_tcslen( hTrace->m_strLogDir );
	if ( hTrace->m_strLogDir[len-1] == '/' || hTrace->m_strLogDir[len-1] == '\\' )
		hTrace->m_strLogDir[len-1] = '\0';
	make_full_path(hTrace->m_strLogDir);
	return 0;
}


void *mlog_init(LPCTSTR LogDir, LPCTSTR BaseName )
{
	HTRACE hTrace = (HTRACE)malloc(sizeof(TraceStruct) );
	if ( hTrace==NULL )
		return hTrace;

	memset( hTrace, 0, sizeof(TraceStruct) );
	hTrace->nMagicNo = MAGIC_NO;
	hTrace->m_bKeepOpen = TRUE;
	hTrace->m_bEnable = TRUE;
	hTrace->sz_limit = DEF_LOGSIZE * 1024;
	hTrace->m_rollcnt = DEF_ROLLCOUNT;
	hTrace->m_storedays = DEF_STOREDAYS;
#ifdef ENABLE_MUTEX
#if defined _WIN32 || defined _WIN64
	hTrace->m_hMutex  = CreateMutex(
						NULL,				// default security attributes
						FALSE,				// initially not owned
						NULL);				// mutex name (NULL for unname mutex)
#else
	pthread_mutex_init(&hTrace->m_hMutex, NULL);
#endif
#endif
	_tcscpy(hTrace->m_strRawLogDir, LogDir==NULL ? __def_log_dir : LogDir);
	make_log_dir(hTrace);
	_tcscpy(hTrace->m_strRawBaseName, BaseName==NULL ? DEF_LOG_BASEFILE : BaseName);
	replace_basename_macro(hTrace);
	make_log_filename(hTrace);
	hTrace->m_fp  = NULL; // fopen( hTrace->m_strLogFile, "a" );

	return hTrace;
};

void mlog_close(void *h)
{
	if ( ISVALID_HANDLE(h) )
	{
		HTRACE hTrace = (HTRACE)h;
		if ( hTrace->m_fp != NULL )
			fclose( hTrace->m_fp );
#ifdef ENABLE_MUTEX
#if defined _WIN32 || defined _WIN64
		if ( hTrace->m_hMutex )
			CloseHandle( hTrace->m_hMutex );
#else
		pthread_mutex_destroy(&hTrace->m_hMutex);
#endif
#endif
		free( hTrace );
	}
}

void mlog_setpath( void *h, const char *path)
{
	HTRACE hTrace = (HTRACE)h;

	if ( h==NULL )
		strcpy(__def_log_dir, path);
	else
	{
		if ( !ISVALID_HANDLE(h) )  return;
		
		Mutex_Lock( hTrace->m_hMutex );
		if ( hTrace->m_fp != NULL )
			fclose(hTrace->m_fp);
		hTrace->m_fp = NULL;
		_tcscpy(hTrace->m_strRawLogDir, path==NULL ? __def_log_dir : path);
		make_log_dir(hTrace);
		make_log_filename(hTrace);
		Mutex_Unlock( hTrace->m_hMutex );
	}
}


void mlog_setbasename(HANDLE h, LPCTSTR base)
{
	HTRACE hTrace = (HTRACE)h;

	if ( !ISVALID_HANDLE(h) )  return;

	Mutex_Lock( hTrace->m_hMutex );
	if ( hTrace->m_fp != NULL )
		fclose(hTrace->m_fp);
	hTrace->m_fp = NULL;
	if ( base )
	{
		_tcsncpy(hTrace->m_strRawBaseName, base, MAX_BASE_NAME);
		hTrace->m_strRawBaseName[MAX_BASE_NAME-1] = '\0';
		replace_basename_macro(hTrace);
		make_log_filename(hTrace);
	}
	Mutex_Unlock( hTrace->m_hMutex );
}

static void log_rollforward(HTRACE hTrace)
{
	TCHAR	cLog1[ MAX_PATH ], cLog2[ MAX_PATH ];
	TCHAR  roll_format[MAX_PATH];
	int		i;

	make_rollover_fileformat(hTrace, roll_format);
	_stprintf( cLog2, roll_format, hTrace->m_rollcnt );
	_tunlink( cLog2 );					// unlink last one
	for( i=hTrace->m_rollcnt-1; i>0; i-- )
	{
		_stprintf( cLog1, roll_format, i );
		_trename( cLog1, cLog2 );	
		_tcscpy( cLog2, cLog1 );
	}
	_trename( hTrace->m_strLogFile, cLog1 );
}

static void purge_old_files(HTRACE hTrace)
{
#ifdef linux
	pthread_t  threadId;
	pthread_create( &threadId, NULL, purge_files_fxc, (void *)hTrace);
#else
	DWORD	threadId;
	HANDLE hThread = CreateThread(
						NULL,						// default security attributes
						0,								// use default stack size
(LPTHREAD_START_ROUTINE)purge_files_fxc,		// thread function
						hTrace,						// argument to thread function
						0,								// thread will be suspended after created.
						&threadId);				// returns the thread identifier
	CloseHandle(hThread);
#endif
}

static LPCTSTR time_stamp()
{
	static TCHAR timestr[64];
#ifdef linux
	char *p = timestr;
	struct timeval tv;
	gettimeofday( &tv, NULL );

	strftime( p, sizeof(timestr), "%y/%m/%d %H:%M:%S", localtime( &tv.tv_sec ) );
	sprintf( p + strlen(p), ".%03lu", tv.tv_usec/1000 );

#else
		SYSTEMTIME	tnow;
		GetLocalTime( &tnow );
		_stprintf( timestr, _T("%04d/%02d/%02d %02d:%02d:%02d.%03d"), 
					tnow.wYear, tnow.wMonth, tnow.wDay, 
					tnow.wHour, tnow.wMinute, tnow.wSecond, tnow.wMilliseconds );
#endif
	return timestr;
}

int mtrace_vlog(void *h, LPCTSTR fmt, va_list va)
{
#define BUFFERLEN		8092
	TCHAR		str[ BUFFERLEN ] = _T("");
	int			rc = 0;
	struct	stat	st;
	HTRACE hTrace = (HTRACE)h;
	
	if ( !ISVALID_HANDLE(h) || !hTrace->m_bEnable ) 
		return 0;
	
	Mutex_Lock( hTrace->m_hMutex );
	_vsntprintf(str, BUFFERLEN-1, fmt, va);

	if ( (hTrace->m_bHasDateFormat || hTrace->m_bHasDateDir) && date_changed(hTrace) )
	{
		if ( hTrace->m_fp )
		{
			fclose( hTrace->m_fp );
			hTrace->m_fp = NULL;
		}
		if ( hTrace->m_bHasDateDir )
			make_log_dir(hTrace);
		if ( hTrace->m_bHasDateFormat )
			replace_basename_macro(hTrace);
		make_log_filename(hTrace);
		purge_old_files(hTrace);
	}

	if ( hTrace->m_fp == NULL )
		hTrace->m_fp = _tfopen( hTrace->m_strLogFile, _T("a") );

	if ( hTrace->m_fp != NULL )
	{
		BOOL over_size = FALSE;
	#if defined _WIN32 || defined _WIN64
		fstat( _fileno(hTrace->m_fp), &st );
		over_size = st.st_size > (_off_t)hTrace->sz_limit;
	#else
		fstat( fileno(hTrace->m_fp), &st );
		over_size = st.st_size > (off_t)hTrace->sz_limit;
	#endif 	
		if (over_size)
		{
			fclose( hTrace->m_fp );
			hTrace->m_fp = NULL;
			// 文件basename没有$TIME,直接roll-over.
			if ( !hTrace->m_bHasTimeFormat )
				log_rollforward(hTrace);
			else
			{
				// 文件basename有$TIME, 产生新文件名,不要Roll-over
				replace_basename_macro(hTrace);
				make_log_filename(hTrace);
			}
		}
	}

	if ( hTrace->m_fp==NULL )
		hTrace->m_fp = _tfopen( hTrace->m_strLogFile, _T("a") );

	if ( hTrace->m_fp != NULL )
	{
		if ( fmt[0] != '\t' )
		{
			rc = _ftprintf(hTrace->m_fp, _T("[%s] %s"), time_stamp(),  str);
			printf(_T("[%s] %s"), time_stamp(),  str);
		}
		else
		{
			rc = _ftprintf(hTrace->m_fp, _T("%26s%s"), _T(" "), str+1 );
			printf( _T("%26s%s"), _T(" "), str+1 );
		}
		if ( hTrace->m_bKeepOpen )
			fflush( hTrace->m_fp );
		else
		{
			fclose( hTrace->m_fp );
			hTrace->m_fp = NULL;
		}
	}
	Mutex_Unlock( hTrace->m_hMutex );
	return rc;	
}

int mtrace_log (void *h, LPCTSTR fmt, ...)
{
	va_list	va;
	int rc;
	va_start(va, fmt);
	rc = mtrace_vlog(h, fmt, va);
	va_end(va);

	return rc;
}

void mlog_enable( void *h, BOOL bEnable )
{
	if ( ISVALID_HANDLE(h) )
	{
		HTRACE hTrace = (HTRACE)h;
		hTrace->m_bEnable  = bEnable;
	}
}

void mlog_keepopen( void *h, BOOL bKeepOpen )
{
	if ( ISVALID_HANDLE(h) )
	{
		HTRACE hTrace = (HTRACE)h;
		hTrace->m_bKeepOpen  = bKeepOpen;
		if ( !bKeepOpen && hTrace->m_fp!=NULL )
		{
			fclose( hTrace->m_fp );
			hTrace->m_fp = NULL;
		}
	}
}

void mlog_setlimitcnt( void *h, int limit, int cnt )
{
	if ( ISVALID_HANDLE(h) )
	{
		HTRACE hTrace = (HTRACE)h;
		if ( limit > MAX_LOGSIZE )
			limit = MAX_LOGSIZE;
		hTrace->sz_limit = limit * 1024;
		hTrace->m_rollcnt = cnt;
		if ( hTrace->m_rollcnt  > MAX_ROLLCOUNT )
			hTrace->m_rollcnt = MAX_ROLLCOUNT;
	}
}

void mlog_setlifedays( void *h, int nday)
{
	if ( ISVALID_HANDLE(h) )
	{
		HTRACE hTrace = (HTRACE)h;
		hTrace->m_storedays = nday;
	}
}

int mlog_is_enabled( void *h)
{
	if ( ISVALID_HANDLE(h) )
	{
		HTRACE hTrace = (HTRACE)h;
		return hTrace->m_bEnable;
	}
	return 0;
}

static int recursively_purge(TCHAR *root_dir, time_t del_time)
{
	DIR* dir;
	struct dirent *direntry;
	struct stat st;
	TCHAR temp[MAX_PATH];
	TCHAR dir_node[MAX_PATH];
	int  n_aged = 0;
	int len;
	
	_tcscpy(dir_node, root_dir);
	
    if ( (dir = opendir(dir_node)) == NULL )
		return 0;

	len = _tcslen(dir_node);
	if (dir_node[len-1] != '/' && dir_node[len-1]!='\\' )
	{
		dir_node[len++] = '/';
		dir_node[len] = '\0';
	}

	while ( (direntry=readdir(dir)) != NULL )
    {
        if (!_tcscmp( direntry->d_name, _T(".") ) || !_tcscmp( direntry->d_name, _T("..") ) )
            continue;

		_tcscpy( temp, dir_node ); 
        _tcscat( temp, direntry->d_name );
        if (stat(temp, &st) == 0)
        {
            if( (st.st_mode&S_IFDIR) == S_IFDIR )
			{
                  n_aged +=  recursively_purge(temp, del_time);
				  _trmdir(temp);		// 当文件夹空的时候才会成功
			}
            else if ( (st.st_mode&S_IFREG) == S_IFREG &&
						 _tcsstr(direntry->d_name, _T(".log")) != NULL &&
						 st.st_mtime < del_time )
			{
				_tunlink(temp);
				n_aged++;		// 只计算删除过期的日志文件
			}
        }
    }
    closedir(dir);	
   return n_aged;
}

static THREADCALL purge_files_fxc(PVOID lpParameter)
{
	HTRACE hTrace = (HTRACE)lpParameter;
	TCHAR root_dir[MAX_PATH] = _T(".");
	time_t time_2del = time(NULL) - hTrace->m_storedays*86400;		// 文件早于这个时间的要删除, 1天是86400秒

	if ( hTrace->m_bHasDateDir )
	{
		TCHAR *ptr1 = _tcsrchr(hTrace->m_strLogDir, '\\');
		TCHAR *ptr2 = _tcsrchr(hTrace->m_strLogDir, '/');
		TCHAR *ptr;
		if ( ptr1!=NULL && ptr2 != NULL )
			ptr = ptr1 > ptr2 ? ptr1 : ptr2;
		else
			ptr = ptr1==NULL ? ptr2 : ptr1;
		if ( ptr != NULL )
		{
			int len = ptr - hTrace->m_strLogDir;
			_tcsncpy(root_dir, hTrace->m_strLogDir, len);
			root_dir[len] = '\0';
		}
	}
	else
	{
		_tcscpy(root_dir, hTrace->m_strLogDir);
	}
	
	recursively_purge(root_dir, time_2del);
	return THREADRET;
}

#if defined _WIN32 || defined _WIN64
static DIR*  opendir(LPCTSTR dir)  
{  
    DIR   *dp;  
    TCHAR   filespec[NAME_MAX];  
    long   handle;  
    int   index;  

    _tcsncpy(filespec, dir, NAME_MAX);  
    index   =  _tcslen(filespec)   -   1;  
    if   (index   >=   0   &&   (filespec[index]   ==  '/'   ||   filespec[index]   ==  '\\'))  
        filespec[index]   =  '\0';  
    _tcscat(filespec,  _T("/*"));  
    dp  =  (DIR   *)malloc(sizeof(DIR));  
    dp->offset   =   0;  
    dp->finished   =   0;  
    dp->dir   =   _tcsdup(dir);  
    if   ((handle  =  _tfindfirst(filespec,   &(dp->fileinfo)))   <   0)    
    {  
        //if   (errno   ==   ENOENT)  
        //    dp->finished   =   1;  
        // else  
            return   NULL;  
    }  
    dp->handle   =   handle;  
    return   dp;  
}  

static struct  dirent*  readdir(DIR   *dp)  
{  
    if   (!dp   ||   dp->finished)    
        return   NULL;  

    if   (dp->offset   !=   0)    
    {  
        if   (_tfindnext(dp->handle,   &(dp->fileinfo))   <   0)    
        {  
            dp->finished   =   1;  
            return   NULL;  
        }  
    }  
    dp->offset++;  
    _tcscpy(dp->dent.d_name,   dp->fileinfo.name);  
    dp->dent.d_ino   =   1;  
    dp->dent.d_reclen   =  _tcslen(dp->dent.d_name);  
    dp->dent.d_off   =   dp->offset;  
    return   &(dp->dent);  
}  

static int closedir(DIR   *dp)
{  
    if ( !dp )    
        return   0;  
    _findclose( dp->handle );  
    if ( dp->dir )    
    {  
        free( dp->dir );  
        dp->dir = NULL;  
    }  
    free( dp );  
    dp = NULL;  
    return   0;  
}
#endif

///==============================================================================================
// test code
//
#if defined EN_MTRACE_TESTCODE && defined linux
#define ENABLE_LOG
#include "utils_mtrace.h"

#define HiRand(lo,hi)		( (lo) + (int)(((hi)-(lo)) * (((float)rand()) / RAND_MAX) ) )

void *hlog = NULL;
int  working_thread_num = 0;

int getint(char *buf, int *val, char **endptr)
{
	char *ptr = buf;
	// skip until a digit 
	for( ; !isdigit(*ptr) && *ptr; ptr++ );
	if ( !isdigit(*ptr) )
		return -1;
	*val = atoi(ptr);
	// skip to end of digit
	if ( endptr )
	{
		for( ; isdigit(*ptr) && *ptr; ptr++ );
		*endptr = ptr;
	}
	return 0;
}

void *logger_thread_fxc(void *param)
{
	pthread_t tid = pthread_self();
	int line = 0;
	working_thread_num++;
	for(;hlog!=NULL;)
	{
		int pause = HiRand(10,100);
		usleep(1000*pause);
		MTRACE_LOG(hlog,"[TID:%u] - this is log line %d...\n", tid, ++line);
	}
	working_thread_num--;
	return NULL;
}

int main(int argc, char * const argv[])
{
	int nthread = 1;
	char buf[80];
	int i, len;
	pthread_t  tid;
	const char *path = NULL;
	const char *base_name = NULL;

	if ( argc > 1 && atoi(argv[1]) > 1 )
		nthread = atoi(argv[1]);
	if ( argc > 2 )
		path = argv[2];
	if ( argc > 3 )
		base_name = argv[3];

	srand(time(NULL));
	hlog = MLOG_INIT(path, base_name);
	if ( hlog==NULL )
	{
		printf("failed to create log handle!\n");
		return -1;
	}
	
	for(i=0; i<nthread; i++)
	    pthread_create(&tid, NULL, logger_thread_fxc, NULL);
	    
	for(;;)
	{
		char *ptr;
		printf("cmd>");
		gets(buf);
		if ( strcmp(buf,"quit")==0 )
			break;
		else if ( strncmp(buf,"path",4)==0 )
		{
			ptr = buf+4;
			for(;*ptr && *ptr==' '; ptr++);
			MLOG_SETPATH(hlog, ptr);
		}
		else if ( strncmp(buf, "base", 4)==0 )
		{
			ptr = buf+4;
			for(;*ptr && *ptr==' '; ptr++);
			MLOG_SETBASENAME(hlog, ptr);
			printf("set base name: %s\n", ptr);
		}
		else if ( strncmp(buf,"limit", 5)==0 )
		{
			int limit=0, cnt=0;
			char *ptr;
			 if ( getint(buf+5,&limit,&ptr)==0 && limit>1 &&
		  	      getint(ptr,&cnt,NULL)==0 && cnt>1 )
		 	{
		  		MLOG_SETLIMITCNT(hlog, limit, cnt);
		  		printf("set limit=%d KB, rollback count=%d\n", limit, cnt);
			}
			else
		  		printf("invalid argument!\n");
		}
		else if ( strncmp(buf, "life", 4)==0 )
		{
			int nday;
			char *ptr;
			if ( getint(buf+5,&nday,&ptr)==0 && nday>0 )
			{
				MLOG_SETLIFEDAYS(hlog, nday);
				printf("set life days=%d\n", nday);
			}
			else
				printf("Invalid argument!\n");
		}
		else if ( strcmp(buf,"enable")==0 )
		{
			MLOG_ENABLE(hlog,TRUE);
			printf("enable log!\n");
		}
		else if ( strcmp(buf,"disable")==0 )
		{
			MLOG_ENABLE(hlog,FALSE);
			printf("disable log!\n");
		}
		else if ( strcmp(buf,"open")==0 )
		{
			MLOG_KEEPOPEN(hlog,TRUE);
			printf("keep open after write log line!\n");
		}
		else if ( strcmp(buf,"close")==0 )
		{
			MLOG_KEEPOPEN(hlog,FALSE);
			printf("close after write log line!\n");
		}
	}
	MLOG_CLOSE(hlog);
	hlog = NULL;
	while (working_thread_num>0)
		usleep(100000);
	return 0;
}

#endif
