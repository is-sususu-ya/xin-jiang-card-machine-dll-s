

#include <stdio.h>
#include <pthread.h>
#include <error.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <utils_ptrlist.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utils_net.h>
#include <utils_tty.h>
#include <fcntl.h>
#include "led.h"

#define WORK_TTY 1
#define WORK_TCP 2
#define WORK_UDP 3

typedef short u16;
typedef unsigned char u8;
typedef int COLORREF;
#define OBJ_VALID( h ) ( h->magic == OBJ_MAGIC )
#define OBJ_MAGIC 0xaabbccdd

typedef struct tagOBJECT
{
    int magic;
	int fd;
    HANDLE handle;
	SOCKADDR_IN addr;
	int run;
    int work_mode;
    char name[20];
	char chModel[8];
    PtrList MsgList;
    pthread_t thread;
    pthread_mutex_t recognize_lock; // 识别数据锁
    pthread_mutex_t data_lock;
    pthread_mutex_t lock;
    pthread_cond_t cond;
}OBJECT_S;

static void *work_thread(void *arg);
static LED_CallBack call_back;
#ifdef LED_TEST
#define trace_log printf
#else
extern void trace_log(const char *fmt, ...);
#endif

typedef struct tagLedMsg
{
    int len;
    char data[1024];
} LedMsg;

int led_set_callback( HANDLE h, LED_CallBack cb )
{
	OBJECT_S *pHvObj = (OBJECT_S *)h;
    if (!OBJ_VALID(pHvObj))
    {
        printf("OBJECT VALID..\r\n");
        return;
    }
	call_back = cb;
}

static void led_wait(HANDLE h, int sec)
{
    struct timespec ts;
    int rc;
    OBJECT_S *pHvObj = (OBJECT_S *)h;
    if (!OBJ_VALID(pHvObj))
    {
        printf("OBJECT VALID..\r\n");
        return;
    }
    pthread_mutex_lock(&pHvObj->lock);
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += sec;
    rc = pthread_cond_timedwait(&pHvObj->cond, &pHvObj->lock, &ts);
    pthread_mutex_unlock(&pHvObj->lock);
}

static void led_signal(HANDLE h)
{
    OBJECT_S *pHvObj = (OBJECT_S *)h;
    if (!OBJ_VALID(pHvObj))
    {
        printf("OBJECT VALID..\r\n");
        return;
    }
    // 消息通知
    pthread_mutex_lock(&pHvObj->lock);
    pthread_cond_signal(&pHvObj->cond);
    pthread_mutex_unlock(&pHvObj->lock);
}

static u16 bcd2u16(u8 *bcd)
{
    int i = 2;
    u16 u16 = 0;

    for (; i--;)
    {
        u16 *= 100;
        u16 += (((*bcd) & 0xf0) >> 4) * 10 + ((*bcd) & 0x0f);
        bcd++;
    }
    return u16;
}

static void led_append(HANDLE h, LedMsg *obj)
{
    OBJECT_S *pHvObj = (OBJECT_S *)h;
    if (!OBJ_VALID(pHvObj))
    {
        printf("OBJECT VALID..\r\n");
        return;
    }
    pthread_mutex_lock(&pHvObj->data_lock);
    PtrList_append(&pHvObj->MsgList, obj);
    pthread_mutex_unlock(&pHvObj->data_lock);
    led_signal( h );
}

HANDLE led_init(const char *dev)
{
    OBJECT_S *pHvObj = (OBJECT_S *)malloc(sizeof(OBJECT_S));
    memset(pHvObj, 0, sizeof(OBJECT_S));
    pHvObj->magic = OBJ_MAGIC;
    strcpy(pHvObj->name, dev);
    if (strstr(dev, "tty"))
    {
        pHvObj->work_mode = WORK_TTY;
    }
    else
    {
        if (inet_addr(dev) == -1)
        {
            printf("input ip errror:%s \r\n", dev);
            free(pHvObj);
            return NULL;
        }
        pHvObj->work_mode = WORK_UDP;
    }
    strcpy(pHvObj->name, dev );
    pthread_mutex_init(&pHvObj->recognize_lock, NULL);
    pthread_mutex_init(&pHvObj->data_lock, NULL);
    pthread_mutex_init(&pHvObj->lock, NULL);
    pthread_cond_init(&pHvObj->cond, NULL);
    pthread_create(&pHvObj->thread, NULL, work_thread, pHvObj);
    return pHvObj;
}

