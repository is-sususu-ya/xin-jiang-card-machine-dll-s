/***************************************************************************
                          utils_net.h  -  description
                             -------------------
    begin                : Thu Dec 20 2001
    copyright            : (C) 2001 by Liming Xie
    email                : liming_xie@yahoo.com
    
    Revised by		: Thomas Chang
    						: : Many revisions
    2016-03-26		: Thomas Chang
    	大幅修改，增加IPv6功能
    	
 ***************************************************************************/

#ifndef _UTILS_NET6_H_
#define _UTILS_NET6_H_

// you can use this macro to identify which version of source you are deal with. 
#define UTILSNET_VERSION	2				// define version since this version. Old version one does not have this macro

#define		MSEC_PER_SEC		1000
#define		USEC_PER_SEC		1000000

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr_in6 SOCKADDR_IN6;
typedef struct sockaddr_storage SOCKADDR_STORAGE;
typedef struct sockaddr	SOCKADDR;
typedef struct in_addr  IN_ADDR;		// sin_addr member of SOCKADDR_IN
typedef struct in6_addr IN6_ADDR;		// sin6_addr member of SOCKADDR_IN6
typedef	union {
		IN_ADDR	in_addr;
		IN6_ADDR in6_addr;
	} IN_ADDR_UNION;
typedef union {
		SOCKADDR_IN		saddr_in;
		SOCKADDR_IN6	saddr_in6;
		SOCKADDR_STORAGE saddr_st;
	} SOCKADDR_UNION;

typedef struct {
	char ifa_name[16];				// interface adapter name like "eth0". 16 is maximum for Linux (visible is 15)
	unsigned int ifa_flags;	// Flags as from SIOCGIFFLAGS ioctl.
	SOCKADDR_STORAGE	ifa_addr;
	SOCKADDR_STORAGE	ifa_netmask;			
	SOCKADDR_STORAGE	ifa_bcastaddr;		// broadcast address. valid if IFF_BROADCAST bit is set in `ifa_flags'
} IFADDRS_INFO;
// note: ifa_flags 的所有flag在 man netdevice(7) 里面有
	
// for source compatible with winsock
typedef int SOCKET;
#define INVALID_SOCKET	(SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#define sock_close(s)	close(s)
/* 
 * macros common used for special address define in netinet/in.h (linux) and ws2def.h (windows)
for IPv4
#define INADDR_ANY					((in_addr_t) 0x00000000)
#define INADDR_BROADCAST       ((in_addr_t) 0xffffffff)
# define INADDR_LOOPBACK        ((in_addr_t) 0x7f000001)  // Inet 127.0.0.1.
extern const struct in6_addr in6addr_any;			 notation is [::]		-- all 16 bytes zero 
extern const struct in6_addr in6addr_loopback;     notation is [::1]   
#define IN6ADDR_ANY_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } } }		// in6_addr initializer for ANY
#define IN6ADDR_LOOPBACK_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } } }  // in6_addr initializer for Loopback

#define INET_ADDRSTRLEN 16    // IPv4地址最长字串 （包括0结尾）不包括端口号
#define INET6_ADDRSTRLEN 46		// IPv6地址最长字串 （包括0结尾）不包括端口号 最长表达式样为 "CDCD:910A:2222:5498:8475:201.199.244.101/64"
*/
#define LOCAL_HOST	"127.0.0.1"
#define LOCAL_HOST6	"::1"
#define ANY_HOST		"0:0:0:0"
#define ANY_HOST6		"::"

/* 'sock_connect' is the top level function. host can be a 
   1. IPv4 addr string such as "192.168.1.100"
   2. IPv6 addr string such as "fe80::41b4:3ede:9eca:7119"
 sock_connect will test which family the 'host' should be and invoke sock_connect4 or sock_connect6
 sock_connect is high level function and will invoke sock_connect4 or sock_connect6, depends on 'host' value.
 if .'inaddr' argument is null, sock_connect4, sock_connect6 will connect loopback address.
   
*/
extern int sock_connect( const char *host, int port );
extern int sock_connect4(const IN_ADDR *inaddr, int port);		//  尽量用这个版本	
extern int sock_connect6(const IN6_ADDR *inaddr, int port);	
extern int sock_connect0( unsigned long net_order_ip, int port );		// 老版本接口【NOTE: net_order_ip是 network byte order】

