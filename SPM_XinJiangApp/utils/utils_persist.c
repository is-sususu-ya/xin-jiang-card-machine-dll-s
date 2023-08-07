/*
 * source code of persist storage utility function
 * Provide a simple and general mechanism to store temporary data as persist buffer between
 * data producer and data consumer. Data will be stored in persist file and will not be lost
 * when application stopped or crashed. All data can be restored when application restarted.
 * 
 * Author: Thomas Chang
 * Date: 2018-01-08
 * Version: 1.0
 *
 * Note: to generate standalone executable code
 * gcc -g -DENABLE_PERSIST_TEST utils_persist.c -lpthread -luuid -opsist
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/file.h>
#include "utils_persist.h"
typedef int uuid_t;

typedef struct {
	int size;			// size of this structure
	
	int recsize;	// record size (64/128/256/512/1024)
	int ps_size;	// persist storage size in bytes
	int r_size;		// ring size 
	int r_head;
	int r_tail;
	int last_sn;	// sn for last record in persist
	
	// file
	char *dir;		// directory of persist storage
	char *name;		// persist storage name
	int fd;				// fd of persist storage
	char *smap;		// persist storage mapped address
	// asynchorous control 
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
	int  nr_wait;	// number of threads wait in persist_read
	int  nw_wait;	// number of threads wait in persist_write
	int	 ab2close; // persist is about to close. consumer thread shall return after awaken
} PERSIST;

#define is_valid_persist(h)		( (h) && ((PERSIST *)h)->size == sizeof(PERSIST) )
//#define NextTailPosit(hp)		( (hp)->r_tail==(hp)->r_size-1 ? 0 : (hp)->r_tail+1 )
//#define NextHeadPosit(hp)	( (hp)->r_head==(hp)->r_size-1 ? 0 : (hp)->r_head+1 )
#define IsRingEmpty(hp)		( (hp)->r_head==(hp)->r_tail )
#define IsRingFull(hp)			( NextTailPosit(hp) == (hp)->r_head )
#define RingElements(hp)		(((hp)->r_tail >= (hp)->r_head) ? ((hp)->r_tail - (hp)->r_head) : ((hp)->r_size - (hp)->r_head + (hp)->r_tail) )
#define	PrevPosit(hp,n)		( (n)==0 ? (hp)->r_size-1 : (n)-1 )
#define	NextPosit(hp,n)		( (n)==(hp)->r_size-1 ? 0 : (n)+1 )
#define NextTailPosit(hp)	NextPosit(hp,(hp)->r_tail)
#define NextHeadPosit(hp)	NextPosit(hp,(hp)->r_head)
// pos 是在ring_buf里的位置，返回是第几个数据（由head开始算是0）
#define PositIndex(hp,posit)		( (posit)==(hp)->r_tail ? -1 : ((posit)>=(hp)->r_head ? (posit)-(hp)->r_head : (((hp)->r_size-(hp)->r_head-1) + (posit))) )
// PositIndex的反操作，idx是由head开始的第几个数据（head是0），返回值是在ring_buffer里的位置。
#define IndexPoist(hp,idx)	(  (idx)+(hp)->r_head<(hp)->r_size ? (idx)+(hp)->r_head : (idx+(hp)->r_head+1-(hp)->r_size) )
	
#define RecordDataLimit(hp)					( (hp)->recsize - 3*sizeof(int) )
#define GetPersistFileSize(hp)			( ((hp)->r_size+1)*(hp)->recsize )
#define GetRecordHeadPtr(hp,pos)		(int *)( (hp)->smap + (pos)*(hp)->recsize )
#define GetRecordDataPtr(hp,pos)		( (hp)->smap + (pos)*(hp)->recsize + 3*sizeof(int))
#define GetMasterRecordPtr(hp)			( (hp)->smap + (hp)->r_size*(hp)->recsize )
#define GetMasterRecordOffset(hp)		( (hp)->r_size*(hp)->recsize )

/*
 * NOTE - master block is on last record with following field.
 * [Marker] [ring_head] [ring_tail] [last_sn]
 * where 
 * [Marker] is 4-bytes of 0xAA
 * [recsize] record size
 * [ps_size] persist storage file size
 * [r_size]  ring buffer size (number of entry)
 * [r_head] is current ring head
 * [r_tail] is current ring tail
 * [last_sn] is sn of last record in persist storage.
 */
 
