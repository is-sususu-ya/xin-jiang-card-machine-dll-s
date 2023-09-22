/*
 * @Description:
 * @version:
 * @Author: YangHui
 * @Contact: yh.86@outlook.com
 * @Date: 2023-08-18 15:55:50
 * @LastEditors: YangHui
 * @LastEditTime: 2023-08-31 18:42:16
 * @FilePath: /08_Prj520_Special_XinJiang/SPM_XinJiangApp/corelib/ts_robot.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

// #include "wintype.h"
#include "utils_tty.h"
#include "utils_mtrace.h"
#include "gpio.h"
#include <pthread.h>
#include <fcntl.h>
#include "utils_thread.h"

#include "ts_robot.h"
#include "typedef.h"
#include "lc_config.h"

// 串口协议机械手
/*======================================================================================================*/
extern void ltrace(const char *fmt, ...);
extern SystemConfig g_apconfig;
volatile int robot_recv_timeout_flag1 = 0;

ROBOTARM_S g_Robot_Handle = {0};
static pthread_t robot_thread = 0;

const char *getCOMString(int port)
{
	switch (port)
	{
	case 1:
		return "/dev/ttyAMA1";
		break;
	case 2:
		return "/dev/ttyAMA2";
		break;
	case 3:
		return "/dev/ttyAMA3";
		break;
	case 4:
		return "/dev/ttyUSB0";
		break;
	case 5:
		return "/dev/ttyUSB1";
		break;
	case 6:
		return "/dev/ttyUSB2";
		break;
	case 7:
		return "/dev/ttyUSB3";
		break;
	default:
		return "/dev/ttyAMA2";
		break;
	}
}

static int robot_trace_log(const char *fmt, ...)
{
	int rc;
	va_list va;
	ROBOTARM_S *p_Handle = &g_Robot_Handle;
	va_start(va, fmt);
	rc = mtrace_vlog(p_Handle->hLog, fmt, va);
	va_end(va);
	return rc;
}

static int isCrcValid(char *data, int len)
{
	uint8_t tmp = 0x00;
	int i = 0;
	for (i = 0; i < (len - 1); i++)
		tmp ^= data[i];
	return data[len - 1] == tmp ? 1 : 0;
}

static char *show_hex(const char *ch, int rlen)
{
	int i = 0;
	char *ptr = (char *)ch;
	static char buf[1024];
	char *off = buf;
	unsigned char val;
	int len = rlen > 300 ? 300 : rlen;
	memset(buf, 0, sizeof(buf));
	for (i = 0; i < len; i++)
	{
		val = *ptr++;
		sprintf(off, "%02x ", val);
		off += 3;
	}
	*off = '\0';
	return buf;
}

static uint8_t CalculateXOR(uint8_t xor, char *param, int len)
{
	for (; len--;)
		xor ^= *(param++);
	return xor;
}

int robot_receive(int fd, uint8_t *send)
{
	ROBOTARM_S *p_Handle = &g_Robot_Handle;

	int recv_len = 0;
	int ret;
	uint8_t recv[64] = {0};
	// uint8_t *ptr = recv;
	while (0x33 != tty_getc(fd, 100, 0))
	{
		robot_recv_timeout_flag1++;
		if (robot_recv_timeout_flag1 > 20)
		{
			robot_trace_log("等待机械手-回复-超时\r\n");
			robot_recv_timeout_flag1 = 0;
		}
		return 0;
	}
	recv[0] = 0x33;
	recv_len = tty_read_until(fd, (char *)&recv[1], sizeof(recv), 0x0D);
	if (recv_len <= 0)
	{
		return -1;
	}
	recv_len++;
	// if ((recv[0] != send[0]) || (recv[1] != send[1]))
	if ((recv[0] != 0x33) || (recv[1] != 0x31))
	{
		printf("响应错误\r\n");
		robot_trace_log("receive :%s\r\n", show_hex((const char *)recv, recv_len));
		return -1;
	}
	switch (recv[2])
	{
	case 0xCF:
		p_Handle->RSW1_Last = p_Handle->RSW1_This;
		p_Handle->RSW1_This = recv[5];
		// TODO
		// robot_trace_log("返回机械手状态\r\n");
		break;
	case 0xBF:
		ret = recv[3] == 0x01 ? 1 : 0;
		robot_trace_log("机械手缩回【%s】\r\n", ret ? "成功" : "失败");
		break;
	case 0xBE:
		ret = recv[3] == 0x01 ? 1 : 0;
		robot_trace_log("机械手伸出【%s】\r\n", ret ? "成功" : "失败");
		break;
	default:
		break;
	}

	return ret;
}

/**
 * @description: 0x33<CH><CMD><LEN><PARAM><XOR>0x0D
 * @param {HANDLE} h
 * @param {char} cmd
 * @param {int} len
 * @return {*}
 */
