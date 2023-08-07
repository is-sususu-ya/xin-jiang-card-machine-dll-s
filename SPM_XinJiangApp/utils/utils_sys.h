 
#ifndef _UTILS_SYS_INCLUDED_
#define _UTILS_SYS_INCLUDED_

// all operation system close related utility functions shall be put in this file

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef linux


// 使用clock_gettime 获取系统开机到当前的msec (get_sys_tick/get_sys_tick64)和sec数(get_sys_sec)
long get_sys_tick();
signed long long  get_sys_tick64();
long get_sys_sec();

// itimer operation
void init_itimer();
long get_itimer();
void waitfor( int msec );

#else		// windows

// TODO

#endif

#ifdef __cplusplus
};
#endif

#endif