#define SYNC_MODE		MS_ASYNC 	// Asynchrous mode for performance issue. Use MS_SYNC instead if application more care about data safety write
#define SZ_UUID			30			// size of uuid string (inlcude null terminator) like 1b4e28ba-2fa1-11d2-883f-b9a76
#define PATH_MAX  	256

static void make_persist_path(char *path, const char *dir, const char *name)
{
	int len;
	strcpy(path, dir);
	len = strlen(path);
	if ( path[len-1] != '/' )
		strcat(path, "/");
	strcat(path, name);
	strcat(path, ".dat");
}

static void make_recfile_path(PERSIST *hp, int pos, char *path )
{
	strcpy(path, hp->dir);
	strcat(path, GetRecordDataPtr(hp,pos));
}

static int fill_master_block( PERSIST *hp, int *mblock)
{
	mblock[0] = 0xaaaaaaaa;
	mblock[1] = hp->recsize;
	mblock[2] = hp->ps_size;
	mblock[3] = hp->r_size;
	mblock[4] = hp->r_head;
	mblock[5] = hp->r_tail;
	mblock[6] = hp->last_sn;
	return 7*sizeof(int);
}

static void psist_init_file( PERSIST *hp )
{
	char block_data[128];
	int  mblk[8];
	int	 i, nb = GetPersistFileSize(hp) / 128;
	long offset = GetMasterRecordOffset(hp);
	
	memset(block_data,0,128);
	for(i=0; i<nb; i++)
		write(hp->fd, block_data, 128);
	lseek(hp->fd, offset, SEEK_SET);
	i = fill_master_block(hp,mblk);
	write(hp->fd, mblk, i);
}

static int psist_load_mlock(PERSIST *hp)
{
	unsigned char block[1024];
	int  mblock[8];
	int i;
	
	lseek( hp->fd,-1024, SEEK_END);
	read(hp->fd, block, 1024);
	lseek(hp->fd, 0, SEEK_SET);
	for(i=0; i<1020; i++)
		if ( block[i]==0xaa && block[i+1]==0xaa && block[i+2]==0xaa && block[i+3]==0xaa )
			break;
	if ( i==1020 )
		return -1;
	memcpy(mblock, block+i, sizeof(mblock));
	hp->recsize = mblock[1];
	hp->ps_size = mblock[2];
	hp->r_size = mblock[3];
	hp->r_head = mblock[4];
	hp->r_tail = mblock[5];
	hp->last_sn = mblock[6];
	printf("load mblock, recsize=%d, ps_size=%d, r_size=%d, r_head=%d, r_tail=%d, last_sn=%d\n",
			hp->recsize, hp->ps_size, hp->r_size, hp->r_head, hp->r_tail,  hp->last_sn );
	return 0;
}

static void psist_update_mblock(PERSIST *hp)
{
	int *mblock = (int *)GetMasterRecordPtr(hp);
	int size = fill_master_block(hp,mblock);
	// sync to storage
	msync(mblock, size, SYNC_MODE);	
}
/*
static int psist_find_data(PERSIST *hp, int sn)
{
	int pos = hp->r_head;
	int *rechead;
	if ( IsRingEmpty(hp) )
		return 0;
	while( pos != hp->r_tail )
	{
		rechead = GetRecordHeadPtr(hp,pos);
		if ( (sn==0 && rechead[0]>0) || rechead[0] == sn )
			return pos;
		pos = NextPosit(hp,pos);
	}
	return 0;
}
*/

