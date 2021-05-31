/*
 *	longtime.c
 *
 *	handle the time in msec resolution.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include "longtime.h"

#ifdef _WIN32
#include <windows.h>

static __int64 _GetOffset() 
{
	SYSTEMTIME st1;
	LONGLONG lt1;
	memset( &st1, 0, sizeof( st1 ) );
	st1.wYear = 1970;
	st1.wMonth = 1;
	st1.wDay = 1;
	SystemTimeToFileTime( &st1, (FILETIME*)&lt1 );
	return lt1;
}

static __int64 llOffset = 0;
//static const time_t _secondOffset = ( 365 * 31 + 8 ) * 24 * 60 * 60;	// time offset from 1970.1.1 to 2001.1.1, 8 leap years( 72, 76, 80, 84, 88, 92, 96, 00 )
static const time_t _secondOffset = ( 365 * 30 + 7 ) * 24 * 60 * 60;	// time offset from 1970.1.1 to 2000.1.1, 7 leap years( 72, 76, 80, 84, 88, 92, 96 )
 
struct timezone {
    int  tz_minuteswest; // minutes W of Greenwich 
    int  tz_dsttime;		 // type of dst correction 
};

static int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	LONGLONG ft;
	GetSystemTimeAsFileTime( (FILETIME*)&ft );

	if( llOffset == 0 )
		llOffset = _GetOffset();
	tv ->tv_usec = (long)( ft % 10000000 ) / 10;
	tv ->tv_sec = (long)(( ft - llOffset ) / 10000000);

	return 0;
}

int localtime_r(const time_t *t, struct tm *ptm)
{
	struct tm *pm = localtime(t);
	if ( pm != NULL )
	{
		memcpy(ptm, pm, sizeof(struct tm));
		return 0;
	}
	return -1;
}

_longtime FileTimeToLongTime(LONGLONG ft)
{
	long  tv_sec, tv_usec;

	if( llOffset == 0 )
		llOffset = _GetOffset();

	tv_usec = (long)( ft % 10000000 ) / 10;
	tv_sec = (long)(( ft - llOffset ) / 10000000);
	return (_longtime)( tv_sec - _secondOffset ) * 1000 + tv_usec / 1000;
}

#else	// linux
#include <sys/time.h>
#endif

#ifdef linux
_longtime timeGetMonotonicTime()
{
	struct timespec tp;
	_longtime lt;
	if ( clock_gettime(CLOCK_MONOTONIC,&tp)==0 )
	{
		lt = tp.tv_sec * 1000;
		lt += tp.tv_nsec / 1000000;
		return lt;
	}
	return 0;
}
#endif		
_longtime timeGetLongTime()
{
	struct timeval tv;
	if( gettimeofday( &tv, NULL ) )
	{
		return -1;
	}
	return (_longtime)( tv.tv_sec - _secondOffset ) * 1000 + tv.tv_usec / 1000;
}

_longtime timeGetOldLongTime()
{
	struct timeval tv;
	if( gettimeofday( &tv, NULL ) )
	{
		return -1;
	}
	return (_longtime)tv.tv_sec * 1000000 + tv.tv_usec;
}

time_t timeLongTimeToTime( _longtime t )
{
	return (time_t)(t / 1000) + _secondOffset;
}

_longtime timeLongTimeFromTime( time_t t )
{
	return (_longtime)( t - _secondOffset ) * 1000;
}

time_t timeGetBODTime( time_t t )
{
//	struct tm	_tm;

	if ( t == 0 ) t = time(NULL);
#if 0
	localtime_r( &t, &_tm );
	_tm.tm_sec = 0;
	_tm.tm_min = 0;
	_tm.tm_hour = 0;
	return mktime( &_tm );
#endif
	// t -= timezone;		// convert to localtime
	t -= 0;		// convert to localtime
	return t - (t % 86400);
}

time_t timeGetEODTime( time_t t )
{
//	struct tm	_tm;

	if ( t == 0 ) t = time(NULL);
#if 0
	localtime_r( &t, &_tm );
	_tm.tm_sec = 0;
	_tm.tm_min = 0;
	_tm.tm_hour = 0;
	_tm.tm_mday++;
	return mktime( &_tm );
#endif
/* EOD is next day's BOD */
	t -= 0;
	return t + 86400 - (t % 86400);
}

unsigned long timeGetDays( _longtime t )
{
	return (unsigned long)( t / 1000 / 86400 );
}

unsigned long timeGetUSec( _longtime t )
{
	return (unsigned long)(( t % 1000 ) * 1000);
}

unsigned long timeGetMSec( _longtime t )
{
	return (unsigned long)( t % 1000);
}

unsigned long timeGetSec( _longtime t )
{
	return (unsigned long)( t / 1000);
}

