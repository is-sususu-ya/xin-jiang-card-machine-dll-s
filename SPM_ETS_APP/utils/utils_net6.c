/***************************************************************************
                          utils_net6.c  -  description
                             -------------------
    begin                : Thu Dec 20 2001
    copyright            : (C) 2001 by Liming Xie
    email                : liming_xie@yahoo.com

    Version: 	1.0.1
    Date:	2005-03-06
    Author:	Thomas Chang

		Vseriosn: 1.1.1
    1. Add the "get_host_ip" and "get_ifadapter_ip" functions
		Version: 1.1.2
		1. Add ifconfig, get_netaddr, set_netaddr, get_netmask, set_netmask, get_gatewayaddr,
		   get_mac_addr set_mac_addr
		
		version:  2.0.1
		Date: 2016-03-28
		1. Add IPv6 support, make it dual stack support. 
		2. Add functions and mcros for IPv6
		
		version: 2.0.2
		Date: 2016-04-07
		1. Full support of IPv6, including get/set interface addr. gateway etc.
		
 ***************************************************************************/
#ifdef ENABLE_IPV6		// this compiler option must be given in Makefile if IPv6+IPv4 are applied

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#ifdef linux
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/poll.h>
#include <sys/socket.h>   
#include <linux/sockios.h>		// for sock_oqueue SIOCOUTQ
#include <sys/ioctl.h>   
#include <netinet/in.h>
#include <ifaddrs.h>
//#include <net/if.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <linux/route.h>
#include <linux/ipv6_route.h>
#ifndef min
#define min( x, y ) ( (x) > (y) ? (y) : (x) )
#endif
#ifndef max
#define max( x, y ) ( (x) < (y) ? (y) : (x) )
#endif

#else

#include "windows/types.h"

#endif

#include "utils_net6.h"
//#include "utils_str.h"

//#define DEBUGREQ

//#define NET_TIMEDOUT_ENABLE

static	long	NET_TIMEDOUT_MSEC = 60000;		/* 60 sec == 1 min */

int sock_set_timeout( long time_ms )
{
	long	old = NET_TIMEDOUT_MSEC;

	NET_TIMEDOUT_MSEC = time_ms;

	return old;
}

int sock_set_recvlowat(int fd, int bytes)
{
	return setsockopt(fd, SOL_SOCKET, SO_RCVLOWAT,&bytes,sizeof(int));
}

int sock_connect0( unsigned long ipaddr, int port )
{
	SOCKADDR_IN	dstaddr;
	int 			fd = 0;

	memset( & dstaddr, 0, sizeof(dstaddr) );
	dstaddr.sin_family = AF_INET;
	dstaddr.sin_addr.s_addr = ipaddr;
	dstaddr.sin_port = htons( port );

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0)   return -1;

	if ( connect(fd, (struct sockaddr *)&dstaddr, sizeof(dstaddr)) < 0 )
	{
		close(fd);
		return -1;
	}
	return fd;
}

int sock_connect4(const IN_ADDR *inaddr, int port)
{
	SOCKADDR_IN	dstaddr;
	int	fd = INVALID_SOCKET;

	memset( & dstaddr, 0, sizeof(dstaddr) );
	dstaddr.sin_family = AF_INET;
	dstaddr.sin_port = htons( (short)port );
	if ( inaddr==NULL )
		dstaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	else
		dstaddr.sin_addr.s_addr = inaddr->s_addr;
	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd != INVALID_SOCKET )   
	{
		if ( connect(fd, (struct sockaddr *)&dstaddr, sizeof(dstaddr)) == 0 )
			return fd;
	}
	if ( fd != INVALID_SOCKET )
		sock_close(fd);
	return  INVALID_SOCKET;
}

// [NOTE] Linux IPv6 connection has bug will result in segfault. 
// Winsock2 is OK. Have to find bug fix on Linux IPv6
int sock_connect6(const IN6_ADDR *in6addr, int port)
{
	SOCKADDR_IN6	dstaddr;
	int fd = INVALID_SOCKET;

	memset( &dstaddr, 0, sizeof(dstaddr) );
	dstaddr.sin6_family = AF_INET6;
	if ( in6addr==NULL )
		memcpy(&dstaddr.sin6_addr, &in6addr_loopback, sizeof(IN6_ADDR));
	else
		memcpy(&dstaddr.sin6_addr, in6addr, sizeof(IN6_ADDR));
	dstaddr.sin6_port = htons( (short)port );
	fd = socket(PF_INET6, SOCK_STREAM, 0);
	if (fd != INVALID_SOCKET )   
	{
		if ( connect(fd, (struct sockaddr *)&dstaddr, sizeof(dstaddr)) == 0 )
		{
			return fd;
		}
	}
	if ( fd != INVALID_SOCKET )
		sock_close(fd);
	return  INVALID_SOCKET;
}

int sock_connect( const char *host, int port )
{
		int	fd=INVALID_SOCKET;
		int  af = AF_INET;
		IN_ADDR_UNION u_inaddr;

		if (host != NULL )
		{
			if ( (af = get_addrfamily(host)) == -1 )
				return INVALID_SOCKET;
			if ( inet_pton(af, host, &u_inaddr) <= 0 )
				return INVALID_SOCKET;
		}
		else
			u_inaddr.in_addr.s_addr = htonl(INADDR_LOOPBACK);
		if ( af==AF_INET )
			fd = sock_connect4(&u_inaddr.in_addr, port);
		else
			fd = sock_connect6(&u_inaddr.in6_addr, port);
		return fd;
}

int sock_listen( int port, const char *ipbind, int backlog )
{
		int		fd=INVALID_SOCKET;
		int af = AF_INET6;
		IN_ADDR_UNION u_addr;

		if (ipbind != NULL )
		{
			af = get_addrfamily(ipbind);
			if ( af==-1 )
				return INVALID_SOCKET;
			if ( inet_pton(af, ipbind, &u_addr) <= 0 )
				return INVALID_SOCKET;
			if ( af==AF_INET )
				fd = sock_listen4(port, &u_addr.in_addr, backlog);
			else
				fd = sock_listen6(port, &u_addr.in6_addr, backlog);
		}
		else
		{
			if ( (fd = sock_listen6(port, NULL, backlog)) < 0 )
				fd = sock_listen4(port, NULL, backlog);
		}		
		return fd;
}

int sock_listen4( int port, const IN_ADDR *inaddr, int backlog )
{
	SOCKADDR_IN 	my_addr;
	int fd, tmp = 1;

	fd = socket(PF_INET,SOCK_STREAM,0);
	if (fd < 0) 
  	 return INVALID_SOCKET;

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&tmp, sizeof(tmp));

	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons ((short)port);
	if( inaddr != NULL ) {
		my_addr.sin_addr.s_addr = inaddr->s_addr;
	} else {
		my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	if( 0 == bind (fd, (SOCKADDR *) &my_addr, sizeof (my_addr)) &&
			0 == listen(fd, backlog) )
		return fd;
	// error return
	sock_close(fd);
	return INVALID_SOCKET;
}

