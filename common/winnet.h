/*
 *	winnet.h
 *
 *  Socket i/o and control functions
 */
#ifndef _WINNET_H_
#define _WINNET_H_

#if !(defined(_WINSOCKAPI_) || defined(_WINSOCKAPI2_))
#include <winsock2.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define IFNAME_LEN	64
#define INET_ADDRSTRLEN	22

typedef struct {
	char ifa_name[IFNAME_LEN];
	IN_ADDR	addr;
	IN_ADDR mask;
	IN_ADDR broadcast;
} IF_ATTR4;	

// winsock startup and clean
extern int winsock_startup( void );
extern int winsock_cleanup( void );
extern const char *sock_ntoa( DWORD dwIP );		// 【重要: dwIP是 host byte order】
extern unsigned long sock_aton( const char *str );	// 【重要】这个函数返回的是host byte order
extern unsigned long INET_ATON( const char *ipstr );	//【重要】返回的是network byte order
extern const char *INET_NTOA( unsigned long ip );		// 【重要: ip是 network byte order】
extern const char *INET_NTOA2(unsigned long ip, char *str);

// create a TCP socket with server and return socket
/*
 * 'sock_set_timeout' - set socket send/receive time-out value (in msec)
 * 'which' is combination of _SO_SEND or _SO_RECV
 */
#define _SO_SEND	0x01
#define _SO_RECV	0x02
extern int sock_set_timeout(SOCKET fd, int which, int tout);
/*
 * 'sock_wait_for_io_or_timeout' - wait until timed-out or socket is avaible for 
 *  reading (when 'for_read' is not 0), or for writing (when 'for_read' is 0)
 *  msec > 0 will block 'msec', msec==0 non-block, msec==-1 block forever until data available
 * return 
 *  0 for specified R/W operation available, -1 not available
 */
extern int sock_wait_for_io_or_timeout(SOCKET fd, int for_read, long msec);
#define sock_dataready(fd,tout)	(sock_wait_for_io_or_timeout(fd,1,tout)==0)

extern SOCKET sock_connect_timeout( const char *host, int port, int tout );
#define sock_connect(host, port )		sock_connect_timeout(host,port,0)
extern SOCKET sock_connect0( DWORD dwIP, int port );

// set minimum number of bytes to recv before wakeup (hance, select/poll will response readable)
// only for TCP
extern int sock_set_recvlowat(SOCKET fd, int bytes);

// server side listen to a port on given IP (ipbind). If ipbind == NULL, listen to all IP in local machine.
extern SOCKET sock_listen( int port, const char *ipbind, int backlog );
extern SOCKET sock_accept2(SOCKET listen_sock, SOCKADDR *caddr);
#define sock_accept( s )	sock_accept2(s,NULL)

extern int sock_read(SOCKET s, void *buf, int size);
extern int sock_write(SOCKET s, const void *buf, int size);
int sock_read_nowait( SOCKET fd, void* buffer, int buf_size );
int sock_write_nowait( int fd, const void* buffer, int buf_size );
#define sock_close( s )				closesocket( s )

int sock_read_until_tout(SOCKET fd, char* buffer, int buf_size, int eol, int tout);
int sock_peek_until_tout(SOCKET fd, char* buffer, int buf_size, int eol, int tout);
#define sock_peek_until(fd,buffer,buf_size,eol)	sock_peek_until_tout(fd,buffer,buf_size,eol,-1)
#define sock_read_until(fd,buffer,buf_size,eol)		sock_read_until_tout(fd,buffer,buf_size,eol,-1)
#define sock_read_line(fd,buf,sz)	sock_read_until(fd,buf,sz,(int)'\n')
#define sock_peek_line(fd,buf,sz)	sock_read_until(fd,buf,sz,(int)'\n')

/* read one character from socket stream with tout option.
 * tout: 0 no wait
 *       -1 wait indefinitely
 *       >0 milli-seconds, either timeout or 
 */
extern int sock_getc( SOCKET sock, long tout );
extern int sock_puts( SOCKET sock, const char *line);

/* read EXACT n bytes from socket,
 * return: bytes readed, or -1 when failed */
extern int sock_read_n_bytes_tout(SOCKET fd, void* buffer, int n, long tout);
#define sock_read_n_bytes(s,b,n)	sock_read_n_bytes_tout(s,b,n,-1)

extern int sock_skip_n_bytes_tout(SOCKET fd, int n, long tout);
#define sock_skip_n_bytes(fd,n)	sock_skip_n_bytes_tout(fd,n,-1)

/* write exact n bytes. return number of bytes written */
extern int sock_write_n_bytes(SOCKET fd, void *buffer, int n);
extern int sock_peek_n_bytes(SOCKET fd, void* buffer, int n);

/* 
 * 'sock_drain' clean all buffered input data in tcp/ip stack. available for both TCP and UDP
 * 'sock_drain_until' drop tcp stream input queue until byte sequence 'soh' which has 'ns' bytes 
 * are encounted. soh in input stream will not be dropped. Only all data before 'soh'. 
 * Useful to sync. to next packet (application protocol packet) started and packet has a constant 
 * SOH byte sequence.
 */
extern int sock_drain( SOCKET fd );
extern int sock_drain_until( SOCKET fd, void *soh, int ns );

/* check if socket is still connected (return 1 yes, 0 no) */
extern int sock_is_connected( SOCKET fd );

