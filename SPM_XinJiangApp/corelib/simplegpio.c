#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/mman.h>
#include "spm_config.h"
#include "gpio.h"

// #define CHIP_3520_ALB
// #define CHIP_3516C_V200

#define 	HIGH 1
#define 	LOW 0

#define DO_ON	HIGH
#define DO_OFF	LOW

// [GPIO]
#define NORMAL_OPEN		0
#define NORMAL_CLOSE	1
#define DIR_OUT		1		// GPIO DIR register bit value 1 as output
#define DIR_IN		0
// GPIO interrupt control registers address offset (within a 64K bank)
#define GPIO_IS		0x404
#define GPIO_IBE	0x408
#define GPIO_IEV	0x40c
#define GPIO_IE		0x410
#define GPIO_RIS	0x414
#define GPIO_MIS	0x418
#define GPIO_IC		0x41c
 
// 3531 3520 都是用这个基址
#ifdef CHIP_3516C_V200
#pragma  message(" complier with 3516 3518E... ")
#define GPIO_BASE_ADDR	0x20140000	// this is 3516A GPIO0 base addr
// 3516要打开NORMAL_OPEN宏
#define EXTDI_MODE		NORMAL_CLOSE
#define EXTDO_MODE		NORMAL_CLOSE
#elif defined CHIP_3520_ATC 
#pragma  message(" complier with 3520 atc... ")
#define GPIO_BASE_ADDR	0x20150000	// this is 3520A GPIO0 base addr
#define EXTDI_MODE		NORMAL_CLOSE
#define EXTDO_MODE		NORMAL_OPEN
#elif defined CHIP_3520 
#pragma message(" complier with 520a-520c   ... ")
#define GPIO_BASE_ADDR	0x20150000	// this is 3520A GPIO0 base addr
#define EXTDI_MODE		NORMAL_CLOSE
#define EXTDO_MODE		NORMAL_OPEN
#else
#pragma message(" complier withand 520alb ... ")
#define GPIO_BASE_ADDR	0x20150000	// this is 3520A GPIO0 base addr
#define EXTDI_MODE		NORMAL_CLOSE
#define EXTDO_MODE		NORMAL_CLOSE
#endif


// #define ENABLE_THREADPOOL

#ifdef ENABLE_THREADPOOL
#include "utils_thread.h"
#define THREAD_CREATE(ptid,pattr,fxc,arg)		THREAD_create(ptid,pattr,fxc,arg,NULL)
#define THREAD_CANCEL(tid)						THREAD_cancel(tid)
#else		
#include <pthread.h>
#define	THREAD_CREATE(ptid,pattr,fxc,arg)		pthread_create(ptid,pattr,fxc,arg)
#define THREAD_CANCEL(tid)						pthread_kill(tid,SIGKILL)
#endif
#define THREAD_KILL(tid)						pthread_kill(tid,SIGALRM)

typedef unsigned char		HI_U8;

typedef struct tagMutilRegConf
{
	unsigned int addr;
	unsigned int value;
}MutilRegConf;


#define MAX_MAP 10
typedef struct {
	int Bank;
	HI_U8 *MappedAddr;
} MappedInfo;
static MappedInfo mapInfo[MAX_MAP];
static int nMapped = 0;

static int monitor_run=0;
static int mmap_gpio();
static void unmmap_gpio();
void alarm_handle(int signo) 
{
	printf("[ThreadId=%lu][pid=%d] thread receive a signal %d\r\n",pthread_self(),getpid(), signo );
}

static void waitfor( int msec )
{
	usleep( msec * 1000 );
}

// [GPIO] for alarm, lighting etc.	
static pthread_t g_gpioThreadId=0;
int  g_gpioPID=0;

static int	nExtDIVal_this = 0;
static int	nExtDIVal_last = 0;
static int	nExtDOVal = 0;
static int	nDebounce = 3;
static int	NUM_EXTDI = 0;
static int	NUM_EXTDO = 0;
HI_U8 *g_pAddrExtDI[20];
HI_U8 *g_pAddrExtDO[20];