extern void trace_log(const char *fmt, ...);
static const char *show_hex(const char *ch, int rlen);
static int led_real_write(HANDLE h, char *buf, int len)
{
    OBJECT_S *pobj = (OBJECT_S *)h;
    int fd = pobj->fd;
    int slen;
    if (fd < 0)
    {
        printf("设备未打开忽略该发送数据..\r\n");
        return 0;
    }
    // tty
    if (pobj->work_mode == WORK_TTY)
    {
        slen = write(fd, buf, len);
		trace_log("write fd[%d] len:%d [%s] \r\n", fd, slen, show_hex( buf, len ));
    }
    else
    {
        // udp
        slen = sendto(fd, buf, len, 0, (const struct sockaddr *)&pobj->addr, sizeof(struct sockaddr_in));
    }
    if (len != slen)
        printf("写入长度:%d， 还差长度：%d 需要写入！\r\n", slen, len - slen);
    return slen;
}

static int led_read(HANDLE h, char *buf, int maxlen)
{
    OBJECT_S *pobj = (OBJECT_S *)h;
    int fd = pobj->fd;
    int rlen;
    if (fd < 0)
    {
        printf("设备未打开忽略该发送数据..\r\n");
        return 0;
    }
    // tty
    if (pobj->work_mode == WORK_TTY)
    {
        rlen = tty_read(fd, buf, maxlen, 400 );
        if (rlen == 0)
            printf("串口已断开连接，请注意！\r\n");
    }
    else
    {
        rlen = read(fd, buf, maxlen);
    }
    return rlen;
}

int tty_open_dd(const char *device)
{
	return	open(device, O_RDWR|O_NOCTTY|O_NDELAY|O_NONBLOCK);
}

static const char *show_hex(const char *ch, int rlen)
{
    int i = 0;
    char *ptr = (char *)ch;
    static char buf[640];
    char *off = buf;
    unsigned char val;
    memset(buf, 0, sizeof(buf));
    int len = rlen > 200 ? 200 : rlen;
    for (i = 0; i < len; i++)
    {
        val = *ptr++;
        sprintf(off, "%02x, ", val);
        off += 3;
    }
    *(off - 1) = '\0';
    return buf;
}

static int isCrcValid(char *data, int len)
{
    u8 tmp = 0x00;
    int i = 0;
    for (i = 0; i < (len - 1); i++)
        tmp ^= data[i];
    return data[len - 1] == tmp ? 1 : 0;
}

static int parse_input_text(const char *text)
{
	return 0;
}

static void parse_qr_data( u8 *buffer, int len  )
{
    char qr_code[128] = {0};
    if( buffer[0] == 0xf1 && ( buffer[len-1] == 0xff  || buffer[len-1] == 0x0a )  )
    {
        memcpy( qr_code, buffer + 1, len - 2 );
        printf("Report QrCode:[%s]\r\n", qr_code );
	    if( call_back )
			call_back( EVT_QR_READY, qr_code );
    }
}

static void parse_input_data(char *buffer, int len )
{
	char qr_code[128] = {0};
	if( len < 5 )
		return;
	if( buffer[0] == 0xa1 && buffer[1] == 0xb1 && buffer[2] == 0xc1 && buffer[3] == 0xd1 && buffer[4] == 0x40 )
	{
		int len = bcd2u16(  buffer + 5 );
		if( len < 0 || len > sizeof( qr_code ))
		{
			printf("二维码长度异常%d ！\r\n", len );
			return -1;
		}
		// len后面有一个字节，是判定那个串口的
		memcpy( qr_code, buffer + 8 , len );
		qr_code[len-1] = '\0';
		printf("QR:[%s]\r\n", qr_code );
		if( call_back )
			call_back( EVT_QR_READY, qr_code );
	}
}

