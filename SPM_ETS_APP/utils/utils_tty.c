#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <termios.h>
#include "utils_tty.h"

int tty_open(const char *device)
{
	return open(device, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
}

void tty_get(int fd, struct termios *p_ttyios)
{
	tcgetattr(fd, p_ttyios);
}

void tty_set(int fd, struct termios *p_ttyios)
{
	tcsetattr(fd, TCSADRAIN, p_ttyios);
}

static speed_t get_speed_bits(int speed)
{
	switch (speed)
	{
	case 50:
		return B50;
	case 75:
		return B75;
	case 110:
		return B110;
	case 134:
		return B134;
	case 150:
		return B150;
	case 200:
		return B200;
	case 300:
		return B300;
	case 600:
		return B600;
	case 1200:
		return B1200;
	case 1800:
		return B1800;
	case 2400:
		return B2400;
	case 4800:
		return B4800;
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	}
	return 0;
}

static int get_data_bits(int nbits)
{
	switch (nbits)
	{
	case 5:
		return CS5;
	case 6:
		return CS6;
	case 7:
		return CS7;
	case 8:
		return CS8;
	}
	return CS8;
}

#define get_parity_bits(p)  ( (p)==2 ?

/* init terminal so that we can grab keys */
int tty_raw(int fd_tty, int baud, int databits, int parity)
{
	struct termios tty;

	if (tcgetattr(fd_tty, &tty) != 0)
		return -1;
	;

	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tty.c_oflag &= ~(OPOST | OLCUC | ONLCR | OCRNL | ONOCR | ONLRET);
	tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tty.c_cflag &= ~(CSIZE | PARENB);
	tty.c_cflag |= get_data_bits(databits);
	if (parity == 1)
	{
		tty.c_cflag |= (PARENB | PARODD);
	}
	else if (parity == 2)
	{
		tty.c_cflag |= PARENB;
		tty.c_cflag &= (~PARODD);
	}
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;
	// set line speed
	if (baud > 0)
	{
		speed_t line_speed = get_speed_bits(baud);
		if (line_speed != 0)
			cfsetspeed(&tty, line_speed);
	}
	return tcsetattr(fd_tty, TCSADRAIN, &tty);
}

int tty_setraw(int fd, struct termios *tio)
{
	struct termios ttyios;
	if (tcgetattr(fd, tio) != 0)
		return -1;
	memcpy(&ttyios, tio, sizeof(ttyios));
	ttyios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	ttyios.c_oflag &= ~(OPOST | OLCUC | ONLCR | OCRNL | ONOCR | ONLRET);
	ttyios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	ttyios.c_cflag &= ~(CSIZE | PARENB);
	ttyios.c_cflag |= CS8;
	ttyios.c_cc[VMIN] = 1;
	ttyios.c_cc[VTIME] = 0;
	return tcsetattr(fd, TCSANOW, &ttyios);
}

/*
 * check if any character is in the input buffer within specified time (msec)
 */
int tty_ready(int fd, int tout)
{
	int n = 0;
	fd_set set;
	struct timeval tval, *ptval;

	ptval = &tval;
	if (tout >= 1000)
	{
		tval.tv_sec = tout / 1000;
		tval.tv_usec = (tout % 1000) * 1000;
	}
	else if (tout > 0)
	{
		tval.tv_sec = 0;
		tval.tv_usec = (tout % 1000) * 1000;
	}
	else if (tout == 0)
	{
		tval.tv_sec = 0;
		tval.tv_usec = 0;
	}
	else /* < 0 */
		ptval = NULL;

	FD_ZERO(&set);
	FD_SET(fd, &set);
	n = select(fd + 1, &set, NULL, NULL, ptval);

	return n == 1 && FD_ISSET(fd, &set);
}

/*
 * read one raw character from tty device.
 * fd_tty: fd of tty (must call term_raw once before use this function)
 * tout: time out setting.
 *	 > 0 time out in milli-seconds.
 *         0 no time out, return immediately if no input in buffer.
 *        -1 wait until a character is read.
 *  return value:
 *      > 0: character received from tty
 *       -1: no data.
 */
#define MAX_SEQ 3
#define MSBCH 10 // 10 ms delay time between characters

int tty_getc(int fd, int tout, int esc_on)
{
	int rc;
	int ch = 0;

	if ((rc = tty_ready(fd, tout)) > 0)
	{
		rc = read(fd, &ch, 1);
		/* if this is a <Esc>, check for escape sequence
		 * If input is not a valid escape sequence. The char has been
		 * read and checked for will be lost. Sorry, I cannot unget the chars.
		 */
#if 0 		 
  		if ( ch == KEY_ESC && esc_on && tty_ready(fd,MSBCH) )
  		{
  			char	ch1, ch2;
				int	key;

  			read(fd, &ch, 1);
  			if ( ch != '[' )
  				return ch;	// Sorry, Esc is gone.

  			read(fd, &ch1, 1);
  			switch( ch1 )
  			{
  				case 'A':	return KEY_UP;
  				case 'B':	return KEY_DOWN;
  				case 'C':	return KEY_RIGHT;
  				case 'D':	return KEY_LEFT;
  				case 'P':	return KEY_PAUSE;
  				case '1':	key = KEY_HOME;		break;
  				case '2':	key = KEY_INS;		break;
  				case '3':	key = KEY_DEL;		break;
  				case '4':	key = KEY_END;		break;
  				case '5':	key = KEY_PGUP;		break;
  				case '6':	key = KEY_PGDN;		break;
  				default:
  					key = 0;
  			}
  			if ( key != 0 && tty_ready(fd,MSBCH) )
  			{
  				read( fd, &ch2, 1 );
  				if ( ch2 == '~' ) return key;
  				if ( key == KEY_HOME )		// <Esc>[1x~
  				{	/* check for F1 ~ F12 */
  					if ( ch2 >= '1' && ch2 <= '5' )
  						key = KEY_FN(ch2-'0');		// F1 ~ F5
  					else if ( ch2 >= '7' && ch2 <= '9' )
  						key = KEY_FN(ch2-'1');		// F6 ~ F8
  					else
  						key = 0;
  				}
  				else if ( key == KEY_INS )	// <Esc>[2x~
  				{
  					if ( ch2 == '0' || ch2 == '1' )
  						key = KEY_FN(ch2-'0'+9);	// F9, F10
  					else if ( '3' <= ch2 && ch2 <= '6' )
  						key = KEY_SFN(ch2-'2');		// Shift-F1 ~ Shift-F4
  					else if ( ch2=='8' || ch2=='9' )
  						key = KEY_SFN(ch2-'3');		// Shift-F5, Shift-F6
  					else
  						key = 0;
  				}
  				else if ( key == KEY_DEL )	// <Esc>[3x~
  				{
  					if ( '1' <= ch2 && ch2 <= '4' )
  						key = KEY_SFN(ch2-'1'+7);
  					else
  						key = 0;
  				}
  				if ( key != 0 )
  				{
  					read( fd, &ch2, 1 );		// eat last '~'
  					if ( ch2 != '~' )
  						key = 0;
  				}
  			}
				return key == 0 ? ch1 : key;	// sorry drop few characters.
  		}
#endif
		return rc == 1 ? ch : -1;
	}
	return -1;
}

/*
 * parameters:
 *   tout:
 *		  0: No wait. Get until no character or 'eol' is received.
 *     -1: Get until buffer full or eol is get.
 *     >0: 'tout' msec after last character is received.
 *    eol: end of line character. If not 0 then receive until this 'eol' is encounted
 *         (or tout, or buffer full).
 *     ec: adress to hold the end code (0 normal ending, -1 tout, -2 buffer full)
 * return value:
 *    >0: number of bytes received.
 */
int tty_gets(int fd, char *buf, int bufsize, int tout, int eol, int *ec)
{
	int nget = 0;
	int ch = 0;

	while (nget < bufsize && (ch = tty_getc(fd, tout, 0)) >= 0) // while ( nget < bufsize && (ch=tty_getc( fd, tout, 0 ))!=0 ) 20151020 ºú
	{
		*buf++ = ch;
		nget++;
		if (eol != 0 && eol == ch)
			break;
	}
	if (ch < 0 && ec)
		*ec = -1;
	else if (ch != eol && nget == bufsize && ec)
		*ec = -2;
	return nget;
}

// read N bytes. return number of bytes read.
// tout is timeout between each character.
//   0: no wait.
//  -1: wait until request # of character is filled
//  >0: msec to wait for next character. If over, return to caller.
int tty_read(int fd, char *buf, int bufsize, int tout)
{
	int nc = 0;

	if (tout == -1)
		return read(fd, buf, bufsize);
	else
	{
		while (nc < bufsize && tty_ready(fd, tout) > 0)
		{
			read(fd, buf++, 1);
			nc++;
		}
		return nc;
	}
}

//-------------------  terminal display control ---------------------
//
void tty_goto(int fd, int x, int y)
{
	char *escape = "\033[%d;%dH";
	char cmd[64];

	sprintf(cmd, escape, x, y);
	write(fd, cmd, strlen(cmd));
}

// attribute list argument must be terminated by -1
// ex.: tty_attribute( 1, 1, 5, 34, 40, -1 );

void tty_attribute(int tty, ...)
{
	va_list va;
	int attr, narg = 0;
	char cmd[80] = "\033[";
	char *ptr = cmd + 2;

	va_start(va, tty);

	while ((attr = va_arg(va, int)) != -1)
	{
		if (narg == 0)
			sprintf(ptr, "%d", attr);
		else
			sprintf(ptr, ";%d", attr);
		ptr += strlen(ptr);
		narg++;
	}

	va_end(va);

	if (narg)
	{
		*ptr++ = 'm';
		*ptr++ = '\0';
		write(tty, cmd, strlen(cmd));
	}
}

/*
 * tty_iqueue:
 *	Return the number of bytes of data avalable to be read in the serial port
 *********************************************************************************
 */

int tty_iqueue(const int fd)
{
	int result;

	if (ioctl(fd, FIONREAD, &result) == -1)
		return -1;

	return result;
}

int tty_read_until(int fd, char *buf, int size, int eol)
{
	int i = 0, rc;

	while ((rc = tty_getc(fd, 20, 0)) >= 0)
	{
		buf[i++] = (char)rc;
		if (i == size || rc == eol)
			break;
	}
	return i;
}
