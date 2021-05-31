/*
 * winnet.c
 *	windows socket utility functions
 */
#include <stdio.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <fcntl.h>
//#include <stdlib.h>
//#include <string.h>
//#include <ctype.h>
#include <errno.h>
//#include <time.h>
#include "winnet.h"

#ifndef _WINTYPES_H
#define MSG_NOSIGNAL		0
#define MSG_DONTWAIT		0
#endif

#define ENABLE_PING					// 使能ping函数
#define ENABLE_NETINFO_API		// 使能特殊接口，获取网络接口信息  XP 没有这个功能

#if _WIN32_WINNT >= 0x0600 && (defined ENABLE_NETINFO_API || defined ENABLE_PING)
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")
#endif

#pragma comment(lib, "ws2_32.lib")

int winsock_startup( void )
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
 
	wVersionRequested = MAKEWORD( 2, 2 );
 
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) 
	{
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                 */
		return err;
	}
 
    return 0; 
}

int winsock_cleanup( void )
{
    return WSACleanup( );
}

const char *sock_ntoa( DWORD dwIP )
{
	struct in_addr  iaddr;
	static char strIP[ 16 ];

	iaddr.s_addr = htonl(dwIP);
	strcpy( strIP, inet_ntoa( iaddr ) );
	return strIP;
}

// return is host byte order not network byte order
unsigned long sock_aton( const char *str )
{
	unsigned long ip = inet_addr(str);
	ip = ntohl(ip);
	return ip;
}

// return is network byte order
unsigned long INET_ATON( const char *ipstr )
{
	return (unsigned long) inet_addr( ipstr );
}

const char *INET_NTOA( unsigned long ip )
{
	struct in_addr inaddr;
	inaddr.s_addr = ip;
	return inet_ntoa( inaddr );
}

const char *INET_NTOA2( unsigned long ip, char *numdot )
{
	struct in_addr inaddr;
	inaddr.s_addr = ip;
	strcpy(numdot, inet_ntoa(inaddr));
	return numdot;
}

int sock_set_recvlowat(SOCKET fd, int bytes)
{
	return setsockopt(fd, SOL_SOCKET, SO_RCVLOWAT,(const char *)&bytes,sizeof(int));
}
#ifdef linux
int sock_set_nonblock(SOCKET fd, int non_block)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if ( non_block )
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;
	return fcntl(fd, F_SETFL, flags);	
}
#endif

int sock_set_timeout(SOCKET fd, int which, int msec)
{
	int opt=0;
	int ret;
#ifdef linux
	struct timeval tv;
	
	tv.tv_sec = msec / 1000;
	tv.tv_usec = (msec % 1000) * 1000;
#else	// winsock
	int tv = msec;
#endif	
	if ( which & _SO_SEND )
		opt |= SO_SNDTIMEO;
	if ( which & _SO_RECV )
		opt |= SO_RCVTIMEO;
  ret = setsockopt(fd,SOL_SOCKET,opt,(const char*)&tv,sizeof(tv));
  return ret;
}

int sock_get_timeout(SOCKET fd, int which)
{
	int opt=0;
	int val=0;
	int val_len = sizeof(val);
	int ret;

	if ( which == _SO_SEND )
		opt = SO_SNDTIMEO;
	else if ( which == _SO_RECV )
		opt = SO_RCVTIMEO;
  ret = getsockopt(fd,SOL_SOCKET,opt,(char *)&val,&val_len);
  return val;
}

#ifdef linux
int sock_wait_for_io_or_timeout(int fd, int for_read, long timeout)
{
	struct pollfd 	poll_table[1], *poll_entry = & poll_table[0];
	int				ret;

	poll_entry->fd = fd;
	poll_entry->events = for_read ? POLLIN : POLLOUT;
    do {
		ret = poll(poll_table, 1, timeout);
	} while (ret == -1);

	return (poll_entry->revents>0) ? 0 : -1; // ETIMEDOUT;
}
#else
int sock_wait_for_io_or_timeout(SOCKET fd, int for_read, long timeout)
{
	fd_set fs;
	struct timeval tv, *ptv;
	int				ret;
	
	if ( timeout >= 0 )
	{
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = timeout % 1000 * 1000;
		ptv = &tv;
	}
	else
		ptv = NULL;
	FD_ZERO( &fs );
	FD_SET( fd, &fs );
	
	do {
		if( for_read )
			ret = select( fd + 1, &fs, NULL, NULL, ptv );
		else
			ret = select( fd + 1, NULL, &fs, NULL, ptv );
	} while( ret == -1 );

	return (ret) ? 0 : -1;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TCP

SOCKET sock_connect0( DWORD dwIP, int port )
{
	struct sockaddr_in	destaddr;
	SOCKET 			fd;

	memset( & destaddr, 0, sizeof(destaddr) );
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons( (short)port );
	destaddr.sin_addr.s_addr = dwIP;		// 不要用 htonl(dwIP), 因为参数已经是network byte order

	if ( (fd = socket(PF_INET, SOCK_STREAM, 0)) != INVALID_SOCKET )
	{
		if ( connect(fd, (struct sockaddr *)&destaddr, sizeof(destaddr)) == SOCKET_ERROR )
		{
			sock_close( fd );
			fd = INVALID_SOCKET;
		}
		else
		{
			int tmp=1;
			setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&tmp, sizeof(tmp));
		}
	}
	return fd;
}