#ifdef CHIP_3531
int		nExtDiBank[] = {1,2,2,2,1,1,1,1,1,1,1};
int		nExtDiPin[] =  {6,0,2,1,7,5,1,3,4,2,0};
int		nExtDoBank[] = 	{8,8,8,8,9,9,9,9};
int		nExtDoPin[] = 	{1,3,5,7,1,3,2,0};
#elif defined CHIP_3520
// 通用3520
int		nExtDiBank[] = 	{4,4,4,4,4,4};
int		nExtDiPin[] = 	{0,1,2,3,4,5};
int		nExtDoBank[] = 	{3,3,3,3};
int		nExtDoPin[] = 	{2,0,3,1};

MutilRegConf reg_table[] =
{
	{0x200F0068,0x00},
	{0x200F006C,0x00},
	{0x200F0070,0x00},
	{0x200F0074,0x00},
	{0x200F00B4,0x00},
	{0x200F00A8,0x00},	
	{0x200F0008,0x01},
	{0x200F0004,0x01}
};
#elif defined CHIP_3516C_V200
//3516cv200不带GPIO输入
int nExtDiBank[] = {6};
int nExtDiPin[] = {3};
#if 1
int nExtDoBank[] = {7,7};
int nExtDoPin[] = {2,3};
#else
int nExtDoBank[] = {7};
int nExtDoPin[] = {3};
#endif

MutilRegConf reg_table[] =
{
	//UART1
	{0x200F00BC,0x03},
	{0x200F00C0,0x03},
	{0x200F00C4,0x03},
	{0x200F00C8,0x03},
	//UART2
	{0x200F00CC,0x03},
	{0x200F00D0,0x03},
	//7_2
	{0x200F00E8,0x01},
	//7_3
	{0x200F00EC,0x01},
	// 配置6_3,UART2 为输入信号
	{0x200F00CC,0x00},
	// 配置为输入下拉
	{0x200F08F8,0x200}
	//	{0x200F08F8,0x00}
};

#elif defined CHIP_3520_ALB
// 3520摆闸
int		nExtDiBank[] = 	{6,6,6,6,0,0,4,4,4,4,4,4,4,8,3,3,3,3,3,3,3,3};
int		nExtDiPin[] = 	{0,1,2,3,7,4,1,2,3,4,5,6,7,6,7,6,5,4,3,2,1,0};
int		nExtDoBank[] = 	{0,0,0,0,0,0,1,1};
int		nExtDoPin[] = 	{5,6,0,2,3,1,3,4};

MutilRegConf reg_table[] =
{
	{0x200F0068,0x00},
	{0x200F006C,0x00},
	{0x200F0070,0x00},
	{0x200F0074,0x00},
	{0x200F00B4,0x00},
	{0x200F00A8,0x00},	
	{0x200F0008,0x01},
	{0x200F0004,0x01}
};

#elif defined CHIP_3520_CLOUD
int		nExtDiBank[] = 	{4,4,4,4,4,8};
int		nExtDiPin[] = 	{1,2,3,4,7,6};
int		nExtDoBank[] = 	{4,4};
int		nExtDoPin[] = 	{5,6};

MutilRegConf reg_table[] =
{
	{0x200F0008,0x01},// GPIO4复用
	{0x200F0004,0x01},// 8_6 复用
};
 
#elif defined CHIP_3520_ATC

#if 0
int		nExtDiBank[] = 	{0,0,5,5,6,6,6,6, 4,4,4,4,4,4,4,4};
int		nExtDiPin[] = 	{7,4,0,2,0,1,2,3, 4,5,6,7,0,1,2,3};
int		nExtDoBank[] = 	{3,3,3,3,3,3,3,3, 0,0,0,0,0,0,1,1,8};   // 8_7 运行灯
int		nExtDoPin[] = 	{0,1,2,3,4,5,6,7, 5,6,0,2,3,1,3,4,7};
#else
int		nExtDiBank[] = 	{6,6,6,6,0,0,5,5, 4,4,4,4,4,4,4,4};
int		nExtDiPin[] = 	{0,1,2,3,7,4,0,2, 0,1,2,3,4,5,6,7};
int		nExtDoBank[] = 	{0,0,0,0,0,0,1,1, 3,3,3,3,3,3,3,3};
int		nExtDoPin[] = 	{5,6,0,2,3,1,3,4, 0,1,2,3,4,5,6,7};
#endif

