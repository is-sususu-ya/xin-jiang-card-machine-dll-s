/***************************************************************************
						  utils_net.c  -  description
							 -------------------
	begin                : Thu Dec 20 2001
	copyright            : (C) 2001 by Liming Xie
	email                : liming_xie@yahoo.com

	Version: 	1.0.1
	Date:	2005-03-06
	Author:	Thomas Chang

	1. Add the "get_host_ip" and "get_ifadapter_ip" functions

		Version: 1.1.1
		Date 2019-01-06
		Author: Thomas Chang
		Description:
		   1) Add too many new functions since 1.0.1
		   2) replace obsoleted function gethostbyname
			 3) sock_connect return -1 on error, instead of negative error code
			 4) add sock_connect_timeout function prevent connection operation on
				an unready destination hang for long time.
 ***************************************************************************/
#ifndef ENABLE_IPV6
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <linux/sockios.h> // for sock_oqueue SIOCOUTQ
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <ifaddrs.h>
// #include <net/if.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <linux/if_arp.h>
#include <linux/route.h>
#ifndef min
#define min(x, y) ((x) > (y) ? (y) : (x))
#endif
#ifndef max
#define max(x, y) ((x) < (y) ? (y) : (x))
#endif
#include "utils_net.h"
// #include "longtime.h"

// #define NET_TIMEDOUT_ENABLE

#define LOCAL_HOST 0x0100007fL // this is 127.0.0.1

// static	long	NET_TIMEDOUT_MSEC = 60000;		/* 60 sec == 1 min */
int sock_set_recvlowat(int fd, int bytes)
{
	return setsockopt(fd, SOL_SOCKET, SO_RCVLOWAT, &bytes, sizeof(int));
}

int sock_set_nonblock(int fd, int non_block)
{
	int flags = fcntl(fd, F_GETFL, 0);
	int rc;
	if (non_block)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;
	rc = fcntl(fd, F_SETFL, flags);
	return rc;
}

int sock_set_timeout(int fd, int which, int msec)
{
	int opt = 0;
	int ret;
#ifdef linux
	struct timeval tv;

	tv.tv_sec = msec / 1000;
	tv.tv_usec = (msec % 1000) * 1000;
#else // winsock
	int tv = msec;
#endif
	if (which & _SO_SEND)
		opt |= SO_SNDTIMEO;
	if (which & _SO_RECV)
		opt |= SO_RCVTIMEO;
	ret = setsockopt(fd, SOL_SOCKET, opt, (const char *)&tv, sizeof(tv));
	return ret;
}

int sock_get_timeout(SOCKET fd, int which)
{
	int opt = 0;
	int val = 0;
	int val_len = sizeof(val);
	int ret;

	if (which == _SO_SEND)
		opt = SO_SNDTIMEO;
	else if (which == _SO_RECV)
		opt = SO_RCVTIMEO;
	ret = getsockopt(fd, SOL_SOCKET, opt, (char *)&val, &val_len);
	return val;
}

int sock_connect_timeout(const char *host, int port, int tout)
{
	struct sockaddr_in destaddr;
	struct hostent *hp;
	int fd = 0;
	int tout_val;

	memset(&destaddr, 0, sizeof(destaddr));
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons((short)port);
	if ((inet_aton(host, &destaddr.sin_addr)) == 0)
	{
		hp = gethostbyname(host);
		if (!hp)
			return -1;
		memcpy(&destaddr.sin_addr, hp->h_addr, sizeof(destaddr.sin_addr));
	}

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;

	if (tout > 0)
	{
		tout_val = sock_get_timeout(fd, _SO_SEND);
		sock_set_timeout(fd, _SO_SEND, tout);
	}

	if (connect(fd, (struct sockaddr *)&destaddr, sizeof(destaddr)) < 0)
		return -1;

	if (tout > 0)
		sock_set_timeout(fd, _SO_SEND, tout_val);

	return fd;
}

int sock_listen(int port, const char *ipbind, int backlog)
{
	struct sockaddr_in my_addr;
	int fd, tmp = 1;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));

	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons((short)port);
	if (ipbind != NULL)
	{
		inet_aton(ipbind, &my_addr.sin_addr);
	}
	else
	{
		my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	if (0 == bind(fd, (struct sockaddr *)&my_addr, sizeof(my_addr)))
	{
		if (0 == listen(fd, backlog))
		{
			return fd;
		}
	}

	close(fd);
	return -1;
}

int sock_accept(int fd_listen)
{
	struct sockaddr_in fromaddr;
	socklen_t addrlen = sizeof(fromaddr);
	int fd;
	int tmp = 1;

	if ((fd = accept(fd_listen, (struct sockaddr *)&fromaddr, &addrlen)) != -1)
		setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&tmp, sizeof(tmp));
	return fd;
}

