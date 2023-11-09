/*
	this is a simple application for read the input data from linux standard input dev, such as keyboard, 
	it just support some en code
	some key we don't express just use the "*"
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/input.h>
#include <errno.h>
#include <time.h>

#define INPUT_NAME "WCH.CN"

static const char chr_table[129] = {
[0] = '\0', [1] = '*', [2] = '1', [3] = '2', [4] = '3', [5] = '4', [6] = '5', [7] = '6', [8] = '7', [9] = '8', [10] = '9',
[11] = '0', [12] = '-', [13] = '=', [14] = '\b', [15] = '\t', [16] = 'q', [17] = 'w', [18] = 'e', [19] = 'r', [20] = 't', [21] = 'y',
[22] = 'u', [23] = 'i', [24] = 'o', [25] = 'p', [26] = '{', [27] = '}', [28] = '\n', [29] = '*', [30] = 'a', [31] = 's', [32] = 'd',
[33] = 'f', [34] = 'g', [35] = 'h', [36] = 'j', [37] = 'k', [38] = 'l', [39] = ':', [40] = '"', [41] = '*', [42] = '*', [43] = '\\',
[44] = 'z', [45] = 'x', [46] = 'c', [47] = 'v', [48] = 'b', [49] = 'n', [50] = 'm', [51] = ',', [52] = '.', [53] = '/', [54] = '*', 
[55] = '*', [56] = '*', [57] = ' ', [58] = '*', [59] = '*', [60] = '*', [61] = '*', [62] = '*', [63] = '*', [64] = '*', [65] = '*', 
[66] = '*', [67] = '*', [68] = '*', [69] = '*', [70] = '*', [71] = '*', [72] = '*', [73] = '*', [74] = '*', [75] = '*', [76] = '*', 
[77] = '*', [78] = '*', [79] = '*',[80] = '*', [81] = '*', [82] = '*', [83] = '*', [84] = '*', [85] = '*', [86] = '*', [87] = '*', 
[88] = '*', [89] = '*', [90] = '*', [91] = '*', [92] = '*', [93] = '*', [94] = '*', [95] = '*', [96] = '*', [97] = '*', [98] = '*', 
[99] = '*', [100] = '*', [101] = '*', [102] = '*', [103] = '*', [104] = '*', [105] = '*', [106] = '*', [107] = '*', [108] = '*',
[109] = '*', [110] = '*', [111] = '*', [112] = '*', [113] = '*', [114] = '*', [115] = '*', [116] = '*', [117] = '*', [118] = '*', 
[119] = '*', [120] = '*', [121] = '*', [122] = '*', [123] = '*', [124] = '*', [125] = '*', [126] = '*', [127] = '*', [128] = '*'
};

// 入口参数为 INPUT_NAME宏
int open_input_dev( const char *input_name )
{
	char buf[128];
	int fd, flags;
	int i = 0;
	if( access( input_name, F_OK ) != 0 )
		return -1;
	if ((fd = open(input_name, O_RDONLY, 0)) >= 0) {
		ioctl(fd, EVIOCGNAME(sizeof(buf)), buf);
		printf("name:%s \r\n", buf );
		flags = fcntl(fd,F_GETFL,0);  
		flags |= O_NONBLOCK;  
		if( 0 != fcntl(fd,F_SETFL,flags) )
		{
			close( fd );
			fd = -1;
			printf("fcntl failed!:%s \r\n", strerror( errno ));
		}
		return fd;
	}
	return -1;
}

int input_ready( int fd, int tout )
{
    fd_set  rfd_set;
    struct  timeval tv, *ptv;
    int nsel;

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

int input_read( int fd, char *buf , int maxsize )
{
	struct input_event event;
	int rc;
	int n = 0;
	char *ptr = buf;
	char ch;
	int index;
	int last_key;
	do{
		if( (rc = read(fd, &event, sizeof(event))) < 0)
			return -1;
		if( event.type == EV_KEY && event.value == 1 )
		{
			index = event.code & 0xff;
			if( index  == 42 )
			{
				last_key = 42;
				continue;
			}
			if( index < 128 )
			{
				ch = chr_table[index];
				if( ch != 0 && ch != '*' )
				{
					if( last_key == 42 )
						*ptr++ = ch - ( 'a' - 'A' );
					else
						*ptr++ = ch;
					last_key = 0;
					n++;
				}
			}
		}
	}while( input_ready( fd, 100 ) > 0 && n < maxsize );
	*ptr = '\0';
	return n;
}
