#ifndef RW_DEVICE_HEADER
#define RW_DEVICE_HEADER

// 费显显示
#define IOC_LED_DISPLAY 1
//　语音播报
#define IOC_VOICE_PLAY  2

// 事件号-
// 二维码准备好
#define EVT_QR_READY    1
// 页面切换指令，param 表示切换到第几页，arg如果是切换到显示二维码界面，则arg表示二维码数据
// 其中param：
// 0: 首页，默认界面
// 1: 广告语，滚动界面
// 2: 显示等待支付界面,显示支付二维码
// 3: 显示扫码成功，等待支付完成界面
// 4：显示支付成功界面
// 5：支付失败界面
#define EVT_CHANGE_PAGE	        2 

typedef void (*RWDevCallBack)(int code, int param, void *arg);
extern RWDevCallBack DevCallBack;

int RWDevStart();
int RWDevRegisterCallBack( RWDevCallBack cb );
int RWDevStop();
int RWDevIoctl( int code, int param, void *arg );


#endif
