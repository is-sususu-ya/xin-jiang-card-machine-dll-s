/*
 *   lvp_client.c
 *
 *	Implement the client communication 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <pthread.h>
#include "lvp_client.h"
#include "vlprotocol.h"
#include <dbg_printf.h>

#if 0
#define PRINTF1 printf
#define PRINTF2 printf
#define PRINTF3 printf
#define PRINTF4 printf
#endif

#define DEBUG_LOG	PRINTF4

//static pthread_mutex_t	Client_lock = PTHREAD_MUTEX_INITIALIZER;	
PtrList  Client_list = PTRLIST_INITIALIZER;
#define MAX_REQ_PENDED		100

//#define CLIST_Lock()		pthread_mutex_lock(&Client_lock)
//#define CLIST_Unlock()	pthread_mutex_unlock(&Client_lock)
//#define REQQ_Lock(cl)		pthread_mutex_lock(&cl->lock_reqq)
//#define REQQ_Unlock(cl)	pthread_mutex_unlock(&cl->lock_reqq)
/* 
 * NOTE - we do not apply lock/unlock mechanism to client list and request list is because
 *        there is no chance for racing condition to happen. If however, application is modified in future
 *        at anyway that racing condition could exist, lock/unlock shall be applied.
 */
#define CLIST_Lock()
#define CLIST_Unlock()
#define REQQ_Lock(cl)		
#define REQQ_Unlock(cl)	
//#define ENABLE_CHKSTRAYREQ

typedef struct tagClientOutputRequest
{
	DataHeader header;
	void	*payload;
	int	 refcount;
	int  sn;
} ClientOutputRequest;


static _longtime GetTickCount()
{
	static signed long long begin_time = 0;
	static signed long long now_time;
	struct timespec tp;
	unsigned long tmsec = 0;
	if (clock_gettime(CLOCK_MONOTONIC, &tp) != -1)
	{
		now_time = tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
	}
	if (begin_time == 0)
		begin_time = now_time;
	tmsec = (unsigned long)(now_time - begin_time);
	return tmsec;
}


static ClientOutputRequest * CreateOutputRequest( void *hdr, void *payload )
{
	static int reqSN = 0;
	ClientOutputRequest *req = malloc(sizeof(ClientOutputRequest));
	assert(req!=NULL);
	memcpy( &req->header, hdr, sizeof(DataHeader) );
	req->refcount = 0;
	req->sn = ++reqSN;
	if ( req->header.size > 0 )
	{
		req->payload = malloc( req->header.size );
		assert(req->payload != NULL);
		memcpy( req->payload, payload, req->header.size );
	}
	else
		req->payload = NULL;
	return req;
}

static void DestroyOutputRequest(ClientOutputRequest *pReq)
{
	if ( --pReq->refcount <= 0 )
	{
		if ( pReq->payload != NULL )
			free( pReq->payload );
		free( pReq );
	}
}

static void *client_threadfxc( void *param )
{
	ClientOutputRequest *pReq;
	LVPClient* client = (LVPClient *)param;
	static int exit_code=0;
	
	PRINTF4("client (%s) Tx thread start and running...(fd=%d)\n", client->peer_name, client->fd );
	for(;client->fd!=-1;)
	{
		REQQ_Lock(client);
		pReq = (ClientOutputRequest *)PtrList_remove_head( &client->outList ); 
		REQQ_Unlock(client);
		if ( pReq != NULL )
		{
			int wait_ack = pReq->header.DataType==DTYP_TRANS;
			CLIENT_CLR_ACKSGLD(client);
			DEBUG_LOG("[%s] Client Tx: header type=0x%x, Id=%d, size=%d, queue=%d..\n", TIMESTAMP0(),
				pReq->header.DataType, pReq->header.DataId, pReq->header.size, client->outList.count );
			sock_write( client->fd, &pReq->header, sizeof(DataHeader) );
			if ( pReq->payload && pReq->header.size>0 )
				sock_write( client->fd, pReq->payload, pReq->header.size );
			client->lt_lasttx = GetTickCount();
			
			DestroyOutputRequest( pReq );
			if ( wait_ack && !CLIENT_IS_ACKSGLD(client) )
			{
				struct timespec ts;
				int rc=0;
				clock_gettime( CLOCK_REALTIME, &ts ); 
				ts.tv_sec++;
				if ( ts.tv_nsec > 600000000UL )
					ts.tv_sec++;
				CLIENT_SET_WAITACK(client);
				DEBUG_LOG("[%s] - TX thread wait for ACK!\n", TIMESTAMP0());
				pthread_mutex_lock( & client->lock );
				rc = pthread_cond_timedwait( & client->cond, & client->lock, &ts );
				pthread_mutex_unlock( & client->lock );
				CLIENT_CLR_WAITACK(client);
				if ( rc!=0 && errno==ETIMEDOUT )
					DEBUG_LOG("[%s] -- warning - wait for client's ACK timed out.\n", TIMESTAMP0());
				else
					DEBUG_LOG("[%s] --> Client TX thread wake up and send next packet.\n", TIMESTAMP0());
			}
		}
		else	// output list is empty, wait for new mission
		{
			struct timespec ts;
			CLIENT_SET_WAITDATA(client);
			// if over 1 second idle on Tx, send a heart beat packet to client let it aware that connection is still sound-and-safe
			if ( ( GetTickCount()-client->lt_lasttx ) >= 1000 )
			{
				DataHeader header;
				HBEAT_HEADER_INIT(header);
				sock_write( client->fd, (const char *)&header, sizeof(header) );
				client->lt_lasttx = GetTickCount();
			}
			clock_gettime( CLOCK_REALTIME, &ts ); 
			ts.tv_sec++;
			pthread_mutex_lock( & client->lock );
			pthread_cond_timedwait( & client->cond, & client->lock, &ts );
			pthread_mutex_unlock( & client->lock );
			CLIENT_CLR_WAITDATA(client);
		}
	}
	// remove all pending output request
	while ( (pReq=(ClientOutputRequest *)PtrList_remove_head(&client->outList)) != NULL )
		DestroyOutputRequest( pReq );
	PRINTF4("client (%s) Tx thread exit...\n", client->peer_name );
	return &exit_code;
}