SOCKET sock_connect_timeout(const char *host, int port, int tout)
{
	SOCKET	fd = INVALID_SOCKET;
	int  tout_val = 0;

	if ( tout > 0 )
	{
		tout_val = sock_get_timeout(fd, _SO_SEND);
		sock_set_timeout(fd,_SO_SEND,tout);
	}
	fd = sock_connect0(INET_ATON(host),port);
	if ( tout > 0 )
		sock_set_timeout(fd,_SO_SEND, tout_val);

	return  fd;
}

SOCKET sock_listen( int port, const char *ipbind, int backlog )
{
		struct sockaddr_in 	my_addr;
		SOCKET		fd;

    	if ( (fd = socket(AF_INET,SOCK_STREAM,0)) != INVALID_SOCKET )
		{
			BOOL 		bReuseAddr = FALSE;
   			setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseAddr, sizeof(bReuseAddr));

    		memset(&my_addr, 0, sizeof(my_addr));
			my_addr.sin_family = AF_INET;
			my_addr.sin_port = htons ((short)port);
    		if( ipbind != NULL ) 
			{
    			my_addr.sin_addr.s_addr = INET_ATON(ipbind);
    		} 
			else 
			{
	    		my_addr.sin_addr.s_addr = htonl (INADDR_ANY);
			}

    		if ( bind (fd, (struct sockaddr *) &my_addr, sizeof (my_addr)) == 0 )
    		{
    			if( listen(fd, backlog) == 0 ) 
				{
    				return fd;
    			}
    		}
		}
		if ( fd != INVALID_SOCKET )
			sock_close(fd);
		return INVALID_SOCKET;
}

SOCKET sock_accept2(SOCKET listen_sock, SOCKADDR *caddr)
{
	SOCKADDR_IN	fromaddr;
	int		addrlen = sizeof(fromaddr);
	SOCKET	sock;
	int		tmp=1;

	if ( (sock = accept( listen_sock, (struct sockaddr *) & fromaddr, & addrlen )) != INVALID_SOCKET )
		setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char*)&tmp, sizeof(tmp));
	if ( caddr!=NULL )
		memcpy(caddr, &fromaddr, addrlen );
	return sock;
}

int sock_read_nowait( SOCKET fd, void* buffer, int buf_size )
{
	return recv(fd, (char *)buffer, buf_size, MSG_DONTWAIT|MSG_NOSIGNAL);
}

int sock_write_nowait( int fd, const void* buffer, int buf_size )
{
	return send(fd, (const char *)buffer, buf_size, MSG_DONTWAIT);
}

int sock_read(SOCKET s, void *buf, int size)
{
	return recv( s, (char *)buf, size, 0 );
}

int sock_write(SOCKET s, const void *buf, int size)
{
	return send(s, (const char *)buf, size, 0);
}

int sock_getc( SOCKET sock, long tout )
{
	int		rc;
	unsigned char	ch = 0;

    if ( (rc = sock_dataready( sock, tout )) > 0 )
    {
    	rc = sock_read( sock, (char *)&ch, 1 );
        return rc == 1 ? ch : -2;
    }
    /* timed out or I/O error */
   	return rc-1;	/* 0 -> -1, -1 -> -2 */
}

int sock_puts( SOCKET sock, const char *line)
{
	int len = strlen(line);
	len = sock_write(sock, line, len);
	return len;
}