const char *sock_getpeername(int fd, int *port)
{
	struct sockaddr_in peer_addr;
	socklen_t addrlen = sizeof(peer_addr);

	if (getpeername(fd, (struct sockaddr *)&peer_addr, &addrlen) == 0)
	{
		if (port)
			*port = ntohs(peer_addr.sin_port);
		return inet_ntoa(peer_addr.sin_addr);
	}
	else
		return ""; // NOTE - donot return NULL, we may use this function return value in printf argument for text message output
}

int sock_getpeeraddr(int fd, struct sockaddr *saddr)
{
	SOCKADDR_STORAGE ss;
	socklen_t addrlen = sizeof(ss);
	int rc = -1;

	if (getpeername(fd, (struct sockaddr *)&ss, &addrlen) == 0)
	{
		memcpy(saddr, &ss, addrlen);
		rc = AF_INET;
	}
	return rc;
}

const char *sockaddr_in_tostring(SOCKADDR_IN *addr, int withport)
{
	static char straddr[INET_ADDRSTRLEN + 6];
	short port;

	struct in_addr inaddr;
	inaddr.s_addr = addr->sin_addr.s_addr;
	port = addr->sin_port;
	port = ntohs(port);
	strcpy(straddr, inet_ntoa(inaddr));
	if (withport)
		sprintf(straddr + strlen(straddr), ":%u", (unsigned short)port);
	return straddr;
}

int is_sockaddr_equal(void *ss1, void *ss2)
{
	SOCKADDR_IN *sa1 = (SOCKADDR_IN *)ss1;
	SOCKADDR_IN *sa2 = (SOCKADDR_IN *)ss2;
	return sa1->sin_addr.s_addr == sa2->sin_addr.s_addr;
}

#ifdef linux
int sock_wait_for_io_or_timeout(int fd, int for_read, long timeout)
{
	struct pollfd poll_table[1], *poll_entry = &poll_table[0];
	int ret;

	poll_entry->fd = fd;
	poll_entry->events = for_read ? POLLIN : POLLOUT;
	do
	{
		ret = poll(poll_table, 1, timeout);
	} while (ret == -1);

	return (poll_entry->revents > 0) ? 0 : -1;
}
#else
int sock_wait_for_io_or_timeout(int fd, int for_read, long timeout)
{
	fd_set fs;
	struct timeval tv;
	int ret;
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = timeout % 1000 * 1000;
	FD_ZERO(&fs);
	FD_SET(fd, &fs);

	do
	{
		if (for_read)
			ret = select(fd + 1, &fs, NULL, NULL, &tv);
		else
			ret = select(fd + 1, NULL, &fs, NULL, &tv);
	} while (ret == -1);

	return (ret) ? 0 : -1;
}
#endif

int sock_read(int fd, void *buffer, int buf_size)
{
	return recv(fd, buffer, buf_size, MSG_NOSIGNAL);
}

int sock_write(int fd, const void *buffer, int buf_size)
{
	return send(fd, buffer, buf_size, MSG_NOSIGNAL);
}

int sock_read_nowait(int fd, void *buffer, int buf_size)
{
	return recv(fd, buffer, buf_size, MSG_DONTWAIT | MSG_NOSIGNAL);
}

int sock_write_nowait(int fd, const void *buffer, int buf_size)
{
	return send(fd, buffer, buf_size, MSG_DONTWAIT);
}

int sock_read_until_tout(int fd, char *buffer, int buf_size, int eol, int tout)
{
	int len, size, buf_left, i, found = 0;
	char *ptr, *cmpptr;

	for (ptr = buffer, size = 0, buf_left = buf_size - 1; buf_left > 0; /**/)
	{
		/* peek the data, but not remove the data from the queue */
		// NOTE - 为了避免发送端发送错误的数据，或是部分数据，我们需要检查是否有数据才去读
		//       否则会堵塞住。设置recv超时没有用，sock_set_timeout里对TCP做
		// 	     setsockopt(fd,SOL_SOCKET,opt...
		// 	 返回errno=92 (Protocol not available). 第一次读肯定是会有东西(先select才读的)
		//   但是没有读到指定的结尾，我们继续recv时，如果没有东西，会卡死在里面。
		//   因此每次recv前需要检查是否有数据，给一个固定的超时时间 100 msec。如果没有东西，返回
		//	 已经读取到的字节数。
		if (0 != sock_wait_for_io_or_timeout(fd, 1, tout))
		{
			//   我们不返回0，因为程序会以为TCP断啦 (select有，读回是0字节)
			buffer[size] = 0;
			return size;
		}
		len = recv(fd, ptr, buf_left, MSG_PEEK);
		if ((len == 0) || (len < 0 && (errno != EAGAIN || errno != EINTR)))
			return -1;
		else if (len < 0)
			continue;

		/* try finding 'eol' in the data */
		for (i = 0, cmpptr = ptr; i < len; i++, cmpptr++)
		{
			if (*cmpptr == eol)
			{
				len = i + 1;
				found = 1;
				break;
			}
		}

		/* get the data we need, and remove from the queue */
		len = recv(fd, ptr, len, 0);
		if ((len == 0) || (len < 0 && (errno != EAGAIN || errno != EINTR)))
			return -1;
		else if (len < 0)
			continue;

		ptr += len;
		size += len;
		buf_left -= len;

		if (found)
		{
			buffer[size] = '\0';
			return size;
		}
	}
	// 到此是buffer已经填满但是还没遇见'eol'字节，返回所有peek到的内容
	return size;
	// return 0;		// 这里不能返回0，上层会以为socket断掉
}