int robot_cmd(uint8_t cmd)
{
	ROBOTARM_S *p_Handle = &g_Robot_Handle;
	int ret = 0;
	if (p_Handle == NULL)
	{
		robot_trace_log("rebot_cmd_send: p_Handle == NULL\r\n");
		return 0;
	}
	uint8_t send[64] = {0};
	uint8_t xor = 0;
	// uint8_t cmdc = 0;
	uint8_t *ptr = send;
	*ptr++ = 0x33;
	*ptr++ = 0x31;
	*ptr++ = cmd;
	if (cmd == 0x40 || cmd == 0x41)
	{
		*ptr++ = 0x01;
		*ptr++ = 0x00;
	}
	else
		*ptr++ = 0x00;
	xor = CalculateXOR(0, (char *)send + 1, (int)(ptr - send - 1));
	*ptr++ = xor;
	*ptr++ = 0x0D;
	if (cmd != ARM_STATUS)
	{
		robot_trace_log("发送命令->机械手控制器:%s\r\n", show_hex((const char *)send, (int)(ptr - send)));
	}

	pthread_mutex_lock(&(p_Handle->fd_lock));

	tty_flush_in(p_Handle->fd);

	ret = tty_write(p_Handle->fd, send, (int)(ptr - send));
	// robot_trace_log("tty_write return %d\r\n", ret);

	usleep(10 * 1000);
	ret = robot_receive(p_Handle->fd, send);

	pthread_mutex_unlock(&(p_Handle->fd_lock));

	return ret;
}

void Robot_Status_Parsing(void)
{
	if (_Robot_isJustReady())
	{
		robot_trace_log("机械手-就绪\r\n");
	}
	// if (_Robot_isJustNotReady())
	// {
	// 	robot_trace_log("机械手-未就绪\r\n");
	// }

	if (_Robot_isJustFail())
	{
		robot_trace_log("机械手-故障\r\n");
	}
	// if (_Robot_isJustClearFail())
	// {
	// 	robot_trace_log("机械手-故障清除\r\n");
	// }

	if (_Robot_isJustBusy())
	{
		robot_trace_log("机械手-忙碌\r\n");
	}
	// if (_Robot_isJustFree())
	// {
	// 	robot_trace_log("机械手-空闲\r\n");
	// }

	if (_Robot_isJustRest())
	{
		robot_trace_log("机械手-复位\r\n");
	}
	// if (_Robot_isJustSet())
	// {
	// 	robot_trace_log("机械手-未复位\r\n");
	// }

	return;
}

int robot_open(const char *dev, int baudate)
{
	ROBOTARM_S *p_Handle = &g_Robot_Handle;

	memset(p_Handle, 0, sizeof(ROBOTARM_S));
	strcpy(p_Handle->dev, dev);
	p_Handle->fd = open(p_Handle->dev, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
	if (p_Handle->fd < 0)
	{
		ltrace("打开串口[%s]失败！\r\n", p_Handle->dev);
		free(p_Handle);
		return -1;
	}
	ltrace("打开机械手串口[%s]成功！\r\n", p_Handle->dev);
	// tty_raw(p_Handle->fd, NULL, baudate);
	tty_raw(p_Handle->fd, baudate, 8, 0);

#ifdef ENABLE_LOG
	p_Handle->hLog = mlog_init(LOG_PATH, "robot");
	mlog_setlimitcnt(p_Handle->hLog, LOG_LIMIT, LOG_ROLL_CNT);
#else
	p_Handle->hLog = NULL;
#endif
	return 0;
}

int robot_close(void)
{
	ROBOTARM_S *p_Handle = &g_Robot_Handle;

	if (p_Handle == NULL)
	{
		robot_trace_log("robot_close: p_Handle == NULL\r\n");
		return 0;
	}
	if (p_Handle->fd > 0)
	{
		close(p_Handle->fd);
		p_Handle->fd = -1;
	}
	free(p_Handle);
	return 0;
}

void workthread_robot(void)
{
	ROBOTARM_S *p_Handle = &g_Robot_Handle;
	uint8_t Buff[128];

	// if (robot_open(getCOMString(2), 9600) < 0)
	// {
	// 	ltrace("robot_open fail!\n");
	// }
	// pthread_mutex_init(&(p_Handle->fd_lock), NULL);
	// ltrace("启动机械手模块：%s\n", getCOMString(2));

	while (p_Handle->run)
	// while (!bQuit)
	{
		robot_cmd(ARM_STATUS);
		Robot_Status_Parsing();
		// robot_receive(p_Handle->fd, Buff);
		usleep(50 * 1000);
	}

	robot_close();
	ltrace("robot_close()\n");

	return;
}

void robot_init(void)
{
	ROBOTARM_S *p_Handle = &g_Robot_Handle;

	if (robot_open(g_apconfig.robot_hand, 9600) < 0)
	{
		ltrace("robot_open fail!\n");
	}
	pthread_mutex_init(&(p_Handle->fd_lock), NULL);
	ltrace("启动机械手模块：%s\n", g_apconfig.robot_hand);

	p_Handle->run = 1;
	ltrace("g_Robot_Handle.run = %d\n", p_Handle->run);

	pthread_create(&robot_thread, NULL, workthread_robot, NULL);
	if (robot_thread)
	{
		robot_trace_log("robot thread start done [%d]\r\n", robot_thread);
	}
	return;
}

void robot_thread_stop(void)
{
	ROBOTARM_S *p_Handle = &g_Robot_Handle;

	p_Handle->run = 0;
	ltrace("g_Robot_Handle.run = %d\n", p_Handle->run);
	return;
}