int sock_read_until_tout(SOCKET fd, char* buffer, int buf_size, int eol, int tout)
{
	int len, size, buf_left, i, found=0;
	char *ptr, *cmpptr;

	for (ptr=buffer,size=0,buf_left=buf_size-1; buf_left>0; /**/) 
	{
		/* peek the data, but not remove the data from the queue */
		// NOTE - 为了避免发送端发送错误的数据，或是部分数据，我们需要检查是否有数据才去读
		//       否则会堵塞住。设置recv超时没有用，sock_set_timeout里对TCP做
		// 	     setsockopt(fd,SOL_SOCKET,opt...
		// 	 返回errno=92 (Protocol not available). 第一次读肯定是会有东西(先select才读的)
		//   但是没有读到指定的结尾，我们继续recv时，如果没有东西，会卡死在里面。
		//   因此每次recv前需要检查是否有数据，给一个固定的超时时间 100 msec。如果没有东西，返回
		//	 已经读取到的字节数。
		if( 0 != sock_wait_for_io_or_timeout(fd, 1, tout) ) 
		{
			//   我们不返回0，因为程序会以为TCP断啦 (select有，读回是0字节)
			buffer[size] = 0;
			return size;
		}
		len = recv(fd, ptr, buf_left, MSG_PEEK);
		if( (len == 0) || (len < 0 && (errno != EAGAIN || errno != EINTR) ) )
			return -1;
		else if( len < 0 )
			continue;

		/* try finding 'eol' in the data */
		for(i=0, cmpptr=ptr; i<len; i++, cmpptr++) 
		{
			if(*cmpptr == eol) 
			{
				len = i+1;
				found = 1;
				break;
			}
		}

		/* get the data we need, and remove from the queue */
		len = recv(fd, ptr, len, 0);
		if( (len == 0) || (len < 0 && (errno != EAGAIN || errno != EINTR) ) )
			return -1;
		else if( len < 0 )
			continue;

		ptr += len;
		size += len;
		buf_left -= len;

		if( found ) {
			buffer[size] = '\0';
			return size;
		}
	}
	// 到此是buffer已经填满但是还没遇见'eol'字节，返回所有read到的内容
	return size;
	// return 0;		// 这里不能返回0，上层会以为socket断掉
}


// copy w/o remove data from socket buffer
int sock_peek_until_tout(SOCKET fd, char* buffer, int buf_size, int eol, int tout)
{
	int len, size, buf_left, i, found=0;
	char *ptr, *cmpptr;

	for (ptr=buffer,size=0,buf_left=buf_size-1; buf_left>0; /**/) 
	{
		/* peek the data, but not remove the data from the queue */
		if( 0 != sock_wait_for_io_or_timeout(fd, 1, tout) ) 
		{
			//   我们不返回0，因为程序会以为TCP断啦 (select有，读回是0字节)
			buffer[size] = 0;
			return size;
		}
		len = recv(fd, ptr, buf_left, MSG_PEEK);
		if( (len == 0) || (len < 0 && (errno != EAGAIN || errno != EINTR) ) )
			return -1;
		else if( len < 0 )
			continue;

		/* try finding 'eol' in the data */
		for(i=0, cmpptr=ptr; i<len; i++, cmpptr++) 
		{
			if(*cmpptr == eol) {
				len = i+1;
				found = 1;
				break;
			}
		}
		ptr += len;
		size += len;
		buf_left -= len;

		if( found ) {
			buffer[size] = '\0';
			return size;
		}
	}
	return size;
}

int sock_read_n_bytes_tout(SOCKET fd, void* buffer, int n, long tout)
{
	char *ptr = (char *)buffer;
	int len;
	while( n > 0 ) 
	{
		// 避免发送方发送字节数比较少，结果卡死在这里
		if( 0 != sock_wait_for_io_or_timeout(fd, 1, tout) ) 
			return (ptr-(char*)buffer);
		len = recv(fd, ptr, n, MSG_NOSIGNAL);
		if( len == 0 ) break;
		if( len < 0 ) {
			if( errno == EAGAIN || errno == EINTR ) continue;
			else {
				return -1;
			}
		}
		ptr += len;
		n -= len;
	}
	return (ptr-(char*)buffer);
}

int sock_write_n_bytes(SOCKET fd, const void* buffer, int size)
{
	int len;
	const char *ptr = (const char *)buffer;
	
	while ( size > 0 )
	{
		len = send(fd, ptr, min(size,1024), MSG_NOSIGNAL);
		if( (len == 0) || (len < 0 && (errno != EAGAIN || errno != EINTR) ) )
			return -1;
		else if( len < 0 )		// this muust be due to EAGAIN or EINTR (send is interrupted)
			continue;

		ptr += len;
		size -= len;
	}
	return ptr - (char *)buffer;
}

int sock_skip_n_bytes_tout(SOCKET fd, int n, long tout)
{
	char buffer[ 1024 ];
	int len;
	int left = n;
	while( left > 0 ) 
	{
		if( 0 != sock_wait_for_io_or_timeout(fd, 1, tout) ) 
			break;
		len = recv(fd, buffer, min( n, sizeof( buffer ) ), MSG_NOSIGNAL);
		if( len == 0 ) break;
		if( len < 0 )
		{
			if( errno == EAGAIN || errno == EINTR )
				continue;
			else
				return -1;
		}
		left -= len;
	}
	return (n-left);
}

// drain all data currently in socket input buffer
int sock_drain( SOCKET fd )
{
	int	rlen, n = 0;
	char	buf[ 1024 ];

	while ( sock_wait_for_io_or_timeout( fd, 1, 0 )==0 )
	{
		rlen = recv(fd, buf, sizeof(buf), MSG_NOSIGNAL);
		if ( rlen > 0 )
			n += rlen;
		else
			return -1;
	}
	return n;
}