/* obtain and bind a socket to local address - if bind failed, socket created is not closed */
extern SOCKET sock_udp_open();
extern SOCKET sock_udp_bindLocalIP( unsigned long ulIP, int port );
#define sock_udp_bind( port )	sock_udp_bindLocalIP( INADDR_ANY, port )

/* set UDP socket options. return value 0: OK, 0-1 Failed. */
extern int sock_udp_broadcast( SOCKET sock, BOOL bEnable );
extern int sock_udp_timeout( SOCKET sock, int nTimeOut );

/* obtain and bind a socket for UDP broadcasting */
extern SOCKET sock_udp_bind_broadcast( int port, long nTimeOut );

/* send a UDP packet
 * sock_udp_send to specified destination
 * 'sock' == INVALID_SOCKET sock_udp_send will open a udp to send then close it.
 * ip==NULL means broadcasting
 * return value:
 *  >0:number of bytes sent:
 *  -1: Error
 */
extern int sock_udp_send( SOCKET sock, const char *ip, int port, const void* msg, int len );
extern int sock_udp_send0( SOCKET udp_fd, unsigned long ip, int port, const void * msg, int len );
extern int sock_udp_sendX( SOCKET fd, SOCKADDR_IN *paddr, int ndst, const void* msg, int len );
/* receive a UDP datagram 
 *   'sock' shall be a socket bind to UDP port
 *   'buf' is the caller's storage for data received. 
 *    'size' is size of buf in byte. If size less then UDP packet data, extra data will be lost
 *	         maximum UDP data size is 1024 bytes.
 *    'IPSender' is the address to a DWORD to hold the UDP sender IP. Can be NULL if caller does
 *    not care the sender IP.
 *  return value:
 *    >0:  Number of bytes received.
 *     0:  socket 'sock' is closed.
 *    -1:  Error. To obtain detail error code, invoke WSAGetLastError()
 */
extern int sock_udp_recv( SOCKET sock, void *buf, int size, DWORD *IPSender );
extern int sock_udp_recv0( SOCKET sock, void *buf, int size, SOCKADDR_IN *saIn );
#define sock_udp_recv2		sock_udp_recv0
/* 
 * obtain number of bytes available in input buffer - workable for both TCP and UDP.
 * for TCP, return value is number of bytes can be read by a single 'recv' call.
 * for UDP, return value is size of first data gram packet in buffer.
 */
extern unsigned long sock_iqueue( SOCKET sock );
/*
 * set in-buffer size of a socket (useful for TCP when AP data size is much longer than default). For example, if AP wants to receive live video stream
 * and wants to implements simple drop frame mechanism when incoming data is faster than AP can decode and render. AP needs larger buffer to
 * identify if input buffer is full (small buffer always full in this application)
 */
extern int sock_set_iqueue( SOCKET sock, int size );
extern int sock_set_oqueue( SOCKET sock, int size );

// get host IP address by domain name. dotnum IP address string is also copy to buf.
//extern unsigned long get_host_ip( const char *hostname, char *buf );	// obosoleeted, don't use
//extern int sock_getlocalname( SOCKET sock, DWORD *myIP, WORD *myPort );	// obosoleeted, don't use
extern const char* sock_gethostname( SOCKET fd, int *port);
extern const char* sock_getpeername( SOCKET fd, int *port );
extern int sock_getpeeraddr(SOCKET fd, SOCKADDR_IN *saddr);
extern int sock_gethostaddr(SOCKET fd, SOCKADDR_IN *saddr);
extern const char *sockaddr_in_tostring(SOCKADDR_IN *addr, int withport);
#define sockaddr_tostring(addr,withport)	sockaddr_in_tostring((SOCKADDR_IN*)addr,withport)
#define SOCKADDR_TOSTRING(saddr,p)	sockaddr_tostring((SOCKADDR *)saddr,p)


extern int get_all_interface(IF_ATTR4 ifattr[], int size);

/*
 * ping 某个IP(IPv4 addr)，使用ICMP包发送。 'ping_times'是测试次数，
 * 'tout_count' 用来保存超时没有返回的次数。可以给NULL。
 * 函数返回值是平均返回时间(ms)，只计入有返回的包时间。 如果所有ping包都超时没有收到，函数返回-1.
 * 如果输入的IP非法或是domain name无法解析，也是返回-1
 */
extern int ping(const char *IP, int ping_times, int *tout_count);

#define SOCKADDR_IP( sa )	( (sa).sin_addr.s_addr )		// sa must be SOCKADDR_IN
#define SOCKADDR_PORT( sa )	( ntohs((sa).sin_port) )
#define SOCKADDR_FAMILY( sa ) ( (sa).sin_family )
#define SOCKADDR_IN_FILL(sa, ip, port) \
	do { \
	  (sa).sin_addr.s_addr = ip; \
	  (sa).sin_port = htons(port); \
	  (sa).sin_family = AF_INET; \
	} while (0)

#if _WIN32_WINNT < 0x0600	
#define INET6_ADDRSTRLEN 65
#define AF_INET6        23              // Internetwork Version 6
typedef struct in6_addr {
    union {
        UCHAR       Byte[16];
        USHORT      Word[8];
    } u;
} IN6_ADDR;
#define s6_addr			u.Byte
#define s6_addr16		u.Word

extern CONST IN6_ADDR in6addr_any;

const char *inet_ntop( int family, void *saddr, char *buf, int size);
int inet_pton( int family,  const char *addrstr, void *saddr);
const char *in6addr_tostring( IN6_ADDR *in6addr, int prefix);

#endif

#ifdef __cplusplus
}
#endif

#endif

