#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "utils_tty.h"

#ifdef linux
#include <unistd.h>
#endif
/* init terminal so that we can grab keys */
void tty_set( int fd, struct termios *p_tty )
{
#ifdef linux
    tcsetattr (fd, TCSADRAIN, p_tty);
#endif
}

int tty_raw( int fd_tty, struct termios *p_tty, int baud )
{
#ifdef linux
   	 static int baudrate[] = {600, 1200, 1800, 2400, 4800, 9600, 19200, 38400, /* 0~7*/
   	 	57600, 115200, 230400, 460800, 500000, 576000, 921600,	/* 8~14 */
   	 	1000000, 1152000, 1500000, 2000000 };		/* 15 ~ 18 */
    	struct termios tty;

    	if ( tcgetattr (fd_tty, &tty) != 0 )
				return -1;;
    	if ( p_tty != NULL )
    		memcpy( p_tty, &tty, sizeof( struct termios ) );
#if 1
    	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
                          |INLCR|IGNCR|ICRNL|IXON);
    	tty.c_oflag &= ~(OPOST|OLCUC|ONLCR|OCRNL|ONOCR|ONLRET);
    	tty.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
    	tty.c_cflag &= ~(CSIZE|PARENB);
    	tty.c_cflag |= CS8;
    	tty.c_cc[VMIN] = 1;
    	tty.c_cc[VTIME] = 0;
#else
    	if (cfmakeraw( &tty ) != 0 );		// Same effect as code in above block
    		return -1;
#endif
    	if ( baud > 0 )
    	{
				speed_t line_speed = B9600;
				int i, n = sizeof(baudrate)/sizeof(int);
		
				for (i=0; i<n; i++)
	    		if ( baud == baudrate[i] ) break;
				if ( i < 8 )
	    		line_speed = B600 + i;
	    	else if (i < 15)
	    		line_speed = B57600 + i - 8;
	    	else if ( i < n )
	    		line_speed = B1000000 + i - 15;
	    	if ( cfsetspeed( &tty, line_speed ) != 0 )
	    		return -1;
     	}
    	return tcsetattr (fd_tty, TCSADRAIN, &tty);
#else
	return 0;
#endif
}

/*
 * check if any character is in the input buffer within specified time (msec)
 */
int tty_ready( int fd, int tout )
{
	int	n=0;
  fd_set	set;
	struct timeval tval, *ptval;

	ptval = &tval;
	if ( tout > 0 )
	{
		tval.tv_sec = tout / 1000;
		tval.tv_usec = (tout % 1000) * 1000;
	}
	else if ( tout == 0 )
	{
		tval.tv_sec = 0;
		tval.tv_usec = 0;
	}
	else	/* < 0, blocked until data available to read */
		ptval = NULL;

	FD_ZERO(& set);
	FD_SET(fd, & set);
	n = select( fd +1, & set, NULL, NULL, ptval );

	return n==1 && FD_ISSET(fd, &set);
}

/*
 * read one raw character from tty device.
 * fd_tty: fd of tty (must call term_raw once before use this function)
 * tout: time out setting.
 *	 > 0 time out in milli-seconds.
 *         0 no time out, return immediately if no input in buffer.
 *        -1 wait until a character is read.
 *  return value:
 *      >= 0: character received from tty
 *       -1: no data.
 */
int tty_getc( int fd, int tout, int esc_on )
{
	int		rc=-1;
	unsigned char	ch = 0;

	if ( (rc = tty_ready( fd, tout )) > 0 )
	{
		rc = read( fd, &ch, 1 );
	}
 	return rc == 1 ? ch : -1;
}

int tty_putc(int fd, int ch)
{
	return write(fd, &ch, 1);
}

/*
 * parameters:
 *   tout:
 *	0: No wait. Get until no character or 'eol' is received.
 *     -1: Get until buffer full or eol is get.
 *     >0: 'tout' msec after last character is received.
 *    eol: end of line character. If not 0 then receive until this 'eol' is encounted
 *         (or tout, or buffer full).
 *     ec: adress to hold the end code (0 normal ending, -1 tout, -2 buffer full)
 * return value:
 *    >0: number of bytes received.
 */