MutilRegConf reg_table[] =
{
	{0x200F0068,0x00},  // 6_0复用
	{0x200F006C,0x00},  // 6_1复用
	{0x200F0070,0x00},  // 6_2复用
	{0x200F0074,0x00},  // 6_3复用
	
	{0x200F00B4,0x00},  // 0_7复用
	{0x200F00A8,0x00},  // 0_4复用
 
	{0x200F004C,0x00},  // 5_0复用
	{0x200F0054,0x00},  // 5_2复用

	{0x200F0008,0x01},  // 4_0-7, 8_7复用 

	{0x200F0004,0x01},  // 3_0-7复用,IO

	{0x200F00AC,0x00},  // 0_5复用
	{0x200F00B0,0x00},  // 0_6复用

	{0x200F0098,0x00},  // 0_0 复用
	{0x200F00A0,0x00},  // 0_2 复用
	{0x200F00A4,0x00},  // 0_3 复用
	{0x200F009C,0x00},  // 0_1 复用
	{0x200F00C4,0x00},  // 1_3 复用
	{0x200F00C8,0x00},  // 1_4 复用 
};
#endif


void GPIO_register_callback( void(*notice)(int, int));

void *hHandleDI;
void (*user_di_callback)( int di_this, int di_last) = NULL;

void SetUserDiData(void *h)
{
	hHandleDI = h;
	printf("addr = %d ",h);
}
void (*di_int_handler[30])(int di_point) = {NULL};

void himm(unsigned int phy_addr, unsigned int ulnew )
{
#define page_size 0x1000
#define page_size_mask 0xfffff000
    void *pmem;
    unsigned int phy_addr_in_page;
    unsigned int page_diff;
    unsigned int size_in_page;
    unsigned int size = 128; 
    unsigned int ulold;
    unsigned int last_phy_addr = 0x00;
    int fd = 0; 
    void *addr=NULL;
    phy_addr_in_page = phy_addr & page_size_mask;
    page_diff = phy_addr - phy_addr_in_page;
    size_in_page =((size + page_diff - 1) & page_size_mask) + page_size;
	if ((fd = open ("/dev/mem", O_RDWR | O_SYNC) ) < 0) 
	{    
		printf("unable to open /dev/mem: %s\n", strerror(errno));
		return;
	}    
	
	/* addr align in page_size(4k) */
	addr = mmap ((void *)0, size_in_page, PROT_READ|PROT_WRITE, MAP_SHARED, fd,  phy_addr_in_page);
	if (addr == MAP_FAILED)
	{    
		printf("himm run failed!\n" );
		close( fd );
		return;
	}
	printf("set mutilreg:%#x = %#x \r\n", phy_addr, ulnew );
    pmem = (void *)(addr+page_diff);
    ulold = *(unsigned int *)pmem;
    *(unsigned int *)pmem = ulnew;
	if( addr != NULL )
		munmap(addr, size_in_page);
//  printf("pyaddr:%08x :0x%08x --> 0x%08x \n", phy_addr, ulold, ulnew);
}

static init_mutils_table()
{
	int i = 0;
	for( i = 0; i < sizeof(reg_table)/sizeof(MutilRegConf); i++ )
	{
		himm( reg_table[i].addr, reg_table[i].value );
	}
}

static void add_map_info(int bank, HI_U8 *addr)
{
	if ( addr )
	{
		int i;
		for(i=0; i<nMapped; i++ )
		if (  mapInfo[i].Bank == bank ) break;
		if ( i==nMapped && nMapped<MAX_MAP )
		{
			mapInfo[i].Bank = bank;
			mapInfo[i].MappedAddr = addr;
			nMapped ++;
		}
	}
}

static HI_U8 *find_map_addr( int bank )
{
	int i;
	for(i=0; i<nMapped; i++ )
	if (  mapInfo[i].Bank == bank )
	return mapInfo[i].MappedAddr;
	return NULL;
}

static char *time_stamp()
{
	static char timestr[30];
	char *p = timestr;
	struct timeval tv;
	gettimeofday( &tv, NULL );
	strftime( p, sizeof(timestr), "%F %H:%M:%S", localtime( &tv.tv_sec ) );
	return timestr;
}

static HI_U8 *do_mapping(int fd, int bank, off_t offset )
{
	void *maddr;
	if (offset==0)
	offset = GPIO_BASE_ADDR + (bank << 16);
	maddr = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, offset);
	if ( maddr==MAP_FAILED) 
	{
		fprintf(stderr, "GPIO mmap bank %d failed: %s\r\n", bank, strerror(errno));
		maddr = 0;
	}
	else
	printf("Map GPIO bank %d (phy-addr=0x%#x) to addr. %p\r\n", bank, (unsigned int)offset, maddr );
	return maddr;
}