HANDLE persist_create(const char *dir, const char *name, int recsize, int maxrec)
{
	struct stat st;
	char path[PATH_MAX];
	PERSIST *hp = NULL;
	int n;
	
	if ( stat(dir, &st)!=0 )
	{
		printf("persist_create - persist storage directory %s not exist!\n", dir );
		return NULL;
	}
	if (recsize!=0 && recsize!=64 && recsize!=128 && recsize!=256 && recsize!=512 && recsize!=1024)
	{
		printf("persist_create - invalid recsize (%d)!\n", recsize);
		return NULL;
	}
	if ( maxrec < 100 )  maxrec = 100;
	make_persist_path(path, dir, name);
	hp = calloc(1,sizeof(PERSIST));
	hp->size = sizeof(PERSIST);
	if ( stat(path,&st)!=0 )
	{
		if ( recsize != 0 )
		{
			// new persist
			hp->recsize = recsize;
			hp->r_size = maxrec + 1;
			hp->ps_size = GetPersistFileSize(hp);		// must be init after hp->r_size and hp->recsize inited
			hp->fd = open(path, O_RDWR|O_CREAT, 0640);
			if ( hp->fd==-1 )
			{
				printf("persist_create - failed to create persist storage file %s, err=%s\n", path, strerror(errno));
				goto create_error;
			}
			psist_init_file(hp);
		}
		else
		{
			printf("persist_create - persist storage not exist!\n");
			goto create_error;
		}
	}
	else 
	{
		// open exist persist storage
		hp->fd = open(path, O_RDWR);
		if ( hp->fd==-1 )
		{
			printf("persist_create - failed to open persist storage file %s, err=%s\n", path, strerror(errno));
			goto create_error;
		}
		if ( psist_load_mlock(hp)==-1 )
		{
			printf("persist_create - failed to load master block of persist storage file %s!\n", path);
			goto create_error;
		}
	}
	// try to lock file - prevent multiple open
	if ( flock(hp->fd, LOCK_EX|LOCK_NB)==-1 )
	{
		if ( errno==EWOULDBLOCK )
			printf("persist_create - persist file (%s) is locked by other process!\n", path);
		else
			printf("persist_create - failed to lock persist file %s, err=%s\n", path, strerror(errno));
		goto create_error;
	} 
	// mapping persist storage into memory
	hp->smap = mmap(0, hp->ps_size, PROT_READ|PROT_WRITE, MAP_SHARED, hp->fd, 0);
	if ( hp->smap==NULL )
	{
		printf("persist_create - failed to map persist storage into memory, err=%s\n", strerror(errno));
		goto create_error;
	}
	// init mutex and conditon
	pthread_mutex_init(&hp->mutex,NULL);
	pthread_cond_init(&hp->cond,NULL);
	strcpy(path, dir);
	n = strlen(path);
	if ( path[n-1] != '/' )
		strcat(path, "/");
	hp->dir = strdup(path);
	hp->name = strdup(name);
	return hp;
	
create_error:
	if ( hp->smap != NULL )
		munmap(hp->smap, hp->ps_size);
	if ( hp->fd != -1 )
		close(hp->fd);
	free(hp);
	return NULL;	
}

int persist_terminate(HANDLE h)
{
	PERSIST *hp = (PERSIST *)h;
	int i;
	
	if ( !is_valid_persist(h) )
		return -1;
	hp->ab2close = 1;
	for( i=0; hp->nr_wait+hp->nw_wait > 0 && i<100; i++)
	{
		pthread_mutex_lock(&hp->mutex);
		pthread_cond_signal(&hp->cond);
		pthread_mutex_unlock(&hp->mutex);
		usleep(1000);
	}
	// release all resource
	pthread_mutex_destroy(&hp->mutex);
	pthread_cond_destroy(&hp->cond);
	if ( hp->smap)
		munmap(hp->smap, hp->ps_size);
	if ( hp->fd )
		close(hp->fd);
	free(hp->dir);
	free(hp->name);
	free(hp);
	return 0; 
}

int persist_reset(HANDLE h)
{
	PERSIST *hp = (PERSIST *)h;
	char path[PATH_MAX];
	int i;
	
	if ( !is_valid_persist(h) )
		return -1;
	pthread_mutex_lock(&hp->mutex);
	i = hp->r_head;
	while( i!=hp->r_tail)
	{
		int *rechead = GetRecordHeadPtr(hp,i);
		if ( rechead[2] < 0 )
		{
			make_recfile_path(hp,i,path);
			unlink(path);
		}
		memset(rechead, 0, hp->recsize);
		i = NextPosit(hp,i);
	}
	// reset ring pointer
	hp->r_head = hp->r_tail = 0;
	hp->last_sn = 0;
	psist_update_mblock(hp);
	pthread_mutex_unlock(&hp->mutex);
	return 0;
}

