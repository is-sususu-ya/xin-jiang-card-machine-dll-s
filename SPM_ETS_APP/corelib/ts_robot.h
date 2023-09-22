/*
 * @Description:
 * @version:
 * @Author: YangHui
 * @Contact: yh.86@outlook.com
 * @Date: 2023-08-18 16:27:23
 * @LastEditors: YangHui
 * @LastEditTime: 2023-08-31 16:08:02
 * @FilePath: /08_Prj520_Special_XinJiang/SPM_XinJiangApp/corelib/ts_robot.h
 */

#ifndef __TS_ROBOT_H__
#define __TS_ROBOT_H__

#include <stdint.h>
// #include "wintype.h"
#include "utils_tty.h"
#include "utils_mtrace.h"
#include "gpio.h"
#include "typedef.h"

#include <pthread.h>
#include <fcntl.h>
// 串口协议机械手

#define _EN_PushPull_Module (1)

#ifndef bool
typedef int bool;
#endif

// #ifndef *HANDLE
// typedef void *HANDLE;
// #endif

/*======================================================================================================*/
#ifndef ROBOTARM_S
typedef struct tagRobetArm
{
	bool run;
	int fd;
	unsigned char TxLast[64];
	int TxLen;
	unsigned char RxLast[64];
	int RxLen;
	char dev[32];
	int BaudRate;
	pthread_mutex_t fd_lock;
	HANDLE hLog;
	uint8_t RSW1_Last;
	uint8_t RSW1_This;
} ROBOTARM_S;
#endif

extern ROBOTARM_S g_Robot_Handle;

#define ENABLE_LOG
// #define lock_fd() pthread_mutex_lock(&(g_Robot_Handle.fd_lock))
// #define unlock_fd() pthread_mutex_unlock(&(g_Robot_Handle.fd_lock))
#define LOG_PATH "./log"
#define LOG_LIMIT 200
#define LOG_ROLL_CNT 10

#define ARM_PUSH 0x41
#define ARM_PULL 0x40
#define ARM_STATUS 0x30

#define _Robot_isJustReady() (!(g_Robot_Handle.RSW1_Last & (0x01 << 0)) && (g_Robot_Handle.RSW1_This & (0x01 << 0)))
#define _Robot_isJustNotReady() ((g_Robot_Handle.RSW1_Last & (0x01 << 0)) && !(g_Robot_Handle.RSW1_This & (0x01 << 0)))
#define _Robot_isJustFail() (!(g_Robot_Handle.RSW1_Last & (0x01 << 1)) && (g_Robot_Handle.RSW1_This & (0x01 << 1)))
#define _Robot_isJustClearFail() ((g_Robot_Handle.RSW1_Last & (0x01 << 1)) && !(g_Robot_Handle.RSW1_This & (0x01 << 1)))
#define _Robot_isJustBusy() (!(g_Robot_Handle.RSW1_Last & (0x01 << 2)) && (g_Robot_Handle.RSW1_This & (0x01 << 2)))
#define _Robot_isJustFree() ((g_Robot_Handle.RSW1_Last & (0x01 << 2)) && !(g_Robot_Handle.RSW1_This & (0x01 << 2)))
#define _Robot_isJustRest() (!(g_Robot_Handle.RSW1_Last & (0x01 << 4)) && (g_Robot_Handle.RSW1_This & (0x01 << 4)))
#define _Robot_isJustSet() ((g_Robot_Handle.RSW1_Last & (0x01 << 4)) && !(g_Robot_Handle.RSW1_This & (0x01 << 4)))

// const char *getCOMString(int port);
// static int robot_trace_log(const char *fmt, ...);
// static int isCrcValid(char *data, int len);
// static char *show_hex(const char *ch, int rlen);
// static uint8_t CalculateXOR(uint8_t xor, char *param, int len);
extern int robot_receive(int fd, uint8_t *send);
extern int robot_cmd(uint8_t cmd);
extern int robot_open(const char *dev, int baudate);
extern int robot_close(void);

extern void robot_init(void);
extern void robot_thread_stop(void);

#endif /* __TS_ROBOT_H__ */
