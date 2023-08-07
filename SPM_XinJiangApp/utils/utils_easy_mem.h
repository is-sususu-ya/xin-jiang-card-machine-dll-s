
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>

// #define ENABLE_DBGMEMLEAK

#ifndef _UTILS_MEM_
#define _UTILS_MEM_

#ifdef ENABLE_DBGMEMLEAK
extern void *mem_alloc( const char *name, int line, size_t size );
extern void *mem_realloc( const char *name, int line, void *ptr, size_t new_size );
extern void mem_free( void *ptr );
extern void mem_mark();				// forget all memoried malloc or realloc data. 
extern int mem_chkleak(logger_fxc);			// check all memory chunk not free
#define MALLOC( s )	 		mem_alloc( (__FILE__) , (__LINE__), (s) )
#define REALLOC(p,s)		mem_realloc(( __FILE__) ,( __LINE__), (p), (s) )
#define FREE(p)				mem_free(p)
#define MEM_MARK()			mem_mark()
#define MEM_CHKLEAK(lf)		mem_chkleak(lf)

#else

#define MALLOC( s )		malloc(s)
#define REALLOC(p,s)	realloc(p,s)
#define FREE(p)			free(p)
#define MEM_MARK()
#define MEM_CHKLEAK(logger_fxc)	(0)

#endif

#endif