int persist_destroy(const char *dir, const char *name)
{
	PERSIST *hp = persist_create(dir,name,0,0);
	if ( hp != NULL )
	{
		char path[PATH_MAX];
		persist_reset(hp);
		persist_terminate(hp);
		make_persist_path(path, dir, name);
		unlink(path);
		return 0;
	}
	printf("persist_destroy - persist storage not found!\n");
}

int persist_count(HANDLE h)
{
	PERSIST *hp = (PERSIST *)h;
	int count;
	
	if ( !is_valid_persist(h) )
		return -1;
	pthread_mutex_lock(&hp->mutex);
	count = RingElements(hp);
	pthread_mutex_unlock(&hp->mutex);
	return count;	
}


int persist_write(HANDLE h, int mode, void *data, int size)
{
	PERSIST *hp = (PERSIST *)h;
	int *rechead;
	void *recdata;
	int rec_size;
	int sn;
	
	if ( !is_valid_persist(h) )
		return -1;
	
	pthread_mutex_lock(&hp->mutex);

	while ( IsRingFull(hp) && !hp->ab2close )
	{
		if ( mode==0 )
		{
			pthread_mutex_unlock(&hp->mutex);
			return 0;
		}
		// wait for signal send by consumer thread
		hp->nw_wait++;
		pthread_cond_wait(&hp->cond, &hp->mutex);
		hp->nw_wait--;
	}
	if ( hp->ab2close )
	{
		pthread_mutex_unlock(&hp->mutex);
		return 0;
	}
	rechead = GetRecordHeadPtr(hp,hp->r_tail);
	recdata = GetRecordDataPtr(hp,hp->r_tail);
	hp->last_sn++;
	if (hp->last_sn < 0)
		hp->last_sn = 1;
	rechead[0] = sn = hp->last_sn;
	rechead[1] = time(NULL);
	rechead[2] = size > RecordDataLimit(hp) ? -SZ_UUID : size;
	if ( rechead[2] < 0 )
	{
		// large record, write to a separate file
		char path[PATH_MAX];
    uuid_t uuid;
    char str_uuid[32];
    int fd;
    int rc=0;
    // geneata a uuid string as file name and store in recdata field
    uuid_generate(uuid);
    uuid_unparse(uuid, str_uuid);
		strcpy(recdata, str_uuid);
		// create uuid file and write real record data in file
		strcpy(path, hp->dir);
		strcat(path, str_uuid);
		fd = open(path, O_WRONLY|O_TRUNC|O_CREAT, 0644);
		if (fd == -1)
		{
			printf("persist_write - failed to create separated record file %s - %s\n", path, strerror(errno) );
			pthread_mutex_unlock(&hp->mutex);
			return -1;
		}
		rc = write(fd, data, size);
		close(fd);
		if ( rc != size )
		{
			printf("persist_write - failed to write record file %s - (%s)\n", path, strerror(errno) );
			unlink(path);
			pthread_mutex_unlock(&hp->mutex);
			return -1;
		}
	}
	else
		memcpy( recdata, data, size);
	rec_size = sizeof(int)*3 + (rechead[2] > 0 ? rechead[2] : SZ_UUID);
	msync(rechead, rec_size, SYNC_MODE);
	hp->r_tail = NextTailPosit(hp);
	psist_update_mblock(hp);
	
	// wake-up any consumer who is blocked due to empty ring
	if ( hp->nr_wait > 0 )
		pthread_cond_signal(&hp->cond);
		
	pthread_mutex_unlock(&hp->mutex);
	return sn;
}