static void *work_thread(void *arg)
{
    OBJECT_S *pHvObj = (OBJECT_S *)arg;
    LedMsg *msg;
    char buffer[1024];
    char recv_buf[1024];
    int  recv_len;
	printf("led work start..\r\n");
	pHvObj->run = 1;
    while( pHvObj->run )
    {
        if (pHvObj->fd <= 0)
        {
            if (pHvObj->work_mode == WORK_TTY)
            {
                pHvObj->fd = open(pHvObj->name, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
                if (pHvObj->fd < 0)
                {
                    printf("open %s error \r\n", pHvObj->name);
                    sleep(10);
                    continue;
                }
                // tty_raw(pHvObj->fd, 115200, 8, 0);
                tty_raw(pHvObj->fd, 9600, 8, 0);
            }
            else
            {
                pHvObj->fd = socket(AF_INET, SOCK_DGRAM, 0);
                if (pHvObj->fd < 0)
                {
                    exit(0);
                    printf("open %s error \r\n", pHvObj->name);
                    continue;
                }
            }
        }
        int fd = pHvObj->fd;
		if( fd <= 0 )
		{
			usleep( 10000 );
			continue;
		}        
		fd_set rfd_set;
        struct timeval tv, *ptv;
        int nsel;
        FD_ZERO(&rfd_set);
        FD_SET(fd, &rfd_set);
        tv.tv_sec = 0;
        tv.tv_usec = 500 * 1000;
        ptv = &tv;
        nsel = select(fd + 1, &rfd_set, NULL, NULL, ptv);
        if (nsel > 0 && FD_ISSET(fd, &rfd_set))
        {
            recv_len = led_read( pHvObj, (char *)recv_buf, sizeof(recv_buf));
            if( recv_len > 0 && recv_buf[0] == 0xf1 )
            {
                parse_qr_data( recv_buf, recv_len );   
            }else{
                printf("hex:%s \r\n", show_hex( recv_buf, recv_len ));
                if (recv_len < 4)
                {
                    printf("获取的数据长度异常！不做处理！\r\n");
                    continue;
                }
                if (0 == isCrcValid(recv_buf, recv_len))
                {
                    printf("收取的数据Crc非法, 请确定是否有其他程序占用此串口，或者费显版本不匹配！\r\n");
                    continue;
                }
                parse_input_data( recv_buf, recv_len );
            }
        }
        if (pHvObj->MsgList.count == 0)
            continue;
        pthread_mutex_lock(&pHvObj->data_lock);
        while( NULL != (msg = PtrList_remove_head(&pHvObj->MsgList) ))
		{
   		    if (msg->len > 0)
   	   		{
	            memcpy(buffer, msg->data, msg->len);
//				printf("write :%s \r\n", show_hex( buffer, msg->len ));
           		led_real_write(pHvObj, buffer, msg->len);
        	}
        	free(msg);
		}
        pthread_mutex_unlock(&pHvObj->data_lock);
    }
	return NULL;
}

static void new_led_msg(OBJECT_S *obj, char *buffer, int len)
{
    if (len <= 0)
        return;
    LedMsg *msg = malloc(sizeof(LedMsg));
    memset(msg, 0, sizeof(msg));
    len = len > 1024 ? 1024 : len;
    msg->len = len;
    memcpy(msg->data, buffer, len);
    led_append(obj, msg);
}

static u8 CalculateXOR(u8 xor, char *param, int len)
{
    for (; len--;)
        xor ^= *(param++);
    return xor;
}

static void u16bcd( u16 u16Value, char *bcd )
{
	u8 nibble;
	nibble = (u8)( u16Value / 100 );
	bcd[0] = ((nibble/10) << 4) + (nibble%10);
	u16Value %= 100;
	nibble = (u8)u16Value;
	bcd[1] = ((nibble/10) << 4) + (nibble%10);
}

int led_clear(HANDLE h)
{
    OBJECT_S *pobj = (OBJECT_S *)h;
    char dat[1024];
    char *ptr = dat;
    u8 xor ;
    int len = 0;
    *ptr++ = 0xa0;
    *ptr++ = 0xb0;
    *ptr++ = 0xc0;
    *ptr++ = 0xd0;
    *ptr++ = 0x04; // clear
    *ptr++ = 0;
    *ptr++ = 0;
    xor = CalculateXOR(0, dat, (int)(ptr - dat));
    *ptr++ = xor;
    len = ptr - dat;
    new_led_msg(pobj, dat, len);
	return len;
}

static int led_get_devinfo(HANDLE h)
{
    OBJECT_S *pobj = (OBJECT_S *)h;
    char dat[1024] = {0};
    char *ptr = dat;
    unsigned char xor ;
    int len = 0;
    *ptr++ = 0xa0;
    *ptr++ = 0xb0;
    *ptr++ = 0xc0;
    *ptr++ = 0xd0;
    *ptr++ = 0x02; // 获取info
    *ptr++ = 0;
    *ptr++ = 0;
    xor = CalculateXOR(0, dat, (int)(ptr - dat));
    *ptr++ = xor;
    len = ptr - dat;
    new_led_msg(pobj, dat, len);
	return len;
}

int led_send_line(HANDLE h, int nLineNo, int nColor, int nRollType, int nRolltime, const char *szText)
{
    OBJECT_S *pobj = (OBJECT_S *)h;
    if(!szText || *szText == '\0' ) return 0;
    char dat[1024];
    char *ptr = dat;
    unsigned char xor ;
    int len = 0;
    *ptr++ = 0xa0;
    *ptr++ = 0xb0;
    *ptr++ = 0xc0;
    *ptr++ = 0xd0;
    *ptr++ = 0x20; // 显示字符
    u16bcd((u16)(strlen(szText) + 1 + 3 + 1), ptr);
    ptr += 2;
    *ptr++ = nLineNo;
    *ptr++ = (nRollType & 0x0f) | ((nRolltime & 0x03) << 6); //format_roll&0x0f + (blink_time&0x03)<<4 + (roll_time&0x03)<<6;
    *ptr++ = (unsigned char)(nColor & 0x000000ff);
    *ptr++ = (unsigned char)((nColor & 0x0000ff00) >> 8);
    *ptr++ = (unsigned char)((nColor & 0x00ff0000) >> 16);
    strcpy((char *)ptr, (char *)szText);
    ptr += strlen(szText);
    xor = CalculateXOR(0, dat, (int)(ptr - dat));
    *ptr++ = xor;
    len = ptr - dat;
//  trace_log("led line%d-> [%s] \r\n",nLineNo, szText);
    new_led_msg(pobj, dat, len);
	return len;
}

#define MODE_ROW 1
#define MODE_PIC 2
#define MODE_MIX 3
#define MODE_PAGE 4
static void led_set_mode(HANDLE h, int mode, u8 *pRowType )
{
    OBJECT_S *pobj = (OBJECT_S *)h;
    char dat[1024];
    char *ptr = dat;
    unsigned char xor ;
    int len = 0;
    *ptr++ = 0xa0;
    *ptr++ = 0xb0;
    *ptr++ = 0xc0;
    *ptr++ = 0xd0;
    *ptr++ = 0x05;
    *ptr++ = 0;
    *ptr++ = 9;
    *ptr++ = mode;
    memcpy((char *)ptr, (char *)pRowType, 8);
    ptr += 8;
    xor = CalculateXOR(0, dat, (int)(ptr - dat));
    *ptr++ = xor;
    len = ptr - dat;
    new_led_msg(pobj, dat, len);
}

static void led_set_alarm(HANDLE h, unsigned char bAlarm, unsigned char nTime)
{
    OBJECT_S *pobj = (OBJECT_S *)h;
    char dat[1024];
    char *ptr = dat;
    unsigned char xor ;
    int len = 0;
    *ptr++ = 0xa0;
    *ptr++ = 0xb0;
    *ptr++ = 0xc0;
    *ptr++ = 0xd0;
    *ptr++ = 0x03; // set alarm
    *ptr++ = 0;
    *ptr++ = 2;
    *ptr++ = bAlarm;
    *ptr++ = nTime % 4;
    xor = CalculateXOR(0, dat, (int)(ptr - dat));
    *ptr++ = xor;
    len = ptr - dat;
    new_led_msg(pobj, dat, len);
}

int led_send_kd_voice( const char *stVoice, char *buf )
{
	// 科大讯飞发送的语音讯息
	char dat[1024] = {0};
	int len = strlen( stVoice ) + 2;
	dat[0] = 0xfd;
	dat[1] = (len >> 8 ) & 0xff;
	dat[2] = len & 0xff;
	dat[3] = 0x01;
	dat[4] = 0x00;
	strcpy( dat + 5, stVoice );
	memcpy( buf, dat, strlen( stVoice ) + 7 );
	return strlen( stVoice ) + 7;
}

void led_play_du( HANDLE h , int vol )
{
    OBJECT_S *pobj = (OBJECT_S *)h;
    unsigned char dat[40] = {"[v3][x1]sound123"};
	dat[2] = vol + '0';
	dat[2] = (dat[2] < '0' || dat[2] > '8') ? '3' : dat[2];
	char buffer[129] = {0};
	buffer[0] = '<';
	buffer[1] = 'A';
	strcpy( buffer + 2, dat );
	int len = strlen( buffer );
	buffer[len] = '>';
    new_led_msg(pobj, buffer, len + 1);
}

void led_send_voice(HANDLE h, const char *stVoice, int voul )
{
    OBJECT_S *pobj = (OBJECT_S *)h;
    if (!stVoice || *stVoice == '\0')
        return;
    char dat[512] = {0x3c, 'A', '[', 'v', '8', ']', '[', 's', '5', ']', '[', 't', '5', ']', '[', 'm', '3', ']'};
    dat[4] = ( voul < 0 || voul > 8 ) ? '8' : '0' + voul; 
    char *ptr = dat + 18;
    int len;
    if (strlen(stVoice) > sizeof(dat) - 24)
    {
        return;
    }
    strcpy((char *)ptr, (char *)stVoice);
    ptr += strlen(stVoice);
    *ptr++ = 0x3e;
    len = ptr - dat;
    new_led_msg(pobj, dat, len);
}

void led_set_picture( HANDLE h, unsigned int nColor, int x, int bOpen)
{
	OBJECT_S *pobj = (OBJECT_S *)h;
	char tvb[15];
    char dat[1024];
    char *ptr = dat;
	struct timeval tv;
	gettimeofday( &tv, NULL );
	strftime( tvb, sizeof(tvb), "%y%m%d%H%M%S", localtime( &tv.tv_sec ) );
	tvb[14] = 0;
    unsigned char xor ;
    int len = 0;
    *ptr++ = 0xa0;
    *ptr++ = 0xb0;
    *ptr++ = 0xc0;
    *ptr++ = 0xd0;
    *ptr++ = 0x21;
	*ptr++ = 0;
	*ptr++ = 0x5;
	*ptr++ = bOpen & 0xff;
	*ptr++ = x & 0xff;
	*ptr++ = (unsigned char)(nColor & 0x0000ff);
	*ptr++ = (unsigned char)((nColor & 0x00ff00) >> 8);
	*ptr++ = (unsigned char)((nColor & 0xff0000)>>16);
    xor = CalculateXOR(0, dat, (int)(ptr - dat));
    *ptr++ = xor;
    len = ptr - dat;
    new_led_msg(pobj, dat, len);
}


void led_sync_time( HANDLE h)
{
	OBJECT_S *pobj = (OBJECT_S *)h;
	char tvb[15];
    char dat[1024];
    char *ptr = dat;
	struct timeval tv;
	gettimeofday( &tv, NULL );
	strftime( tvb, sizeof(tvb), "%y%m%d%H%M%S", localtime( &tv.tv_sec ) );
	tvb[14] = 0;
    unsigned char xor ;
    int len = 0;
    *ptr++ = 0xa0;
    *ptr++ = 0xb0;
    *ptr++ = 0xc0;
    *ptr++ = 0xd0;
    *ptr++ = 0x01;
	*ptr++ = 0;
	*ptr++ = 0x14;
	memcpy( ptr, tvb, 14 );
	ptr += 14;
    xor = CalculateXOR(0, dat, (int)(ptr - dat));
    *ptr++ = xor;
    len = ptr - dat;
    new_led_msg(pobj, dat, len);
}

void led_show_time( HANDLE h, int row, int col, int type, unsigned int nColor )
{
    OBJECT_S *pobj = (OBJECT_S *)h;
    char dat[1024];
    char *ptr = dat;
    unsigned char xor ;
    int len = 0;
    *ptr++ = 0xa0;
    *ptr++ = 0xb0;
    *ptr++ = 0xc0;
    *ptr++ = 0xd0;
    *ptr++ = 0x06;
	*ptr++ = 0;
	*ptr++ = 6;
    *ptr++ = row & 0xff;
    *ptr++ = col & 0xff;
	type = type == 3 ? 3 : 6;
	*ptr++ = type & 0xff;
    *ptr++ = (unsigned char)(nColor & 0x000000ff);
    *ptr++ = (unsigned char)((nColor & 0x0000ff00) >> 8);
    *ptr++ = (unsigned char)((nColor & 0x00ff0000) >> 16);
    xor = CalculateXOR(0, dat, (int)(ptr - dat));
    *ptr++ = xor;
    len = ptr - dat;
    new_led_msg(pobj, dat, len);
}

void led_exit( HANDLE h )
{
	 OBJECT_S *pobj = (OBJECT_S *)h;
	 pobj->run = 0;
	 pthread_cancel( pobj->thread );
	 pthread_join( pobj->thread, NULL );
}

#if 0
// 显示时间的请求没啥用，没实现
int main(int argc, char *argv[])
{
	HANDLE h = led_init( argv[1] );
	int index = 0;
	char text[129];
	while( 1 )
	{
		led_sync_time( h );
		led_show_time( h, 1, 0, 3, 0xffff00 );
//		sprintf( text, "Hello:%d", index++ );
//		led_send_line( h, 1, 0xffff00ff, 5, 2, text );
		led_send_line( h, 2, 0xffff00ff, 5, 2, "3你好世界!" );
		sleep( 1 );
//		usleep( atoi(argv[1])*10000 );
	}
	led_exit( h );
	return 0;
}
#endif
