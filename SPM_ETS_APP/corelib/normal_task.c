#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <stdint.h>

#include "nromal_task.h"
#include "utils_ptrlist.h"  

#define DLLEXPORT
#define CALLTYPE

typedef struct tagTaskCont
{
    int type;
    int param;
    void *msg;
    int size;
}TaskCont;
 
static pthread_t task_thread;
static int b_quit = 0;
static PtrList task_list = PTRLIST_INITIALIZER;
static pthread_mutex_t data_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t thread_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t thread_cond = PTHREAD_COND_INITIALIZER;
   
#define DataLock()  pthread_mutex_lock( &data_lock )
#define DataUnLock()  pthread_mutex_unlock( &data_lock )
 
static void task_wait(int sec)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += sec; 
	pthread_mutex_lock(&thread_lock);
	pthread_cond_timedwait(&thread_cond, &thread_lock, &ts);
	pthread_mutex_unlock(&thread_lock);
}

static void task_wakeup()
{
	pthread_mutex_lock(&thread_lock);
	pthread_cond_broadcast( &thread_cond );
	pthread_mutex_unlock(&thread_lock);
}

extern void LED_Display(const char *text);

static void *normal_task(void *arg)
{
    int index;
    TaskCont *t;
    while(!b_quit)
    {
        if( task_list.count == 0 )
        {
            task_wait( 2 );
            continue;
        }
        DataLock();
        t = PtrList_remove_head( &task_list );
        DataUnLock();
        switch( t->type )
        {
            case TASK_LED_SHOW:
            {
                struct task_content *led = (struct task_content*)t->msg;
                LED_Display(led->text);
            }
            break;
            case TASK_SOUND_PLAY:
            {     
                struct task_content *led = (struct task_content*)t->msg;
                LED_Display(led->text);
            }
            break;
            default:
            break;
        }
        if( t->msg )
            free( t->msg );
        free(t);
    } 
}

void task_add( int type, int param, void *msg, int size )
{ 
    TaskCont *t = malloc( sizeof(TaskCont ) ); 
    memset(t, 0, sizeof(TaskCont));
    if( size > 0 && msg )
    {
        t->msg = malloc( size );
        memcpy( t->msg, msg, size );
    }
    t->size = size;
    t->param = param;
    t->type = type;
    DataLock();
    PtrList_append( &task_list, t );
    DataUnLock(); 
    task_wakeup(); 
}

void task_init()
{
    pthread_create( &task_thread, NULL, normal_task, NULL );
}

void task_exit()
{
    b_quit = 1;
    pthread_cancel(&task_thread);
    pthread_join(&task_thread, NULL );
}