int persist_read(HANDLE h, int mode, void **data, int *size, time_t *datatime)
{
	PERSIST *hp = (PERSIST *)h;
	int *rechead;
	char *recdata;
	char *rec_alloc=NULL;
	int rec_size=0;
	int sn = 0;
	
	if ( !is_valid_persist(h) )
		return -1;
	
	pthread_mutex_lock(&hp->mutex);
	while ( IsRingEmpty(hp) && !hp->ab2close )
	{	
		if ( mode==0 )
		{
			pthread_mutex_unlock(&hp->mutex);
			return 0;
		}
		hp->nr_wait++;
		pthread_cond_wait(&hp->cond, &hp->mutex);
		hp->nr_wait--;
	}
	if ( hp->ab2close )
	{
		pthread_mutex_unlock(&hp->mutex);
		return 0;
	}
	rechead = GetRecordHeadPtr(hp,hp->r_head);
	recdata = GetRecordDataPtr(hp,hp->r_head);
	sn = rechead[0];
	if ( rechead[2] < 0 )
	{
		int fd;
		struct stat st;
		char path[PATH_MAX];
		
		strcpy(path, hp->dir);
		strcat(path, recdata);
		fd = open(path, O_RDONLY);
		if ( fd==-1 )
			printf("persist_read - failed to open record data file %s - %s\n", path, strerror(errno));
		else
		{
			fstat(fd, &st);
			rec_size = st.st_size;
			rec_alloc = malloc(rec_size);
			if ( rec_alloc==NULL )
			{
				printf("persist_read - system error, out of memory!\n");
				pthread_mutex_unlock(&hp->mutex);
				return -1;
			}
			else
				read(fd,rec_alloc,rec_size); 
			close(fd);
		}
	}
	else
	{
		rec_size = rechead[2];
		rec_alloc = malloc(rec_size);
		if ( rec_alloc==NULL )
		{
			printf("persist_read - system error, out of memory!\n");
			pthread_mutex_unlock(&hp->mutex);
			return -1;
		}
		else
			memcpy(rec_alloc, recdata, rec_size); 
	}
	pthread_mutex_unlock(&hp->mutex);
	*data = rec_alloc;
	*size = rec_size;
	if (datatime)
		*datatime = rechead[1];
	return sn;
}

int persist_purge(HANDLE h, int sn)
{
	PERSIST *hp = (PERSIST *)h;
	int *rechead;
	char *recdata;
	int rc=-1;
		
	if ( !is_valid_persist(h) )
		return -1;
	
	pthread_mutex_lock(&hp->mutex);
	if ( !IsRingEmpty(hp) )
	{
		rechead = GetRecordHeadPtr(hp,hp->r_head);
		recdata = GetRecordDataPtr(hp,hp->r_head);
		if ( rechead[0]==sn )
		{
			if ( rechead[2] < 0 )
			{
				// this is a long record, unlink record data file
				char path[PATH_MAX];
				strcpy(path, hp->dir);
				strcat(path, recdata);
				if ( unlink(path) != 0 )
					printf("persist_purge - failed to delete record data file %s (%s)\n", path, strerror(errno));
			}
			hp->r_head = NextHeadPosit(hp);
			memset(rechead,0,hp->recsize);
			psist_update_mblock(hp);
			rc = 0;
			// wake up producer who is blocked due to ring full
			if ( hp->nw_wait > 0 )
				pthread_cond_signal(&hp->cond);
		}
		else
			printf("persist_purge - sn (%d) is not ring-head record sn (%d). ignored!\n", sn, rechead[0] );
			
	}
	else
		printf("persist_purge - sn not found, ignored!\n");	

	pthread_mutex_unlock(&hp->mutex);
	return rc;
}

int persist_param(HANDLE h, int *recsize, int *maxrec)
{
	PERSIST *hp = (PERSIST *)h;
		
	if ( !is_valid_persist(h) )
		return -1;
	*recsize = hp->recsize;
	*maxrec = hp->r_size - 1;
	return 0;
}

