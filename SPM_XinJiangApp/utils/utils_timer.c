/*
 * utils_timer.c
 *
 * implement a simple easy to used timer mechanism. Application can easily add multiple timer
 * (one-shot or periodic) each one with same or different handler function.
 * all timer are identified by an unique Id specified by user when added.. 
 * handler function can identify which timer is fired by this 'id' when same fucntion handle
 * multiple timer.
 *
 * Author: Thomas Chang
 * Date: 2016-09-12
 */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef linux
#include <windows.h>
#else
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#include "utils_timer.h"
#include "utils_ptrlist.h"
#include "longtime.h"

#ifdef linux
static pthread_mutex_t	list_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t	w_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t	w_cond = PTHREAD_COND_INITIALIZER;
static pthread_t	tmr_thread;

#define list_lock()			pthread_mutex_lock(&list_lock)
#define list_unlock()		pthread_mutex_unlock(&list_lock)

#else
static HANDLE list_lock = NULL;
static HANDLE w_lock = NULL;
static HANDLE w_cond = NULL;
static HANDLE tmr_thread;

#define list_lock()			WaitForSingleObject( list_lock, INFINITE )
#define list_unlock()		ReleaseMutex( list_lock )

#endif

static PtrList		tmr_list = PTRLIST_INITIALIZER;

static int				in_service = 1;

#define  ARG_IGNOR		0xffffffff			// timer handler function w/o argument

typedef struct {
	int		id;
	tmr_hander	handle;
	void *arg;						// argument for handle function, ARG_IGNOR means no argument
	int 	count;
	_longtime	lt_fire;		// time to fired
	int		t_period;				// 0 for one-shot timer
} tmr_entry;

static int tmr_find(int id)
{
	POSITION pos;
	list_lock();
	for(pos=tmr_list.head; pos!=NULL; pos=pos->next)
	{
		tmr_entry *member = (tmr_entry *)pos->ptr;
		if ( member->id == id ) break;
	}
	list_unlock();
	return pos!=NULL;
}

static void tmr_insert( tmr_entry *entry)
{
	POSITION pos;
	list_lock();
	for(pos=tmr_list.head; pos!=NULL; pos=pos->next)
	{
		tmr_entry *member = (tmr_entry *)pos->ptr;
		if ( entry->lt_fire < member->lt_fire )
		{
			PtrList_insert_before(&tmr_list, pos, entry);
			break;
		}
	}
	if ( pos==NULL )
		PtrList_append(&tmr_list, entry);
	list_unlock();
}


static void *timer_server_fxc(void *lparam)
{
#ifdef linux
	struct timespec ts;
#endif
	int delta_ms;
	tmr_entry  *tmr_head;
	
	printf("------------ timer_server_fxc start ------------\n");

	for(;in_service;)
	{
		list_lock();
		_longtime lt_now = timeGetLongTime();
	#ifdef linux
		clock_gettime( CLOCK_REALTIME, &ts );
	#endif 
	
		if ( tmr_list.count == 0 )
		{
#ifdef linux
			ts.tv_sec += 60;		// for empty list, sleep 60 second
#else
			delta_ms = 60000;
#endif
			tmr_head = NULL;
		}
		else
		{
			tmr_head = (tmr_entry *)tmr_list.head->ptr;
			if ( tmr_head->lt_fire <= lt_now )
			{
				// head entry fire time is due, remove it out of list
				PtrList_remove_head(&tmr_list);
			}
			else
			{
				// head entry fire time later than now. calculate time to wait.
				delta_ms = (int)((tmr_head->lt_fire % 1000) - (lt_now % 1000));
#ifdef linux
				ts.tv_sec += (int)(tmr_head->lt_fire - lt_now)/1000;
			
				ts.tv_nsec += delta_ms * 1000000;
				if ( ts.tv_nsec < 0 )
				{
					ts.tv_sec--;
					ts.tv_nsec += 1000000000;
				}
#endif
				tmr_head = NULL;		// set as NULL when no one shall be fired.
			}			
		}
		list_unlock();
		
		// if head entry need to be fired
		if ( tmr_head != NULL )
		{
			// if this is periodic timer, calculate next fired time before invoke handler
			if ( tmr_head->t_period > 0 )
				tmr_head->lt_fire = lt_now + tmr_head->t_period;
			if ( tmr_head->arg != (void *)ARG_IGNOR )
				tmr_head->handle(tmr_head->id, tmr_head->arg);
			else
				tmr_head->handle(tmr_head->id);
			// check to delete or insert back to list for next time to fire
			if ( tmr_head->count==TMR_INFINITE || --tmr_head->count > 0 )
			{
				tmr_insert(tmr_head);
			}
			else
				free(tmr_head);
		}
		else
		{
			// empty list or fire time of head timer not due yet. wait
		#ifdef linux
			pthread_mutex_lock( &w_lock );
			pthread_cond_timedwait( &w_cond, &w_lock, &ts );
			pthread_mutex_unlock( &w_lock );
		#else
			WaitForSingleObject( w_lock, INFINITE );
			WaitForSingleObject( w_cond, delta_ms );
			ReleaseMutex( w_lock );
		#endif
		}
	}
	
	in_service = 2;
	printf("------------ timer_server_fxc exit ------------\n");
	
	return NULL;
}