int sock_drain_until( SOCKET fd, void *soh, int ns )
{
	char buffer[1024];
	char *ptr;
	int i, len, nskip=0;
	
	while ( sock_dataready(fd,100) )
	{
		/* peek the data, but not remove the data from the queue */
		if ( (len = recv(fd, buffer, sizeof(buffer), MSG_PEEK)) == SOCKET_ERROR || len<=0 )
			return -1;

		/* try to locate soh sequence in buffer */
		for(i=0, ptr=buffer; i<=len-ns; i++, ptr++) 
			if ( 0==memcmp( ptr, soh, ns) )
				break;
		nskip += ptr - buffer;
		if ( i > len-ns )
			recv( fd, buffer, len, 0 );
		else
			recv( fd, buffer, (ptr-buffer), 0 );
		if ( i <= len-ns )
			break;
	}
	return nskip;
}

int sock_is_connected( SOCKET fd )
{
#ifdef linux
	struct tcp_info info; 
	int len=sizeof(info); 
	getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len); 
	return info.tcpi_state==TCP_ESTABLISHED;
#else
	int opt;
	int opt_len = sizeof(opt);
	int ret = getsockopt(fd,SOL_SOCKET ,SO_RCVBUF, (char*)&opt, &opt_len);
	return ret==0;
#endif
}

const char* sock_getpeername( SOCKET fd, int *port )
{
	SOCKADDR_IN	peer_addr;
	int		addrlen = sizeof(peer_addr);
	
	if ( getpeername( fd,  (struct sockaddr *) &peer_addr, &addrlen ) == 0 )
	{
		if ( port ) *port = ntohs( peer_addr.sin_port );
		return inet_ntoa( peer_addr.sin_addr );
	}
	else
		return "";			
}

const char* sock_gethostname( SOCKET fd, int *port)
{
	SOCKADDR_IN sa;
	int namelen=sizeof(sa);

	if ( getsockname( fd, (SOCKADDR *)&sa, &namelen) == 0 )
	{
		if ( port ) *port = ntohs(sa.sin_port);
		return inet_ntoa(sa.sin_addr);
	}
	return "";
}

int sock_gethostaddr(SOCKET fd, SOCKADDR_IN *saddr)
{
	int		addrlen = sizeof(SOCKADDR_IN);
	return getsockname( fd,  (SOCKADDR *) saddr, &addrlen );
}

int sock_getpeeraddr(SOCKET fd, SOCKADDR_IN *saddr)
{
	int		addrlen = sizeof(SOCKADDR_IN);
	
	return getpeername( fd,  (SOCKADDR *)saddr, &addrlen );
}

const char *sockaddr_in_tostring(SOCKADDR_IN *addr, int withport)
{
	static char straddr[INET_ADDRSTRLEN+6];
	short port;
	
	struct in_addr inaddr;
	inaddr.s_addr = addr->sin_addr.s_addr;
	port = addr->sin_port;
	port = ntohs(port);
	strcpy(straddr, inet_ntoa(inaddr));
	if ( withport )
		sprintf(straddr+strlen(straddr), ":%u", (unsigned short)port);
	return straddr;
}

int sock_getlocalname( SOCKET sock, DWORD *myIP, WORD *myPort )
{
	struct sockaddr_in 	my_addr;
	if ( sock_gethostaddr( sock, &my_addr ) == 0 )
	{
		*myIP = my_addr.sin_addr.s_addr; // ntohl(my_addr.sin_addr.s_addr);
		*myPort = ntohs(my_addr.sin_port);
		return 0;
	}
	return SOCKET_ERROR;
}
//================================ U D P ==============================
SOCKET sock_udp_open()
{
	return socket( AF_INET, SOCK_DGRAM, 0 );
}

SOCKET sock_udp_bindLocalIP( unsigned long ulIP, int port )
{
	struct sockaddr_in 	my_addr;
	SOCKET		udp_fd;

	if ( (udp_fd = socket( AF_INET, SOCK_DGRAM, 0 )) != INVALID_SOCKET  )
	{
		BOOL bReuseAddr = FALSE;
		setsockopt( udp_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseAddr, sizeof(bReuseAddr));

		memset(&my_addr, 0, sizeof(my_addr));
		my_addr.sin_family = AF_INET;
		my_addr.sin_port = htons ((short)port);
//		my_addr.sin_addr.s_addr = htonl (ulIP);
		my_addr.sin_addr.s_addr = ulIP;

		if (bind ( udp_fd, (struct sockaddr *) &my_addr, sizeof (my_addr)) != 0)
		{
	        	sock_close( udp_fd );
	        	udp_fd = INVALID_SOCKET;
	    }
	}
    return udp_fd;
}