int persist_extend(HANDLE h, int new_maxsize)
{
	PERSIST *hp = (PERSIST *)h;
	PERSIST *hp_new;
	char path[PATH_MAX], path_new[PATH_MAX];
	int *rechead, *rechead_new;
	int pos, pos_new;
	
	if ( !is_valid_persist(h) )
		return -1;
	
	if (new_maxsize < RingElements(hp) )
		return -1;
		
	pthread_mutex_lock(&hp->mutex);
	hp_new = persist_create(hp->dir, "__TEMPDATA__", hp->recsize, new_maxsize);
	if ( hp_new==NULL )
	{
		pthread_mutex_unlock(&hp->mutex);
		return -1;
	}
	
	// copy data
	for(pos_new=0,pos=hp->r_head; pos!=hp->r_tail; pos_new++)
	{
		rechead = GetRecordHeadPtr(hp,pos);
		rechead_new = GetRecordHeadPtr(hp_new,pos_new);
		memcpy(rechead_new, rechead, hp->recsize);
		pos = NextPosit(hp,pos);
	}
	hp_new->r_head = 0;
	hp_new->r_tail = pos_new;
	hp_new->last_sn = hp->last_sn;
	psist_update_mblock(hp_new);
	if ( RingElements(hp_new) > 0 )
		msync(hp_new->smap, RingElements(hp_new)*hp_new->recsize, SYNC_MODE);
	
	make_persist_path(path, hp->dir, hp->name);
	make_persist_path(path_new, hp_new->dir, hp_new->name);
	
	// close and unmap file
	munmap(hp->smap, hp->ps_size);
	close(hp->fd);
	munmap(hp_new->smap, hp_new->ps_size);
	hp_new->smap = NULL;
	close(hp_new->fd);
	hp_new->fd = 0;
	// rename file
	unlink(path);
	rename(path_new,path);
	
	// open and map again
	hp->fd = open(path, O_RDWR);
	flock(hp->fd, LOCK_EX|LOCK_NB);
	hp->r_head = 0;
	hp->r_tail = pos_new;
	hp->r_size = new_maxsize + 1;
	hp->ps_size = GetPersistFileSize(hp);
	hp->smap = mmap(0, hp->ps_size, PROT_READ|PROT_WRITE, MAP_SHARED, hp->fd, 0);
	
	// terminate temporary persist 
	persist_terminate(hp_new);
	
	// if any producer is waiting for free space, wake it up
	if ( hp->nw_wait > 0 )
		pthread_cond_signal(&hp->cond);

	pthread_mutex_unlock(&hp->mutex);
	
}


////////////////////////////////////////////////////////////////////
#ifdef ENABLE_PERSIST_TEST
#include <signal.h>

#define PERSIST_DIR		"./persistdata/"
#define PERSIST_NAME  "test"
#define REC_SIZE			64
#define REC_MAX				128
#define MAX_WRITER		10

#define HiRand(lo,hi)		( (lo) + (int)(((hi)-(lo)) * (((float)rand()) / RAND_MAX) ) )
static void *persist_handle = NULL;
static int run_test = 1;
static int faster_reader = 0;

int generate_data(char *data)
{
	int i, len;
	if ( rand()%2 )
	{
		// produce data > REC_SIZE
		len = HiRand(REC_SIZE,255);
		for(i=0; i<len; i++)
			data[i] = HiRand(32,126);
	}
	else
	{
		// produce data < REC_SIZE
		len = HiRand(16,REC_SIZE);
		for(i=0; i<len; i++)
			data[i] = HiRand(32,126);
	}
	data[len] = '\0';	
	return len+1;
}

void * thread_producer(void *param)
{
	PERSIST *hp = (PERSIST *)param;
	char buf[256];
	int sn, size;
	pthread_t tid = pthread_self();
	
	for(;run_test && !hp->ab2close; )
	{
		size = generate_data(buf);
		if ( run_test )
		{
			sn = persist_write(hp, 1, buf, size);
			if ( sn > 0 )
			{
				buf[32] = '\0';
				printf("producer[%lu]: sn=%d, size=%d (%s ...)\n", tid, sn, size, buf);
				usleep(HiRand(0,1000)*1000);		// sleep 0~1000 msec
			}
		}
	}
	printf("PRODUCER THREAD END! (%lu)\n", tid);
	return NULL;
}

void *thread_consumer(void *param)
{
	PERSIST *hp = (PERSIST *)param;
	char *data;
	int sn, size;
	int	delay_factor = faster_reader ? 100 : 1000;
		
	while( run_test && (sn=persist_read(hp,1,(void **)&data,&size,NULL)) > 0 )
	{
		// process data
		// ...
		if ( size > 32 )
			data[32] = '\0';
		printf("CONSUMER: sn=%d, size=%d, data=%s ...\n", sn, size, data);
		free(data);
		persist_purge(hp,sn);
		printf("record left in persist storage = %d\n", persist_count(hp));
		usleep(HiRand(0,1000)*delay_factor);		// sleep 0~1000 msec or 0 ~ 100 msec (faster reader)
	}
	printf("CONSUMER THREAD END!\n");
	return NULL;
}
//--------------------------------------------------
static void signal_quit(int signo)
{
	run_test = 0;
}