static void client_attachRequest(LVPClient* client, ClientOutputRequest *pOutReq)
{
	if ( client->outList.count < MAX_REQ_PENDED )
	{
		pOutReq->refcount++;
		REQQ_Lock(client);
		PtrList_append( &client->outList, pOutReq );
		PRINTF5("client_attachRequest: header type=0x%x, Id=%d, size=%d, refcount=%d, ReqList.count=%d.\n",
				pOutReq->header.DataType, pOutReq->header.DataId, pOutReq->header.size, pOutReq->refcount, client->outList.count );
		REQQ_Unlock(client);
	}
	else
		PRINTF4("Client (%s) pended with too many output request (%d), drop new request.\n",
			client->peer_name, client->outList.count );
}

/////////////////////////////////////////////////////////////
int Client_has_queued_raw(LVPClient *client)
{
	POSITION pos;
	int nFound=0;
	ClientOutputRequest *pReq;
	
	REQQ_Lock(client);
	for(pos=client->outList.head; pos!=NULL; pos=pos->next)
	{
		nFound++;
#if 0
		pReq = (ClientOutputRequest *)pos->ptr;
		if ( pReq->header.DataType==DTYP_RAW /* && pReq->header.DataId==image_id */ )
			nFound++;
#endif
	}
	REQQ_Unlock(client);
	return nFound;
}

LVPClient *Client_create(int fd)
{
//	LVPClient *nc = MEM_ALLOC( sizeof(LVPClient), MKFCC("clnt") );
	LVPClient *nc = malloc( sizeof(LVPClient) );
	memset( nc, 0, sizeof(LVPClient) );
	nc->fd = fd;
	//CLIENT_SET_NEEDLIVE(nc);  默认不需要原始数据
	strcpy( nc->peer_name, sock_getpeername( fd, &nc->peer_port) );
	PRINTF1("client created, peer addr=%s\n", nc->peer_name);
	
	// create work thread cond and lock
	pthread_mutex_init( &nc->lock, NULL );
	pthread_mutex_init( &nc->lock_reqq, NULL );
	pthread_cond_init( &nc->cond, NULL );
	// append new client to list
	CLIST_Lock();
	PtrList_append( &Client_list, nc );
	CLIST_Unlock();
	// run client data Tx thread
	if ( pthread_create( &nc->threadId, NULL, client_threadfxc, nc )!= 0 )
	{
		PRINTF1("--> failed to create client Tx thread. errno=%d\n", errno );
		Client_destroy(nc);
		free(nc);
		nc = NULL;
	}
	return nc;
}

