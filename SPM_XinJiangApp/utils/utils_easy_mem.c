#include "./utils_easy_mem.h"

#ifdef ENABLE_DBGMEMLEAK
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <linux/types.h>
#include <pthread.h>
#include <utils_ptrlist.h>


static PtrList MemList = PTRLIST_INITIALIZER;
static pthread_mutex_t	MemList_lock = PTHREAD_MUTEX_INITIALIZER;	
#define MEMList_Lock()		pthread_mutex_lock(&MemList_lock)
#define MEMList_Unlock()	pthread_mutex_unlock(&MemList_lock)

typedef struct tagMemNode {
	char name[64];
	int line;
	void *ptr;
	size_t size;
} MemNode;

static MemNode *mem_find( void *ptr )
{
	if( ptr == NULL ) return NULL;
	POSITION pos;
	MemNode *node;
	for( pos=MemList.head; pos!=NULL; pos=pos->next)
	{
		node = pos->ptr;
		if ( node->ptr == ptr ) 
			return node;
	}
	return NULL;
}

void *mem_alloc( const char *name, int line, size_t size )
{
	void *ptr = malloc(size);
	if ( ptr != NULL )
	{
		MemNode *node = malloc(sizeof(MemNode));
		node->ptr = ptr;
		strncpy( node->name, name, sizeof( node->name ));
		node->line = line;
		node->size = size;
		MEMList_Lock();
		PtrList_append( &MemList, node );
		MEMList_Unlock();
	}
	return ptr;
}

void *mem_realloc( const char *name, int line, void *ptr, size_t new_size )
{
	MemNode *node = NULL;
	MEMList_Lock();
	node = mem_find(ptr);
	MEMList_Unlock();
	if ( ptr != NULL && node )
	{
		if ( new_size > 0 )
		{
			void *new_ptr = realloc( ptr, new_size );
			if ( new_ptr != NULL )
			{
				strncpy( node->name, name, sizeof( node->name ));
				node->line = line;
				node->size = new_size;
				node->ptr = new_ptr;
			}
		}
		else
		{
			mem_free(ptr);
		}
	}
	else
	{
		return mem_alloc( name, line, new_size );
	}
}

void mem_free( void *ptr )
{
	POSITION pos;
	MemNode *node;

	free( ptr );
	
	MEMList_Lock();
	node = mem_find(ptr);
	if ( (node) == NULL )
		;//	printf("MEM_FREE WARNING: addr %p not in list. free it anyway\n", ptr );
	else
	{
		pos = PtrList_find(&MemList,node);
		PtrList_delete(&MemList,pos);
	}
	MEMList_Unlock();
}

void mem_mark()
{
	MEMList_Lock();
	PtrList_remove_all(&MemList);
	MEMList_Unlock();
}

int mem_chkleak(void (logger)(const char *fmt,...))
{
	size_t total_leak=0;
	POSITION pos;
	printf("????????????????  CHECK MEMORY LEAKAGE ????????????????\n");
	MEMList_Lock();
	for(pos=MemList.head; pos!=NULL; pos=pos->next)
	{
		MemNode *node = pos->ptr;
		printf("Leaked memory: ptr=%p, file=%s,  line:%d size=%d\n",\
				node->ptr, node->name, node->line,  node->size );
		if ( logger!=NULL)
			logger("Leaked memory: ptr=%p, file=%s,  line:%d size=%d\n",\
				node->ptr, node->name, node->line,  node->size );
		total_leak += node->size;
	}
	MEMList_Unlock();
	printf("Total leakage = %d\n", total_leak );
	if ( logger )
		logger("Total leakage = %d\n", total_leak );
	return total_leak;
}
#endif
