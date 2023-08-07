
#ifndef SPM_CONFIG_H
#define SPM_CONFIG_H

 
// 二维码上报方式， 仅有一项可选
// 使用TFI协议格式 0x40+二维码
// #define USE_TFI_PROTOCOL
// #define USE_TFI_EXP1_PROTOCOL
// 二维码使用 F1 开头 FF结尾格式
// #define USE_FF_PROTOCOL
// 以上都不选中的话，使用SPM内置协议
//  0x40+二维码编号+二维码
  
// 使能仅串口上报二维码信息，山东需求，二维码上报使用串口，网口使用SPM协议接入
// #define ENABLE_TTY_ONLY_QRCODE
  
#if defined(TMPE32) 
#define ENBALE_TFI_LED              // 使能LED功能
#define DEV_LED_MASTER  "/dev/ttyAMA3"  
#define ENABLE_SLAVE_LED            // 使能LED从机
#define DEV_LED_SLAVE   "/dev/ttyAMA2" 
#define DEV_PORT_PC     "/dev/ttyAMA1"
#define CHIP_3520
#elif defined(TMPE42)
#define ENBALE_TFI_LED              // 使能LED功能
#define ENABLE_SCAL_NET             // 使能网络扫码功能
#define DEV_LED_MASTER	"/dev/ttyAMA0"
#define DEV_PORT_PC     "/dev/ttyAMA1"
#define CHIP_3516C_V200
#elif defined(TMP55)    // 带费显控制板
#define ENBALE_TFI_LED              // 使能LED功能
#define DEV_LED_MASTER  "/dev/ttyAMA3"  
#define ENABLE_SLAVE_LED            // 使能LED从机
#define DEV_LED_SLAVE   "/dev/ttyAMA2"
#define DEV_PORT_PC     "/dev/ttyAMA1"
#define CHIP_3520
#elif defined(TSP86) 
/** TSP86 **/ 
#define ENABLE_GPIO
#define DEV_SOUND_PORT  "/dev/ttyAMA2"
#define ENABLE_SCAN_TTY             // 使能网络扫码功能 
#define DEV_SCAN_UP     "/dev/ttyUSB0"
#define ENABLE_SCAN_SLAVE           // 使能2号扫码
#define DEV_SCAN_DWN    "/dev/ttyUSB1" 
#define DEV_PORT_PC     "/dev/ttyAMA3"
#define CHIP_3520_ALB
#elif defined(TMP3520TEST)        //  使用3520硬件测试
#define ENBALE_TFI_LED              // 使能LED功能
#define DEV_LED_MASTER  "/dev/ttyAMA3"   
#define DEV_PORT_PC     "/dev/ttyAMA2"
#else 
#define DEV_PORT_PC     "/dev/ttyAMA1"
#endif
 

#endif