int TMR_initialize()
{
	printf("[Call] TMR_initialize()\n");

	in_service = 1;
#ifdef linux
	return pthread_create(&tmr_thread, NULL, timer_server_fxc, NULL);
#else
//	int nErr;
	DWORD dwThreadId;

	w_lock = CreateMutex(
				NULL,       // default security attributes
				FALSE,      // initially not owned
				NULL);		// mutex name (NULL for unname mutex)
				
	w_cond = CreateMutex(
				NULL,       // default security attributes
				FALSE,      // initially not owned
				NULL);		// mutex name (NULL for unname mutex)
				
	list_lock = CreateMutex(
				NULL,       // default security attributes
				FALSE,      // initially not owned
				NULL);		// mutex name (NULL for unname mutex)
				
	if( w_lock == NULL || w_cond == NULL || list_lock == NULL )
	{
		//nErr = (int)GetLastError();
		
		if( w_lock != NULL )
			CloseHandle( w_lock );
		
		if( w_cond != NULL )
			CloseHandle( w_cond );
		
		if( list_lock != NULL )
			CloseHandle( list_lock );
		
		return -1;
	}
	
	tmr_thread = CreateThread(
						NULL, 	// default security attributes
						0,		// use default stack size
	(LPTHREAD_START_ROUTINE)timer_server_fxc,  // thread function
						NULL,	// argument to thread function
						0,		// use default creation flags
						&dwThreadId);	// returns the thread identifier

	if( tmr_thread == NULL )
	{
		//nErr = (int)GetLastError();
		
		CloseHandle( w_lock );
		CloseHandle( w_cond );
		CloseHandle( list_lock );
		
		w_lock = w_cond = list_lock = NULL;
		
		return -1;
	}
	
	return 0;
#endif
}

void TMR_terminate()
{
	// stop working thread
#ifndef linux
	int i;
#endif
	
	in_service = 0;
	
#ifdef linux
	pthread_mutex_lock(&w_lock);
	pthread_cond_signal(&w_cond);
	pthread_mutex_unlock(&w_lock);
	pthread_join(tmr_thread,NULL);
#else
	WaitForSingleObject( w_lock, INFINITE );
	ReleaseMutex( w_cond );
	ReleaseMutex( w_lock );
	for( i=0; i<100 && in_service != 2; i++ )
	{
		Sleep(10);
	}
	
	if( in_service != 2 )
		TerminateThread( tmr_thread, 0 );
	
	if( w_lock != NULL )
		CloseHandle( w_lock );
	
	if( w_cond != NULL )
		CloseHandle( w_cond );
	
	if( list_lock != NULL )
		CloseHandle( list_lock );
	
	w_lock = w_cond = list_lock = NULL;
#endif
	// delete all timer entries in list
	PtrList_delete_all(&tmr_list);
}