// copy w/o remove data from socket buffer
int sock_peek_until_tout(int fd, char *buffer, int buf_size, int eol, int tout)
{
	int len, size, buf_left, i, found = 0;
	char *ptr, *cmpptr;

	for (ptr = buffer, size = 0, buf_left = buf_size - 1; buf_left > 0; /**/)
	{
		/* peek the data, but not remove the data from the queue */
		if (0 != sock_wait_for_io_or_timeout(fd, 1, tout))
		{
			//   我们不返回0，因为程序会以为TCP断啦 (select有，读回是0字节)
			// buffer[size] = 0;
			return size;
		}
		len = recv(fd, ptr, buf_left, MSG_PEEK);
		if ((len == 0) || (len < 0 && (errno != EAGAIN || errno != EINTR)))
			return -1;	  // len==0 是socket断了，返回-1。len=-1是有IO error，如果错误不是EAGAIN或是EINTR也返回-1
		else if (len < 0) // EAGAIN, EINTR
			continue;

		/* try finding 'eol' in the data */
		for (i = 0, cmpptr = ptr; i < len; i++, cmpptr++)
		{
			if (*cmpptr == eol)
			{
				len = i + 1;
				found = 1;
				break;
			}
		}
		ptr += len;
		size += len;
		buf_left -= len;

		if (found)
		{
			buffer[size] = '\0';
			return size;
		}
	}
	return size;
}

int sock_read_n_bytes_tout(int fd, void *buffer, int n, int tout)
{
	char *ptr = buffer;
	int len;
	while (n > 0)
	{
		if (0 != sock_wait_for_io_or_timeout(fd, 1, tout))
			return (ptr - (char *)buffer);

		len = recv(fd, ptr, n, MSG_NOSIGNAL);
		if (len == 0)
			break;
		if (len < 0)
		{
			if (errno == EAGAIN || errno == EINTR)
				continue;
			else // 其他错误，一般是EPIPE (socket关闭)
				return -1;
		}
		ptr += len;
		n -= len;
	}
	return (ptr - (char *)buffer);
}

int sock_write_n_bytes(int fd, const void *buffer, int size)
{
	int len;
	const char *ptr = (const char *)buffer;

	while (size > 0)
	{
		// MSG_NOSIGNAL 当socket被断开时别发送SIGPIPE,程序如果没有处理好这个消息(SIG_IGNORE)会死掉
		len = send(fd, ptr, size, MSG_NOSIGNAL);
		if ((len == 0) || (len < 0 && (errno != EAGAIN || errno != EINTR)))
			return -1;
		else if (len < 0) // this muust be due to EAGAIN or EINTR (send is interrupted)
			continue;

		ptr += len;
		size -= len;
	}
	return ptr - (char *)buffer;
}

int sock_skip_n_bytes_tout(int fd, int n, int tout)
{
	char buffer[1024];
	int len;
	int left = n;
	while (left > 0)
	{
		if (0 != sock_wait_for_io_or_timeout(fd, 1, tout))
			break;
		// len = recv(fd, ptr, n, MSG_WAITALL);
		len = recv(fd, buffer, min(n, sizeof(buffer)), MSG_NOSIGNAL);
		if (len == 0)
			break;
		if (len < 0)
		{
			if (errno == EAGAIN || errno == EINTR)
				continue;
			else
				return -1;
		}
		left -= len;
	}
	return (n - left);
}

// drain all data currently in socket input buffer
int sock_drain(int fd)
{
	int rlen, n = 0;
	char buf[1024];

	while (sock_wait_for_io_or_timeout(fd, 1, 0) == 0)
	{
		rlen = recv(fd, buf, sizeof(buf), MSG_NOSIGNAL);
		if (rlen > 0)
			n += rlen;
		else
			return -1;
	}
	return n;
}

int sock_drain_until(SOCKET fd, void *soh, int ns)
{
	char buffer[1024];
	char *ptr;
	int i, len, nskip = 0;
	while (sock_wait_for_io_or_timeout(fd, 1, 100) == 0)
	{
		/* peek the data, but not remove the data from the queue */
		if ((len = recv(fd, buffer, sizeof(buffer), MSG_PEEK)) == SOCKET_ERROR || len <= 0)
			return -1;

		/* try to locate soh sequence in buffer */
		for (i = 0, ptr = buffer; i <= len - ns; i++, ptr++)
			if (0 == memcmp(ptr, soh, ns))
				break;
		nskip += (int)(ptr - buffer);
		if (i > len - ns)
			recv(fd, buffer, len, 0);
		else
			recv(fd, buffer, (int)(ptr - buffer), MSG_NOSIGNAL);
		if (i <= len - ns)
			break;
	}
	return nskip;
}