/*
 * persist_storage tester
 * usage:
 *  -a<step> - auto extend. When ring is full, automatically extend the maxrec by <step> records.
 *  -w<n> - number of data producer [1~10] default is 1
 *  -s<x> - record size, default is 64
 *  -m<y> - maximum number of record, default is 128
 *  -d<dir> - persist storage directory (defaut is ./persistdata/)
 *  -f - faster reader
 *  -n<name> - persist storage name, default is "test".
 *  -D - destroy persist storage
 *  -p - obtain persist storage parameter
 *  -e<z> - extend persist storage capacity to new maximum size <z>
 */
int main(int argc, char * const argv[])
{
	int opt;
 	int i, nw=1;
 	int destroy=0;
 	int auto_extend = 0;
 	int info = 0;
 	int extend = 0;
 	const char *dir = PERSIST_DIR;
 	const char *name = PERSIST_NAME;
 	int recsize = REC_SIZE;
 	int maxrec = REC_MAX;
 	int new_max;
 	struct stat st;
  // parse run time argumnets
  while ( (opt=getopt(argc, argv, "Dpa:e:fw:s:m:d:n:")) != -1 )
	{
		switch (opt)
		{
			case ':':
				printf("missing required parameter for command option %c.\n", opt);
				return -1;
			case '?':
				printf("unknown option %c, ignored\n", optopt );
				break;
			case 'D':
				destroy = 1;
				break;
			case 'a':
				auto_extend = atoi(optarg);
				break;
			case 'p':
				info = 1;
				break;
			case 'd':
				dir = strdup(optarg);
				break;
			case 'e':
				extend = 1;
				new_max = atoi(optarg);
				break;
			case 'f':
				faster_reader = 1;
				break;
			case 's':
				recsize = atoi(optarg);
				break;
			case 'm':
				maxrec = atoi(optarg);
				break;
			case 'n':
				name = strdup(optarg);
				break;
			case 'w':
				nw = atoi(optarg);
				break;
		}							 
	}
	if ( nw > MAX_WRITER )
		nw = MAX_WRITER;
		
	//printf("arguments: persist dir=%s, name=%s, recsize=%d, maxrec=%d, destroy=%s, producer=%d\n",
	//	dir, name, recsize, maxrec, destroy ? "yes" : "no", nw);
					
	if (destroy)
	{
		persist_destroy(dir, name);
		return 0;
	}
			
	sigset(SIGINT, signal_quit);
	sigset(SIGTERM, signal_quit);
	
	if ( lstat(dir,&st)!=0 && mkdir(dir, 0777)!=0 )
	{
		printf("persist directory (%s) not exist and failed to create!\n", dir);
		return 0;
	}
	persist_handle = persist_create(dir, name, recsize, maxrec);
	if ( persist_handle==NULL )
		printf("Failed to create/load persist!!!\n");
	else if ( info )
	{
		persist_param(persist_handle, &recsize, &maxrec);
		printf("persist storage record size=%d, maxrec=%d\n", recsize, maxrec);
		persist_terminate(persist_handle);
	}
	else if ( extend )
	{
		if ( new_max <= persist_count(persist_handle) )
			printf("error - persit storage current elements=%d, new maxrec must be greater than it!\n", persist_count(persist_handle));
		else
		{
			persist_param(persist_handle, &recsize, &maxrec);
			printf("extend persist storage record number %d -> %d\n", maxrec, new_max);
			persist_extend(persist_handle, new_max);
		}
		persist_terminate(persist_handle);
	}
	else
	{
		// create roducer and consumer thread
		pthread_t tid_producer, tid_consumer;
		srand(time(NULL));
		for(i=0; i<nw; i++)
			pthread_create(&tid_producer, NULL, thread_producer, persist_handle);
		pthread_create(&tid_consumer, NULL, thread_consumer, persist_handle);

		for(;run_test;)
		{
			if ( auto_extend > 0 )
			{
				persist_param(persist_handle, &recsize, &maxrec);
				if ( persist_count(persist_handle) == maxrec && maxrec < 1280 )
				{
					maxrec += auto_extend;
					printf("[AUTO EXTEND] - ring buffer is full, extend limit to %d\n", maxrec );
					persist_extend(persist_handle, maxrec);
				}
			}
			usleep(100);
		}
			
		persist_terminate(persist_handle);
		sleep(1);		// wait for all producers and consumer thread stop
	}
}

#endif
