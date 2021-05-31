/*
 *	utils_tty.h
 *
 *  TTY i/o and control functions
 */
#ifndef _UTILS_TTY_H_
#define _UTILS_TTY_H_

#include <termios.h>

#ifdef __cplusplus
extern "C" {
#endif


/*   fd_tty: the fd for a tty.
 *   p_tty:  point to caller's buffer to store the original setting of the fd.
 *   baud:   the baud rate setting. 0 means do not change the orignal line speed.
 */
int tty_raw( int fd_tty, struct termios *p_tty, int baud );

/*
 * tty_set: set the tty device termios. Ususally, resume the orginal setting get from tty-set
 */
void tty_set( int fd_tty, struct termios *p_tty );

int tty_ready( int fd, int tout );

/*
 * tty_getc: read one raw character from tty device. Special key with escape sequence
 *           will be converted to single key code.
 *   fd_tty: fd of tty (must call term_raw once, before use this function)
 *   tout: time out setting.
 *	 > 0 time out in mini-seconds.
 *         0 no time out, return immediately if no input in buffer.
 *        -1 wait until a character is read.
 *   esc_on: enable the escape sequence interpret. 0 will turn it off.
 *    return value:
 *       > 0: character received from tty
 *         0: no data.
 *        -1: I/O error
 */
int tty_getc( int fd, int tout, int esc_on );
int tty_putc(int fd, int ch);

/* tty_gets: read one line which terminated with specified character
 * parameters:
 *   tout:
 *	0: No wait. Get until no character or 'eol' is received.
 *     -1: Get until buffer full or eol is get.
 *     >0: 'tout' msec after last character is received.
 *    eol: end of line character. If not 0 then receive until this 'eol' is encounted
 *         (or tout, or buffer full).
 *     ec: address to hold the end code (0 normal ending, -1 tout, -2 buffer full)
 * return value:
 *    >0: number of bytes received.
 */
int tty_gets( int fd, char *buf, int bufsize, int tout, int eol, int *ec );

// Followung function uses the terminal's Escape Sequence to control the
// display effect. Don't set tty to raw mode. Just open it then control.
// This is a easy way to do the simple control w/o the huge curset.

// get binary data. return number of bytes read.
// tout is time out in msec to wait for next byte. (tout between bytes).
//   0: no wait
//  -1: wait forever until 'bufsize' bytes are received.
//  >0: msec to wait for next byte.
int tty_read_tout( int fd, char *buf, int bufsize, int tout );

// discard specified queue (input or output)
#define TTY_IQUEUE	TCIFLUSH
#define TTY_OQUEUE	TCOFLUSH
#define tty_flush(fd, qsel)		tcflush(fd,qsel)
#define tty_flush_in(fd)		tcflush(fd,TCIFLUSH)
#define tty_flush_out(fd)		tcflush(fd,TCOFLUSH)
#define tty_flush_io(fd)		tcflush(fd,TCIFLUSH|TCOFLUSH)
#define tty_clear(fd)           tcflush(fd,TCIFLUSH|TCOFLUSH)
// wait for output queue drain (send out)
#define tty_drain(fd)					tcdrain(fd)
// tty read/write
#define tty_write(fd,buf,sz)	write(fd,buf,sz)
#define tty_read(fd,buf,sz)		read(fd,buf,sz)

int tty_skip_until(int fd, unsigned char *soh, int size, int *found );
int tty_read_until(int fd, char *buf, int size, int eol);
#define tty_read_line(fd, bf, sz)	tty_read_until(fd, bf, sz, '\n');
int tty_skip_until_oneof(int fd, unsigned char *soh[], int nh, int size, int *soh_index );
int tty_read_n_bytes(int fd, void *buffer, int len);
int tty_skip_n_bytes( int fd, int n );

#define tty_close(fd)		close(fd)

int tty_iqueue(int fd);		// get number of bytes in input buffer available for reading
int tty_oqueue(int fd);		// get number of bytes in output buffer to be transmitted



#ifdef __cplusplus
};
#endif

#endif