int sock_is_connected(int fd)
{
	fd_set set;
	struct timeval tv;
	int ret;

	FD_ZERO(&set);
	FD_SET(fd, &set);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	ret = select(fd + 1, &set, NULL, NULL, &tv);
	if (ret > 0)
	{
		// try to peek data
		char buf[8];
		ret = recv(fd, buf, 8, MSG_NOSIGNAL | MSG_PEEK);
		if (ret <= 0)
			return 0;
	}
	else if (ret < 0)
	{
		// select error
		return 0;
	}
	else
	{
		// select timeout
	}

	return 1;
}

//================================ U D P ===========================
int sock_udp_open()
{
	return socket(AF_INET, SOCK_DGRAM, 0);
}

// Set UDP receiving time out. nTimeOut in msec.
int sock_udp_timeout(int fd, int nTimeOut)
{
	return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&nTimeOut, sizeof(nTimeOut));
}

int sock_udp_broadcast(int fd, int enable)
{
	return setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char *)&enable, sizeof(enable));
}

SOCKET sock_udp_bindLocalIP(unsigned long ulIP, int port)
{
	struct sockaddr_in my_addr;
	SOCKET udp_fd;

	if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) != INVALID_SOCKET)
	{
		// int bReuseAddr = 1;
		// setsockopt( udp_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseAddr, sizeof(bReuseAddr));

		memset(&my_addr, 0, sizeof(my_addr));
		my_addr.sin_family = AF_INET;
		my_addr.sin_port = htons((short)port);
		//		my_addr.sin_addr.s_addr = htonl (ulIP);
		my_addr.sin_addr.s_addr = ulIP;

		if (bind(udp_fd, (struct sockaddr *)&my_addr, sizeof(my_addr)) != 0)
		{
			sock_close(udp_fd);
			udp_fd = INVALID_SOCKET;
		}
	}
	return udp_fd;
}

int sock_udp_bindhost(int port, const char *host)
{
	return sock_udp_bindLocalIP(host ? INET_ATON(host) : INADDR_ANY, port);
}

int sock_udp_send(const char *ip, int port, const void *msg, int len)
{
	SOCKADDR_IN udp_addr;
	int sockfd, ret;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd <= 0)
		return -1;

	// setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));

	memset(&udp_addr, 0, sizeof(udp_addr));
	udp_addr.sin_family = AF_INET;
	if (ip == NULL)
	{
		udp_addr.sin_addr.s_addr = INADDR_BROADCAST;
		sock_udp_broadcast(sockfd, 1);
	}
	else
		udp_addr.sin_addr.s_addr = INET_ATON(ip);
	udp_addr.sin_port = htons((short)port);

	ret = sendto(sockfd, msg, len, 0, (const struct sockaddr *)&udp_addr, sizeof(udp_addr));
	close(sockfd);
	return ret;
}

int sock_udp_send0(int udp_fd, unsigned long ip, int port, const void *msg, int len)
{
	SOCKADDR_IN udp_addr;
	int ret;

	memset(&udp_addr, 0, sizeof(udp_addr));
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_addr.s_addr = ip; // NOTE - dwIP is already network format. DON'T USE htonl(dwIP)
	udp_addr.sin_port = htons((short)port);
	if (len == 0)
		len = strlen(msg);

	ret = sendto(udp_fd, msg, len, MSG_DONTWAIT, (const struct sockaddr *)&udp_addr, sizeof(udp_addr));

	return ret;
}

int sock_udp_sendX(int fd, SOCKADDR_IN *paddr, int ndst, const void *msg, int len)
{
	int i;
	int send_len = 0;
	SOCKADDR_IN baddr;

	if (len == 0)
		len = strlen((char *)msg);
	if (ndst == 0 || paddr == NULL)
	{
		paddr = &baddr;
		paddr->sin_addr.s_addr = htonl(INADDR_BROADCAST);
		paddr->sin_family = AF_INET;
		ndst = 1;
	}
	for (i = 0; i < ndst; i++)
		send_len += sendto(fd, msg, len, 0, (const struct sockaddr *)(paddr + i), sizeof(SOCKADDR_IN));
	return send_len;
}

int sock_udp_recv2(int fd, void *buf, int size, SOCKADDR *src, int *paddrlen)
{
	SOCKADDR_IN sender;
	socklen_t addrlen = sizeof(sender);
	int ret;

	if ((ret = recvfrom(fd, buf, size, 0, (struct sockaddr *)&sender, &addrlen)) > 0)
	{
		if (src != NULL)
			memcpy((void *)src, (void *)&sender, sizeof(SOCKADDR_IN));
		if (paddrlen != NULL)
			*paddrlen = addrlen;
	}
	return ret;
}

//========================== c o m m o n ==============================
int sock_iqueue(int fd)
{
	unsigned long ul;

	if (ioctl(fd, FIONREAD, &ul) == 0)
		return (int)ul;
	return 0;
}

int sock_oqueue(int fd)
{
	size_t left;
	if (ioctl(fd, SIOCOUTQ, &left) == 0)
		return (int)left;
	return 0;
}