void timeLongTimeToCTM( _longtime lt, PCTM ctm )
{
	time_t t;
	struct tm	_tm;

	if ( lt==0 )
		lt = timeGetLongTime();	//t = time(NULL); // 2015.1.5
	t = timeLongTimeToTime(lt);
	if (localtime_r( &t, &_tm )==0 )
	{
		ctm->year = _tm.tm_year + 1900;
		ctm->month = _tm.tm_mon + 1;
		ctm->day = _tm.tm_mday;
		ctm->hour = _tm.tm_hour;
		ctm->minute = _tm.tm_min;
		ctm->second = _tm.tm_sec;
	}
	else
		memset(ctm,0,sizeof(CTM));
	ctm->msec = (int)(lt % 1000);	
}

_longtime timeLongTimeFromCTM( PCTM ctm )
{
	time_t t;
	struct tm	_tm;
	_longtime lt;
	
	_tm.tm_year = ctm->year - 1900;
	_tm.tm_mon = ctm->month - 1;
	_tm.tm_mday = ctm->day;
	_tm.tm_hour = ctm->hour;
	_tm.tm_min = ctm->minute;
	_tm.tm_sec = ctm->second;
	t = mktime(&_tm);
	lt = timeLongTimeFromTime(t) + ctm->msec;
	return lt;
}

static int is_all_digit(const TCHAR *str, int len)
{
	int i;
	for(i=0; i<len; i++)
		if ( !_istdigit(str[i]) )
			return (str[i]==':' || str[i]==' ' || str[i]=='/' || str[i]=='.' || str[i]=='-') ? 0 : -1;
	return 1;
}

int timeGetCTMFromStr(const TCHAR *strTime, PCTM ctm)
{
	int rc, len;
	struct tm	_tm;
	time_t tnow;
	
	memset(ctm, 0, sizeof(CTM) );
	tnow=time(NULL);
	// �ȸ�ֵ�����������
	localtime_r( &tnow, &_tm );
	ctm->year = _tm.tm_year + 1900;
	ctm->month = _tm.tm_mon + 1;
	ctm->day = _tm.tm_mday;
	
	if ( (len=(int)(_tcslen(strTime)))<6 ) return -1;
	if ( (rc=is_all_digit(strTime, len))==-1 )	
	{
		//printf("���������ִ�����: %s (len=%d)\n", 	strTime, len );	
		return -1;
	}
	if ( rc )	// all digit
	{
		if ( len==6 )	
		{
			int n1, n2, n3;
			if ( _stscanf(strTime, _T("%02d%02d%02d"), &n1, &n2, &n3 ) != 3 )
				return -1;
			if ( n2 <= 12 && n3 <= 31 && n1>0 && n2>0 && n3>0 )
			{
				// 6λ��������
				ctm->year = n1 + 2000;
				ctm->month = n2;
				ctm->day = n3;
			}
			else
			{
				ctm->hour = n1;
				ctm->month = n2;
				ctm->second = n3;
			}
		}
		else if ( len==8 )	// һ��������
		{
			_stscanf(strTime, _T("%04d%02d%02d"), &ctm->year, &ctm->month, &ctm->day);
		}
		else if ( len==9 )	// ʱ��+����
		{
			_stscanf(strTime,_T("%02d%02d%02d%03d"), &ctm->hour, &ctm->month, &ctm->second, &ctm->msec);
		}
		else if ( len==12 )	// YYMMDDhhmmss
		{
			_stscanf(strTime, _T("%02d%02d%02d%02d%02d%02d"), 
				&ctm->year, &ctm->month, &ctm->day, &ctm->hour, &ctm->minute, &ctm->second);
				ctm->year += 2000;
		}
		else if ( len==14 )	// YYYYMMDDhhmmss
		{
			_stscanf(strTime, _T("%04d%02d%02d%02d%02d%02d"),
				&ctm->year, &ctm->month, &ctm->day, &ctm->hour, &ctm->minute, &ctm->second);
		}
		else if ( len==15 )	// YYMMDDhhmmssmsc
		{
			_stscanf(strTime, _T("%02d%02d%02d%02d%02d%02d%03d"), 
				&ctm->year, &ctm->month, &ctm->day, &ctm->hour, &ctm->minute, &ctm->second, &ctm->msec);
			ctm->year += 2000;
		}
		else if ( len==17 )	// YYYYMMDDhhmmssmsc
		{
			_stscanf(strTime, _T("%04d%02d%02d%02d%02d%02d%03d"), 
				&ctm->year, &ctm->month, &ctm->day, &ctm->hour, &ctm->minute, &ctm->second, &ctm->msec);
		}
	}
	else 	// ����ʽ
	{
		TCHAR *ptr;
		if ( (ptr=_tcschr(strTime,'/')) != NULL || (ptr=_tcschr(strTime,'-')) != NULL )
		{
			// �д���ʽ������
			ctm->year = _tstoi(strTime);
			if ( ctm->year < 100 ) ctm->year += 2000;
			ptr++;
			ctm->month = _tcstol(ptr,&ptr,10);
			ctm->day = _tcstol(ptr+1, &ptr, 10);
			if ( *ptr==' ' ) ptr++;
		}
		else if ( ptr==NULL )	// û�����ڣ�ֻ��ʱ��
			ptr = (TCHAR *)strTime;
		ctm->hour = _tcstol(ptr,&ptr,10);
		ctm->minute = _tcstol(ptr+1,&ptr,10);
		ctm->second = _tcstol(ptr+1,&ptr,10);
		if ( *ptr=='.' )
		{
			ctm->msec = _tcstol(ptr+1,&ptr,10);
		}
	}
	// �����
	//printf("����ʱ����������%04d/%02d/%02d %02d:%02d:%02d.%03d\n",
		//	ctm->year, ctm->month, ctm->day, ctm->hour, ctm->minute, ctm->second, ctm->msec );
	rc = (ctm->month<=12 && ctm->day<=31 && 
				 ctm->hour<24 && ctm->minute<60 && ctm->second<60 && ctm->msec < 1000) ?
				 0 : -1;
	return rc;
}

