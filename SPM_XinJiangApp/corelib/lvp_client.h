#ifndef _LVP_CLIENT_INCLUDED_
#define _LVP_CLIENT_INCLUDED_

#include <sys/select.h>
#include <pthread.h>
#include "./vlprotocol.h"
#include <utils_ptrlist.h>
#include <longtime.h>
#include <utils_net.h>

#define MAX_HBEAT_SILENCE		10000		// 10 seconds.

typedef struct tagLVPClient {
	// connection
	int		fd;						// socket
	int		state;				// 0 no data, 1 data ready for input
	char peer_name[64];
	int	 peer_port;
	_longtime	lt_lastbeat;	// last time we have client's heartbeat
	_longtime	lt_lasttx;		// last time we send a packet to this client
	
	// thread
	pthread_t			threadId;
	pthread_mutex_t		lock;
	pthread_mutex_t		lock_reqq;
	pthread_cond_t		cond;
	// I/O
	PtrList			outList;
	DataHeader	inHeader;			// for input
	void	*inPayload;					// for input
} LVPClient;

LVPClient *Client_create(int fd);
void Client_destroy( LVPClient* client );
void Client_send( LVPClient* client, void *header, void *payload  );
void Client_wait( LVPClient* client, int tout );
int Client_read( LVPClient* client );
int Client_fdset( fd_set *set );
int Client_markset(fd_set *set );
LVPClient *Client_find( int fd );
int Client_anyOneNeedsRaw();
void Client_destroyAll();
void Client_WaitforTxIdle(LVPClient* client, int msec );
void Client_reset_hbeat(LVPClient* client);	// 2015-10-29 V1.3.1.25

#define CSTAT_DATAREADY  	0x01
#define CSTAT_NEEDRAW 		0x02
#define CSTAT_WAITDATA		0x04
#define CSTAT_WAITACK			0x08
#define CSTAT_ACKSGLD			0x10
#define CSTAT_FEEDBACK		0x80000000		// this client start the feedback

#define CLIENT_IS_DATAREADY(cl)		( (cl)->state & CSTAT_DATAREADY )
#define CLIENT_SET_DATAREADY(cl)	( (cl)->state |= CSTAT_DATAREADY )
#define CLIENT_CLR_DATAREADY(cl)	( (cl)->state &= ~CSTAT_DATAREADY )

#define CLIENT_IS_NEEDRAW(cl)		( (cl)->state & CSTAT_NEEDRAW )
#define CLIENT_SET_NEEDRAW(cl)		( (cl)->state |= CSTAT_NEEDRAW )
#define CLIENT_CLR_NEEDRAW(cl)		( (cl)->state &= ~CSTAT_NEEDRAW )

#define CLIENT_IS_WAITACK(cl)		( (cl)->state & CSTAT_WAITACK )
#define CLIENT_SET_WAITACK(cl)	( (cl)->state |= CSTAT_WAITACK )
#define CLIENT_CLR_WAITACK(cl)	( (cl)->state &= ~CSTAT_WAITACK )

#define CLIENT_IS_WAITDATA(cl)		( (cl)->state & CSTAT_WAITDATA )
#define CLIENT_SET_WAITDATA(cl)		( (cl)->state |= CSTAT_WAITDATA )
#define CLIENT_CLR_WAITDATA(cl)		( (cl)->state &= ~CSTAT_WAITDATA )

#define CLIENT_IS_ACKSGLD(cl)			( (cl)->state & CSTAT_ACKSGLD )
#define CLIENT_SET_ACKSGLD(cl)		( (cl)->state |= CSTAT_ACKSGLD )
#define CLIENT_CLR_ACKSGLD(cl)		( (cl)->state &= ~CSTAT_ACKSGLD )

#define CLIENT_IS_FEEDBACK(cl)		( (cl)->state & CSTAT_FEEDBACK )
#define CLIENT_SET_FEEDBACK(cl)		( (cl)->state |= CSTAT_FEEDBACK )
#define CLIENT_CLR_FEEDBACK(cl)		( (cl)->state &= ~CSTAT_FEEDBACK )

#define CLIENT_IS_BUSY(cl)		( (cl)->outList.count > 0 || CLIENT_IS_WAITACK(cl) )

#define CLIENT_GET_NAME(cl)					((cl)->peer_name)
	
extern PtrList  Client_list;

#define Client_count()		Client_list.count
#define Client_head()			Client_list.head
#define Client_next(pos)	pos->next
#define Client_get(pos)		(LVPClient *)(pos->ptr)


#endif