int sock_listen6( int port, const IN6_ADDR *in6addr, int backlog )
{
	SOCKADDR_IN6 	my_addr;
	int fd, tmp = 1;

	fd = socket(PF_INET6,SOCK_STREAM,0);
	if (fd < 0) 
  	 return INVALID_SOCKET;
	// enable SO_REUSEADDR for TCP
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&tmp, sizeof(tmp));
		/*
		 * clear socket option IPV6_V6ONLY make our socket dual stack support. In Linux, disable is the default value.
		 * but not true for winsock2. We have to disable it manually.
		 */
	tmp = 0;
	if ( setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&tmp, sizeof(int)) == -1)
		printf("<WARNING> - disable IPV6_V6ONLY failed --> %s\n", strerror(errno) );

	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin6_family = AF_INET6;
	my_addr.sin6_port = htons ((short)port);
	if( in6addr != NULL ) {
		memcpy(&my_addr.sin6_addr, in6addr, sizeof(IN6_ADDR));
	} else {
  	memcpy(&my_addr.sin6_addr, &in6addr_any, sizeof(IN6_ADDR));
	}
	if( 0 == bind (fd, (SOCKADDR *) &my_addr, sizeof (my_addr)) &&
			0 == listen(fd, backlog) )
		return fd;
	// error return
	sock_close(fd);
	return INVALID_SOCKET;
}
/*
int sock_accept( int fd_listen )
{
	SOCKADDR_STORAGE	fromaddr;
	socklen_t		addrlen = sizeof(fromaddr);
	int		fd;
	int		tmp;

	if ( (fd = accept( fd_listen, (SOCKADDR *) & fromaddr, & addrlen )) != -1 )
		setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &tmp, sizeof(tmp));
	return fd;
}
*/
int sock_accept2(int listen_sock, SOCKADDR *caddr)
{
	SOCKADDR_STORAGE	fromaddr;
	socklen_t		addrlen = sizeof(fromaddr);
	int	sock;
	int		tmp=1;

	if ( (sock = accept( listen_sock, (struct sockaddr *) & fromaddr, & addrlen )) != INVALID_SOCKET )
		setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char*)&tmp, sizeof(tmp));
	if ( caddr!=NULL )
		memcpy(caddr, &fromaddr, addrlen );
	return sock;
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

	return (poll_entry->revents>0) ? 0 : ETIMEDOUT;
}
#else
int sock_wait_for_io_or_timeout(int fd, int for_read, long timeout)
{
	fd_set fs;
	struct timeval tv;
	int				ret;
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = timeout % 1000 * 1000;
	FD_ZERO( &fs );
	FD_SET( fd, &fs );

	do {
		if( for_read )
			ret = select( fd + 1, &fs, NULL, NULL, &tv );
		else
			ret = select( fd + 1, NULL, &fs, NULL, &tv );
	} while( ret == -1 );

	return (ret) ? 0 : ETIMEDOUT;
}
#endif

int sock_read( int fd, void* buffer, int buf_size )
{
#ifdef NET_TIMEDOUT_ENABLE
	if( 0 != sock_wait_for_io_or_timeout(fd, 1, NET_TIMEDOUT_MSEC) ) return -1;
#endif

	return recv(fd, buffer, buf_size, 0);
}

int sock_write( int fd, const void* buffer, int buf_size )
{
#ifdef NET_TIMEDOUT_ENABLE
	if( 0 != sock_wait_for_io_or_timeout(fd, 0, NET_TIMEDOUT_MSEC) ) return -1;
#endif
	
	return send(fd, buffer, buf_size, MSG_NOSIGNAL);
}