/*
 * 'sock_listen' create a tcp socket, bind and listen to specifed interface IPv4 or IPv6 address given in 'ipbind' at port nnumber 'port'.
 *  if 'ipbind' is null, it bind to ANY interface address of IPv4 network on local machine
 *  'backlog' is number of pending connection request is allowed in the queue. for busy server such as HTTP server, this number shall be larger.
 * 'sock_listen4' and 'sock_listen6' is lower level functions invoked by sock_listen. User can also invoke these two functions directly.
 * for these two lower level functions, if 'inaddr' is null, socket will listen to ANY addresses in local machine
 */
extern int sock_listen( int port, const char *ipbind, int backlog );	
extern int sock_listen4(int port, const IN_ADDR *inaddr, int backlog );
extern int sock_listen6(int port, const IN6_ADDR *inaddr, int backlog );
extern int sock_accept2(int listen_sock, SOCKADDR *caddr);
#define sock_accept( s )	sock_accept2(s,NULL)		// 提供和旧版本兼容的宏

extern int sock_set_timeout( long time_ms );  // not useful
/*
 * 'sock_set_recvlowat'
 * set minimum number of bytes to be received before wakeup (therefore, select/poll will detect readable set)
 * only for TCP socket.
 */
extern int sock_set_recvlowat(int fd, int bytes);

extern int sock_read(int fd, void* buffer, int buf_size);
extern int sock_write(int fd, const void* buffer, int buf_size);

/* 
 * read/peek input queue until 'eol' byte is encounted ('eol' is also read/peek into buffer)
 * return value is number of bytes has been read. buffer is terminated by '\0'
 */
extern int sock_peek_until(int fd, char* buffer, int buf_size, int eol);
extern int sock_read_until(int fd, char *buffer, int buf_size, int eol);
#define sock_read_line(fd,buf,sz)	sock_read_until(fd,buf,sz,(int)'\n')
#define sock_peek_line(fd,buf,sz)	sock_read_until(fd,buf,sz,(int)'\n')

/* read EXACT n bytes from socket,
 * return: bytes readed, or -1 when failed */
extern int sock_read_n_bytes(int fd, void* buffer, int n);
extern int sock_write_n_bytes(int fd, void *buffer, int n);
extern int sock_skip_n_bytes(int fd, int n);
/* 
 * 'sock_drain' clean all buffered input data in tcp/ip stack. available for both TCP and UDP
 * 'sock_drain_until' drop tcp stream input queue until byte sequence 'soh' which has 'ns' bytes 
 * are encounted. soh in input stream will not be dropped. Only all data before 'soh'. 
 * Useful to sync. to next packet (application protocol packet) started and packet has a constant 
 * SOH byte sequence.
 */
extern int sock_drain( int fd );
extern int sock_drain_until( int fd, void *soh, int ns );

extern int sock_is_connected( int fd );

/*
 * 'sock_udp_open' open a UDP socket on specified address network 'af' (AF_INET or AF_INET6)
 * 'sock_udp_bind4' - create a UDP socket on IPv4 network of interface 'paddr'. If paddr is
 * null, it bind to all interface.
 * 'sock_udp_bind6' - same as sock_udp_bind, only it work on IPv6 network.
 * 'sock_udp_bindhost' - accept interface address 'host' to bind. 'host' can be IPv4 or IPv6
 *  	if 'host' is NULL, it will be same as sock_udp_bind4(NULL,port)
 *	  macro LOCAL_HOST or LOCAL_HOST6 can be used in 'host' for loopback address.
 */
extern int sock_udp_open(int af); 
extern int sock_udp_bind4( int port, const IN_ADDR *paddr );	
extern int sock_udp_bind6( int port, const IN6_ADDR *paddr6 );
#define sock_udp_bind(p)	sock_udp_bind4(p,NULL)		// compatible with old version which is a function
extern int sock_udp_bindhost(int port, const char *host);
extern int sock_udp_bind_broadcast( int port );
// 'sock_udp_broadcast - enable/disable a IPv4 UDP broadcasting (remember, there is no such thing 'broadcasting' for IPv6)
extern int sock_udp_broadcast( int fd, int enable );	