#define MAP_GPIO_BANK( fdmem, bank_vaddr, bank, bank_paddr) \
	if ( (bank_vaddr=find_map_addr(bank))==NULL ) \
	{ \
		bank_vaddr = do_mapping(fdmem,bank,bank_paddr); \
		add_map_info(bank, bank_vaddr); \
	}

#define GPIO_SET_DIR(pbank, pin, val) \
	if ( val ) \
		*( pbank + 0x400) |= (1 << pin); \
	else \
		*(pbank + 0x400) &= ~(1 << pin)

#define GPIO_READ_VALUE(v,pbank,pin,dimode)	\
	do { \
		int pin_on_val = ( (1-dimode) << pin ); \
		v = *( pbank + (1<<( pin +2)))== pin_on_val ? 1 : 0; \
	} while (0)

#define GPIO_WRITE_VALUE(v,pbank,pin,domode)	\
	do { \
		if ( v ) \
		*( pbank + (1<<( pin +2))) = ( (1-domode) << pin ); \
		else \
		*( pbank + (1<<( pin +2))) = domode << pin; \
	} while (0)

static void *Thread_gpio(void *augument);
static int mmap_gpio();
static void unmmap_gpio();

int gpio_getpid()
{
	return (int)g_gpioPID;
}

void gpio_init_intr(int di_point)
{
	if ( g_pAddrExtDI[di_point]==NULL ) return;
	*(g_pAddrExtDI[di_point]+GPIO_IS) = 0;			// edge trigger for all pin
#if EXTDI_MODE==NORMAL_OPEN
	*(g_pAddrExtDI[di_point]+GPIO_IEV) = 0xff;	// rising edge
#else
	*(g_pAddrExtDI[di_point]+GPIO_IEV) = 0;			// falling edge (our control is pull-low)
#endif	
	*(g_pAddrExtDI[di_point]+GPIO_IBE) = 0;			// single edge trigger
	*(g_pAddrExtDI[di_point]+GPIO_IC) = 0xff;		// clear interrupt for every GPIO pin in this bank
}

void gpio_enable_intr(int di_point, int bEnable)
{
	printf("GPIO interrupt for Ext DI%d is %s\n", di_point, bEnable ? "ENABLED" : "DISABLED" );
	*(g_pAddrExtDI[di_point]+GPIO_IE) = bEnable ? (1<<nExtDiPin[di_point]) : 0;
}

void gpio_register_int_handle(int di_point, void (*int_handler)(int))
{
	printf("reg int handle..\r\n");
	di_int_handler[di_point] = int_handler;
}

static void gpio_reset_intr(int di_point)
{
	*(g_pAddrExtDI[di_point]+GPIO_IC) = (1<<nExtDiPin[di_point]);		// clear interrupt for lighting control 
}

static int gpio_read_ris(int di_point)
{
	return (*(g_pAddrExtDI[di_point]+GPIO_RIS)) & (1<<nExtDiPin[di_point]);
}

void gpio_set_do( int nPoint, int val )
{
	if ( ( 0 <= nPoint ) && (nPoint < NUM_EXTDO) )
	{
		printf("out put nPoint=%d,val=%d %d \r\n", nPoint, val, EXTDO_MODE );
		GPIO_WRITE_VALUE(val, g_pAddrExtDO[nPoint], nExtDoPin[nPoint], EXTDO_MODE );  
		if ( val )
		nExtDOVal |= (1 << nPoint);
		else
		nExtDOVal &= ~(1 << nPoint);
	}
}

static int  gpio_read_di()
{
	return nExtDIVal_this;
}

static int	 gpio_read_do()
{
	return nExtDOVal;
}

#if 0
static void gpio_set_debounce(int count)
{
	nDebounce = count;
}

static void register_di_callback( void(*user_callback)(int, int) )
{
	user_di_callback = user_callback;
}
#endif

static int read_di()
{
	int i, bv, di_new = 0;
	for(i=0; i<NUM_EXTDI; i++)
	{
		GPIO_READ_VALUE(bv, g_pAddrExtDI[i], nExtDiPin[i], EXTDI_MODE);
		di_new |= (bv << i);
	}
	return di_new;
}