void Client_destroy( LVPClient* client )
{
	// remove client from list
	CLIST_Lock();
	POSITION pos = PtrList_find( &Client_list, client );
	assert ( pos!=NULL );
	PtrList_remove( &Client_list, pos );
	CLIST_Unlock();
	// close socket
	if ( client->fd != -1 )
		close( client->fd );
	client->fd = -1;
	if ( client->threadId != 0 )
	{
		pthread_mutex_lock( & client->lock );
		pthread_cond_signal( & client->cond );		// wake up client thread
		pthread_mutex_unlock( & client->lock );
		pthread_cancel( client->threadId );
		// wait for thread exit
		if ( pthread_join(client->threadId, NULL) != 0 )
		{
			ClientOutputRequest *req;
			
			pthread_kill( client->threadId, SIGKILL );
			// remove all output request, in case Tx thread does not do it.
			while ( (req=(ClientOutputRequest *)PtrList_remove_head(&client->outList)) != NULL )
				DestroyOutputRequest( req );
		}
	}
	// destroy resource
	pthread_mutex_destroy( &client->lock );
	pthread_mutex_destroy( &client->lock_reqq );
	pthread_cond_destroy( & client->cond );
	free( client );
}

void Client_destroyAll()
{
	POSITION pos;
	while ( (pos=Client_head()) != NULL )
	{
		LVPClient *cl = (LVPClient *)Client_get(pos);
		Client_destroy(cl);
	}
}

void Client_send( LVPClient* client, void *hdr, void *payload )
{
	DataHeader *header = (DataHeader *)hdr;
	ClientOutputRequest *pOutReq;
	// int raw_frame = header->DataType==DTYP_RAW;
	int raw_frame = 1;
			
	if ( client != NULL )
	{
		// if this is raw data frame and client is busy or client don't need raw data -> ignore it
		//if ( raw_frame && (Client_has_queued_raw(client)>5 || !CLIENT_IS_NEEDRAW(client)) )	
		//			return;
		pOutReq = CreateOutputRequest( hdr, payload );
		PRINTF4("Client_send: append an output request (type=0x%0x, Id=0x%0x, bdy=%d) to client %s, nReq=%d, W4D=%s\n",
			header->DataType, header->DataId, header->size, client->peer_name, client->outList.count, CLIENT_IS_WAITDATA(client) ? "yes" : "no" );
		client_attachRequest( client, pOutReq ); 
		// wake up client thread if it is waiting for data
		if ( CLIENT_IS_WAITDATA(client) )
		{
			pthread_mutex_lock( & client->lock );
			pthread_cond_signal( & client->cond );		
			pthread_mutex_unlock( & client->lock );
		}
	}
	else
	{
		int i=0;
		pOutReq = CreateOutputRequest( hdr, payload );
		pOutReq->refcount=1;		// set as 1 so when client tx and destroy won't really destroy
		POSITION pos = PtrList_get_head(&Client_list);
		for( ; pos != NULL; pos=pos->next )
		{
			client = Client_get(pos);  i++;
			PRINTF4("[%s] Client_send: append output (type=0x%0x,Id=0x%0x, bd=%d) to %s, nReq=%d, W4D=%s\n",
					TIMESTAMP0(), header->DataType, header->DataId, header->size, 
					client->peer_name, client->outList.count, CLIENT_IS_WAITDATA(client) ? "yes" : "no" );
			client_attachRequest( client, pOutReq ); 
			if ( CLIENT_IS_WAITDATA(client) )
			{
				pthread_mutex_lock( & client->lock );
				pthread_cond_signal( & client->cond );		
				pthread_mutex_unlock( & client->lock );
			}
		}
		DestroyOutputRequest( pOutReq );	// if no client need it, we can destroy by this. or when last client destroy will really destroy
	}
}

void Client_wait( LVPClient* client, int tout )
{
	_longtime texp = GetTickCount() + tout;
	if ( client != NULL )
	{
		while ( (!CLIENT_IS_WAITDATA(client) || client->outList.count>0) && GetTickCount() < texp )
			usleep(2000);
		while ( sock_oqueue(client->fd) > 0 && GetTickCount() < texp )
			usleep(2000);
	}
	else
	{
		POSITION pos = PtrList_get_head(&Client_list);
		LVPClient* cl;
		for( ; pos != NULL; pos=pos->next )
		{
			cl = Client_get(pos);
			Client_wait( cl, tout );
		}
	}
}

int Client_fdset( fd_set *set )
{
		int nfds=0;
		LVPClient *client;
		POSITION pos = Client_head();
		
		for( ; pos != NULL; pos=pos->next )
		{
			client = Client_get(pos);
			
			CLIENT_CLR_DATAREADY(client);
			FD_SET( client->fd, set );
			if ( client->fd > nfds )
				nfds = client->fd;
		}
		return nfds;
}