static TCHAR _mybuf[ LONG_TIME_FORMAT_SIZE ];
TCHAR *timeFormatLongTime( _longtime t, TCHAR *buffer )
{
	TCHAR	*p = buffer ? buffer : _mybuf;
/*		
	struct tm _tm;
	if ( t==0 ) t = timeGetLongTime();
	time_t time = timeLongTimeToTime( t );
	localtime_r( &time, &_tm );
	strftime( p, LONG_TIME_FORMAT_SIZE-5, "%y/%m/%d %H:%M:%S", &_tm );
	sprintf( p + strlen(p), ".%03lu", timeGetMSec( t ) );
*/
	CTM  ctm;
	timeLongTimeToCTM(t,&ctm);
	_stprintf(p, _T("%02d/%02d/%02d %02d:%02d:%02d.%03d"),
		ctm.year % 100, ctm.month, ctm.day,
		ctm.hour, ctm.minute, ctm.second, ctm.msec );
	return p;
}

TCHAR *timeFormatLongTimeEx(_longtime t, TCHAR *buffer, int format)
{
	TCHAR	*p = buffer ? buffer : _mybuf;
	TCHAR *ptr = p;
/*		
	struct tm _tm;
	
	if ( t==0 ) t = timeGetLongTime();
	time_t time = timeLongTimeToTime( t );
	localtime_r( &time, &_tm );
	if ( format & DT_FMT_DATE )
	{
		strftime( ptr, LONG_TIME_FORMAT_SIZE, "%y/%m/%d", &_tm );
		ptr += strlen(ptr);
	}
	if ( format & DT_FMT_TIME )
	{
		if ( ptr != p )
			*(ptr++) = ' ';
		strftime( ptr, LONG_TIME_FORMAT_SIZE, "%H:%M:%S", &_tm );
		ptr += strlen(ptr);
		if ( format & DT_FMT_MSEC )
			sprintf( ptr, ".%03lu", timeGetMSec( t ) );
	}
*/
	CTM  ctm;
	
	timeLongTimeToCTM(t,&ctm);
	
	if ( format & DT_FMT_DATE )
	{
		if ( format & DT_FMT_NOYEAR )
		{
			_stprintf( ptr, _T("%02d/%02d"), ctm.month, ctm.day );
		}
		else
		{
			if (format & DT_FMT_CENTRY)
				_stprintf( ptr, _T("%04d/%02d/%02d"), ctm.year, ctm.month, ctm.day );
			else
				_stprintf( ptr, _T("%02d/%02d/%02d"), ctm.year%100, ctm.month, ctm.day );
		}
		ptr += strlen(ptr);
	}
	if ( format & DT_FMT_TIME )
	{
		if ( ptr != p )
			*(ptr++) = ' ';
		_stprintf(ptr, _T("%02d:%02d:%02d"), ctm.hour, ctm.minute, ctm.second );
		ptr += strlen(ptr);
		if ( format & DT_FMT_MSEC )
			_stprintf( ptr, _T(".%03d"), ctm.msec );
	}
	
	return p;
}

int timeCheckLongTime(_longtime lt)
{
	// 2000/1/1 ~ 2100/12/31
	return (0 < lt && lt < 3155760000000LL) ? 0 : -1;
}