#ifndef _LONGTIME_H
#define _LONGTIME_H

#include <sys/types.h>
#ifdef linux
#define TCHAR		char
typedef signed long long 	_longtime;
#else
#include "tchar.h"
typedef signed __int64		_longtime;
#endif

#ifdef __cplusplus
extern "C" {
#endif

_longtime timeGetLongTime();
time_t timeLongTimeToTime( _longtime t );
_longtime timeLongTimeFromTime( time_t t );

_longtime timeGetOldLongTime();			// get time in usec

time_t timeGetBODTime( time_t );		// get begin of day (localtime)
time_t timeGetEODTime( time_t );		// get end of day

unsigned long timeGetDays( _longtime t );
unsigned long timeGetUSec( _longtime t );
unsigned long timeGetMSec( _longtime t );
unsigned long timeGetSec( _longtime t );

#define LONG_TIME_FORMAT_SIZE	24
#define DT_FMT_DATE		0x01
#define DT_FMT_TIME		0x02
#define DT_FMT_MSEC		0x04
#define DT_FMT_CENTRY	0x08		// include centry like 2015
#define DT_FMT_NOYEAR	0x10		// ��DT_FMT_DATE��λʱ����Ч��ֻ��ʾ��/��
#define DT_FMT_LONG		0x0f		// full date/time 
#define DT_FMT_DTIM		0x03		// date and time
#define DT_FMT_TMSEC	0x06		// time+msec only w/o date

TCHAR *timeFormatLongTime( _longtime t, TCHAR *buffer );	// buffer must have space >= LONG_TIME_FORMAT_SIZE
TCHAR *timeFormatLongTimeEx(_longtime t, TCHAR *buffer, int format);
#define TIMESTAMP()	timeFormatLongTime(0,NULL)
#define TIMESTAMP0() timeFormatLongTimeEx(0,NULL,DT_FMT_TMSEC)
#define TIMESTAMP1() timeFormatLongTimeEx(0,NULL,DT_FMT_LONG)
#define TIMESTAMPX(t) timeFormatLongTimeEx(t,NULL,DT_FMT_TMSEC)
#define TIMESTAMPY(t) timeFormatLongTimeEx(t,NULL,DT_FMT_LONG)
#define TIMESTAMPX_R(t,str) timeFormatLongTimeEx(t,str,DT_FMT_TMSEC)
#define TIMESTAMPY_R(t,str) timeFormatLongTimeEx(t,str,DT_FMT_LONG)

#pragma pack (push,2)
typedef struct {
	int	year;			// western calendar year
	int month;			// 1 ~ 12
	int day;				// day of month 1~31
	int hour;			// hour in a day 0~23
	int	minute;		// minutes in hour 0~59
	int second;		// seconds in minutes
	int msec;			// milli-seconds 0~999
} CTM, *PCTM;
#pragma pack (pop)

void timeLongTimeToCTM( _longtime lt, PCTM ctm );
_longtime timeLongTimeFromCTM( PCTM ctm );
#if defined _WIN32 || defined _WIN64
_longtime FileTimeToLongTime(__int64 ft);
#endif
// - timeGetCTMFromStr:
// ����0�ɹ���-1��ʽ����
// strTime������/ʱ���ִ������Խ��ܵ�ʱ���ִ���ʽΪ:
// 1) 2015/11/12 12:13:46.234
// 2) 2015-11-12 12:13:46.234
// 3) 20151112121346234
// 4) 20151112121346.234
// 5)~8) - ͬ1)~4), ������Ĳ���ֻ��2λ����
// ������ͬ�ϣ�����û�к��벿��,����û�����ڲ��֣�����û��һ���ڵ�ʱ�䲿�֡�
// û�����ڵĻ���ʹ�õ������ڣ�û��ʱ��Ĳ��֣�ʱ���֡��롢����ȫ��Ϊ0.
// ���ֻ��6λ���֣������޷����������ڻ���ʱ�䣬��ʱ�����ȡ�
int timeGetCTMFromStr(const TCHAR*strTime, PCTM ctm);

// ���_longtime ֵ�Ƿ�Ϸ� 
// ���� 0 ��ȷ�� -1 ����
int timeCheckLongTime(_longtime lt);

#ifdef __cplusplus
};
#endif

#endif	// _LONGTIME_H
