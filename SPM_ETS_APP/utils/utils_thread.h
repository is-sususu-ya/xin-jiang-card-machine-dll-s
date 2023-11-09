/***************************************************************************
	 utils_thread.h  -  description
 *                                                                         *
 ***************************************************************************/

#ifndef _UTILS_THREAD_H_
#define _UTILS_THREAD_H_

#include <pthread.h>

// PTHREAD_THREADS_MAX = 1024
// pthread_create cannot create more thread when reach the limit,
// so i must write this package to reuse the threads.

extern int THREAD_create(pthread_t *thread,
						 pthread_attr_t *attr,
						 void *(*start_routine)(void *),
						 void *arg,
						 const char *comment);

extern int THREAD_cancel(pthread_t thread);

// initializing before call the above functions
extern int THREAD_initialize(void);

// terminating before exit process
extern int THREAD_terminate(void);

// get number of threads in pool
void THREAD_get_count(int *active_cnt, int *idle_cnt);

#endif /* _CORE_THREAD_H_ */
