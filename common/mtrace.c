/*
 * mtrace.c
 * for multiple log files application using this instead of trace.c
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <direct.h>
#include <fcntl.h>
#if defined _WIN32 || defined _WIN64
//#define _CRT_SECURE_NO_WARNINGS 		// disable the annoying warning 4996
#include <windows.h>
#include <io.h>
#include <direct.h>
#include "tchar.h"
#define DEF_LOG_DIR			_T("/rwlog")
typedef HANDLE HMUTEX;
#else		// [Linux]
#include <pthread.h>
#include <error.h>
#include <errno.h>
#define DEF_LOG_DIR			"/var/log"
typedef int		BOOL;
typedef pthread_mutex_t		HMUTEX;
// mimic Windows SDK call
#define GetLastError()		(errno)
#define CreateDirectory(path, attr)		(mkdir(path, attr==NULL ? 777 : 777)!=-1)
#define MAX_PATH 256
#define	_T(str)		str
#define TCHAR		char
#ifndef _TRUEFALSE_DEFINED_
#define _TRUEFALSE_DEFINED_
#define TRUE	1
#define FALSE	0
#define true	1
#define false	0
#endif

#include <fcntl.h> 
#include <unistd.h>
#include <sys/time.h> 
#endif

#define ENABLE_MUTEX
#define DEF_LOG_BASEFILE	_T("RunWellAP")
#define MAX_LOGSIZE			8388608		// 8 MB
#define MAX_ROLLCOUNT		30			// <BaseName>01.log ~ <BaseName>30.log
#define MAGIC_NO				0xcb34ae78
#define ISVALID_HANDLE(h) ( (h)!=NULL && ((HTRACE) h)->nMagicNo == MAGIC_NO )

typedef struct {
	int		nMagicNo;
	TCHAR m_strLogFile[MAX_PATH];
	TCHAR m_strFormat[MAX_PATH];
	TCHAR m_strBaseName[64];
	size_t  sz_limit;
	int	   m_rollcnt;
	FILE	*m_fp;
	BOOL m_bEnable;
	BOOL m_bKeepOpen;
	HMUTEX	m_hMutex;
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

static int set_log_dir( HTRACE hTrace, LPCTSTR logdir )
{
	int len;

	if ( logdir==NULL )
		_tcscpy( hTrace->m_strLogFile, DEF_LOG_DIR );
	else
		_tcscpy( hTrace->m_strLogFile, logdir );
	len = (int)_tcslen( hTrace->m_strLogFile );
	if ( hTrace->m_strLogFile[len-1] == '/' || hTrace->m_strLogFile[len-1] == '\\' )
		hTrace->m_strLogFile[len-1] = '\0';
	if ( !CreateDirectory( hTrace->m_strLogFile, NULL ) )
		len = GetLastError();			// for debugger to see the error code

	_tcscat( hTrace->m_strLogFile, _T("/") );
	_tcscat( hTrace->m_strLogFile, hTrace->m_strBaseName );
	_tcscpy( hTrace->m_strFormat, hTrace->m_strLogFile );
	_tcscat( hTrace->m_strLogFile, _T(".log") );
	_tcscat( hTrace->m_strFormat, _T("-%02d") );
	_tcscat( hTrace->m_strFormat, _T(".log") );
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
	hTrace->sz_limit = MAX_LOGSIZE;
	hTrace->m_rollcnt = MAX_ROLLCOUNT;
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

	_tcscpy(hTrace->m_strBaseName, BaseName==NULL ? DEF_LOG_BASEFILE : BaseName);
	set_log_dir(hTrace, LogDir);
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

void mlog_setpath( void *h, LPCTSTR path)
{
	HTRACE hTrace = (HTRACE)h;

	if ( !ISVALID_HANDLE(h) )  return;
	if ( hTrace->m_fp != NULL )
		fclose(hTrace->m_fp);
	hTrace->m_fp = NULL;

	set_log_dir(hTrace, path);
}

void mlog_setbasename(HANDLE h, LPCTSTR base)
{
	HTRACE hTrace = (HTRACE)h;

	if ( !ISVALID_HANDLE(h) )  return;
	if ( hTrace->m_fp != NULL )
		fclose(hTrace->m_fp);
	hTrace->m_fp = NULL;
	_tcscpy(hTrace->m_strBaseName, base);
}

static void log_rollforward(HTRACE hTrace)
{
	TCHAR	cLog1[ MAX_PATH ], cLog2[ MAX_PATH ];
	int		i;

	_stprintf( cLog2, hTrace->m_strFormat, hTrace->m_rollcnt );
	_tunlink( cLog2 );					// unlink last one
	for( i=hTrace->m_rollcnt-1; i>0; i-- )
	{
		_stprintf( cLog1, hTrace->m_strFormat, i );
		_trename( cLog1, cLog2 );	
		_tcscpy( cLog2, cLog1 );
	}
	_trename( hTrace->m_strLogFile, cLog1 );
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

	if ( hTrace->m_fp == NULL )
		hTrace->m_fp = _tfopen( hTrace->m_strLogFile, _T("a") );

	if ( hTrace->m_fp != NULL )
	{
	#if defined _WIN32 || defined _WIN64
		fstat( _fileno(hTrace->m_fp), &st );
		if ( st.st_size > (_off_t)hTrace->sz_limit )
	#else
		fstat( fileno(hTrace->m_fp), &st );
		if ( st.st_size > (off_t)hTrace->sz_limit )
	#endif 	
		{
			fclose( hTrace->m_fp );
			log_rollforward(hTrace);
			hTrace->m_fp = _tfopen( hTrace->m_strLogFile, _T("a") );
		}
	}
	if ( hTrace->m_fp != NULL )
	{
		if ( fmt[0] != '\t' )
		{
			rc = _ftprintf(hTrace->m_fp, _T("[%s] %s"), time_stamp(),  str);
		}
		else
		{
			rc = _ftprintf(hTrace->m_fp, _T("%26s%s"), _T(" "), str+1 );
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
		hTrace->sz_limit  = limit * 1024;
		hTrace->m_rollcnt = cnt;
	}
}
