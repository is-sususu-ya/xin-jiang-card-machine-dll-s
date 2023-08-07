#ifndef RWPAY_CONFIG_HEADER_H
#define RWPAY_CONFIG_HEADER_H


#if defined(CHIP_3516C_V200)
#define CHIP_3516C_V200
#elif defined(CHIP_3520_CLOUD) 
#define CHIP_3520_CLOUD
#elif defined(CHIP_3520)
#define CHIP_3520
#endif

#define  APP_VERSION    	"V1.0.1"
#define DEV_TYPE        	"RWPAY"
#define DEV_NAME        	"苏州德亚/朗为"
#define CORE_CONFIG_FILE    "rwpay.conf"
// #define  MODE_TEST		// 是否启用测试模式，用于生成测试程序



#endif