// enable/disable the broadcast transmission on given UDP socket
// return value: 0 OK, -1 error (call WSAGetLastError() for reason of failure)
int sock_udp_broadcast( SOCKET sock, BOOL bEnable )
{
	return setsockopt( sock, SOL_SOCKET, SO_BROADCAST, (CHAR *)&bEnable, sizeof ( BOOL ));
}

// Set UDP receiving time out. nTimeOut in msec.
int sock_udp_timeout( SOCKET sock, int nTimeOut )
{
	return setsockopt( sock, SOL_SOCKET, SO_RCVTIMEO, (CHAR *)&nTimeOut, sizeof ( nTimeOut ));
}
#if 1
// after bind to a broadcast socket, use sock_udp_send/sock_udp_send0 (with target IP as IPADDR_BROADCAST)
// use sock_udp_recv to receive reply from server.
SOCKET sock_udp_bind_broadcast( int port, long nTimeOut )
{
	BOOL	bBroadcast = TRUE;
	BOOL 	bReuseAddr = FALSE;
	SOCKADDR_IN saUdpClient;
	SOCKET sock_bc = socket(AF_INET,SOCK_DGRAM,0); 
	if ( setsockopt( sock_bc, SOL_SOCKET, SO_BROADCAST, (CHAR *)&bBroadcast, sizeof ( BOOL )) == SOCKET_ERROR )
	{
		closesocket( sock_bc );
		return INVALID_SOCKET;
	}
	if ( setsockopt ( sock_bc, SOL_SOCKET, SO_RCVTIMEO, (CHAR *) &nTimeOut, sizeof (nTimeOut)) == SOCKET_ERROR )
	{
		closesocket( sock_bc );
		return INVALID_SOCKET;
	}
	if ( setsockopt(sock_bc, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseAddr, sizeof(bReuseAddr)) == SOCKET_ERROR )
	{
		closesocket( sock_bc );
		return INVALID_SOCKET;
	}
	saUdpClient.sin_family = AF_INET; 
	saUdpClient.sin_port = htons((u_short)port); 
	saUdpClient.sin_addr.s_addr = htonl(INADDR_ANY); 
	if(bind( sock_bc, (SOCKADDR *)&saUdpClient, sizeof(SOCKADDR_IN)) != 0) 
	{
		closesocket( sock_bc );
		return INVALID_SOCKET;
	}
	return sock_bc;
}
#endif

int sock_udp_send( SOCKET sock, const char *ip, int port, const void* msg, int len )
{
	SOCKADDR_IN	udp_addr;
	SOCKET	sockfd=sock, ret;

	if ( sock==INVALID_SOCKET )
		sockfd = socket( AF_INET, SOCK_DGRAM, 0 );
	if( sockfd <= 0 ) return -1;

  //setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));

	memset( & udp_addr, 0, sizeof(udp_addr) );
	udp_addr.sin_family = AF_INET;
	if ( ip==NULL )
	{
		udp_addr.sin_addr.s_addr = INADDR_BROADCAST;
		sock_udp_broadcast(sockfd,1);
	}
	else
		udp_addr.sin_addr.s_addr = INET_ATON( ip );
	udp_addr.sin_port = htons( (short)port );

	ret = sendto( sockfd, (const char *)msg, len, 0, (const struct sockaddr *)&udp_addr, sizeof(udp_addr) );
	if ( sock==INVALID_SOCKET )
		closesocket( sockfd );
	return ret;
}

int sock_udp_send0( SOCKET udp_fd, unsigned long ip, int port, const void * msg, int len )
{
	SOCKADDR_IN	udp_addr;
	int	ret;
	
	memset( & udp_addr, 0, sizeof(udp_addr) );
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_addr.s_addr = ip;		// NOTE - dwIP is already network format. DON'T USE htonl(dwIP)
	udp_addr.sin_port = htons( (short)port );

	if ( len == 0 )
		len = strlen( (char *)msg );

	ret = sendto( udp_fd,(const char *) msg, len, MSG_DONTWAIT, (const struct sockaddr *) & udp_addr, sizeof(udp_addr) );

	return ret;
}

int sock_udp_sendX( SOCKET fd, SOCKADDR_IN *paddr, int ndst, const void* msg, int len )
{
	int i;
	int send_len=0;
	SOCKADDR_IN  baddr;
	
	if ( len == 0 )
		len = strlen( (char *)msg );
	if ( ndst==0 || paddr==NULL )
	{
		paddr = &baddr;
		paddr->sin_addr.s_addr = htonl(INADDR_BROADCAST);
		paddr->sin_family = AF_INET;
		ndst = 1;
	}
	for(i=0; i<ndst; i++)
		send_len += sendto( fd, (const char *)msg, len, MSG_DONTWAIT, (const struct sockaddr *)(paddr+i), sizeof(SOCKADDR_IN) );
	return send_len;
}


