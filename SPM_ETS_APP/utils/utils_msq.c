#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "dbg_printf.h"

#define MY_MSGMAX		1024
typedef struct {
	long mtype;
	char mtext[MY_MSGMAX];
} msgbuf_s;

#define min(a,b)	( (a)<(b) ? (a) : (b) )
	
int msq_create(const char *path, int progid, int share)
{
	key_t	dev_key = ftok( path, progid );
	int msgflg = IPC_CREAT|0644;
	int msqid;
	
	if ( dev_key==-1 )
	{
		perror("ftok");
		return -1;
	}
	if ( !share ) msgflg |= IPC_EXCL;
	msqid = msgget(dev_key, msgflg);
	if ( msqid==-1 )
		printf("msgget - %s\n", strerror(errno));
	return msqid;
}

int msq_destroy(int msqid)
{
	struct msqid_ds qstat;
	int rc = msgctl(msqid, IPC_RMID, &qstat);
	if ( rc==-1 )
		printf("msgctl(IPC_RMID) - %s\n", strerror(errno));
	return rc;
}

int msq_attach(const char *path, int progid)
{
	key_t	dev_key = ftok( path, progid );
	int msqid;
	
	if ( dev_key==-1 )
	{
		printf("ftok - %s\n", strerror(errno));
		return -1;
	}
	msqid = msgget(dev_key, 0644);
	if ( msqid==-1 )
		printf("msgget() - %s\n", strerror(errno));
	return msqid;
}

int msq_send(int msqid, long mtype, void *mtext, int size, int nowait)
{
	msgbuf_s mbuf;
	int rc;
	
	if ( size > MY_MSGMAX )
	{
		errno = E2BIG;
		return -1;
	}
	mbuf.mtype = mtype;
	if (mtext && size > 0)
		memcpy(mbuf.mtext, mtext, size );
	rc = msgsnd(msqid, &mbuf, size, nowait ? IPC_NOWAIT : 0);
	if ( rc==-1 )
	{
		printf("msgsnd - %s\n", strerror(errno));
	}
	return rc;
}

int msq_recv(int msqid, long *mtype, void *mtext, int size, int nowait)
{
	msgbuf_s mbuf;
	int rc, msgflg = MSG_NOERROR;
	
	if ( nowait ) msgflg |= IPC_NOWAIT;
	rc = msgrcv(msqid, &mbuf, MY_MSGMAX, *mtype, msgflg );
	if ( rc >= 0 )
	{
		*mtype = mbuf.mtype;
		if ( rc>0 && mtext )
		{
			int len = min(rc,size);
			memcpy(mtext, mbuf.mtext, len);
			rc = len;
		}
	}	
	else
	{
		PRINTF1("msgrcv - %s\n", strerror(errno) );
	}
	return rc;
}

int msq_info(int msqid, int *qnum, int *qbytes, int *qfree)
{
	struct msqid_ds  msqinfo;
	int rc;
	rc = msgctl(msqid, IPC_STAT, &msqinfo);
	if ( rc==0 )
	{
		if (qnum) *qnum = msqinfo.msg_qnum;
		if (qbytes) *qbytes = msqinfo.msg_qbytes;
		if (qfree) *qfree = msqinfo.msg_qbytes - msqinfo.__msg_cbytes;
	}
	else
		printf("msgctl(IPC_STAT) - %s\n", strerror(errno));
	return rc;
}

int msq_extend(int msqid, int mnb)
{
	struct msqid_ds  msqinfo;
	int rc;
	rc = msgctl(msqid, IPC_STAT, &msqinfo);
	if ( rc==0 )
	{
		msqinfo.msg_qbytes = mnb;
		rc = msgctl(msqid, IPC_SET, &msqinfo);
		if ( rc == -1 )
			printf("msgctl(IPC_SET) - %s\n", strerror(errno));
	}
	else
		printf("msgctl(IPC_STAT) - %s\n", strerror(errno));
	return rc;
}

#ifdef EN_MSQ_TESTCODE

#include <time.h>
#include <sys/time.h>
#include <ctype.h>

#define HiRand(lo,hi)		( (lo) + (int)(((hi)-(lo)) * (((float)rand()) / RAND_MAX) ) )

int getint(char *buf, int *val, char **endptr)
{
	char *ptr = buf;
	// skip until a digit 
	for( ; !isdigit(*ptr) && *ptr; ptr++ );
	if ( !isdigit(*ptr) )
		return -1;
	*val = atoi(ptr);
	// skip to end of digit
	if ( endptr )
	{
		for( ; isdigit(*ptr) && *ptr; ptr++ );
		*endptr = ptr;
	}
	return 0;
}

