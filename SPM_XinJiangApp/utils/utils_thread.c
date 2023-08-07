/***************************************************************************
    utils_thread.c  -  description
 
 Simple thread pool manager can delight the extensive thread create/destroy operation.
 Application needs extensive thread creation/destroy operation with each thread run for
 very short period of time, thiis thread utility function would help. Good example is http server.
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#include "utils_thread.h"
#include "utils_ptrlist.h"

// PTHREAD_THREADS_MAX = 1024
// pthread_create cannot create more thread when reach the limit,
// so i must write this package to reuse the threads.

static pthread_mutex_t	THREAD_lock = PTHREAD_MUTEX_INITIALIZER;
static PtrList		THREAD_active_threads = PTRLIST_INITIALIZER;
static PtrList		THREAD_idle_threads = PTRLIST_INITIALIZER;

typedef struct tagThreadInfo {
	pthread_t		thread;
	pthread_mutex_t		lock;
	pthread_cond_t		cond;

	int			exit;
	int			idle;
	void*			(*start_routine)( void * );
	void*			arg;
	char* 		comment;
	pid_t			pid;
} ThreadInfo;

// default start routine, thread will not exit after create,
// either in active state ( running user's start routine ),
// or sleep in idle list waiting for summon.
void* THREAD_start_routine( void * arg )
{
	ThreadInfo	* ti = arg;
	POSITION	pos = NULL;

	pthread_mutex_lock( & THREAD_lock );
	ti->thread = pthread_self();
	ti->pid = getpid();
	pthread_mutex_unlock( & THREAD_lock );

	for(;;) {
		if( ti->exit ) break;

		// call user's start routine
		if( ti->start_routine != NULL ) {
			ti->start_routine( ti->arg );
		}

		// we need at least one cancel point for safe calling of pthread_cancel()
		pthread_testcancel();
		if( ti->exit ) break;

		// after doing the work, move the thread into the idle list.
		pthread_mutex_lock( & THREAD_lock );
		pos = PtrList_find(& THREAD_active_threads, ti );
		if( pos ) {
			PtrList_remove( & THREAD_active_threads, pos );
			ti->start_routine = NULL;
			ti->arg = NULL;
			if( ti->comment ) {
				free( ti->comment );
				ti->comment = NULL;
			}
			ti->idle = 1;
			PtrList_append( & THREAD_idle_threads, ti );
		}
		pthread_mutex_unlock( & THREAD_lock );

		// wait in queue of idle threads, wait for summon.
		if( pos ) 
		{
			pthread_mutex_lock( & ti->lock );
			pthread_cond_wait( & ti->cond, & ti->lock );
			pthread_mutex_unlock( & ti->lock );
		}
	}

	// thread exit, free the resources if needed.
	pthread_mutex_lock( & THREAD_lock );
	if( (pos = PtrList_find(& THREAD_active_threads, ti )) != NULL ) 
	{

		PtrList_remove( & THREAD_active_threads, pos );

		pthread_mutex_destroy( & ti->lock );
		pthread_cond_destroy( & ti->cond );
		if( ti->comment ) free( ti->comment );
		free( ti );

	} 
	else if( (pos = PtrList_find(& THREAD_idle_threads, ti )) != NULL ) 
	{

		PtrList_remove( & THREAD_idle_threads, pos );

		pthread_mutex_destroy( & ti->lock );
		pthread_cond_destroy( & ti->cond );
		if( ti->comment ) free( ti->comment );
		free( ti );
	}
	pthread_mutex_unlock( & THREAD_lock );

	return NULL;
}

int THREAD_create( pthread_t * thread,
	pthread_attr_t * attr, void * (*start_routine)(void *), void * arg,
	const char * comment )
{
	int 		ret;
	ThreadInfo	* ti = NULL;

	pthread_mutex_lock( & THREAD_lock );
	if( THREAD_idle_threads.count > 0 ) {

		// summon 1 thread from idle list.
		ti = PtrList_remove_head( &THREAD_idle_threads );
		ti->idle = 0;
		ti->start_routine = start_routine;
		ti->arg = arg;
		ti->comment = comment ? strdup(comment) : NULL;

		// activate it to do this job
		PtrList_append( & THREAD_active_threads, ti );

		pthread_mutex_lock( & ti->lock );
		pthread_cond_signal( & ti->cond );
		pthread_mutex_unlock( & ti->lock );

		* thread = ti->thread;

		ret = 0;
	} else {
		ti = malloc( sizeof(ThreadInfo) );
		if( ti ) {
			// allocate some resources
			memset( ti, 0, sizeof(ThreadInfo) );
			//ti->idle = 0;
			//ti->exit = 0;
			pthread_mutex_init( & ti->lock, NULL );
			pthread_cond_init( & ti->cond, NULL );
			ti->start_routine = start_routine;
			ti->arg = arg;
			ti->comment = comment ? strdup(comment) : NULL;

			// create thread, use our own start routine function as entry,
			// will call user's start routine in ours
			ret = pthread_create( thread, attr, THREAD_start_routine, ti );
			if( ret == 0 ) {
				ti->thread = * thread;
				// Thread never exit once borne, so its no point to set detach
				// pthread_detach( *thread );
				PtrList_append( &THREAD_active_threads, ti );
			} else {
				pthread_mutex_destroy( & ti->lock );
				pthread_cond_destroy( & ti->cond );
				if( ti->comment ) free( ti->comment );
				free( ti );
				ti = NULL;
			}
		} else {
			ret = ENOMEM;
		}
	}
	pthread_mutex_unlock( & THREAD_lock );

	return ret;
}

int THREAD_cancel( pthread_t thread )
{
	ThreadInfo	* ti = NULL;
	POSITION 	pos = NULL, pos_current = NULL;
	PtrList		* list = & THREAD_active_threads;
	int		ret = ESRCH;

	pthread_mutex_lock( & THREAD_lock );
	for(pos = PtrList_get_head(list); pos != NULL; /**/ ) {
		pos_current = pos;
		pos = pos_current->next;

		ti = (ThreadInfo *)pos_current->ptr;
		if( ti->thread == thread ) {
			ret = pthread_cancel( thread );
			PtrList_remove( list, pos_current );
			pthread_mutex_destroy( & ti->lock );
			pthread_cond_destroy( & ti->cond );
			if( ti->comment ) free( ti->comment );
			free( ti );

			break;
		}
	}
	pthread_mutex_unlock( & THREAD_lock );

	return ret;
}

// initializing before call the above functions
int THREAD_initialize( void )
{
	// Nothing to do so far
	// create 10 thread in idle
	int i;
	pthread_t tid;
	static int has_init = 0;
	
	for(i=0; i<10; i++)
		THREAD_create(&tid,NULL,NULL,NULL,NULL);
	return 0;
}

// terminating before exit process
int THREAD_terminate( void )
{
	ThreadInfo	* ti = NULL;
	POSITION 	pos = NULL;
	PtrList		* list = & THREAD_idle_threads;

	pthread_mutex_lock( & THREAD_lock );
	for(pos=list->head; pos!=NULL; pos=pos->next ) 
	{
		ti = (ThreadInfo *)pos->ptr;
		ti->exit = 1;

		// wake up idle threads
		pthread_mutex_lock( & ti->lock );
		pthread_cond_signal( & ti->cond );
		pthread_mutex_unlock( & ti->lock );
	}
	pthread_mutex_unlock( & THREAD_lock );

	return 0;
}

void THREAD_get_count( int *active, int *idle )
{
	pthread_mutex_lock( & THREAD_lock );
	*active = THREAD_active_threads.count;
	*idle = THREAD_idle_threads.count;
	pthread_mutex_unlock( & THREAD_lock );
}