//========================== o t h e r =================================

unsigned long get_host_ip(const char *hostname, char *buf)
{
	char name[256];
	struct hostent *host;
	unsigned long ipaddr = 0UL;
	int i;

	if (hostname == NULL)
	{
		gethostname(name, sizeof(name));
		hostname = name;
	}

	if ((host = gethostbyname(hostname)) != NULL)
	{
		for (i = 0; host->h_addr_list[i] != NULL; i++)
		{
			memcpy(&ipaddr, host->h_addr_list[0], sizeof(unsigned long));
			if (ipaddr != LOCAL_HOST)
				break;
		}
		if (buf)
			INET_NTOA2(ipaddr, buf);
	}
	return ipaddr;
}

unsigned long get_ifadapter_ip(const char *ifa_name, char *buf, SOCKADDR_IN *netmask, SOCKADDR_IN *ip_bcast)
{
	struct ifaddrs *ifaddr, *ifaddr_head;
	unsigned long ipaddr = 0UL;

	if (getifaddrs(&ifaddr_head) == 0)
	{
		if (ifa_name == NULL)
		{
			ifaddr = ifaddr_head;
			while (ifaddr)
			{
				if (ifaddr->ifa_addr && ifaddr->ifa_addr->sa_family == AF_INET &&
					strcmp(ifaddr->ifa_name, "lo"))
				{
					ipaddr = ((struct sockaddr_in *)(ifaddr->ifa_addr))->sin_addr.s_addr;
					if (netmask)
						memcpy(netmask, ifaddr->ifa_netmask, sizeof(SOCKADDR_IN));
					if (ip_bcast)
						memcpy(ip_bcast, ifaddr->ifa_ifu.ifu_broadaddr, sizeof(SOCKADDR_IN));
					break;
				}
				ifaddr = ifaddr->ifa_next;
			}
		}
		else
		{
			ifaddr = ifaddr_head;
			while (ifaddr)
			{
				if (ifaddr->ifa_addr && ifaddr->ifa_addr->sa_family == AF_INET &&
					!strcmp(ifaddr->ifa_name, ifa_name))
				{
					ipaddr = ((struct sockaddr_in *)(ifaddr->ifa_addr))->sin_addr.s_addr;
					if (netmask)
						memcpy(netmask, ifaddr->ifa_netmask, sizeof(SOCKADDR_IN));
					if (ip_bcast)
						memcpy(ip_bcast, ifaddr->ifa_ifu.ifu_broadaddr, sizeof(SOCKADDR_IN));
					break;
				}
				ifaddr = ifaddr->ifa_next;
			}
		}
		freeifaddrs(ifaddr_head);
	}
	if (buf && ipaddr)
		INET_NTOA2(ipaddr, buf);
	return ipaddr;
}

const char *get_ifadapter_name()
{
	static char if_name[8] = "";
	struct ifaddrs *ifaddr, *ifaddr_head;

	if_name[0] = '\0';
	if (getifaddrs(&ifaddr_head) == 0)
	{
		ifaddr = ifaddr_head;
		while (ifaddr)
		{
			if (ifaddr->ifa_addr && ifaddr->ifa_addr->sa_family == AF_INET &&
				strcmp(ifaddr->ifa_name, "lo"))
			{
				strcpy(if_name, ifaddr->ifa_name);
				break;
			}
			ifaddr = ifaddr->ifa_next;
		}
		freeifaddrs(ifaddr_head);
	}
	return if_name;
}

int get_all_interface(IF_ATTR4 ifattr[], int size)
{
	struct ifaddrs *ifaddr, *ifaddr_head;
	int entry = 0;

	if (getifaddrs(&ifaddr_head) == 0)
	{
		ifaddr = ifaddr_head;
		while (ifaddr && entry < size)
		{
			if (ifaddr->ifa_addr && ifaddr->ifa_addr->sa_family == AF_INET &&
				strcmp(ifaddr->ifa_name, "lo") != 0 &&
				(ifaddr->ifa_flags & IFF_POINTOPOINT) == 0)
			{
				strcpy(ifattr[entry].ifa_name, ifaddr->ifa_name);
				ifattr[entry].addr.s_addr = ((SOCKADDR_IN *)ifaddr->ifa_addr)->sin_addr.s_addr;
				ifattr[entry].mask.s_addr = ((SOCKADDR_IN *)ifaddr->ifa_netmask)->sin_addr.s_addr;
				ifattr[entry].broadcast.s_addr = ((SOCKADDR_IN *)ifaddr->ifa_broadaddr)->sin_addr.s_addr;
				entry++;
			}
			ifaddr = ifaddr->ifa_next;
		}
		freeifaddrs(ifaddr_head);
	}
	return entry;
}