int main(int argc, char * const argv[])
{
	int msqid;
	int rc;
	pid_t pid;
	int share = 0;
	// create message queue

	if ( argc>1 && strcmp(argv[1],"-s")==0 )
		share = 1;		
		
	
	if ( (pid=fork()) > 0 )
	{
		char buf[80];
		int total_send = 0;
		msqid = msq_create("/var/tmp", 1, share);
		if ( msqid==-1 )
			return -1;
		srand(time(NULL));
		for(;;)
		{
			printf("cmd>");
			gets(buf);
			if ( strncmp(buf,"info", 4)==0 )
			{
				int qnum, qbytes, qfree;
				msq_info(msqid,&qnum,&qbytes,&qfree);
				printf("qnum=%d, q-size=%d, q-free=%d\n", qnum, qbytes, qfree);
			}
			else if ( strncmp(buf,"ext", 3)==0 )
			{
				int extend_bytes=0;
				if ( getint(buf+3,&extend_bytes,NULL)==0 && extend_bytes>0 )
				{
					int qbytes;
					msq_info(msqid,NULL,&qbytes,NULL);
					printf("original queue size=%d, extended by %d bytes\n", qbytes, extend_bytes);
					if ( msq_extend(msqid, qbytes+extend_bytes)==0 )
					{
						int new_qbytes;
						msq_info(msqid,NULL,&new_qbytes,NULL);
						printf("after extended, queue size=%d, delta=%d bytes\n", new_qbytes, new_qbytes-qbytes);
					}
				}
				else
					printf("invalid parameter!\n");
			}
			else if ( strncmp(buf,"send",4)==0 )
			{
				int mtype;
				int len;
				int cnt=1;
				char *ptr;
				if ( getint(buf+4,&mtype,&ptr)==0 &&
					   getint(ptr, &len, &ptr)==0 && len<=MY_MSGMAX)
				{
					char data[MY_MSGMAX];
					int i, l;
									
					getint(ptr, &cnt,NULL);
					for(l=0; l<cnt; l++)
					{
						for(i=0; i<len; i++)
							data[i] = HiRand(32,126);
						msq_send(msqid,mtype,data,len,0);
						if ( len > 64 )
							strcpy(data+64,"...");
						else
							data[len] = '\0';
						printf("send[%d]: mtype=%d, len=%d, data=%s\n", ++total_send, mtype, len, data);
					}
				}
				else
					printf("invalid parameter!\n");
			}
			else if ( strncmp(buf,"wait",4)==0 )
			{
				int nsec;
			  if ( getint(buf+4, &nsec, NULL)==0 )
			  {
			  	int len;
			  	sprintf(buf,"%d", nsec);
			  	len = strlen(buf);
			  	msq_send(msqid,99,buf,len,0);
			  }
			}
			else if ( strcmp(buf,"rset")==0 )
			{
				msq_send(msqid,999,NULL,0,0);
				total_send = 0;
			}
			else if ( strncmp(buf,"quit",4)==0 )
				break;
		}
	}
	else if ( pid==0 )
	{
		// child process, message receiver
		char mtext[MY_MSGMAX];
		int total_recv = 0;
		sleep(1);
		msqid = msq_attach("/var/tmp", 1);
		if ( msqid==-1 )
			return -1;
		for(;;)
		{
			long mtype = 0;
			int len = msq_recv(msqid,&mtype,mtext,MY_MSGMAX,0);
			if ( len >= 0 )
			{
				mtext[len] = '\0';
				if ( len>64 )
					strcpy(mtext+64, "...");
				if ( mtype==99 )
				{
					// sender ask receiver sleep a while 
					int nsec = atoi(mtext);
					printf("receiver: sleep for %d seconds...\n", nsec);
					sleep(nsec);
				}
				else if ( mtype==999 )
				{
					printf("receiver: reset counter!\n");
					total_recv = 0;
				}
				else
					printf("recv[%d]: mtype=%d, len=%d, text=%s\n", ++total_recv, mtype, len, mtext);
			}
			else if ( errno==EIDRM  )
			{
				printf("recv: message queue removed, end of process!\n" );
				break;
			}
		}
	}
	else
		perror("fork");
		
	if (pid > 0)
	{
		msq_destroy(msqid);
		usleep(100000);
	}
	return 0;
}
#endif