static int _tmr_add(int id, tmr_hander handle_fxc, void *arg, int t_period, int count, _longtime lt_fire)
{
		tmr_entry *entry = malloc(sizeof(tmr_entry));
		if ( entry==NULL )  // out of heap memory
			return -1;
		entry->id = id;
		entry->handle = handle_fxc;
		entry->arg = arg;
		entry->count = count;
		entry->t_period = t_period;
		entry->lt_fire = lt_fire;
		tmr_insert(entry);
		// wake up working thread
	#ifdef linux
		pthread_mutex_lock(&w_lock);
		pthread_cond_signal(&w_cond);
		pthread_mutex_unlock(&w_lock);
	#else
		WaitForSingleObject( w_lock, INFINITE );
		ReleaseMutex( w_cond );
		ReleaseMutex( w_lock );
	#endif
		return 0;	
}
 
// start repeating timer, period is t_period (in msec), first fire time is after 't_period' after added
int TMR_add_repeat(int id, tmr_hander handle_fxc, int t_period, int count)
{
	int rc = -1;
	if ( tmr_find(id)==0 && t_period>0 && handle_fxc )
		rc = _tmr_add(id,handle_fxc,ARG_IGNOR,t_period,count,timeGetLongTime()+t_period);
	return rc;
}
int TMR_add_repeat_arg(int id, tmr_hander handle_fxc, void *arg, int t_period, int count)
{
	int rc = -1;
	if ( tmr_find(id)==0 && t_period>0 && handle_fxc )
		rc = _tmr_add(id,handle_fxc,arg,t_period,count,timeGetLongTime()+t_period);
	return rc;
}


// add one-short timer fired at specified time
int TMR_add_absolute(int id, tmr_hander handle_fxc, time_t t_start)
{
	int rc = -1;
	if ( tmr_find(id)==0 && t_start > time(NULL) && handle_fxc )
		rc = _tmr_add(id,handle_fxc,ARG_IGNOR,0,1,timeLongTimeFromTime(t_start));
	return rc;
}

int TMR_add_absolute_arg(int id, tmr_hander handle_fxc, void *arg, time_t t_start)
{
	int rc = -1;
	if ( tmr_find(id)==0 && t_start > time(NULL) && handle_fxc )
		rc = _tmr_add(id,handle_fxc,arg,0,1,timeLongTimeFromTime(t_start));
	return rc;
}

int TMR_kill(int id)
{
	POSITION pos;
	list_lock();
	for(pos=tmr_list.head; pos!=NULL; pos=pos->next)
	{
		tmr_entry *entry = (tmr_entry *)pos->ptr;
		if ( entry->id == id )
		{
			PtrList_delete(&tmr_list,pos);
			break;
		}
	}
	list_unlock();
	// no need to wake up working thread for one timer entry deleted. 
	return pos==NULL ? -1 : 0;
}

int TMR_has(int id)
{
	POSITION pos;
	list_lock();
	for(pos=tmr_list.head; pos!=NULL; pos=pos->next)
	{
		tmr_entry *entry = (tmr_entry *)pos->ptr;
		if ( entry->id == id )
			break;
	}
	list_unlock();
	return pos!=NULL;
}


#ifdef linux
static void init_itimer(int which)
{
    struct itimerval tmr;

	tmr.it_value.tv_sec = 86400;	// one full day. Shall be enough
	tmr.it_value.tv_usec = 0;
	tmr.it_interval.tv_sec = 86400;
	tmr.it_interval.tv_usec = 0;

	setitimer ( which, &tmr, NULL );
}

static long get_itimer( which )
{
    struct  itimerval tmr;
    long	delta_s, delta_us;

    getitimer( which, &tmr );
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
void TMR_waitfor( int msec )
{
	int msec_left = msec;
	init_itimer( ITIMER_REAL );
	do {
		usleep( msec_left*1000 );
		msec_left = msec - get_itimer(ITIMER_REAL);
	} while ( msec_left > 0 );
}
#endif
