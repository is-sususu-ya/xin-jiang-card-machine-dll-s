#ifndef _UTILS_PERSIST_H_
#define _UTILS_PERSIST_H_
/*
 * persist storage
 * 很多时候，应用程序需要上传交易数据到远端。为了避免远程传输堵塞正常工作，程序一般会区分为
 * “数据生产线程”和“数据消耗线程”。数据生产线程产生的实时交易数据交给数据消耗线程发送到远方。
 * 为了避免程序退出时，还没传输完的数据丢失，程序需要将数据保存在本地储存装置中。这样可以保证
 * 下次程序启动后，这些数据还可以被传送到远端处理。
 * 这组通用函数就是设计一个机制可以方便的实现这个功能。“数据生产者”只需负责把数据往persist storage
 * 写入；“数据消耗者”只需要读取persist storage. 如果没有数据，线程suspend（如果使用堵塞模式），
 * 有数据可以读取的时候会醒来。当数据处理完后，再将它由persist storage里面移除。
 * 当程序启动的时候，persist storage会加载储存体的内容（实际不是加载，只是把文件映射到RAM）。如果有
 * 之前没有消耗完（没有处理完）的数据，数据消耗线程可以继续处理。
 * persist storage是通用机制，不管储存的数据是固定长度或是可变长度，都无所谓。长度大小也没有限制。
 * 当然一般也不会储存特别大的数据进去。毕竟它的设计目的是保存管理需要上传的交易数据。
 * persist store 并不能保证数据在掉电过程不会丢失，只是可以减小丢失的可能。
 */
#ifdef __cplusplus
extern "C" {
#endif

typedef void *  HANDLE;

/*
 * persist_create create a persist safe storage. 'dir' is the path in file system
 * to store the data. 'name' is name of the persist data storage. 'recsize' is record
 * size which shall be one of 64/128/256/512/1024. 'maxrec' is maximum number of records 
 * allowed to hold in the persist storage. if 0 is given for 'maxrec',default value (1024) will apply. 
 * <dir>/<name>.dat is the data storage. each record has 'recsize' bytes which are organized as:
 * 'sn' 'time-stamp' 'size' 'data/uuid'
 * where
 *   'sn' is serial number of record start from 1
 *   'time-stamp' is time_t type which is time when data are write into persist.
 *	'size' is number of bytes of this record
 *   'data/uuid' - for data over 'recsize'-12 bytes, data is stored in a separated file with
 *   a unique uuid string as file name and this file name is stored in 'data/uuid' field.
 *   for data less or equal 'recsize'-12, data itself is stored in 'data/uuid' field of record.
 *
 * The persist file is organized as a ring buffer and mapped into process address space
 * for performance issue.
 * 
 * when 'persist_create' found that <dir>/<name>.dat exist, it simply maps data without create
 * new one. In this case, 'recsize' and 'maxrec' are ignored.
 * If this persist data file do not exist, 'persist_create' will create a new one with size of
 * ('maxrec'+2) * 'recsize' bytes.
 *  
 * persist utility functions do the best to ensure data won't be lost when power off and can be restored
 * after power is restored. But there is no guarrantee that data are always survived throughout power cycling.
 * 
 * if persist storage is loaded, another call to 'persist_create' with same 'dir' and 'name' shall be failed.
 *
 * return value:
 * a handle of persist. If NULL is return, it could be due to application has no write permission
 * on this <dir> or this <dir> does not exist. Another possible of failure is that persist storage 
 * has been loaded and locked.
 * 
 */
HANDLE persist_create(const char *dir, const char *name, int recsize, int maxrec);

/*
 * 'persist_terminte' close persist storage and release all resources.
 */
int persist_terminate(HANDLE h);

/*
 * 'persist_write' write a new record into persist storage. If total number of data record over the
 * capacity of this persist storage, producer thread will be suspended (mode==1) or return with 0 (mode==0).
 * If there is consumer thread that suspended for new record available to read (when persist is empty), 
 * the suspended consumer will be awaken and get the data to process.
 * return value:
 * >0: sn of the record.
 * -1: faiure (invalid handle)
 */
int persist_write(HANDLE h, int mode, void *data, int size);

/*
 * 'persist_read' read the oldest record in persist storage. record content are copy to a buffer
 * allocated by 'persist_read' and address of this allocated buffer is stored in 'data' while the
 * size of this record is stored in 'size' and time stamp of data is stored in 'datatime'. 
 * It is caller's responsibility to release this buffer.
 * If no data is available in persist storage, depends on argument 'mode', caller could be blocked 
 * ('mode'==1) or return with 0 ('mode'==0)
 *
 * return value:
 *  0 - no data is available in non-block mode or persist is about to terminate
 * >0 - sn of data
 * -1 - error (out of memory, invalid handle, file I/O error etc)
 */
int persist_read(HANDLE h, int mode, void **data, int *size, time_t *datatime);

/*
 * 'persist_purge' remove record with serial number 'sn' out of persist storage. When there is any
 * producer thread suspended in persist_write and waiting for free space in persist storage, after
 * 'persist_purge' successfully free a record space, One of producer threads will be resumed for
 * execution.
 * return value:
 * 0 - ok
 * -1 - failure (invalid handle or sn not found)
 */
int persist_purge(HANDLE h, int sn);


/*
 * 'persist_reset' remove all records and separated record files in persist storage.
 */
int persist_reset(HANDLE h);


/*
 * 'persist_count' - get number of records in persist storage.
 * return value:
 * >=0 number of record
 * -1: error (invalid handle)
 */
int persist_count(HANDLE h);

/* 
 * persist_param
 * obtain persist storage parameter
 */
int persist_param(HANDLE h, int *recsize, int *maxrec);

/*
 * persist_destroy
 * destroy the persist storage completely.
 */
int persist_destroy(const char *dir, const char *name);

/* persist_extend
 * extend existing persist storage's maximum number of storage. usually, new_maxrec shall be
 * greater than current maxrec. However, thrink maxrec is also supported, provided that new
 * maxrec shall be greater than current number of ring elements in persist (persist_count return value)
 */
int persist_extend(HANDLE h, int new_maxrec);
 
/*
 * HINT - application can have mutiple producer threads (issue 'persist_write') but
 * only one consumer thread is allowed (issue 'persist_read' and 'persist_purge'). 
 * multiple consumers is not safe therefore is not supported.
 */
#ifdef __cplusplus
};
#endif


#endif