int get_netmask(const char *ifname, SOCKADDR_IN *netmask)
{
	int rc, fd;
	struct ifreq ifr;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;
	bzero((char *)&ifr, sizeof(ifr));
	strcpy(ifr.ifr_name, ifname == NULL ? get_ifadapter_name() : ifname);
	rc = ioctl(fd, SIOCGIFNETMASK, &ifr);
	close(fd);
	if (rc == 0)
	{
		memcpy(netmask, &ifr.ifr_ifru.ifru_netmask, sizeof(struct sockaddr_in));
	}
	return rc;
}

int set_netmask(const char *ifname, SOCKADDR_IN *netmask)
{
	int rc, fd;
	struct ifreq ifr;
	struct sockaddr_in netmask_addr;
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;
	bzero((char *)&ifr, sizeof(ifr));
	strcpy(ifr.ifr_name, ifname == NULL ? get_ifadapter_name() : ifname);
	bzero(&netmask_addr, sizeof(struct sockaddr_in));
	netmask_addr.sin_family = AF_INET;
	netmask_addr.sin_addr = netmask->sin_addr;
	memcpy((char *)&ifr.ifr_ifru.ifru_netmask, (char *)&netmask_addr, sizeof(struct sockaddr_in));
	rc = ioctl(fd, SIOCSIFNETMASK, &ifr);
	close(fd);
	return rc;
}

int get_netaddr(const char *ifname, SOCKADDR_IN *netaddr)
{
	int rc, fd;
	struct ifreq ifr;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;
	bzero((char *)&ifr, sizeof(ifr));
	strcpy(ifr.ifr_name, ifname == NULL ? get_ifadapter_name() : ifname);
	rc = ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);
	if (rc == 0)
	{
		memcpy(netaddr, &ifr.ifr_ifru.ifru_addr, sizeof(struct sockaddr_in));
	}
	return rc;
}