// 'sock_udp_send0' - 旧版本，尽量别用，以后版本可能会删除。注意：ip 已经是network byte order.
extern int sock_udp_send0( int fd, unsigned long net_order_ip, int port, const void *msg, int len ); 
/*
 * 'sock_udp_send' - send a udp packet to 'dstaddr' which can be IPv4 or IPv6 numeric host notation
 * it is caller's response that socket 'fd' is compatible with your destination.
 * if fd is a IPv6 socket, and destiion is IPv4. the sendto call shall fail.
 */
extern int sock_udp_send( int fd, const char *dstaddr, int port, const void* msg, int len );
/* 
 *'sock_udp_send4'
 * send to single dstination IPv4 addr. in 'paddr'. if paddr->sin_addr.s_addr equal INADDR_BROADCAST
 * message is broadcasted on IPv4 network on port paddr->sin_port.
 * you can use macro SOCKADDR_IN_BROADCAST to generate a broadcast SOCKADDR_IN value for this purpose.
 * 'sock_udp_send6' send to IPv6 destination
 * 'paddr' cannot be NULL in both functions.
 */
extern int sock_udp_send4( int fd, SOCKADDR_IN *paddr, const void *msg, int len );
extern int sock_udp_send6( int fd, SOCKADDR_IN6 *paddr, const void *msg, int len );
// macro to send to a IPv4 or IPv6 destination. 
// 'paddr' can be points to SOCKADDR_IN, SOCKADDR_IN6 or SOCKADDR_STORAGE
#define SOCK_UDP_SEND(fd,paddr,msg,len) \
	do { \
		if ( is_sockaddr_v4(paddr) ) \
			sock_udp_send4(fd,(SOCKADDR_IN *)paddr,msg,len); \
		else \
			sock_udp_send6(fd,(SOCKADDR_IN6 *)paddr,msg,len); \
	} while(0)

// send to one or multiple destinations, paddr cannot be NULL, and it must be an array of SOCKADDR_IN or SOCKADDR_IN6
// you cannot mix both in an array of SOCKADDR_STORAGE. 'ndst' is number of entry in address array 'paddr'.
extern int sock_udp_sendX( int fd, SOCKADDR *paddr, int ndst, const void *msg, int len );
/*
 * 'sock_udp_recv' - receive a UDP packet from a IPv4 network (old version). if 'net_order_ip' is not null.
 * the sender's IP address is assigned (in network byte order)
 * 'sock_udp_recv2' - new version, can receive from IPv4 or IPv6. If 'sender' not null (point to SOCKADDR_IN,
 * SOCKADDR_IN6 or SOCKADDR_STORAGE), sender's IP address (IPv4 or IPv6). And if 'addrlen'
 * is not null, the size of sender's address is assigned. User can use value of 'addrlen' to identify 
 * whether 'sender' is a IPv4 address of IPv6 address.
 */
extern int sock_udp_recv( int fd, void *buf, int size, unsigned long *net_order_ip );		// This is version 1.0
//extern int sock_udp_recv2( int fd, void *buf, int size, SOCKADDR *sender );			// This is version 1.1
extern int sock_udp_recv2( int fd, void *buf, int size, SOCKADDR *sender, int *addrlen );

	
// tout in msec
//  -1 means wait until data is ready.
//   0 means no wait at all. Just poll the status
extern int sock_dataready( int fd, int tout );
extern int sock_iqueue(int fd);
extern int sock_oqueue(int fd);

//extern unsigned long get_host_ip( const char *hostname, char *buf );  -- obsoleted.
///////////////////////////////////////////////////////////////////////////
// [IPv4 Only]
/* 
 * in following functions
 * if ifa_name is NULL, then return address of first adapter (exclude "lo")
 */
extern const char *get_ifadapter_name();
extern unsigned long get_ifadapter_ip( const char *ifa_name, char *buf, SOCKADDR_IN *netmask, SOCKADDR_IN *ip_bcast );
/*
 * ifconfig: config network interface adapter 
 * ifname: name of adapter, NULL mean first non lo interface (usually eth0)
 * ip, netmask, gwip: ip address, netmask and defaut gateway IP. NULL means do not change
 */
extern int ifconfig(const char *ifname, const char *ipaddr, const char *netmask, const char *gwip);
//extern int set_netmask( const char *ifname, SOCKADDR* netmask );
extern unsigned long get_gatewayaddr( const char *ifname, char *buf );
extern int get_mac_addr( const char *ifname, unsigned char *hwaddr );		// hwaddr: 6 bytes mac addr.
extern int set_mac_addr( const char *ifname, unsigned char *hwaddr );
extern int get_prefix(unsigned long mask);