void *Thread_gpio(void *augument)
{
	int nHoldCount=0;
	g_gpioPID = getpid();
	printf("GPIO scan thread PID=%d\r\n", g_gpioPID );
	pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL );
	monitor_run = 1;
	for( ;monitor_run; )
	{
		int i, bv, di_new = 0;
		for(i=0; i<NUM_EXTDI; i++)
		{
			if ( di_int_handler[i] != NULL && gpio_read_ris(i) != 0 )
			{
				printf("[%s]【DI%d 中断】 中断产生, 调用注册的中断处理函数\n", time_stamp(), i);
				gpio_reset_intr(i);
			}
			GPIO_READ_VALUE(bv, g_pAddrExtDI[i], nExtDiPin[i], EXTDI_MODE);
			di_new |= (bv << i);
		}
		if ( nExtDIVal_this != di_new )
		{
			nHoldCount = 0;
			nExtDIVal_this = di_new;
		}
		else if ( nExtDIVal_this != nExtDIVal_last && ++nHoldCount >= nDebounce )
		{
			nExtDIVal_this = di_new;
			if ( user_di_callback )
			{
				printf("di change : di_last<0x%x>, di_this<0x%x>\r\n", nExtDIVal_last ,nExtDIVal_this);
				user_di_callback(nExtDIVal_last, nExtDIVal_this );
			}
			nExtDIVal_last = nExtDIVal_this;
		}
		usleep( 2000 );			// wait for 2 msec
		pthread_testcancel();
	}	// for
	g_gpioThreadId = 0;
	return NULL;
}

int GPIO_ReadDI()
{
	return gpio_read_di();
}

int GPIO_ReadDO()
{
	return gpio_read_do();
}

void GPIO_register_callback( void(*notice)(int, int))
{
	//di_notice = notice;
	user_di_callback = notice;
}

int GPIO_input( int pin)
{
	if ( 0<=pin && pin < NUM_EXTDI )
	{
		int bv;
		GPIO_READ_VALUE(bv,g_pAddrExtDI[pin],nExtDiPin[pin], EXTDI_MODE);
		//di_new |= (bv << i);
		return bv;
	}
	printf("GPIO_input pin不在范围内pin=%d, NUM_EXTDI=%d \n",pin,NUM_EXTDI);
	//ltrace("GPIO_input pin不在范围内pin=%d, NUM_EXTDI=%d \n",pin,NUM_EXTDI);
	return 0;
}

int GPIO_output( int pin, int val)
{
	if (val!=0)
		val=1;
	gpio_set_do( pin, val );
	return 0;
}

static int mmap_gpio()
{
	int i, fd;
	if ((fd = open ("/dev/mem", O_RDWR | O_SYNC) ) < 0) 
	{
		fprintf(stderr, "Unable to open /dev/mem: %s\n", strerror(errno));
		return -1;
	}
	NUM_EXTDI = sizeof( nExtDiBank )/sizeof( int );
	NUM_EXTDO = sizeof( nExtDoBank )/sizeof( int );
	// map external DI
	for(i=0; i<NUM_EXTDI; i++)
	{
		MAP_GPIO_BANK(fd, g_pAddrExtDI[i], nExtDiBank[i], 0);
		GPIO_SET_DIR(g_pAddrExtDI[i], nExtDiPin[i], DIR_IN );
	}
	// then external DO
	for(i=0; i<NUM_EXTDO; i++)
	{
		MAP_GPIO_BANK(fd, g_pAddrExtDO[i], nExtDoBank[i], 0);
		GPIO_SET_DIR(g_pAddrExtDO[i], nExtDoPin[i], DIR_OUT );
		// reset DO output
		gpio_set_do(i,0);
	}
	close( fd );
	return 0;
}

static void unmmap_gpio()
{
	int i;
	for(i=0; i<nMapped; i++ )
	{
		munmap( (void *)mapInfo[i].MappedAddr,getpagesize());
	}
	nMapped = 0;
	for(i=0; i<NUM_EXTDI; i++)
	g_pAddrExtDI[i] = 0;
	for(i=0; i<NUM_EXTDO; i++)
	g_pAddrExtDO[i] = 0;
}

static pthread_t thread_pulse;
static void signal_ignore(int code) {}

static unsigned long long GetTickCount()
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
	tmsec = (unsigned long long)(now_time - begin_time);
	return tmsec;
}