int sock_read_until(int fd, char* buffer, int buf_size, int eol)
{
	int len, size, buf_left, i, found=0;
	char *ptr, *cmpptr;

	for (ptr=buffer,size=0,buf_left=buf_size-1; buf_left>0; /**/) 
	{
#ifdef NET_TIMEDOUT_ENABLE
		if( 0 != sock_wait_for_io_or_timeout(fd, 1, NET_TIMEDOUT_MSEC) ) return -1;
#endif
		/* peek the data, but not remove the data from the queue */
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
	return 0;
}

// copy w/o remove data from socket buffer
int sock_peek_until(int fd, char* buffer, int buf_size, int eol)
{
	int len, size, buf_left, i, found=0;
	char *ptr, *cmpptr;

	for (ptr=buffer,size=0,buf_left=buf_size-1; buf_left>0; /**/) 
	{
		/* peek the data, but not remove the data from the queue */
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
	return 0;
}

int sock_read_n_bytes(int fd, void* buffer, int n)
{
	char *ptr = buffer;
	int len;
	while( n > 0 ) {
#ifdef NET_TIMEDOUT_ENABLE
		if( 0 != sock_wait_for_io_or_timeout(fd, 1, NET_TIMEDOUT_MSEC) ) return -1;
#endif

		//len = recv(fd, ptr, n, MSG_WAITALL);
		len = recv(fd, ptr, n, 0);
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

int sock_write_n_bytes(int fd, void* buffer, int size)
{
	int len;
	char *ptr = (char *)buffer;
	
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

int sock_skip_n_bytes(int fd, int n)
{
	char buffer[ 1024 ];
	int len;
	int left = n;
	while( left > 0 ) {
#ifdef NET_TIMEDOUT_ENABLE
		if( 0 != sock_wait_for_io_or_timeout(fd, 1, NET_TIMEDOUT_MSEC) ) return -1;
#endif
		//len = recv(fd, ptr, n, MSG_WAITALL);
		len = recv(fd, buffer, min( n, sizeof( buffer ) ), 0);
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
int sock_drain( int fd )
{
	int	rlen, n = 0;
	char	buf[ 1024 ];

	while ( sock_dataready( fd, 0 ) )
	{
		rlen = recv(fd, buf, sizeof(buf), 0);
		if ( rlen > 0 )
			n += rlen;
		else
			return -1;
	}
	return n;
}

int sock_drain_until( int fd, void *soh, int ns )
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

int sock_is_connected( int fd )
{
	fd_set set;
	struct timeval val;
	int ret;

	FD_ZERO( & set );
	FD_SET( fd, & set );
	val.tv_sec = 0;
	val.tv_usec = 0;
	ret = select( fd +1, & set, NULL, NULL, & val );
	if( ret > 0 ) {
		// try to peek data
		char buf;
		ret = recv( fd, &buf, 1, MSG_NOSIGNAL | MSG_PEEK );
		if( ret <= 0 ) return 0;
	} else if ( ret < 0 ) {
		// select error
		return 0;
	} else {
		// select timeout
	}
	return 1;
}

//================================ U D P ===========================
int sock_udp_open(int af)
{
	return socket( af, SOCK_DGRAM, 0 );
}

int sock_udp_bind4(int port, const IN_ADDR *inaddr)
{
	struct sockaddr_in 	my_addr;
	int			udp_fd;

	// signal init, to avoid app quit while pipe broken
//	signal(SIGPIPE, SIG_IGN);  this is application's job

	if ( (udp_fd = socket( PF_INET, SOCK_DGRAM, 0 )) >= 0 )
	{
		//int tmp = 0;
		//setsockopt( udp_fd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));

		memset(&my_addr, 0, sizeof(my_addr));
		my_addr.sin_family = AF_INET;
		my_addr.sin_port = htons ((short)port);
		if ( inaddr == NULL )
			my_addr.sin_addr.s_addr = INADDR_ANY; // get_ifadapter_ip("eth0",NULL);
		else
			my_addr.sin_addr.s_addr = inaddr->s_addr;

		if (bind ( udp_fd, (struct sockaddr *) &my_addr, sizeof (SOCKADDR_IN)) < 0)
		{
    	close( udp_fd );
    	udp_fd = -1;
  	}
	}
  return udp_fd;
}

int sock_udp_bind6(int port, const IN6_ADDR *paddr)
{
	SOCKADDR_IN6 	my_addr;
	int			udp_fd;

	if ( (udp_fd = socket( PF_INET6, SOCK_DGRAM, 0 )) >= 0 )
	{
		int			tmp=0;		// disable SO_REUSEADDR as we do not want to other process can share same port
		setsockopt( udp_fd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));
		/*
		 * clear socket option IPV6_V6ONLY make our socket dual stack support. In Linux, disable is the default value.
		 * but not true for winsock2. We have to disable it manually.
		 */
		if ( setsockopt(udp_fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&tmp, sizeof(int)) == -1)
			printf("<WARNING> - disable IPV6_V6ONLY failed --> %s\n", strerror(errno) );

		memset(&my_addr, 0, sizeof(my_addr));
		my_addr.sin6_family = AF_INET6;
		my_addr.sin6_port = htons ((short)port);
		if ( paddr==NULL )
			memcpy(&my_addr.sin6_addr, &in6addr_any, sizeof(IN6_ADDR));
		else
			memcpy(&my_addr.sin6_addr, paddr, sizeof(IN6_ADDR));

		if (bind ( udp_fd, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0)
		{
    	close( udp_fd );
    	udp_fd = -1;
  	}
	}
  return udp_fd;
}

int sock_udp_bindhost(int port,const char *host)
{
	IN_ADDR in_addr;
	IN6_ADDR in6_addr;
	int fd=-1;
	
	if ( host == NULL )
	{
		if ( (fd=sock_udp_bind6(port,NULL)) == -1 )
			fd = sock_udp_bind4(port,NULL);
	}
	else if ( inet_pton(AF_INET, host, &in_addr) > 0 )
		fd = sock_udp_bind4(port,&in_addr);
	else if ( inet_pton(AF_INET6, host, &in6_addr) > 0 )
		fd = sock_udp_bind6(port,&in6_addr);
	return fd;
}

int sock_udp_broadcast( int fd, int enable )
{
	return setsockopt( fd, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable) );
}
	
int sock_udp_bind_broadcast( int port )
{
	int	bBroadcast = 1;
	SOCKADDR_IN saUdpClient;

	int sock_bc = socket(AF_INET,SOCK_DGRAM,0); 
	if ( sock_bc != INVALID_SOCKET
	&& setsockopt( sock_bc, SOL_SOCKET, SO_BROADCAST, &bBroadcast, sizeof ( int )) != -1 )
	{
		saUdpClient.sin_family = AF_INET; 
		saUdpClient.sin_port = htons((u_short)port); 
		saUdpClient.sin_addr.s_addr = INADDR_ANY; 
		if (bind( sock_bc, (SOCKADDR *)&saUdpClient, sizeof(SOCKADDR_IN)) == 0) 
			return sock_bc;
	}
	// error return
	if (sock_bc != INVALID_SOCKET ) 
		sock_close( sock_bc );
	return INVALID_SOCKET;
}
	
int sock_udp_send( int fd, const char *caddr, int port, const void* msg, int len )
{
	int	sockfd=fd;
	int ret=-1;
	int af;
	int addrlen;
	IN_ADDR_UNION common_in_addr;	
	SOCKADDR_UNION su;

	if ( caddr!=NULL )
	{
		af = get_addrfamily(caddr);
		if ( (af != AF_INET && af != AF_INET6) || inet_pton(af, caddr, &common_in_addr) <= 0 )
			return -1;
	}
	else
	{
		// if caddr is NULL, we broadcast on IPv4.
		af = AF_INET;
		common_in_addr.in_addr.s_addr = INADDR_BROADCAST;		// note: htonl() is not required. result is the same
	}
	if ( fd == INVALID_SOCKET )
	{
		sockfd = sock_udp_open(af);
		if ( caddr==NULL )
		{
			int enable = 1;
			setsockopt( sockfd, SOL_SOCKET, SO_BROADCAST, (const char *)&enable, sizeof(enable) );
		}
	}
	memset(&su, 0, sizeof(su));
	set_addrmembers(af, &common_in_addr, port, &su);
	addrlen = af==AF_INET ? sizeof(SOCKADDR_IN) : sizeof(SOCKADDR_IN6);
	ret = sendto( sockfd, (const char *)msg, len, 0, (SOCKADDR *) &su, addrlen );
	if ( fd == INVALID_SOCKET )
		sock_close( sockfd );
	return ret;
}

int sock_udp_send0( int udp_fd, unsigned long ip, int port, const void * msg, int len )
{
	SOCKADDR_IN	udp_addr;
	int	 sockfd=udp_fd, ret;
	
	if ( udp_fd == -1 )
	{
		int bBroadcast=1;

		sockfd = socket( AF_INET, SOCK_DGRAM, 0 );
		if( sockfd == -1 ) return -1;
		if ( ip == INADDR_BROADCAST )
			setsockopt( sockfd, SOL_SOCKET, SO_BROADCAST, &bBroadcast, sizeof(int) );
	}

	memset( & udp_addr, 0, sizeof(udp_addr) );
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_addr.s_addr = ip;		// NOTE - dwIP is already network format. DON'T USE htonl(dwIP)
	udp_addr.sin_port = htons( (short)port );
	if ( len == 0 )
		len = strlen( msg );

	ret = sendto( sockfd, msg, len, MSG_DONTWAIT, (const struct sockaddr *) & udp_addr, sizeof(udp_addr) );

	if ( udp_fd == -1 )
		close( sockfd );

	return ret;
}

// NOTE - if application wants to broadcast message (paddr->sin_addr.s_addr==INADDR_BROADCAST), 
// 				it is app's responsibility to enable the broadcast option on socket fd first.
int sock_udp_send4( int fd, SOCKADDR_IN *paddr, const void* msg, int len )
{
	int rc=-1;
	if ( len == 0 )
		len = strlen( (char *)msg );
	if ( paddr!=NULL )
		rc = sendto( fd, msg, len, 0, (SOCKADDR *)paddr, sizeof(SOCKADDR_IN) );
	return rc;
}

int sock_udp_sendX( int fd, SOCKADDR *paddr, int ndst, const void* msg, int len )
{
	int addrlen;
	if ( len == 0 )
		len = strlen( (char *)msg );
	addrlen = is_sockaddr_v4(paddr) ? sizeof(SOCKADDR_IN) : sizeof(SOCKADDR_IN6);
	return sendto( fd, msg, len, 0, paddr, addrlen*ndst );
}

int sock_udp_send6( int fd, SOCKADDR_IN6 *paddr, const void* msg, int len )
{
	int rc=-1;
	if ( len == 0 )
		len = strlen( (char *)msg );
	if ( paddr!=NULL )
		rc = sendto( fd, msg, len, 0, (SOCKADDR *)paddr, sizeof(SOCKADDR_IN6) );
	return rc;
}
/*
int sock_udp_recv( int fd, void *buf, int size, SOCKADDR *src )
{
	SOCKADDR_STORAGE  sender;
	socklen_t		addrlen = sizeof( sender );
	int		ret;

	if ( (ret = recvfrom( fd, buf, size, 0, (struct sockaddr *)&sender, &addrlen )) > 0 )
	{
		if ( src != NULL )
			memcpy(src, &sender, addrlen);
	}
	return ret;
}
*/
int sock_udp_recv( int fd, void *buf, int size, unsigned long *ip )
{
	SOCKADDR_IN  sender;
	socklen_t		addrlen = sizeof( sender );
	int		ret;

	if ( (ret = recvfrom( fd, buf, size, 0, (struct sockaddr *)&sender, &addrlen )) > 0 )
	{
		if ( ip != NULL )
			*ip = sender.sin_addr.s_addr;
	}
	return ret;
}

int sock_udp_recv2( int fd, void *buf, int size, SOCKADDR *src, int *addrsize )
{
	SOCKADDR_STORAGE  sender;
	socklen_t		addrlen = sizeof( sender );
	int		ret;

	if ( (ret = recvfrom( fd, buf, size, 0, (SOCKADDR *)&sender, &addrlen )) > 0 )
	{
		if ( src != NULL )
			memcpy( (void *)src, (void *)&sender, addrlen );
		if ( addrsize)
			*addrsize = addrlen;
	}
	return ret;
}

//========================== c o m m o n ==============================
int sock_dataready( int fd, int tout )
{
	fd_set	rfd_set;
	struct	timeval tv, *ptv;
	int	nsel;

	FD_ZERO( &rfd_set );
	FD_SET( fd, &rfd_set );
	if ( tout == -1 )
	{
		ptv = NULL;
	}
	else
	{
		tv.tv_sec = 0;
		tv.tv_usec = tout * 1000;
		ptv = &tv;
	}
	nsel = select( fd+1, &rfd_set, NULL, NULL, ptv );
	if ( nsel > 0 && FD_ISSET( fd, &rfd_set ) )
		return 1;
	return 0;
}

int sock_iqueue(int fd)
{
	unsigned long ul;

	if ( ioctl( fd, FIONREAD, &ul ) == 0 )
		return (int)ul;
	return 0;
	
}

int sock_oqueue(int fd)
{
	size_t left;
	if (ioctl(fd, SIOCOUTQ, &left) == 0 )
		return (int)left;
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////
// [IPv4 only]
unsigned long get_ifadapter_ip( const char *ifname, char *buf, SOCKADDR_IN *netmask, SOCKADDR_IN *ip_bcast  )
{
	struct	ifaddrs	*ifaddr, *ifaddr_head;
	unsigned long	ipaddr = 0UL;

	if ( getifaddrs( &ifaddr_head ) == 0 )
	{
		ifaddr = ifaddr_head;
		while ( ifaddr )
		{
			if ( ifaddr->ifa_addr->sa_family == AF_INET &&
		     	( (ifname==NULL && strcmp(ifaddr->ifa_name, "lo")) || 
		     	(ifname && strcmp(ifname,ifaddr->ifa_name)==0) ) )
			{
				ipaddr = ((struct sockaddr_in *)(ifaddr->ifa_addr))->sin_addr.s_addr;
				if ( netmask )  memcpy( netmask,ifaddr->ifa_netmask, sizeof(SOCKADDR_IN) );
				if ( ip_bcast )  memcpy( ip_bcast,ifaddr->ifa_ifu.ifu_broadaddr, sizeof(SOCKADDR_IN) );
				break;
			}
			ifaddr = ifaddr->ifa_next;
		}
		freeifaddrs( ifaddr_head );
	}
	if ( buf && ipaddr )
		INET_NTOA2( ipaddr, buf );
	return ipaddr;
}

int get_mac_addr( const char *ifname, unsigned char *ifaddr )
{
	int		rc=-1, skfd;
	struct ifreq 	ifr;

	if ( ifname==NULL ) ifname = get_ifadapter_name();
	strcpy( ifr.ifr_name, ifname);
		
	if( (skfd = socket(PF_INET, SOCK_DGRAM, 0)) >= 0 )
	{
		if (ioctl(skfd, SIOCGIFHWADDR, &ifr) != -1 )
		{
			memcpy(ifaddr, ifr.ifr_hwaddr.sa_data, 6);
  		rc = 0;
		} 
		close( skfd );
	}
	return rc;
}

int set_mac_addr( const char *ifname, unsigned char *ifaddr )
{
	int		rc=-1, skfd;
	struct ifreq 	ifr;

	if ( ifname==NULL ) ifname = get_ifadapter_name();
	bzero(&ifr,sizeof(ifr));
	strcpy( ifr.ifr_name, ifname);
	ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	
	if( (skfd = socket(PF_INET, SOCK_DGRAM, 0)) >= 0 )
	{
		// bring down interface first

		ioctl(skfd, SIOCGIFFLAGS, &ifr);		
		ifr.ifr_flags &= ~IFF_UP;
		rc = ioctl(skfd, SIOCSIFFLAGS, &ifr);
		if ( rc < 0 )
		{
			printf("ioctl(SIOCSIFFLAGS) bring down interface failed (%s)\n", strerror(errno));
			return rc;
		}				
		// now set mac addr.
		memcpy(ifr.ifr_hwaddr.sa_data,ifaddr, 6);
		rc = ioctl(skfd, SIOCSIFHWADDR, &ifr);
		if ( rc < 0 )
		{
			printf("ioctl(SIOCSIFHWADDR) set interface mac address failed (%s)\n", strerror(errno));
			return rc;
		}				

		// then bring it up again
		ifr.ifr_flags |= IFF_UP;
		rc = ioctl(skfd, SIOCSIFFLAGS, &ifr); 
		if ( rc < 0 )
		{
			printf("ioctl(SIOCSIFFLAGS) bring up interface failed (%s)\n", strerror(errno));
			return rc;
		}				
		
		close( skfd );
	}
	return rc;
}

int get_prefix( unsigned long mask)
{
	int prefix = 0;
	int i = 32;
	for(;i--;)
	{
		if ( (mask & 0x01)==0 ) break;
		mask >>= 1;
		prefix++;
	}
	return prefix;
}

unsigned long get_gatewayaddr( const char *ifname, char *buf )
{
	FILE *fp;
	char iface[16];
	char line[128];
	unsigned long gw_addr=INADDR_NONE, dest_addr, gate_addr;
	struct in_addr inaddr;

	fp = fopen("/proc/net/route", "r");
	if (fp == NULL)
		return gw_addr;
	if ( ifname==NULL )
		ifname = get_ifadapter_name();
		
  /* skip title line */
  fgets(line, sizeof(line),fp);
  while (fgets(line, sizeof(line), fp))
  {
  	if ( strlen(line)<10 ) continue;
		if (sscanf(line, "%s\t%lX\t%lX", iface,&dest_addr,&gate_addr) != 3 || 
					dest_addr != 0 ||
					(ifname!=NULL && strcmp(iface, ifname)!=0) )
				continue;
		gw_addr = gate_addr;
		break;
	}
	fclose(fp);
	if ( buf != NULL )
	{
		inaddr.s_addr = gw_addr;
		strcpy( buf, inet_ntoa(inaddr) );
	}
	return gw_addr;
}

int ifconfig(const char *ifname, const char *ipaddr, const char *netmask, const char *gwip)
{
    struct   sockaddr_in   sin;
    struct   ifreq   ifr;
    int   fd;

    bzero(&ifr,   sizeof(struct   ifreq));
    if (ifname == NULL)
        ifname = get_ifadapter_name();
    fd = socket(PF_INET,SOCK_DGRAM,0);
    if (fd==-1)
        return  -1;

		// set IP address
		if ( ipaddr != NULL )
		{
	    strncpy(ifr.ifr_name, ifname,IFNAMSIZ);
	    ifr.ifr_name[IFNAMSIZ-1] = 0;
	    memset(&sin, 0, sizeof(sin));
	    sin.sin_family = AF_INET;
	    sin.sin_addr.s_addr = inet_addr(ipaddr);
	    memcpy(&ifr.ifr_addr, &sin, sizeof(sin));
	    if (ioctl(fd,SIOCSIFADDR,&ifr)<0)
	    {
	    	close(fd);
	      return -1;
	    }
	  }
    // set netmask
    if (netmask != NULL)
    {
	    bzero(&ifr, sizeof(struct ifreq));
	    strncpy(ifr.ifr_name,   ifname,   IFNAMSIZ);
	    ifr.ifr_name[IFNAMSIZ   -   1]   =   0;
	    memset(&sin,   0,   sizeof(sin));
	    sin.sin_family   =   AF_INET;
	    sin.sin_addr.s_addr   =   inet_addr(netmask);
	    memcpy(&ifr.ifr_addr,   &sin,   sizeof(sin));
	    if(ioctl(fd, SIOCSIFNETMASK, &ifr ) < 0)
	    {
	    	close(fd);
        return -1;
      }
	  }
    /////////////////////////////////
    if ( gwip != NULL )
    {
	    struct rtentry rm;
	    bzero(&rm,   sizeof(struct rtentry));
	    rm.rt_dst.sa_family = AF_INET;
	    rm.rt_gateway.sa_family = AF_INET;
	    rm.rt_genmask.sa_family = AF_INET;
	    memset(&sin,0,sizeof(sin));
	    sin.sin_family = AF_INET;
	    sin.sin_addr.s_addr = inet_addr(gwip);
	    memcpy(&rm.rt_gateway, &sin,   sizeof(sin));
	    rm.rt_dev = (char *)ifname;
	    rm.rt_flags = RTF_UP | RTF_GATEWAY ;
	    if(ioctl(fd, SIOCADDRT, &rm ) < 0)
	    {
	    	close(fd);
	     	return -1;
	    }
	  }
    /////////////////////////////////
    // bring up adapter
    ifr.ifr_flags |= IFF_UP|IFF_RUNNING;
    ioctl(fd,SIOCSIFFLAGS,&ifr);
    close(fd);
    return 0;
}
////////////////////////////////////////////////////////////////
// Socket address functions
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
	char	*ptr = numdot;
	unsigned int	a, b, c, d;

	a = ip & 0x000000ff;
	b = (ip & 0x0000ff00) >> 8;
	c = (ip & 0x00ff0000) >> 16;
	d = (ip & 0xff000000) >> 24;

	sprintf( ptr, "%d.%d.%d.%d", a, b, c, d );
	return ptr;
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

const char *sockaddr_in6_tostring(SOCKADDR_IN6 *in6addr, int withport)
{
	static char straddr[INET6_ADDRSTRLEN+8];
	char *ptr = straddr;
	short port;
	
	if ( withport )
		*(ptr++) = '[';
	port = in6addr->sin6_port;
	port = ntohs(port);
	if ( inet_ntop(AF_INET6, &in6addr->sin6_addr, ptr, sizeof(straddr)) != NULL )
	{
		if ( withport )
		{
			ptr += strlen(ptr);
			*(ptr++) = ']';
			sprintf(ptr, ":%u", (unsigned short)port);
		}
	}
	return straddr;
}

const char *in6addr_tostring( IN6_ADDR *in6addr, int prefix)
{
	static char straddr[INET6_ADDRSTRLEN+8];

	if ( inet_ntop(AF_INET6, in6addr, straddr, sizeof(straddr)) != NULL )
		sprintf(straddr+strlen(straddr), "/%d", prefix);
	else
		straddr[0] = '\0';		
	return straddr;
}

const char* sock_gethostname(int fd, int *port)
{
	static char straddr[INET6_ADDRSTRLEN];
	SOCKADDR_STORAGE ss;
	socklen_t namelen=sizeof(ss);
	if ( getsockname( fd, (SOCKADDR *)&ss, &namelen) == -1 )
		return "";
		
	if ( is_sockaddr_v6(&ss) )
	{
		SOCKADDR_IN6 sa6;
		memcpy(&sa6, &ss, sizeof(SOCKADDR_IN6));
		if ( port ) *port = ntohs(sa6.sin6_port);
		return inet_ntop(AF_INET6, &sa6.sin6_addr, straddr, 0);
	}
	else // if ( is_sockaddr_v4(&ss) )
	{
		SOCKADDR_IN sa;
		memcpy(&sa, &ss, sizeof(SOCKADDR_IN));
		if ( port ) *port = ntohs(sa.sin_port);
		return inet_ntop(AF_INET, &sa.sin_addr, straddr, 0);
	}
}

const char* sock_getpeername( int fd, int *port )
{
	SOCKADDR_STORAGE	ss;
	socklen_t		addrlen = sizeof(ss);

	if ( getpeername( fd,  (SOCKADDR *) &ss, &addrlen ) == 0 )
	{
		if ( is_sockaddr_v6(&ss) )
		{
			SOCKADDR_IN6 sa6;
			memcpy(&sa6, &ss, sizeof(SOCKADDR_IN6));
			if ( port ) *port = ntohs(sa6.sin6_port);
			return sockaddr_in6_tostring(&sa6,0);
		}
		else // if ( is_sockaddr_v4(&ss) )
		{
			SOCKADDR_IN sa;
			memcpy(&sa, &ss, sizeof(SOCKADDR_IN));
			if ( port ) *port = ntohs(sa.sin_port);
			return sockaddr_in_tostring(&sa,0);;
		}
	}
	return "";
}

int sock_gethostaddr(int fd, struct sockaddr *saddr)
{
	SOCKADDR_STORAGE	ss;
	socklen_t		addrlen = sizeof(ss);
	int rc = -1;
	
	if ( getsockname( fd,  (struct sockaddr *) &ss, &addrlen ) == 0 )
	{
		memcpy(saddr, &ss, addrlen);
		rc = addrlen==sizeof(SOCKADDR_IN) ? AF_INET : AF_INET6;
	}
	return rc;
}

int sock_getpeeraddr(int fd, struct sockaddr *saddr)
{
	SOCKADDR_STORAGE	ss;
	socklen_t		addrlen = sizeof(ss);
	int rc = -1;
	
	if ( getpeername( fd,  (struct sockaddr *) &ss, &addrlen ) == 0 )
	{
		memcpy(saddr, &ss, addrlen);
		rc = addrlen==sizeof(SOCKADDR_IN) ? AF_INET : AF_INET6;
	}
	return rc;
}

const char* sockaddr_tostring(SOCKADDR *saddr, int withport)
{
	if ( is_sockaddr_v4(saddr) )
		return sockaddr_in_tostring((SOCKADDR_IN *)saddr, withport);

	if ( is_sockaddr_v6(saddr) )
	{
		// for IPv4 mapped or compatible IPv6 address, we output the IPv4 address instead.
		if ( is_sockaddr_v4mapped(saddr) ||
				 is_sockaddr_v4compat(saddr) )
		{
			static char chIP[32];
			SOCKADDR_IN sa4;
			sockaddr_v6to4(saddr, &sa4);
			strcpy(chIP, sockaddr_in_tostring(&sa4, withport));
			if ( is_sockaddr_v4mapped(saddr) )
				strcat(chIP, "(v4m)");
			else
				strcat(chIP, "(v4c)" );
			return chIP;	
		}
		else
			return sockaddr_in6_tostring((SOCKADDR_IN6 *)saddr, withport);
	}

	return "";	
}

//////////////////////////////////////////////////////////////////////////////////
// [IPv4 & IPv6]

void set_addrmembers(int af, IN_ADDR_UNION *inaddr, int port, SOCKADDR_UNION *saddr)
{
		SOCKADDR_UNION *su = saddr;
		IN_ADDR_UNION *u_addr = inaddr;
		if ( af==AF_INET ) { 
			su->saddr_in.sin_family = af; 
			su->saddr_in.sin_port = htons(port); 
			su->saddr_in.sin_addr.s_addr = u_addr->in_addr.s_addr; 
		} else  {
			su->saddr_in6.sin6_family = af; 
			su->saddr_in6.sin6_port = htons(port); 
			memcpy( &su->saddr_in6.sin6_addr, &u_addr->in6_addr, sizeof(IN6_ADDR)); 
		} 
}

const char *get_ifadapter_name()
{
	static char if_name[8] = "";
	struct	ifaddrs	*ifaddr, *ifaddr_head;

	if ( if_name[0] != '\0' )
		return if_name;
		
	if_name[0] = '\0';
	if ( getifaddrs( &ifaddr_head ) == 0 )
	{
		ifaddr = ifaddr_head;
		while ( ifaddr )
		{
			if ( (ifaddr->ifa_addr->sa_family == AF_INET || ifaddr->ifa_addr->sa_family == AF_INET6) &&
			     strcmp( ifaddr->ifa_name, "lo" ) )
			{
				strcpy(if_name, ifaddr->ifa_name);
				break;
			}
			ifaddr = ifaddr->ifa_next;
		}	
		freeifaddrs( ifaddr_head );
	}
	return if_name;
}

// get all interface adapter (exclude "lo") address of specified family
// 'af' is address family, either AF_INET or AF_INET6. 0 for both
// 'ifi' is array of IFADDRS_INFO to store all interface adapter information in local machine
// 'size' is number of elements in array ifaddr.
// return:
// 	>=0 number of entry stored in 'ifaddr'.
//  -1: error.
int get_ifadapter_all( int af, IFADDRS_INFO *ifi, int size )
{
	struct	ifaddrs	*ifaddr, *ifaddr_head;
	int nifaddr = 0;
	
	memset(ifi,0,sizeof(IFADDRS_INFO)*size);
	if ( getifaddrs( &ifaddr_head ) == 0 )
	{
		ifaddr = ifaddr_head;
		while ( ifaddr && nifaddr<size)
		{
			if ( (ifaddr->ifa_addr->sa_family == af ||
						(af==0 && ifaddr->ifa_addr->sa_family== AF_INET) ||
						(af==0 && ifaddr->ifa_addr->sa_family==AF_INET6) ) &&
			     strcmp( ifaddr->ifa_name, "lo" ) )
			{
				int addrlen = ifaddr->ifa_addr->sa_family==AF_INET ? sizeof(SOCKADDR_IN) : sizeof(SOCKADDR_IN6);
					
				strcpy(ifi[nifaddr].ifa_name, ifaddr->ifa_name);
				ifi[nifaddr].ifa_flags = ifaddr->ifa_flags;
				memcpy(&ifi[nifaddr].ifa_addr, ifaddr->ifa_addr, addrlen);
				if ( ifaddr->ifa_netmask != NULL )
					memcpy(&ifi[nifaddr].ifa_netmask, ifaddr->ifa_netmask, addrlen);
				if ( (ifaddr->ifa_flags & IFF_BROADCAST) && ifaddr->ifa_broadaddr != NULL )
					memcpy(&ifi[nifaddr].ifa_bcastaddr, ifaddr->ifa_broadaddr, addrlen);
				nifaddr++;
			}
			ifaddr = ifaddr->ifa_next;
		}
		freeifaddrs( ifaddr_head );
		return nifaddr;
	}
	else
		return -1;
}

int get_ifadapter_addr(const char *ifname, int af, SOCKADDR *saddr, int *addrlen)
{
	struct	ifaddrs	*ifaddr, *ifaddr_head;
	int rc = -1;
	
	if ( getifaddrs( &ifaddr_head ) == 0 )
	{
		ifaddr = ifaddr_head;
		while ( ifaddr )
		{
			if ( (ifaddr->ifa_addr->sa_family == af ||
						(af==0 && ifaddr->ifa_addr->sa_family== AF_INET) ||
						(af==0 && ifaddr->ifa_addr->sa_family==AF_INET6) ) 
						&&
			     	( (ifname==NULL && strcmp(ifaddr->ifa_name, "lo")) || 
			     	(ifname && strcmp(ifname,ifaddr->ifa_name)==0) ) )
			{
				int addrsize = ifaddr->ifa_addr->sa_family==AF_INET ? sizeof(SOCKADDR_IN) : sizeof(SOCKADDR_IN6);
				if ( saddr )				
					memcpy(saddr, ifaddr->ifa_addr, addrsize);
				if ( addrlen )
					*addrlen = addrsize;
				rc = 0;
				break;
			}
			ifaddr = ifaddr->ifa_next;
		}
		freeifaddrs( ifaddr_head );
	}
	return rc;	
}

int get_addrfamily(const char *addr)
{
		int result;
    struct addrinfo hint, *info =0;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    // Uncomment this to disable DNS lookup
    hint.ai_flags = AI_NUMERICHOST;
    if (getaddrinfo(addr, 0, &hint, &info) != 0 )
    {
    	printf("getaddrinfo for address string %s failed.\n", addr);
      return -1;
    }
    result = info->ai_family;
    freeaddrinfo(info);
    return result;
}

int get_hostaddr( const char *hostname, const char *service, void *inaddr, char *straddr, int szaddr )
{
	int rc = 0;
	struct addrinfo hints;
	struct addrinfo *ainfo, *res;	
	struct sockaddr *sa;
	int	salen, error;

	memset(&hints, 0, sizeof(hints));
	/* set-up hints structure */
	hints.ai_family = PF_UNSPEC;
	hints.ai_flags = AI_ADDRCONFIG;			// find both IPv4 and IPv6.
	if ( hostname!=NULL && isxdigit(hostname[0]) )
		hints.ai_flags |= AI_NUMERICHOST;		// hostname is numerichost (like "192.168.1.1")
	if ( service!=NULL && atoi(service) > 0 )
		hints.ai_flags |= AI_NUMERICSERV;								// 'service' is a string represent a port number
	if ( (error=getaddrinfo(hostname, service, &hints, &ainfo))==0 )
	{
		res = ainfo;
		while (res) 
		{
			sa = res->ai_addr;
			salen = res->ai_addrlen;
			if ( (res->ai_family==AF_INET && ((SOCKADDR_IN *)sa)->sin_addr.s_addr != 0x7f000000) ||
					 (res->ai_family==AF_INET6 && !IN6_IS_ADDR_LOOPBACK( &((SOCKADDR_IN6 *)sa)->sin6_addr ) ) )
			{
				// find first non-loopback address, use it
				memcpy(inaddr, sa, salen);
				rc = res->ai_family;
				if ( straddr )
				{
					inet_ntop(res->ai_family,  
						res->ai_family==AF_INET ? (void *)&((SOCKADDR_IN *)sa)->sin_addr : (void *)&((SOCKADDR_IN6 *)sa)->sin6_addr,
						straddr, szaddr );
				}
				break;
			}
			res = res->ai_next;
		}
		freeaddrinfo(ainfo);
	}
	else
	{
		perror(gai_strerror(error));
		rc = -1;
	}
	return rc;
}

int get_afnetaddrmask( const char *ifname, int af, SOCKADDR* netaddr, SOCKADDR *netmask )
{
	int rc=-1;
	struct	ifaddrs	*ifaddr, *ifaddr_head;

	if ( getifaddrs( &ifaddr_head ) == 0 )
	{
		ifaddr = ifaddr_head;
		while ( ifaddr )
		{
			if ( (ifaddr->ifa_addr->sa_family == af) &&
			     	( (ifname==NULL && strcmp(ifaddr->ifa_name, "lo")) || 
			     	(ifname && strcmp(ifname,ifaddr->ifa_name)==0) ) )
			{
				int addrlen = GET_ADDRLEN(af);
				if ( netaddr )
					memcpy(netaddr, ifaddr->ifa_addr, addrlen);
				if ( netmask )
					memcpy(netmask, ifaddr->ifa_netmask, addrlen);
				rc = 0;
				break;
			}
			ifaddr = ifaddr->ifa_next;
		}	
		freeifaddrs( ifaddr_head );	
	}
  return rc;
}
/*
int set_netmask( const char *ifname, SOCKADDR* netmask )
{
	int rc, fd;
  struct ifreq ifr;
 		
  if((fd = socket(AF_INET,SOCK_DGRAM, IPPROTO_IP)) < 0)
		return -1;
  bzero((char *)&ifr, sizeof(ifr));
  strcpy(ifr.ifr_name,ifname==NULL ? get_ifadapter_name() : ifname);
  ((SOCKADDR_IN *)netmask)->sin_family = AF_INET;
  memcpy(&ifr.ifr_netmask, netmask, sizeof(SOCKADDR_IN));
  rc = ioctl(fd,SIOCSIFNETMASK,&ifr);
  close(fd);
  return rc;	
}
*/
int set_afnetaddrmask(const char *ifname, int af, SOCKADDR* netaddr, u_long prefix)
{
	int fd, rc=-1;
	
  if((fd = socket(af, SOCK_DGRAM, IPPROTO_IP)) < 0)
  	return -1;
  if ( ifname==NULL ) ifname = get_ifadapter_name();
  if ( af==AF_INET )
  {
	  struct ifreq ifr;
	  bzero((char *)&ifr, sizeof(ifr));
	  strcpy(ifr.ifr_name,ifname);
	  if ( netaddr != NULL )
	  {
	  	((SOCKADDR_IN *)netaddr)->sin_family = AF_INET;
		  memcpy((char*)&ifr.ifr_addr, netaddr, sizeof(SOCKADDR_IN));
		  rc = ioctl(fd, SIOCSIFADDR, &ifr);
		}
	  if ( prefix != 0 )
	  {
	  	SOCKADDR_IN sin;
	  	sin.sin_family = AF_INET;
	  	sin.sin_addr.s_addr =  prefix;
		  bzero((char *)&ifr, sizeof(ifr));
		  memcpy(&ifr.ifr_netmask, &sin, sizeof(SOCKADDR_IN));
		  rc = ioctl(fd,SIOCSIFNETMASK,&ifr);
	  }
	}
	else
	{
 		// NOTE - this function add a IPv6 address to specified interface adapter w/o replace existing one
		struct in6_ifreq {
		    struct in6_addr addr;
		    uint32_t        prefixlen;
		    unsigned int    ifindex;
		};
    struct ifreq ifr;
    struct in6_ifreq ifr6;

    // Copy the interface name to the ifreq struct
    strcpy(ifr.ifr_name, ifname?ifname:get_ifadapter_name());
    // get the ifrindex of the interface
    rc = ioctl(fd, SIOGIFINDEX, &ifr);
    if ( rc != 0 )
    {
    	perror("set_afnetaddrmask (SIOGIFINDEX)");
    	close(fd);
    	return -1;
		}		    	
    // Prepare the in6_ifreq struct and set the address to the interface
    memcpy( &ifr6.addr, &((SOCKADDR_IN6*)netaddr)->sin6_addr, sizeof(SOCKADDR_IN6));
    ifr6.ifindex = ifr.ifr_ifindex;
    ifr6.prefixlen = prefix;
    rc = ioctl(fd, SIOCSIFADDR, &ifr6);
		if ( rc != 0 )
    	perror("set_afnetaddrmask (SIOCSIFADDR)");
	}
  close(fd);
  return rc;
}

static int get_ifindex(int fd, const char *ifname)
{
	int rc = -1;
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, ifname);

	if (ioctl(fd, SIOGIFINDEX, &ifr)==0)	
		rc = ifr.ifr_ifindex;
	return rc;
}

int add_afgateway( const char *ifname, int af, SOCKADDR* gw, SOCKADDR* dstnet, int prefix)
{
	int rc, fd;
	
  if((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
  	return -1;
  if ( ifname==NULL ) ifname = get_ifadapter_name();
  if ( af==AF_INET )
  {
    struct rtentry rm;
    bzero(&rm,   sizeof(struct rtentry));
    rm.rt_dst.sa_family = AF_INET;
    ((SOCKADDR_IN*)(&rm.rt_dst))->sin_addr.s_addr = dstnet==NULL ? INADDR_ANY : ((SOCKADDR_IN *)dstnet)->sin_addr.s_addr;
    rm.rt_gateway.sa_family = AF_INET;
    rm.rt_genmask.sa_family = AF_INET;
    if ( gw != NULL )
    {
    	((SOCKADDR_IN*)gw)->sin_family = AF_INET;
    	memcpy(&rm.rt_gateway, gw, sizeof(SOCKADDR_IN));
  	}
    rm.rt_dev = (char *)ifname;
    rm.rt_flags = RTF_UP | RTF_GATEWAY ;
    rc = ioctl(fd, SIOCADDRT, &rm );
	}
	else
	{
		struct in6_rtmsg rt;
		SOCKADDR_IN6 sa6;
		if ( dstnet==NULL )
			memset(&sa6,0,sizeof(sa6));		// for any destination
		else
			memcpy(&sa6,dstnet,sizeof(sa6));
		memset((char *) &rt, 0, sizeof(struct in6_rtmsg));
		rt.rtmsg_flags = RTF_UP | RTF_GATEWAY;
		memcpy(&rt.rtmsg_dst, sa6.sin6_addr.s6_addr, sizeof(struct in6_addr));	
		rt.rtmsg_metric = 256;
		rt.rtmsg_dst_len = prefix;
		// get interface index
		if ( ifname!=NULL && (rt.rtmsg_ifindex=get_ifindex(fd,ifname))==-1 )
		{
			printf("add_afgateway - cannot find interface %s\n", ifname);
			close(fd);
			return -1;
		}
		if ( gw != NULL )
			memcpy(&rt.rtmsg_gateway, ((SOCKADDR_IN6*)gw)->sin6_addr.s6_addr, sizeof(IN6_ADDR));
		rc = ioctl(fd, SIOCADDRT, &rt);	
		if (rc != 0)
			perror("add_afgateway (SIOCADDRT)");
	}
	close(fd);
 	return rc;
}

// command "route del default" same as del_afgateway(NULL,AF_INET,NULL,NULL) function does
int del_afgateway( const char *ifname, int af, SOCKADDR* gw, SOCKADDR* dstnet, int prefix)
{
	int rc, fd;
	
  if((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
  	return -1;
  if ( ifname==NULL ) ifname = get_ifadapter_name();
  if ( af==AF_INET )
  {
    struct rtentry rm;
    bzero(&rm,   sizeof(struct rtentry));
    rm.rt_dst.sa_family = AF_INET;
    ((SOCKADDR_IN*)&rm.rt_dst)->sin_addr.s_addr = dstnet==NULL ? INADDR_ANY : ((SOCKADDR_IN *)dstnet)->sin_addr.s_addr;
    rm.rt_genmask.sa_family = AF_INET;
    if ( gw != NULL )
    {
	    ((SOCKADDR_IN*)gw)->sin_family = AF_INET;
    	memcpy(&rm.rt_gateway, gw, sizeof(SOCKADDR_IN));
	    ((SOCKADDR_IN*)&rm.rt_gateway)->sin_addr.s_addr = ((SOCKADDR_IN *)gw)->sin_addr.s_addr;
    }
    rm.rt_dev = (char *)ifname;
    rm.rt_flags = RTF_GATEWAY;
    rc = ioctl(fd, SIOCDELRT, &rm );
		if (rc != 0)
			perror("del_afgateway (SIOCDELRT)");
	}
	else
	{
		// delete specified or default gateway
		struct in6_rtmsg rt;
		SOCKADDR_IN6 sa6;
		if ( dstnet==NULL )
			memset(&sa6,0,sizeof(sa6));		// for any destination
		else
			memcpy(&sa6,dstnet,sizeof(sa6));
		memset((char *) &rt, 0, sizeof(struct in6_rtmsg));
		rt.rtmsg_flags = RTF_UP | RTF_GATEWAY;
		memcpy(&rt.rtmsg_dst, sa6.sin6_addr.s6_addr, sizeof(struct in6_addr));	
		rt.rtmsg_metric = 1;
		rt.rtmsg_dst_len = prefix;
		rt.rtmsg_ifindex = get_ifindex(fd,ifname);
		if ( gw != NULL )
			memcpy(&rt.rtmsg_gateway, ((SOCKADDR_IN6*)gw)->sin6_addr.s6_addr, sizeof(IN6_ADDR));
		rc = ioctl(fd, SIOCDELRT, &rt);
		if (rc != 0)
			perror("del_afgateway (SIOCDELRT)");
	}
	close(fd);
 	return rc;
}

//////////////////////////////////////////////////////////////////////////////////
// [IPv6 only]
int get_ipv6ifaddr( const char *ifname, IN6_ADDR *inaddr6 )
{
	int rc=-1;
#if 1
	// this is a temporary solution by parsing the ip command output (poor performace)
	char cmd[64] = "ip -6 addr show dev ";
	FILE *fp;
	
	strcat(cmd, ifname ? ifname : get_ifadapter_name());
	if ( (fp=popen(cmd, "r"))!=NULL )
	{
		char *cp, *ptr2, buf[128];
		while( fgets(buf, sizeof(buf), fp) != NULL )
		{
			if ( (cp=strstr(buf,"inet6"))!=NULL )
			{
				cp += 6;
				ptr2 = strchr(cp, '/');
				if ( ptr2 != NULL )
					*ptr2 = '\0';
				inet_pton(AF_INET6, cp, inaddr6);
				rc = 0;
				if ( ptr2 != 0 )	rc = atoi(ptr2+1);
				break;
			}
		}
		pclose(fp);
	}
#else	
	// this approach though professional, cannot obtain prefix length
	struct	ifaddrs	*ifaddr, *ifaddr_head;

	if ( getifaddrs( &ifaddr_head ) == 0 )
	{
		ifaddr = ifaddr_head;
		while ( ifaddr )
		{
			if ( (ifaddr->ifa_addr->sa_family == AF_INET6) &&
			     	( (ifname==NULL && strcmp(ifaddr->ifa_name, "lo")) || 
			     	(ifname && strcmp(ifname,ifaddr->ifa_name)==0) ) )
			{
				/* code to get numeric host string. same as use inet_ntop 
				char host[NI_MAXHOST];
				getnameinfo(ifa->ifa_addr,
                      (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                            sizeof(struct sockaddr_in6),
                       host, NI_MAXHOST,
                       NULL, 0, NI_NUMERICHOST);		
        */
       	memcpy(inaddr6, &((SOCKADDR_IN6 *)ifaddr->ifa_addr)->sin6_addr, sizeof(IN6_ADDR));
       	rc = 0;
				break;
			}
			ifaddr = ifaddr->ifa_next;
		}	
		freeifaddrs( ifaddr_head );
	}
#endif	
	return rc;
}

extern int add_ipv6addr( const char *ifname, IN6_ADDR *in6addr, int prefix)
{
	char cmd[128];
	char chipv6addr[64];
	
	if ( ifname == NULL ) ifname = get_ifadapter_name();
	if ( inet_ntop(AF_INET6, in6addr, chipv6addr, sizeof(chipv6addr)) > 0 )
	{
		sprintf(cmd, "ip -6 addr add %s/%d dev %s", chipv6addr, prefix, ifname);
		system(cmd);
		return 0;
	}
	return -1;
}

extern int del_ipv6addr( const char *ifname, IN6_ADDR *in6addr, int prefix)
{
	char cmd[128];
	char chipv6addr[64];
	
	if ( ifname == NULL ) ifname = get_ifadapter_name();
	if ( inet_ntop(AF_INET6, in6addr, chipv6addr, sizeof(chipv6addr)) > 0 )
	{
		sprintf(cmd, "ip -6 addr del %s/%d dev %s", chipv6addr, prefix, ifname);
		system(cmd);
		return 0;
	}
	return -1;
}

int get_ipv6ifgw( const char *ifname, IN6_ADDR* gw )
{
	char buf[128];
	FILE *fp;
	int rc = -1;
	
  if ( ifname==NULL ) ifname = get_ifadapter_name();
  sprintf(buf, "ip -6 route show dev %s", ifname);
// command output will be like:
// 2000::/3  metric 1024  expires 21334302sec mtu 1500 advmss 1440 hoplimit 4294967295
// default  metric 1024  expires 21333205sec mtu 1500 advmss 1440 hoplimit 4294967295
  if ( (fp=popen(buf, "r")) != NULL )
  {
  	while( fgets(buf,sizeof(buf),fp) != NULL )
  	{
  		char *cp = strchr(buf, ' ');
  		if ( cp != NULL )
  		{
  			*cp = '\0';
  			if ( strlen(buf) > 0 )
  			{
  				if ( strcmp(buf,"default")==0 )
  					memset( gw->s6_addr,0, sizeof(IN6_ADDR));
  				else
  				{
  					rc = 0;
  					if ( (cp=strchr(buf, '/')) != NULL )
  					{
  						*cp = '\0';
  						rc = atoi(cp+1);
  					}
  					inet_pton(AF_INET6, buf, gw);
  				}
  				break;
  			}
  		}
  	}
  	pclose(fp);
  }
  return rc;
}

int find_ipv6ifgw( const char *ifname, IN6_ADDR* gw, int prefix )
{
	char buf[128];
	char niddle[64]="";
	FILE *fp;
	int rc = 0;
	
  if ( ifname==NULL ) ifname = get_ifadapter_name();
  sprintf(buf, "ip -6 route show dev %s", ifname);
  inet_ntop(AF_INET6, gw, niddle, sizeof(niddle));
  sprintf(niddle+strlen(niddle), "/%d", prefix);
// command output will be like:
// 2000::/3  metric 1024  expires 21334302sec mtu 1500 advmss 1440 hoplimit 4294967295
// default  metric 1024  expires 21333205sec mtu 1500 advmss 1440 hoplimit 4294967295
  if ( (fp=popen(buf, "r")) != NULL )
  {
  	while( fgets(buf,sizeof(buf),fp) != NULL )
  	{
			if ( strstr(buf,niddle) != NULL )
			{
				rc = 1;
				break;
			}
  	}
  	pclose(fp);
  }
  return rc;
}

int add_ipv6gw( const char *ifname, IN6_ADDR *gw, int prefix)
{
	char cmd[128];
	char chipv6gw[64];
	
	if ( ifname == NULL ) ifname = get_ifadapter_name();
	if ( inet_ntop(AF_INET6, gw, chipv6gw, sizeof(chipv6gw)) > 0 )
	{
		sprintf(cmd, "ip -6 route add %s/%d dev %s", chipv6gw, prefix, ifname);
		system(cmd);
		return 0;
	}
	return -1;
}

int del_ipv6gw( const char *ifname, IN6_ADDR *gw, int prefix)
{
	char cmd[128];
	char chipv6gw[64];
	
	if ( ifname == NULL ) ifname = get_ifadapter_name();
	if ( inet_ntop(AF_INET6, gw, chipv6gw, sizeof(chipv6gw)) > 0 )
	{
		sprintf(cmd, "ip -6 route del %s/%d dev %s", chipv6gw, prefix, ifname);
		system(cmd);
		return 0;
	}
	return -1;
}

/*
int is_sockaddr_v4(void *ss)
{
	SOCKADDR_IN *sa = (SOCKADDR_IN *)ss;
	return sa->sin_family==AF_INET;
}

int is_sockaddr_v6(void *ss)
{
	SOCKADDR_IN6 *sa = (SOCKADDR_IN6 *)ss;
	return sa->sin6_family==AF_INET6;
}
*/		
// test if a IPv6 address is an IPv4 mapped
int is_sockaddr_v4mapped(void *ss)
{
	SOCKADDR_IN6 *sa = (SOCKADDR_IN6 *)ss;
	return sa->sin6_addr.s6_addr32[0]==0  
				&& sa->sin6_addr.s6_addr32[1]==0  
				&& sa->sin6_addr.s6_addr32[2]==htonl(0xffff);
}				
// test if a IPv6 address is an IPv4 compatible one
int is_sockaddr_v4compat(void *ss)
{
	SOCKADDR_IN6 *sa = (SOCKADDR_IN6 *)ss;
	return sa->sin6_addr.s6_addr32[0]==0  
				&& sa->sin6_addr.s6_addr32[1]==0  
				&& sa->sin6_addr.s6_addr32[2]==0 
				&& ntohl(sa->sin6_addr.s6_addr32[3]) > 1;
}
// sa6 is a pointer to SOCKADDR_STORAGE or SOCKADDR_IN6
// sa4 is a pointer to SOCKADDR_STORAGE or SOCKADDR_IN
// this macro is used to rerse the mapped or compatible IPv6 address back to IPv4
void sockaddr_v6to4(void *ss6, void *ss4) 
{
	SOCKADDR_IN6 *sa6 = (SOCKADDR_IN6 *)ss6;
	SOCKADDR_IN *sa4 = (SOCKADDR_IN *)ss4;
	
	sa4->sin_family = AF_INET;
	sa4->sin_port = sa6->sin6_port;
	memcpy( &sa4->sin_addr.s_addr, &sa6->sin6_addr.s6_addr[12], 4);
}

int is_sockaddr_equal(void *ss1, void *ss2)
{
	if ( is_sockaddr_v4(ss1) )
	{
		SOCKADDR_IN *sa1 = (SOCKADDR_IN *)ss1;
		SOCKADDR_IN *sa2 = (SOCKADDR_IN *)ss2;
		return sa1->sin_addr.s_addr == sa2->sin_addr.s_addr;
	} 
	else // if ( is_sockaddr_v6(ss1,ss2) )
	{
		SOCKADDR_IN6 *sa1 = (SOCKADDR_IN6 *)ss1;
		SOCKADDR_IN6 *sa2 = (SOCKADDR_IN6 *)ss2;
		return IN6_ARE_ADDR_EQUAL(&sa1->sin6_addr, &sa2->sin6_addr);
	}
}
#endif