// helper - same as inet_addr. but caller does not have to include many socket header files.
extern unsigned long INET_ATON( const char *ipstr );
extern const char *INET_NTOA( unsigned long ip );		// 【重要: ip是 network byte order】
extern const char *INET_NTOA2( unsigned long ip, char *numdot );	// 【重要: ip是 host byte order】
extern const char *sockaddr_in_tostring(SOCKADDR_IN *addr, int withport);

#define SOCKADDR_IN_ADDR(saddr)		( saddr.sin_addr.s_addr )
#define SOCKADDR_IN_PORT(saddr)		( saddr.sin_port )
#define SOCKADDR_IN_FAMILY(saddr)	( saddr.sin_family )

// 注意：以下三个宏，ip，port都是host byte order
#define SOCKADDR_IN_BROADCAST(saddr,port) \
	do { \
			saddr.sin_family = AF_INET; \
			saddr.sin_addr.s_addr = INADDR_BROADCAST; \
			saddr.sin_port = htons(port); \
	} while(0)

#define SOCKADDR_IN_SET(saddr,ip,port) \
	do { \
			saddr.sin_family = AF_INET; \
			saddr.sin_addr.s_addr = (ip); \
			saddr.sin_port = (port); \
	} while(0)

#define SOCKADDR_IN_GET(saddr,ip,port) \
	do { \
			ip = ntohl(saddr.sin_addr.s_addr); \
			port = ntohs(saddr.sin_port); \
	} while(0)


///////////////////////////////////////////////////////////////////////////////////
// functions or macro for dual stack 		
// [IPv4 and IPv6]
	
// ss must be an address to sockaddr_storage 		
//int is_sockaddr_v4(void *ss);
//int is_sockaddr_v6(void *ss);
#define is_sockaddr_v4(ss) \
	(((SOCKADDR_IN *)ss)->sin_family==AF_INET)
#define is_sockaddr_v6(ss) \
	(((SOCKADDR_IN6 *)ss)->sin6_family==AF_INET6)
// test if a IPv6 address is an IPv4 mapped
int is_sockaddr_v4mapped(void *ss);
// test if a IPv6 address is an IPv4 compatible one
int is_sockaddr_v4compat(void *ss);
int is_sockaddr_equal(void *sa1, void *sa2);

// 'ss' is pointer to SOCKADDR_xxxx
#define SOCKADDR_LEN(ss) (is_sockaddr_v4(ss) ? sizeof(SOCKADDR_IN) : sizeof(SOCKADDR_IN6))
// 'af' is address family, shall be eith AF_INET or AF_INET6
#define GET_ADDRLEN(af)	 ( (af)==AF_INET ? sizeof(SOCKADDR_IN) : sizeof(SOCKADDR_IN6) )

// sa6 is a pointer to SOCKADDR_STORAGE or SOCKADDR_IN6
// sa4 is a pointer to SOCKADDR_STORAGE or SOCKADDR_IN
// this function is used to reverse the mapped or compatible IPv6 address back to IPv4
void sockaddr_v6to4(void *ss6, void *ss4);

// test caddr is AF_INET or AF_INET6 addrss
extern int get_addrfamily(const char *caddr);		// return value is AF_INET6 or AF_INET 
// set address family member in ss. 
//extern void set_addrfamily(int af, void *ss);
#define set_addrfamily(af,ss) \
	do { \
		SOCKADDR_UNION *su = (SOCKADDR_UNION &)ss; \
		if ( af==AF_INET ) \
			su->saddr_in.sin_family = af; \
		else \
			su->saddr_in6.sin6_family = af; \
	} while (0)
// set address sin_addr or sin6_addr member according to 'af'. 
// 'addr' shall point to IN_ADDR, IN_ADDR6 or IN_ADDR_UNION
// 'port' is port number in host byte order.
// 'ss' shall point to SOCKADDR_xx