int sock_udp_recv( SOCKET sock, void *buf, int size, DWORD *IPSender )
{
	SOCKADDR_IN  sender;
	int		addrlen = sizeof( sender );
	int		ret;

	if ( (ret = recvfrom( sock, (char *)buf, size, 0, (struct sockaddr *)&sender, &addrlen )) > 0 )
	{
		if ( IPSender != NULL )
			*IPSender = sender.sin_addr.s_addr;
	}
	return ret;
}

int sock_udp_recv0( SOCKET sock, void *buf, int size, SOCKADDR_IN *saIn )
{
	SOCKADDR_IN  sender;
	int		addrlen = sizeof( sender );
	int		ret;

	if ( (ret = recvfrom( sock, (char *)buf, size, 0, (struct sockaddr *)&sender, &addrlen )) > 0 )
	{
		if ( saIn != NULL )
			memcpy( (void *)saIn, (void *)&sender, sizeof(SOCKADDR_IN) );
	}
	return ret;
}

//========================== c o m m o n ==============================

#ifdef ENABLE_NETINFO_API
#if _WIN32_WINNT < 0x0600	
#else
int get_all_interface(IF_ATTR4 ifattr[], int size)
{
	IP_ADAPTER_INFO* pAdapterInfo = (IP_ADAPTER_INFO *) malloc( sizeof(IP_ADAPTER_INFO) );
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	int		entry = 0;

	 // Make an initial call to GetAdaptersInfo to get the necessary size into the ulOutBufLen variable
	 if (GetAdaptersInfo( pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
	 {
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *) malloc (ulOutBufLen);
	 }

	 if(GetAdaptersInfo(pAdapterInfo,&ulOutBufLen) == NO_ERROR )
	 {
		  IP_ADAPTER_INFO* pAdapter = pAdapterInfo;
		  while (pAdapter)
		  {
			  // we only interest in Ethernet. 
			  if ( pAdapter->Type==MIB_IF_TYPE_ETHERNET )
			  {
					IP_ADDR_STRING  *adp_addr;
					// 一个网卡可以绑定多个IP地址，我们需要都获取
					adp_addr = &pAdapter->IpAddressList;
					while( adp_addr && entry < size )
					{
						strncpy(ifattr[entry].ifa_name, pAdapter->Description, IFNAME_LEN);
						ifattr[entry].ifa_name[IFNAME_LEN-1] = '\0';
						ifattr[entry].addr.s_addr = inet_addr(adp_addr->IpAddress.String);
						ifattr[entry].mask.s_addr = inet_addr(adp_addr->IpMask.String);
						// Windows的IP_ADAPTER_INFO没有提供广播地址，暂时不用，广播都用默认IPv4广播地址
						entry++;
						adp_addr = adp_addr->Next;
					}
			  }
			  pAdapter = pAdapter->Next;
		  }
	 }
	if ( pAdapterInfo )
		free(pAdapterInfo);

	return entry;
}
#endif
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef ENABLE_PING

static unsigned short icmp_checksum(unsigned short *buff,int size)
{
	unsigned long cksum=0;
	while(size>1)
	{
		cksum+=*buff++;
		size -= sizeof(unsigned short);
	}
	if (size)
	{
		cksum += *(unsigned char*)buff;
	}
	cksum = (cksum>>16)+(cksum&0xffff);
	cksum+=cksum>>16;
	return (unsigned short)(~cksum);
}

#if 1
#pragma pack (push,1)
typedef struct {
	unsigned char icmp_type;				//消息类型
	unsigned char icmp_code;			//清息代码
	unsigned short icmp_checksum; //16位效验
	unsigned short icmp_id;				//用来唯一标识些请求的ID号
	unsigned short icmp_sequence;	//序列号
	unsigned long icmp_timestamp;	//时间戳
} ICMPPACK, *PICMPPACK;
#pragma pack (pop)
/*
int sock_get_ttl(SOCKET s)
{
	int ttl_count;
	int len = sizeof(ttl_count);
	int rc = getsockopt(s,IPPROTO_IP,IP_TTL,(char *)&ttl_count,&len);
	return ttl_count;
}
*/
 int ping(const char *IP, int ping_times, int *tout_count)
{
	struct sockaddr_in desAddr;
	SOCKET sock_raw;
	char buff[sizeof(ICMPPACK)+32];	//创建ICMP包
	PICMPPACK pICMP=(PICMPPACK)buff;
	ICMPPACK *pTemp;
	unsigned short nSeq=0;
	char recvBuf[1024];
	struct sockaddr_in from;
	int nLen=sizeof(struct sockaddr_in);
	int count=0;
	int nRet;
	int nTick;
	int	success_times=0;
	int	total_msec = 0;
	int nTimeOut = 1000;

	sock_raw = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
	if( INVALID_SOCKET == sock_raw)
	{
		printf("failed to create raw socket\n");
		return 0;
	}

	// 设置接收超时
    setsockopt(sock_raw, SOL_SOCKET, SO_RCVTIMEO, (char const*)&nTimeOut, sizeof(nTimeOut));

	//设置目标地址
	desAddr.sin_addr.S_un.S_addr=inet_addr(IP);
	desAddr.sin_port = htons(0);
	desAddr.sin_family = AF_INET;

	pICMP->icmp_type=8; //请求回显
	pICMP->icmp_code=0;
	pICMP->icmp_checksum=0;
	pICMP->icmp_id=(unsigned short)GetCurrentProcessId();
	pICMP->icmp_sequence=0;
	pICMP->icmp_timestamp=0;

	memset(&buff[sizeof(ICMPPACK)],'E',32);   //填充用户数据

	while(TRUE)
	{
		pICMP->icmp_checksum=0;
		pICMP->icmp_sequence=nSeq++;
		pICMP->icmp_timestamp = GetTickCount();
		pICMP->icmp_checksum = icmp_checksum((unsigned short *)buff,sizeof(ICMPPACK)+32);
		if ( sendto(sock_raw, buff, sizeof(ICMPPACK)+32, 0, (struct sockaddr *)&desAddr, sizeof(desAddr)) == -1 )
		{
			//printf("sendto() failed \n");
			closesocket(sock_raw);
			return -1;
		}

		if ( (nRet=recvfrom(sock_raw,recvBuf,sizeof(recvBuf),0,(struct sockaddr *)&from,&nLen)) == -1 )
		{
			if (WSAGetLastError()==WSAETIMEDOUT)
			{
				//printf("time out \n");
				continue;
			}
			else
			{
				//printf("recvfrom() failed \n");
				closesocket(sock_raw);
				return -1;
			}
		}

		nTick = GetTickCount();
		if (nRet<20+sizeof(ICMPPACK))
		{
			//printf("too few bytes from %s, ignored \n",inet_ntoa(from.sin_addr));
			continue;
		}
		pTemp=(ICMPPACK *)(recvBuf+20);		// 前面20字节是IP头
		if (pTemp->icmp_type!=0)
		{
			//printf(" not ICMP_ECHO type received\n");
			continue;
		}

		if (pTemp->icmp_id != (unsigned short)GetCurrentProcessId())
		{
			//printf("get some one else packet\n");
			continue;
		}
	/*/printf("%d reply from %s: bytes=%d time<%dms TTL=%d\n",
				pTemp->icmp_sequence,
				inet_ntoa(from.sin_addr),
				nRet,
				nTick-pTemp->icmp_timestamp,
				sock_get_ttl (sock_raw));
				*/
		total_msec += nTick-pTemp->icmp_timestamp;
		success_times++;
		if (++count==ping_times) break;
		Sleep(200);
	}
	if ( success_times > 1 )
		return total_msec/success_times + 1;
	return -1;
}

#else

#pragma pack (push,1)
// IP header structure
typedef struct _iphdr 
{
    //Suppose the BYTE_ORDER is LITTLE_ENDIAN
    unsigned int   h_len:4;			 // Length of the header
    unsigned int   version:4;			// Version of IP
    unsigned char  tos;				// Type of service
    unsigned short total_len;			// Total length of the packet
    unsigned short id;					// Unique identification
    unsigned short frag_offset;		// Fragment offset
    unsigned char  ttl;					// Time to live
    unsigned char  protocol;			// Protocol (TCP, UDP etc)
    unsigned short checksum;      // IP checksum
    unsigned int   sourceIP;			// Source IP
    unsigned int   destIP;				// Destination IP
} IpHeader;

#define ICMP_ECHO				8
#define ICMP_ECHOREPLY   0
#define ICMP_MIN_SIZE		8 // Minimum 8-byte ICMP packet (header)

// ICMP header structure
// This is not the standard header, but we reserve space for time
#define ICMP_DATA_LEN	24
// 整个ICMP数据包64字节，也可以设置更大(ICMP_DATA_LEN增加)，意义不大

typedef struct _icmphdr 
{
    BYTE   icmp_type;
    BYTE   icmp_code;                 // Type sub code
    USHORT icmp_cksum;
    USHORT icmp_icd_id;
    USHORT icmp_icd_seq;
    BYTE  icmp_data[ICMP_DATA_LEN];		// extra part
} IcmpHeader;

// IP option header - use with socket option IP_OPTIONS
typedef struct _ipoptionhdr
{
    unsigned char code;        // Option type
    unsigned char len;         // Length of option hdr
    unsigned char ptr;         // Offset into options
    unsigned long addr[9];     // List of IP addrs
} IpOptionHeader;

#pragma pack (pop)

#define DEF_PACKET_SIZE  32        // Default packet size
#define MAX_PACKET       1024      // Max ICMP packet size
#define MAX_IP_HDR_SIZE  60        // Max IP header size w/ options

static int icmp_encodepacket(int sendseeq, char *sendbuf);
static int icmp_decodepack(char *buf, int len);

int ping(char const *IP, int ping_times, int *tout_times)
{
	char sendbuf[128], recvbuf[128];
	SOCKADDR_IN pingaddr, senderaddr;
	int sockaddr_len = sizeof(SOCKADDR_IN);
	int sockfd;
	double	total_time = 0;
	int nTimeOut = 1000;		// 1000 msec
	int		 rtt;		// 往返时间 in used
	int		 recv_count=0;
	int      seq;

	if ( ping_times<=0 )  return -1;
    memset(&pingaddr, 0, sizeof(SOCKADDR_IN));
    if ( (pingaddr.sin_addr.s_addr=inet_addr(IP)) == INADDR_NONE)
	{
		// try if input is a domain name
		struct hostent *host;
        if ( (host=gethostbyname(IP)) == NULL) 
            return -1;
        pingaddr.sin_addr = *(struct in_addr *)(host->h_addr);
    }
    pingaddr.sin_family = AF_INET;
	pingaddr.sin_port = 0;

    // 创建原始套接字 SOCK_RAW 协议类型 IPPROTO_ICMP
    if ( (sockfd=socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == INVALID_SOCKET)
		return -1;

	// 设置接收超时
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char const*)&nTimeOut, sizeof(nTimeOut));

    // 发包操作
    for(seq=1; seq<=ping_times; seq++)
	{
		int recvlen;
        int sendlen = icmp_encodepacket(seq, sendbuf);

        if ( sendto(sockfd, sendbuf, sendlen, 0, (struct sockaddr *)&pingaddr, sizeof(pingaddr)) != sendlen )
		{
			sock_close(sockfd);
			return -1;
		}

		if ( (recvlen = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&senderaddr, &sockaddr_len)) == -1 &&
			WSAGetLastError()!=WSAETIMEDOUT )
		{
			sock_close(sockfd);
			return -1;
		}
		
		if ( recvlen < ICMP_MIN_SIZE || senderaddr.sin_addr.s_addr != pingaddr.sin_addr.s_addr)
			continue;

		rtt = icmp_decodepack(recvbuf, recvlen);
		if ( rtt != -1 )
		{
			recv_count++;
			total_time += rtt;
		}
		Sleep(100);
    }

	sock_close(sockfd);

	if ( tout_times )
		*tout_times = ping_times - recv_count;
	if ( recv_count > 0 )
		rtt = (int)(total_time / 1000 / recv_count + 0.5);
	else
		rtt = -1;

	return rtt;
}

