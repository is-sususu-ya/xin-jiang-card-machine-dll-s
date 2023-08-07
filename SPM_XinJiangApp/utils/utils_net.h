/***************************************************************************
                          utils_net.h  -  description
                             -------------------
    begin                : Thu Dec 20 2001
    copyright            : (C) 2001 by Liming Xie
    email                : liming_xie@yahoo.com
    
    Revised by		: Thomas Chang
    						  : Many revisions
 ***************************************************************************/

#ifndef _UTILS_NET_H_
#define _UTILS_NET_H_

#ifdef ENABLE_IPV6
#include "utils_net6.h"
#else

#define		MSEC_PER_SEC		1000
#define		USEC_PER_SEC		1000000

#ifdef __cplusplus
extern "C" {
#endif

#ifdef linux
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr_in SOCKADDR_STORAGE;
typedef struct sockaddr	SOCKADDR;
typedef struct in_addr  IN_ADDR;		// sin_addr member of SOCKADDR_IN
#define INET_ADDRSTRLEN	16
#define SOCKADDR_LEN(ss)	sizeof(SOCKADDR_IN)
#define SOCKADDR_UNION	SOCKADDR_IN

#ifndef _TYPEDEF_SOCKET_
#define _TYPEDEF_SOCKET_
typedef int SOCKET;
#endif


#define INVALID_SOCKET	(SOCKET)(~0)
#define SOCKET_ERROR            (-1)

#endif

#define IFNAME_LEN	16
typedef struct {
	char ifa_name[IFNAME_LEN];
	IN_ADDR	addr;
	IN_ADDR mask;
	IN_ADDR broadcast;
} IF_ATTR4;	
	
/* ------------- common socket related input/output ---------- */

extern int sock_connect_timeout( const char *host, int port, int msec );
#define sock_connect(host,port)	sock_connect_timeout(host,port,0)

/* ipbind can be NULL, for any address to bind */
extern int sock_listen( int port, const char *ipbind, int backlog );
#define sock_listen4	sock_listen

extern int sock_accept( int fd_listen );

/* ensure safe read/write, without EAGAIN/EINTR */

/*
 * 'sock_set_recvlowat' 
 * set minimum number of bytes to be received before return from recv call. 
 * NOTE - select and poll system call does not respect this SO_RCVLOWAT setting in socket.
 * select and poll will wake-up even only 1 bytes data is received.
 * 设置 sock_set_recvlowat目的是在recv时候，给最少能让recv返回的字节数，例如
 *
 * sock_set_recvlowat(fd,50)
 * if ( select(fd+1,&rset,NULL,NULL)==1 )	// 即使只有一个字节可读也会醒来
 * {
 * 	  recv(fd,buf,100);		
 * }  
 * 如果目前不到50个字节可以读，recv就会被block住（如果没有设置non-block mode）直到
 * 读到50~100个字节（看底层已经接收到多少字节可以读取)才会返回。
 * 但是如果之前没有设置 sock_set_recvlowat(fd,50)，那么这时候recv就会立刻返回socket底层已经收到
 * 的字节数(1~100)。如果设置了block mode，recv依然是读了所有可读的字节数后返回，只有non-block
 * mode的时候，recv会返回-1,errno会给EAGAIN，表示没有数据可以读。或是设置recv的超时时间，recv
 * 在这段时间内没有任何数据，一样返回-1,errnor为EAGAIN。
 * recv返回-1，errno为EINTR的情况是还没收到任何数据之前，程序收到signal提前返回。
 * 如果程序是先select到可读再去recv，那是不可能有EINTR的情况。
 * socket创建时候，默认的SO_RCVLOWAT是1，所以当select/poll的结果可读，recv是不会被block住的，
 * 因为至少有1个字节可以读。 
 * sock_set_recvlowat 在实际编程时候用处不大，除非是死活要读固定的字节数，直到天荒地老也可以，
 * 那可以就用这个函数设置后，再去recv。
 */
extern int sock_set_recvlowat(int fd, int bytes);

/*
 * 'sock_set_nonblock'
 * set TCP socket as non-blocking mode ('non_block' not 0) or blocking mode ('non_block' is 0)
 * when TCP socket created, it is in blocking mode.
 * 在每次recv时，最后一个参数使用MSG_DONTWAIT也有一样的效果，当socket设置non-blocking mode
 * 相当于在每次recv或是send时候，最后一个参数使用了MSG_DONTWAIT.
 */
int sock_set_nonblock(int fd, int non_block);

/*
 * 'sock_set_timeout' - set socket send/receive time-out value (in msec)
 * 'which' is combination of _SO_SEND or _SO_RECV
 * 
 */
#define _SO_SEND	0x01
#define _SO_RECV	0x02
// set socket send/recv time-out in 'tout' millisecond. If the 'tout' is set to zero 
// (the default) then the operation will never timeout
extern int sock_set_timeout(int fd, int which, int tout);
extern int sock_get_timeout(int fd, int which);
/*
 * 'sock_wait_for_io_or_timeout' - wait until timed-out or socket is avaible for 
 *  reading (when 'for_read' is not 0), or for writing (when 'for_read' is 0)
 *  msec > 0 will block 'msec', msec==0 non-block, msec==-1 block forever until data available
 * return 
 *  0 for specified R/W operation available, -1 not available
 */
extern int sock_wait_for_io_or_timeout(int fd, int for_read, long msec);
#define sock_dataready(fd,msec)	(sock_wait_for_io_or_timeout(fd,1,msec)==0)

extern int sock_read(int fd, void* buffer, int buf_size);
extern int sock_write(int fd, const void* buffer, int buf_size);
// read whatever is available to read then return
extern int sock_read_nowait(int fd, void* buffer, int buf_size);
// write whatever is available to write then return
extern int sock_write_nowait(int fd, const void* buffer, int buf_size);

/* 
 * read/peek input queue until 'eol' byte is encounted ('eol' is also read/peek into buffer)
 * return value is number of bytes has been read. buffer is terminated by '\0'
 */
extern int sock_peek_until_tout(int fd, char* buffer, int buf_size, int eol, int tout);
extern int sock_read_until_tout(int fd, char *buffer, int buf_size, int eol, int tout);
#define sock_peek_until(fd,buffer,buf_size,eol)	sock_peek_until_tout(fd,buffer,buf_size,eol,-1)
#define sock_read_until(fd,buffer,buf_size,eol)	sock_read_until_tout(fd,buffer,buf_size,eol,-1)
#define sock_read_line(fd,buf,sz)	sock_read_until(fd,buf,sz,(int)'\n')
#define sock_peek_line(fd,buf,sz)	sock_read_until(fd,buf,sz,(int)'\n')

/* 
 * 'sock_drain' clean all buffered input data in tcp/ip stack. available for both TCP and UDP
 * 'sock_drain_until' drop tcp stream input queue until byte sequence 'soh' which has 'ns' bytes 
 * are encounted. soh in input stream will not be dropped. Only all data before 'soh'. 
 * Useful to sync. to next packet (application protocol packet) started and packet has a constant 
 * SOH byte sequence.
 */
extern int sock_drain( int fd );
extern int sock_drain_until( SOCKET fd, void *soh, int ns );

 
extern int sock_read_n_bytes_tout(int fd, void *buffer, int n, int tout);
#define sock_read_n_bytes(fd,buffer,n)	sock_read_n_bytes_tout(fd,buffer,n,-1)
extern int sock_write_n_bytes(int fd, const void *buffer, int n);

extern int sock_skip_n_bytes_tout(int fd, int n, int tout);
#define sock_skip_n_bytes(fd,n)	sock_skip_n_bytes_tout(fd,n,-1)


extern int sock_is_connected( int fd );

extern int sock_udp_open();
extern int sock_udp_bindhost(int port, const char *host);
extern int sock_udp_bindLocalIP( unsigned long ulIP, int port );
#define sock_udp_bind(p)	sock_udp_bindhost(p,NULL)
#define sock_udp_bind4		sock_udp_bindhost

extern int sock_udp_broadcast( int fd, int enable );	// enable: 1 yes, 0 no
// create a UDP socket, send messgae then close it. ip==NULL is broadcasting
extern int sock_udp_send( const char *ip, int port, const void* msg, int len );
// 'sock_udp_send0' - 旧版本，尽量别用，以后版本可能会删除。注意：ip 已经是network byte order.
extern int sock_udp_send0( int fd, unsigned long net_order_ip, int port, const void *msg, int len ); 
// send to one or multiple destination. paddr==NULL or ndst==0 means broadcasting
extern int sock_udp_sendX( int fd, SOCKADDR_IN *paddr, int ndst, const void *msg, int len );
// receive a udp packet, fd should be bind to specific port. saIn is sender's net address.
extern int sock_udp_recv2( int fd, void *buf, int size, SOCKADDR *saIn, int *addrlen );
#define sock_udp_recv(fd,buf,sz,saddr)	sock_udp_recv2(fd,buf,sz,(SOCKADDR *)saddr,NULL)
#define sock_udp_recv0 	sock_udp_recv

extern int sock_iqueue(int fd);		// any queued input data (length of data)
extern int sock_oqueue(int fd);		// any pending output data (bytes of pending data)

#define sock_close(s)		close(s)

/*
 * ping 某个IP(IPv4 addr) 及 端口（port可以是0，如果只要检查IP是否存在)，使用ICMP包发送。 'ping_times'是测试次数，
 * 函数返回值是平均返回时间(ms)，只计入有返回的包时间。 如果所有ping包都超时没有收到，函数返回-1.
 * 如果输入的IP非法或是domain name无法解析，也是返回-1
 */
extern int ping(const char *IP, unsigned short port, int ping_times);

/* get host IP address -- first non-loop (127.0.0.1) bound IP */
/* hostname: NULL for local host. Otherwise, the target host name
 *      buf: Output buffer of IP dot number string.
 * return:
 *      packed IP address
 */
extern unsigned long get_host_ip( const char *hostname, char *buf );
/* 
 * in following functions
 * if ifa_name is NULL, then return address of first adapter (exclude "lo")
 */
extern const char *get_ifadapter_name();
extern unsigned long get_ifadapter_ip( const char *ifa_name, char *buf, SOCKADDR_IN *netmask, SOCKADDR_IN *ip_bcast );
extern int get_all_interface(IF_ATTR4 ifattr[], int size);
/*
 * ifconfig: config network interface adapter 
 * ifname: name of adapter, NULL mean first non lo interface (usually eth0)
 * ip, netmask, gwip: ip address, netmask and defaut gateway IP. NULL means do not change
 */
extern int ifconfig(const char *ifname, const char *ipaddr, const char *netmask, const char *gw);
extern int get_netaddr( const char *ifname, SOCKADDR_IN *netaddr );
extern int set_netaddr( const char *ifname, SOCKADDR_IN *netaddr );
extern int get_netmask( const char *ifname, SOCKADDR_IN *netmask );
extern int set_netmask( const char *ifname, SOCKADDR_IN *netmask );
// default gateway operation:
// 
// 'get_gatewayaddr'- return value is IPv4 gateway IP in network byte order
//    this addr. also assigned to 'buf' in readable format if 'buf' is not null.
// 'add_defgateway' - add a default gateway
// 'del_defgateway' - delete a default gateway (gwaddr can be INARRY_ANY, for any defaut gateway)
extern unsigned long get_gatewayaddr( const char *ifname, char *buf );
extern int add_default_gateway(const char *ifname, SOCKADDR_IN *pgwaddr);
extern int del_default_gateway(const char *ifname, SOCKADDR_IN *pgwaddr);
#define del_defgateway(af)	del_default_gateway(NULL,NULL)

extern int get_mac_addr( const char *ifname, unsigned char *hwaddr );		// hwaddr: 6 bytes mac addr.
extern int set_mac_addr( const char *ifname, unsigned char *hwaddr );
extern const char *mac2string(unsigned char *hwaddr);

/* get peer name (usually, IP adddress, unless host name for the ip is given in dns server or
 * 'hosts' file.
 *
 */
extern const char* sock_getpeername( int fd, int *port );
extern int sock_getpeeraddr(int fd, SOCKADDR *saddr);
extern const char *sockaddr_in_tostring(SOCKADDR_IN *addr, int withport);
#define sockaddr_tostring(addr,withport)	sockaddr_in_tostring((SOCKADDR_IN*)addr,withport)
#define SOCKADDR_IN_TOSTRING(saddr,p)	sockaddr_in_tostring((SOCKADDR_IN *)saddr,p)
#define SOCKADDR_TOSTRING(saddr,p)	sockaddr_in_tostring((SOCKADDR_IN *)saddr,p)
int is_sockaddr_equal(void *ss1, void *ss2);

// helper - same as inet_addr. but caller does not have to include many socket header files.
extern unsigned long INET_ATON( const char *ipstr );
extern const char *INET_NTOA( unsigned long ip );
extern const char *INET_NTOA2( unsigned long ip, char *numdot );


#define SOCKADDR_IN_ADDR(saddr)		( saddr.sin_addr.s_addr )
#define SOCKADDR_IN_PORT(saddr)		( saddr.sin_port )
#define SOCKADDR_IN_FAMILY(saddr)	( saddr.sin_family )

#define SOCKADDR_IN_SET(saddr,ip,port) \
	do { \
			saddr.sin_family = AF_INET; \
			saddr.sin_addr.s_addr = ip; \
			saddr.sin_port = port; \
	} while(0)

#define SOCKADDR_IN_SET_BROADCAST(saddr,port) SOCKADDR_IN_SET(saddr,INADDR_BROADCAST,htons(port))

#define SOCKADDR_IN_GET(saddr,ip,port) \
	do { \
			ip = saddr.sin_addr.s_addr; \
			port = saddr.sin_port; \
	} while(0)

#define SOCKADDR_IN_SET_FROMHOST(saddr,ip,port) \
	do { \
			saddr.sin_family = AF_INET; \
			saddr.sin_addr.s_addr = htonl(ip); \
			saddr.sin_port = htons(port); \
	} while(0)

#define SOCKADDR_IN_GET_TOHOST(saddr,ip,port) \
	do { \
			ip = ntohl(saddr.sin_addr.s_addr); \
			port = ntohs(saddr.sin_port); \
	} while(0)
		
// IPV6 and IPV4 compatible source
#define is_sockaddr_v4(saddr)				(1)
#define is_sockaddr_v4mapped(saddr)	(0)
#define sockaddr_v6to4(v6addr,v4addr)			// do nothing
 

#ifdef __cplusplus
};
#endif

#endif		// #ifdef ENABLE_IPV6

#endif /* _UTILS_NET_H_ */
