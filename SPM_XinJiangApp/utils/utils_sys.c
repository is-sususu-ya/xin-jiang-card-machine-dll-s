#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>


long get_sys_sec()
{
 	struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + (ts.tv_nsec > 500000000);
}

long get_sys_tick()
{
 	struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;	
}

signed long long get_sys_tick64()
{
 	struct timespec ts;
	
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (signed long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;	
}

void init_itimer()
{
  struct itimerval tmr;

	tmr.it_value.tv_sec = 86400;	// one full day. Shall be enough
	tmr.it_value.tv_usec = 0;
	tmr.it_interval.tv_sec = 86400;
	tmr.it_interval.tv_usec = 0;

	setitimer ( ITIMER_REAL, &tmr, NULL );
}

long get_itimer()
{
    struct  itimerval tmr;
    long	delta_s, delta_us;

    getitimer( ITIMER_REAL, &tmr );
    delta_s = tmr.it_interval.tv_sec - tmr.it_value.tv_sec - 1;
    delta_us = 1000000 - tmr.it_value.tv_usec;
    if ( delta_us == 1000000 ) {
    	delta_s++;
    	delta_us = 0;
    }
    // when elapsed time is very short, some time you get
    // negative delta_s. Zero it when it happened.
    if ( delta_s < 0 ) {
    	delta_s = 0;
    	delta_us = 0;
    }
    return delta_s * 1000 + delta_us/1000;
}
///////////////////////////////////////////////////////////////////////
/*
 * sleep and usleep will be interrupted by signal and cannot guarenty
 * the time to wait. waitfor do the real wait for specified time period.
 */
void waitfor( int msec )
{
	int msec_left = msec;
	init_itimer( ITIMER_REAL );
	do {
		usleep( msec_left*1000 );
		msec_left = msec - get_itimer(ITIMER_REAL);
	} while ( msec_left > 0 );
}