int set_netaddr(const char *ifname, SOCKADDR_IN *netaddr)
{
	int rc, fd;
	struct ifreq ifr;
	struct sockaddr_in addr;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;
	bzero((char *)&ifr, sizeof(ifr));
	strcpy(ifr.ifr_name, ifname == NULL ? get_ifadapter_name() : ifname);
	bzero(&addr, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr = netaddr->sin_addr;
	memcpy((char *)&ifr.ifr_ifru.ifru_addr, (char *)&addr, sizeof(struct sockaddr_in));
	rc = ioctl(fd, SIOCSIFADDR, &ifr);
	close(fd);
	return rc;
}

int get_mac_addr(const char *ifname, unsigned char *ifaddr)
{
	int rc = -1, skfd;
	struct ifreq ifr;

	if (ifname == NULL)
		strcpy(ifr.ifr_name, get_ifadapter_name());
	else
		strcpy(ifr.ifr_name, ifname);

	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
	{
		// printf("get_mac_addr - if_name = %s\n", ifr.ifr_name );
		if (ioctl(skfd, SIOCGIFHWADDR, &ifr) != -1)
		{
			memcpy(ifaddr, ifr.ifr_hwaddr.sa_data, 6);
			rc = 0;
		}
		close(skfd);
	}
	return rc;
}

int set_mac_addr(const char *ifname, unsigned char *ifaddr)
{
	int rc = -1, skfd;
	struct ifreq ifr;

	if (ifname == NULL)
		strcpy(ifr.ifr_name, get_ifadapter_name());
	else
		strcpy(ifr.ifr_name, ifname);

	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
	{
		// printf("set_mac_addr - if_name = %s\n", ifr.ifr_name );
		ifr.ifr_addr.sa_family = ARPHRD_ETHER;
		memcpy(ifr.ifr_hwaddr.sa_data, ifaddr, 6);
		if ((rc = ioctl(skfd, SIOCSIFHWADDR, &ifr)) != 0)
			perror("SIOCSIFHWADDR");
		close(skfd);
	}
	return rc;
}

const char *mac2string(unsigned char *hwaddr)
{
	static char macaddr[20];
	sprintf(macaddr, "%02x:%02x:%02x:%02x:%02x:%02x",
			hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
	return macaddr;
}

unsigned long get_gatewayaddr(const char *ifname, char *buf)
{
	FILE *fp;
	char iface[16];
	char line[128];
	unsigned long gw_addr = INADDR_NONE, dest_addr, gate_addr;
	struct in_addr inaddr;

	fp = fopen("/proc/net/route", "r");
	if (fp == NULL)
		return gw_addr;
	if (ifname == NULL)
		ifname = get_ifadapter_name();

	/* skip title line */
	fgets(line, sizeof(line), fp);
	while (fgets(line, sizeof(line), fp))
	{
		if (strlen(line) < 10)
			continue;
		if (sscanf(line, "%s\t%lX\t%lX", iface, &dest_addr, &gate_addr) != 3 ||
			dest_addr != 0 ||
			(ifname != NULL && strcmp(iface, ifname) != 0))
			continue;
		gw_addr = gate_addr;
		break;
	}
	fclose(fp);
	if (buf != NULL)
	{
		inaddr.s_addr = gw_addr;
		strcpy(buf, inet_ntoa(inaddr));
	}
	return gw_addr;
}

int add_default_gateway(const char *ifname, SOCKADDR_IN *gwaddr)
{
	int fd, rc;
	struct rtentry rm;
	SOCKADDR_IN sin;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;
	if (ifname == NULL)
		ifname = get_ifadapter_name();
	bzero(&rm, sizeof(struct rtentry));
	rm.rt_dst.sa_family = AF_INET;
	rm.rt_gateway.sa_family = AF_INET;
	rm.rt_genmask.sa_family = AF_INET;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = gwaddr->sin_addr.s_addr;
	memcpy(&rm.rt_gateway, &sin, sizeof(sin));
	rm.rt_dev = (char *)ifname;
	rm.rt_flags = RTF_UP | RTF_GATEWAY;
	rc = ioctl(fd, SIOCADDRT, &rm);
	close(fd);
	return rc;
}

int del_default_gateway(const char *ifname, SOCKADDR_IN *gwaddr)
{
	int fd, rc;
	struct rtentry rm;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;
	if (ifname == NULL)
		ifname = get_ifadapter_name();
	bzero(&rm, sizeof(struct rtentry));
	rm.rt_dst.sa_family = AF_INET;
	((SOCKADDR_IN *)&rm.rt_dst)->sin_addr.s_addr = INADDR_ANY;
	rm.rt_genmask.sa_family = AF_INET;
	if (gwaddr != NULL)
	{
		gwaddr->sin_family = AF_INET;
		memcpy(&rm.rt_gateway, gwaddr, sizeof(SOCKADDR_IN));
	}
	rm.rt_dev = (char *)ifname;
	rm.rt_flags = RTF_GATEWAY;
	rc = ioctl(fd, SIOCDELRT, &rm);
	if (rc != 0)
		perror("del_afgateway (SIOCDELRT)");
	close(fd);
	return rc;
}

int ifconfig(const char *ifname, const char *ipaddr, const char *netmask, const char *gwip)
{
	struct sockaddr_in sin;
	struct ifreq ifr;
	int fd;

	bzero(&ifr, sizeof(struct ifreq));
	if (ifname == NULL)
		ifname = get_ifadapter_name();
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1)
		return -1;

	// set IP address
	if (ipaddr != NULL)
	{
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
		ifr.ifr_name[IFNAMSIZ - 1] = 0;
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = inet_addr(ipaddr);
		memcpy(&ifr.ifr_addr, &sin, sizeof(sin));
		if (ioctl(fd, SIOCSIFADDR, &ifr) < 0)
		{
			close(fd);
			return -1;
		}
	}
	// set netmask
	if (netmask != NULL)
	{
		bzero(&ifr, sizeof(struct ifreq));
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
		ifr.ifr_name[IFNAMSIZ - 1] = 0;
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = inet_addr(netmask);
		memcpy(&ifr.ifr_addr, &sin, sizeof(sin));
		if (ioctl(fd, SIOCSIFNETMASK, &ifr) < 0)
		{
			close(fd);
			return -1;
		}
	}
	/////////////////////////////////
	if (gwip != NULL)
	{
		struct rtentry rm;
		bzero(&rm, sizeof(struct rtentry));
		rm.rt_dst.sa_family = AF_INET;
		rm.rt_gateway.sa_family = AF_INET;
		rm.rt_genmask.sa_family = AF_INET;
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = inet_addr(gwip);
		memcpy(&rm.rt_gateway, &sin, sizeof(sin));
		rm.rt_dev = (char *)ifname;
		rm.rt_flags = RTF_UP | RTF_GATEWAY;
		if (ioctl(fd, SIOCADDRT, &rm) < 0)
		{
			close(fd);
			return -1;
		}
	}
	/////////////////////////////////
	// bring up adapter
	ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
	ioctl(fd, SIOCSIFFLAGS, &ifr);
	close(fd);
	return 0;
}

unsigned long INET_ATON(const char *ipstr)
{
	return (unsigned long)inet_addr(ipstr);
}

const char *INET_NTOA(unsigned long ip)
{
	struct in_addr inaddr;
	inaddr.s_addr = ip;
	return inet_ntoa(inaddr);
}

const char *INET_NTOA2(unsigned long ip, char *numdot)
{
	char *ptr = numdot;
	unsigned int a, b, c, d;

	a = ip & 0x000000ff;
	b = (ip & 0x0000ff00) >> 8;
	c = (ip & 0x00ff0000) >> 16;
	d = (ip & 0xff000000) >> 24;

	sprintf(ptr, "%d.%d.%d.%d", a, b, c, d);
	return ptr;
}

////////////////////////////////////////////////////////////
#pragma pack(push, 1)
typedef struct
{
	unsigned char icmp_type;	  // 消息类型
	unsigned char icmp_code;	  // 清息代码
	unsigned short icmp_checksum; // 16位效验
	unsigned short icmp_id;		  // 用来唯一标识些请求的ID号
	unsigned short icmp_sequence; // 序列号
	unsigned long icmp_timestamp; // 时间戳
} ICMPPACK, *PICMPPACK;
#pragma pack(pop)
/*
int sock_get_ttl(SOCKET s)
{
	int ttl_count;
	int len = sizeof(ttl_count);
	int rc = getsockopt(s,IPPROTO_IP,IP_TTL,(char *)&ttl_count,&len);
	return ttl_count;
}
*/
static unsigned short icmp_checksum(unsigned short *buff, int size)
{
	unsigned long cksum = 0;
	while (size > 1)
	{
		cksum += *buff++;
		size -= sizeof(unsigned short);
	}
	if (size)
	{
		cksum += *(unsigned char *)buff;
	}
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += cksum >> 16;
	return (unsigned short)(~cksum);
}

static unsigned long GetTickCount()
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
#define closesocket(s) close(s)
#define GetCurrentProcessId() getpid()
#define Sleep(n) usleep((n)*1000)

// 返回值
// >=0 平均响应时间(msec)
// -1 没有任何应答
int ping(const char *IP, unsigned short port, int ping_times)
{
	struct sockaddr_in desAddr;
	SOCKET sock_raw;
	char buff[sizeof(ICMPPACK) + 32]; // 创建ICMP包
	PICMPPACK pICMP = (PICMPPACK)buff;
	ICMPPACK *pTemp;
	unsigned short nSeq = 0;
	char recvBuf[1024];
	struct sockaddr_in from;
	int nLen = sizeof(struct sockaddr_in);
	int count = 0;
	int nRet;
	int nTick;
	int success_times = 0;
	int total_msec = 0;
	int nTimeOut = 1000; // 每次发送ping帧只等待1000msec获取应答,没有收到就当作没有应答
	long dwTick2Wait;

	sock_raw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (INVALID_SOCKET == sock_raw)
	{
		// printf("failed to create raw socket\n");
		return 0;
	}

	// 设置接收超时
	setsockopt(sock_raw, SOL_SOCKET, SO_RCVTIMEO, (char const *)&nTimeOut, sizeof(nTimeOut));

	// 设置目标地址
	desAddr.sin_addr.s_addr = inet_addr(IP);
	desAddr.sin_port = htons(port);
	desAddr.sin_family = AF_INET;

	pICMP->icmp_type = 8; // 请求回显
	pICMP->icmp_code = 0;
	pICMP->icmp_checksum = 0;
	pICMP->icmp_id = (unsigned short)GetCurrentProcessId();
	pICMP->icmp_sequence = 0;
	pICMP->icmp_timestamp = 0;

	memset(&buff[sizeof(ICMPPACK)], 'E', 32); // 填充用户数据
	for (count = 0; count < ping_times; count++)
	{

		// wait for a short period before start to send next ICMP packet
		if (count)
			Sleep(100);

		// send ping packet
		pICMP->icmp_checksum = 0;
		pICMP->icmp_sequence = nSeq++;
		pICMP->icmp_timestamp = GetTickCount();
		pICMP->icmp_checksum = icmp_checksum((unsigned short *)buff, sizeof(ICMPPACK) + 32);
		if (sendto(sock_raw, buff, sizeof(ICMPPACK) + 32, 0, (struct sockaddr *)&desAddr, sizeof(desAddr)) == -1)
		{
			// printf("sendto() failed \n");
			closesocket(sock_raw);
			return -1;
		}
		dwTick2Wait = GetTickCount() + nTimeOut;
		// receive ICMP response
		while (GetTickCount() <= dwTick2Wait)
		{
			if ((nRet = recvfrom(sock_raw, recvBuf, sizeof(recvBuf), 0, (struct sockaddr *)&from, &nLen)) <= 0)
			{
				if (nRet == -1) // 超时
				{
					continue; // 第一次不可能time-out, 至少会收到自己发出去的帧
				}
				else // 0 - socket关闭,几乎不可能发生
				{
					closesocket(sock_raw);
					return -1;
				}
			}
			nTick = GetTickCount();
			if (nRet < 20 + sizeof(ICMPPACK))
			{
				// printf("too few bytes from %s, ignored \n",inet_ntoa(from.sin_addr));
				continue;
			}
			pTemp = (ICMPPACK *)(recvBuf + 20); // 前面20字节是IP头
			if (pTemp->icmp_type != 0)
			{
				// printf(" not ICMP_ECHO type received\n");
				continue;
			}

			if (pTemp->icmp_id != (unsigned short)GetCurrentProcessId())
			{
				// printf("get some one else packet\n");
				continue;
			}
			/*/printf("%d reply from %s: bytes=%d time<%dms TTL=%d\n",
				pTemp->icmp_sequence,
				inet_ntoa(from.sin_addr),
				nRet,
				nTick-pTemp->icmp_timestamp,
				sock_get_ttl (sock_raw));
				*/
			// now is the ICMP response from target
			total_msec += nTick - pTemp->icmp_timestamp;
			success_times++;
			break;
		} // while ( GetTickCount() <= dwTick2Wait )
	}	  // for (count=0,...
	if (success_times > 0)
		return total_msec / success_times + 1;
	return -1;
}

#endif
