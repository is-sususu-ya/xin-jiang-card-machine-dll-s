
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <netdb.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <ifaddrs.h>
// #include <net/if.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <linux/if_arp.h>
#include <linux/route.h>

extern void on_qr_ready(const char *msg);

int core_start()
{
	char text[32];
	RWDevStart();
	while (1)
	{
#if 0
		spm_send_qrcode(0,"12312312312");
#endif
		sleep(5);
	}
}

int core_stop()
{
	RWDevStop();
}