// 发送ping数据包
static int icmp_encodepacket(int sendseq, char *buf)
{
    IcmpHeader *icmp_hdr = (IcmpHeader *)buf;
	int  icmp_total_len = sizeof(IcmpHeader);
	LONGLONG llSystemTime;

    icmp_hdr->icmp_type = ICMP_ECHO;
    icmp_hdr->icmp_code = 0;
    icmp_hdr->icmp_icd_id = (USHORT)GetCurrentProcessId();
    icmp_hdr->icmp_icd_seq = sendseq;

    memset(icmp_hdr->icmp_data, 0, ICMP_DATA_LEN);
	GetSystemTimeAsFileTime( (FILETIME *)&llSystemTime );
	memcpy(icmp_hdr->icmp_data, &llSystemTime, sizeof(llSystemTime) );		// 获取到的时间单位是100 nano-seconds since 1601-1-1
    icmp_hdr->icmp_cksum = icmp_checksum((unsigned short *)buf,icmp_total_len);

	return icmp_total_len;
}

static int icmp_decodepack(char *buf, int len)
{
    IpHeader *iphdr;
    IcmpHeader *icmp_hdr;
    int iphdrlen;
    int rtt;	// 往返时间  (usec)
	int	ttl;
	USHORT my_pid = (USHORT)GetCurrentProcessId();
	LONGLONG	send_time, recv_time;

	GetSystemTimeAsFileTime((FILETIME*)&recv_time);
    iphdr = (IpHeader*)buf;
    // ip头部长度
    iphdrlen = iphdr->h_len * 4;		// 一般是20
    ttl = iphdr->ttl;

    icmp_hdr = (IcmpHeader *)(buf + iphdrlen);

    // icmp报文的长度
    len -= iphdrlen;
    if (len < ICMP_MIN_SIZE)
        return -1;

    // 确认是本机发出的icmp报文的响应
    if (icmp_hdr->icmp_type != ICMP_ECHOREPLY || icmp_hdr->icmp_icd_id != my_pid)
        return -1;

    memcpy(&send_time, icmp_hdr->icmp_data, sizeof(LONGLONG));
    rtt = (int)((recv_time - recv_time) / 10);		// usec 差值

    return rtt;
}
#endif

#endif