void set_addrmembers(int af, IN_ADDR_UNION *inaddr, int port, SOCKADDR_UNION *saddr);
/*  这个宏会造成274 SDK使用的cross-compiler出错停止（preprocessor阶段)
#define set_addrmembers(af, inaddr, port, saddr) \
	do { \
		SOCKADDR_UNION *su = (SOCKADDR_UNION *)(saddr); \
		IN_ADDR_UNION *u_addr = (IN_ADDR_UNION *)(inaddr); \
		if ( af==AF_INET ) { \
			su->saddr_in.sin_family = af; \
			su->saddr_in.sin_port = htons(port); \
			su->saddr_in.sin_addr.s_addr = u_addr->in_addr.s_addr; \
		} else  {\
			su->saddr_in6.sin6_family = af; \
			su->saddr_in6.sin6_port = htons(port); \
			memcpy( &su->saddr_in6.sin6_addr, &u_addr->in6_addr, sizeof(IN6_ADDR)); \
		} \
	} while (0)
*/	
// get_hostaddr: obtain host address
// 'hostname' is host domain name or numeric name. can be null. provided that service is not null.
// 'service' is service name on that host, such as "ftp". can be NULL (provided that hostname is not null)
//           it can be a port number string. example: "http", "80".
// 'inaddr' is obtained sockaddr (either SOCKADDR_IN or SOCKADDR_IN6) caller shall declare a
// SOCKADDR_STORAGE type variable and supply its address to this argument.
// 'straddr' the character array to store the readable string of address.
// 'szaddr' is the number of bytes that 'straddr' point to.
// return value:
//  -1: error
// AF_INET: host address is an IPv4 value
// AF_INET6: host address is an IPv6 value
extern int get_hostaddr( const char *hostname, const char *service, void *inaddr, char *straddr, int szaddr );
/* 
 * 'sock_gethostname' obtain socket's local host name (usually the address)
 * 'sock_getpeername' obtain the peer side host name (usually the address, unless peer host is listed in DNS or 'hosts' file)
 * 'sock_gethostaddr' obtain socket's local host address structure (either SOCKADDR_IN or SOCKADDR_IN6). 
 *                return value is AF_INET for IPv4 peer address or AF_INET6 for IPv6.
 * 'sock_getpeeraddr' obtain socket's peer address structure (either SOCKADDR_IN or SOCKADDR_IN6). 
 *                return value is AF_INET for IPv4 peer address or AF_INET6 for IPv6.
 * 'sock_addrtostring' output the socket address string. 
 */
extern const char* sock_gethostname( int fd, int *port ); 
extern const char* sock_getpeername( int fd, int *port );
extern int sock_gethostaddr(int fd, SOCKADDR *saddr);
extern int sock_getpeeraddr(int fd, SOCKADDR *saddr);
// note - saddr should be point to structure SOCKADDR_STORAGE, SOCKADDR_IN, SOCKADDR_IN6, SOCKADDR_UNION
extern const char* sockaddr_tostring(SOCKADDR *saddr, int withport);
// get all interface adapter (exclude "lo") address information of specified family
// 'af' is address family, either AF_INET or AF_INET6. 0 for both
// 'ifi' is array of IFADDRS_INFO to store all interface adapter information in local machine
// 'size' is number of elements in array ifaddr.
// return:
// 	>=0 number of entry stored in 'ifaddr'.
//  -1: error.
extern int get_ifadapter_all( int af, IFADDRS_INFO *ifi, int size );
// get_ifadapter_addr - get specified interface address of specified family
// if ifname is NULL, first non loopback interface is applied.
// return 0: found
//       -1: error or not found.
extern int get_ifadapter_addr(const char *ifname, int af, SOCKADDR *addr, int *addrlen);
/* 
 * get/set network address, netmask for IPv4 and IPv6. 
 * 'ifname' interface name like "eth0". if NULL is given, process on first non loopback interface
 * 'af' is address family, should be AF_INET or AF_INET6
 * 'netaddr','netmask': network address or mask to process. should be pointer to SOCKADDR_xxxx which
 * 'prefix' argument in 'set_afnetaddrmask' is prefix of IPv6 address (# of '1' in netmask addr)
 * 			for IPv4, this is netmask.
 * for IPv4, 'prefix' is netmask.
 * consistance with specified 'af', Can be SOCKADDR_STORAGE, SOCKADDR_UNION for both 'af'
 * return value:
 * 0 success (except get_afnetaddrmask for 'af'==AF_INET6, return value is prefix length)
 * -1: failed.
 */
extern int get_afnetaddrmask( const char *ifname, int af, SOCKADDR* netaddr, SOCKADDR* netmask );
extern int set_afnetaddrmask( const char *ifname, int af, SOCKADDR* netaddr, u_long prefix );
extern int add_afgateway( const char *ifname, int af, SOCKADDR* gw, SOCKADDR* dstnet, int prefix);
extern int del_afgateway( const char *ifname, int af, SOCKADDR* gw, SOCKADDR* dstnet, int prefix);