int tty_gets( int fd, char *buf, int bufsize, int tout, int eol, int *ec )
{
	int	nget = 0;
	int	ch=0;

	while ( nget < bufsize && (ch=tty_getc( fd, tout, 0 ))>=0 )
	{
		*buf++ = ch;
		nget++;
		if ( eol!=0 && eol==ch ) break;
	}
	if ( ch < 0 && ec )
		*ec = -1;
	else if ( ch!=eol && nget==bufsize && ec )
		*ec = -2;
	return nget;
}

// read N bytes. return number of bytes read.
// tout is timeout value.
//   0: no wait.
//  -1: wait until request # of character is filled
//  >0: msec to wait for next character. If timed-out, terminate the read
// return value:
//	 number of bytes read. read terminated when specified number of bytes 
//   has received or time out.
int tty_read_tout( int fd, char *buf, int bufsize, int tout )
{
	int	n, nc = 0;
	
	if ( tout == -1 )
	{
		while ( bufsize > 0 )
		{
			n = read( fd, buf, bufsize );
			nc += n;
			bufsize -= n;
			buf += n;
		}
	}
	else if ( tout == 0 )
	{
		nc = read(fd, buf, bufsize);
	}
	else
	{
		unsigned long lt_end = GetTickCount() + tout;
		do
		{
			n = read( fd, buf, bufsize );
			nc += n;
			bufsize -= n;
			buf += n;
		} while ( bufsize>0 && GetTickCount() < lt_end );
	}
	return nc;
}

int tty_skip_until(int fd, unsigned char *soh, int size, int *found )
{
	int i, rc;
	int nskip = 0;
	*found = 0;
catch_again:
	while ( (rc=tty_getc(fd,size,0))>=0 && (unsigned char)rc != soh[0] ) 
	{
		nskip++;
	}
	if ( rc == -1 ) return nskip;
	for(i=1; i<size; i++)
	{
		if ( (rc=tty_getc(fd,size,0))<0 || (unsigned char)rc!=soh[i] )
		{
			if ( rc>=0 ) 
			{
				nskip++;
				goto catch_again;
			}
			else
				return nskip;
		}
	}
	*found = 1;
	return nskip;
}

static int byte_in_oneof(unsigned char ch, unsigned char *soh[],int index,int num_head)
{
	int i;
	for(i=0; i<num_head; i++)
		if ( ch==soh[i][index] )
			return i;
	return -1;
}

int tty_skip_until_oneof(int fd, unsigned char *soh[], int nh, int size, int *soh_index )
{
	int i, rc;
	int nskip = 0;
	int index;

	*soh_index = -1;
catch_again:
	index = -1;
	// get first byte in any one of SOH sequence
	while ( (rc=tty_getc(fd,size,0))>=0 &&  (index=byte_in_oneof((unsigned char)rc,soh,0,nh))==-1 )
	{
		nskip++;
	}
	if ( rc == -1 ) return nskip;
	for(i=1; i<size; i++)
	{
		if ( (rc=tty_getc(fd,size,0))<0 || byte_in_oneof((unsigned char)rc,soh,i,nh)!=index)
		{
			if ( rc>=0 ) 
			{
				nskip++;
				goto catch_again;
			}
			else
				return nskip;
		}
	}
	*soh_index = index;
	return nskip;
}

int tty_read_until(int fd, char *buf, int size, int eol)
{
	int i=0, rc;

	while ( (rc=tty_getc(fd,20,0))>=0 ) 
	{
		buf[i++] = (char)rc;
		if ( i==size || rc==eol ) break;
	}
	return i;	
}

int tty_read_n_bytes(int fd, void *buffer, int n)
{
	char *ptr = (char *)buffer;
	int len;
	while( n > 0 ) 
	{
		len = tty_read(fd, ptr, n);
		if ( len == 0 )
		{
			usleep(5000);
			len = tty_read(fd, ptr, n);
		}
		if ( len==0 ) break;
		if( len == -1 )
			return -1;
		ptr += len;
		n -= len;
	}
	return (ptr-(char*)buffer);	
}

int tty_iqueue(int fd)
{
	int arg;
	ioctl(fd, TIOCINQ, &arg);
	return arg;
}

int tty_oqueue(int fd)
{
	int arg;
	ioctl(fd, TIOCOUTQ, &arg);
	return arg;
}