int Client_markset(fd_set *set )
{
	int nsel = 0;
	LVPClient *client;
	POSITION pos = Client_head();
	_longtime lt_now = GetTickCount();
		
	for( ; pos != NULL; )
	{
		POSITION posNext = pos->next;
		client = Client_get(pos);
		if ( FD_ISSET( client->fd, set ) )
		{
			CLIENT_SET_DATAREADY(client);
			nsel++;
		}
		else if ( client->lt_lastbeat!=0 && (lt_now-client->lt_lastbeat) > MAX_HBEAT_SILENCE )
		{
			// check for client lost heart-beat
			PRINTF1("client %s lost heartbeat, remove it\n", client->peer_name );
			Client_destroy(client);
		}
		pos = posNext;
	}
	return nsel;
}

int Client_read( LVPClient* client )
{
	int nr = 0;
	if ( CLIENT_IS_DATAREADY(client) )
	{
		nr = sock_read( client->fd, &client->inHeader, sizeof(DataHeader) );
		if ( nr <= 0 )
		{
			PRINTF1("Client_read packet header nr=%d, errno=%d (%s)\n", nr, errno, strerror(errno) );
			return -1;
		}
		else if ( (client->inHeader.DataType & DTYP_MASK) != DTYP_MAGIC )
		{
			unsigned char soh[] = { 0xaa, 0xbb, 0xcc };
			int n = sock_drain_until( client->fd, soh, 3 );
			PRINTF1("Client_read get a packet nr=%d with bad type (0x%x). ignored and drop %d bytes\n", 
					nr, client->inHeader.DataType, n );
			client->lt_lastbeat = GetTickCount();
			return 0;
		}
		else if ( client->inHeader.size > 0 )
		{
			client->inPayload = malloc( client->inHeader.size );
			nr += sock_read_n_bytes( client->fd, client->inPayload, client->inHeader.size ); 
			if ( nr != sizeof(DataHeader)+client->inHeader.size )
			{
				PRINTF1("Client_read: header type=%d, id=%d, size=%d, read only %d bytes for body.\n",
						client->inHeader.DataType, client->inHeader.DataId, client->inHeader.size, nr-sizeof(DataHeader) );
				free(client->inPayload); client->inPayload=NULL;
				return -1;
			}
		}
		DEBUG_LOG("[%s] receive client (%s) data: type=0x%x, Id=%d, size=%d\n", TIMESTAMP0(),
				client->peer_name, client->inHeader.DataType, client->inHeader.DataId, client->inHeader.size);
		client->lt_lastbeat = GetTickCount();	// any packet received will be treated as heart beating
		if ( nr==sizeof(DataHeader) )
		{
			switch (client->inHeader.DataType)
			{
			case DTYP_ACK:
				// this is a ACK packet, wake up client
				PRINTF5("[%s] Client RX ACK packet. wake up client Tx thread.\n", TIMESTAMP0());
				pthread_mutex_lock( & client->lock );
				pthread_cond_signal( & client->cond );		
				pthread_mutex_unlock( & client->lock );
				CLIENT_SET_ACKSGLD(client);
				nr = 0;
				break;
			case DTYP_HBEAT:
				nr = 0;
				break;
			}
		}
	}
	return nr;
}

LVPClient *Client_find( int fd )
{
		POSITION pos = Client_head();
		LVPClient* client;
		
		for( ; pos != NULL; pos=Client_next(pos) )
		{
			client = Client_get(pos);
			if ( client->fd == fd )
				return client;
		}
		return NULL;
}

int Client_anyOneNeedsRaw()
{
		POSITION pos = Client_head();
		int nNeeds=0;
	 	LVPClient* client;
	 	
		for( ; pos != NULL; pos=Client_next(pos) )
		{
			client = Client_get(pos);
			if ( CLIENT_IS_NEEDRAW(client) )
				nNeeds++;
		}
		return nNeeds;
}

void Client_WaitforTxIdle(LVPClient* cl, int msec )
{
	_longtime time_end = GetTickCount() + msec;
	if ( cl != NULL )
	{
		while( cl->outList.count > 0 && GetTickCount() < time_end )  usleep(1000);
	}
	else
	{
		POSITION pos;
		int nReq;
		do {
			nReq = 0;
			usleep(1000);
			for( pos=Client_list.head; pos!=NULL; pos=pos->next )
				nReq += ((LVPClient *)pos->ptr)->outList.count;
		}	while( nReq>0 && GetTickCount() < time_end );
	}
}

void Client_reset_hbeat(LVPClient* client)
{
	if ( client != NULL )
	{
		client->lt_lastbeat = 0;
	}
	else
	{
		POSITION pos;
		for( pos=Client_list.head; pos!=NULL; pos=pos->next )
		{
			client = (LVPClient *)pos->ptr;
			client->lt_lastbeat = 0;
		}
	}
}