// in following macros, saddr, mask and gw should be pointer to SOCKADDR_IN 
#define get_netaddr(ifn,saddr)	get_afnetaddrmask(ifn,AF_INET, (SOCKADDR *)saddr, NULL)
#define get_netmask(ifn,mask)		get_afnetaddrmask(ifn,AF_INET, NULL, (SOCKADDR *)mask)
#define set_netaddr(ifn,saddr)	set_afnetaddrmask(ifn,AF_INET, (SOCKADDR *)saddr, 0)	
#define set_netmask(ifn,mask)		set_afnetaddrmask(ifn,AF_INET, NULL, (mask)->sin_addr.s_addr)	
#define add_gateway(ifn,gw)			add_afgateway(ifn,AF_INET, (SOCKADDR *)gw, NULL, 0)
#define del_gateway(ifn,gw)			del_afgateway(ifn,AF_INET, (SOCKADDR *)gw, NULL, 0)
#define del_defgateway(af)			del_afgateway(NULL,af,NULL,NULL,0)
//////////////////////////////////////////////////////////////////////////		
// [IPv6 Only]		
//
// NOTE - following functions use system invoke 'ip' command. 
// 'add_ipv6addr' and 'add_ipv6gw' are command versions to do what set_afnetaddrmask and add_afgateway do.
// 'del_ipv6gw' is ip command version versa ioctl version of del_afgateway
//
// get a IPv6 address of specified 'ifname' such as "eth0". if ifname is NULL, first non-loopback interface is applied
// return value >=0 is prefix length, -1 is error.
extern int get_ipv6ifaddr( const char *ifname, IN6_ADDR *netaddr );
// add/delete a IPv6 address
extern int add_ipv6addr( const char *ifname, IN6_ADDR *netaddr, int prefix);
extern int del_ipv6addr( const char *ifname, IN6_ADDR *netaddr, int prefix);
// obtain IPv6 gateway on specified interface 'ifname'. If 'ifname' is NULL, first non loopback interface
// is applied.
// return value >=0 is prefix length of gateway, -1 is error.
extern int get_ipv6ifgw( const char *ifname, IN6_ADDR* gw );
// find a IPv6 gateway (exist or not). return 1 exist, 0 not found.
extern int find_ipv6ifgw(const char *ifname, IN6_ADDR *netaddr, int prefix);
// add/delete a IPv6 default gateway
extern int add_ipv6gw( const char *ifname, IN6_ADDR *netaddr, int prefix);
extern int del_ipv6gw( const char *ifname, IN6_ADDR *netaddr, int prefix);

// IPv6 address and prefix notation like 2001:db8:1f70::999:de8:7648:6e8/64
extern const char *in6addr_tostring( IN6_ADDR *in6addr, int prefix);
// IPv6 notation would be something like: [2001:db8:1f70::999:de8:7648:6e8]:100 (RFC 3986, section 3.2.2)	
extern const char *sockaddr_in6_tostring(SOCKADDR_IN6 *in6addr, int withport);

#define SOCKADDR_IN6_ADDR(sa6)		( (sa6).sin6_addr.s6_addr )		// note: s6_addr is defined as in6_u.u6_addr8 which is 16 bytes array
#define SOCKADDR_IN6_PORT(sa6)		( (sa6).sin6_port )
#define SOCKADDR_IN6_FAMILY(sa6)	( (sa6).sin6_family )
#define SOCKADDR_IN6_SCOPE(sa6)		( (sa6).sin6_scope_id )
#define SOCKADDR_IN6_FLOW(sa6)		( (sa6).sin6_flowinfo )
// set sockaddr_in6 member according to IPv6 address in6_addr and port (int host byte order)
#define SOCKADDR_IN6_SET(saddr6, inaddr6, port) \
do { \
  saddr6.sin6_family = AF_INET6; \
  saddr6.sin6_len = sizeof(struct sockaddr_in6); \
  memcpy(&saddr6.sin6_addr, &inaddr6, sizeof(IN6_ADDR)); \
  saddr6.sin6_port = htons(port); \
} while(0)

#ifdef __cplusplus
};
#endif

#endif /* _UTILS_NET_H_ */
