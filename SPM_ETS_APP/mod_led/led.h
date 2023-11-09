/*
 * @Author: link_zhu 417900229@qq.com
 * @Date: 2023-05-09 18:36:42
 * @LastEditors: link_zhu 417900229@qq.com
 * @LastEditTime: 2023-05-09 18:36:42
 * @FilePath: /spm_bsaocpay/mod_led/led.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
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
#define EVT_IOCHANGE 0x02


typedef void (*LED_CallBack)(int code, void *arg);

void led_exit( HANDLE h );
void led_send_voice(HANDLE h, const char *stVoice, int voul );
int led_send_line(HANDLE h, int nLineNo, int nColor, int nRollType, int nRolltime, const char *szText);
int led_clear(HANDLE h);
void led_play_du( HANDLE h, int vol );
HANDLE led_init(const char *dev);
int led_set_callback( HANDLE h, LED_CallBack );

#endif  // LED_H