typedef struct tagOutPutAttr
{
	int pulse_count; 	// 几次脉冲
	int peroid;			// 脉冲持续时间
	int reset;			// is resting..
	int negative;		// 反向输出
	int stat;			// 当前状态
	unsigned long long next_change;
}PulseAttr;

PulseAttr pulse_attr[32] = {0};
static int gpio_pulse_init = 0;

static void *pulse_thread(void *arg)
{
	NUM_EXTDO = sizeof(nExtDoBank) / sizeof(int);
	sigset(SIGALRM, signal_ignore);
	gpio_pulse_init = 1;
	monitor_run = 1;
	int i = 0;
	int m = 0;
	int find;
	int min_sleep = 0;
	while (monitor_run)
	{
		min_sleep = 20;
		for( i = 0; i < NUM_EXTDO; i++ )
		{
			if( pulse_attr[i].pulse_count > 0 )
			{
				if( pulse_attr[i].next_change < GetTickCount() )
				{
					if( pulse_attr[i].reset )
					{
						pulse_attr[i].stat = 0;
						pulse_attr[i].reset = 0;
					}
					pulse_attr[i].stat = pulse_attr[i].stat == 0 ? 1 : 0;
					if( pulse_attr[i].negative )
						GPIO_output( i, pulse_attr[i].stat ? 0 : 1 );
					else
						GPIO_output( i, pulse_attr[i].stat );
					if( pulse_attr[i].stat == 0 )
					{
						pulse_attr[i].pulse_count--;
					}
					pulse_attr[i].next_change = GetTickCount() + pulse_attr[i].peroid;
					min_sleep = min_sleep > pulse_attr[i].peroid ? pulse_attr[i].peroid : min_sleep;
				}
			}
		}
		usleep( min_sleep*1000 );
	}
}

int GPIO_pulse(int pin, int count, int nhalfPeriod)
{
	if( gpio_pulse_init == 0 )
		return;
	if( pin < 0 || pin > 10 )
		return -1;
	pulse_attr[pin].next_change = GetTickCount();
	pulse_attr[pin].reset = 1;
	pulse_attr[pin].pulse_count = count > 0 ? count : 1;
	pulse_attr[pin].peroid = nhalfPeriod > 0 ? nhalfPeriod : 200;
	pthread_kill(thread_pulse, SIGALRM );
}
 
int GPIO_pulse_negative(int pin, int count, int nhalfPeriod)
{
	if( gpio_pulse_init == 0 )
		return;
	if( pin < 0 || pin > 10 )
		return -1;
	pulse_attr[pin].next_change = GetTickCount();
	pulse_attr[pin].reset = 1;
	pulse_attr[pin].pulse_count = count > 0 ? count : 1;
	pulse_attr[pin].peroid = nhalfPeriod > 0 ? nhalfPeriod : 200;
	pulse_attr[pin].negative = 1;
	pthread_kill(thread_pulse, SIGALRM );
}
  
int GPIO_initialize()
{
	init_mutils_table();
	memset(di_int_handler, 0, sizeof(di_int_handler));
	int i;
	mmap_gpio();
	for (i = 0; i < 3 && THREAD_CREATE(&g_gpioThreadId, 0, Thread_gpio, NULL) != 0; i++)
	{
		if (errno != EAGAIN)
		{
			printf("%s: create GPIO scan thread failed!\n", __FUNCTION__);
			return -1;
		}
		else
			waitfor(100);
	}
	pthread_create( &thread_pulse, NULL, pulse_thread, NULL );
	for (i = 0; i < NUM_EXTDO; i++)
	{
		GPIO_output(i, DO_OFF);
	}
	return g_gpioThreadId != 0 ? 0 : -1;
}

int GPIO_terminate()
{
	monitor_run = 0;
	pthread_cancel(g_gpioThreadId);
	pthread_join(g_gpioThreadId, NULL );

	pthread_cancel( thread_pulse );
	pthread_join( thread_pulse, NULL );

	unmmap_gpio();
	g_gpioPID = 0;
	g_gpioThreadId = 0;
	return 0;
}

#if 0
int main(int argc, char *argv[])
{
	GPIO_initialize();
	sleep( 1 );
	while( 1 )
	{
		printf("==>set 2\r\n");
		GPIO_pulse(0, 1, 500 );
		GPIO_pulse(1, 1, 500 );
		sleep( 2 );
	}
	core_start();
}
#endif
