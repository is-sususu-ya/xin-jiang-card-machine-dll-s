/*
 * =====================================================================================
 *
 *       Filename:  led.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2018-09-08 02:43:41 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:   (), 
 *        Company:  
 *
 * =====================================================================================
 */


#ifndef  LED_H
#define  LED_H

#ifndef _TYPEDEF_HANDLE_
#define _TYPEDEF_HANDLE_
typedef void *HANDLE;
#endif


#define EVT_QR_READY 0x01

typedef void (*LED_CallBack)(int code, void *arg);

void led_exit( HANDLE h );
void led_send_voice(HANDLE h, const char *stVoice, int voul );
int led_send_line(HANDLE h, int nLineNo, int nColor, int nRollType, int nRolltime, const char *szText);
int led_clear(HANDLE h);
void led_play_du( HANDLE h, int vol );
HANDLE led_init(const char *dev);
int led_set_callback( HANDLE h, LED_CallBack );

#endif  // LED_H
