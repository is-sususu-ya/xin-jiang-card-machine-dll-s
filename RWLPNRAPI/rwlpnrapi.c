/*
 *  rwlpnrapi.c
 *	
 *	This source file implement the RunWell vehicle license plate number recognition (LPNR) camera
 *  API. 
 *
 *  This source file is dual plateform coded (linux and Windows). In Windows, using VC6 or VS 20xx can generate
 *  a DLL with each exported API in CALLTYPE (defined in rwlpnrapi.h, could be C or PASCAL) calling sequence.
 *  In Linux plateform, it's very easy to create a so file (shared object) for Lane software to bind into their application
 *  in runtime (of cause, to statically link into lane application is also applicable). In Linux, the API calling sequency
 *  is 'C'.
 *
 *  In Windows plateform, Event is forward to application via Windows PostMessage system call, while in LInux
 *  plateform, a callback function assigned by application is invoked to notice following event:
 *	  event number 1: LPNR equipment online
 *    event number 2: LPNR equipment offline
 *    event number 3: LPNR equipment start one image processing
 *    event number 4: LPNR processing done. data is available
 *
 *  Author: Thomas Chang
 *  Copy Right: RunWell Control Technology, Suzhou
 *  Date: 2014-10-29
 *  
 * NOTE 
 *  To build a .so file in linux, use command:
 *  $ gcc -o3 -Wall -shared -orwlpnrapi.so  rwlpnrapi.c
 *  To build a standalone executable tester for linux, include log, use command:
 *  $ gcc -o3 -Wall -DENABLE_LOG -DENABLE_TESTCODE -olpnrtest -lpthread -lrt rwlpnrapi.c 
 *  To enable the program log for so
 *  $ gcc -o3 -Wall -DENABLE_LOG -shared -rwlpnrapi.so -lpthread rwlpnrapi.c
 *
 *  To built a DLL for Windows plateform,  use Microsoft VC or Visual Studio, create a project for DLL and add rwlpnrapi.c, rwlpnrapi.h lpnrprotocol.h
 *  and rwlpnrapi.def (define the DLL export API entry name, otherwise, VC or VS will automatically add prefix/suffix for each exported entries).
 *  
 *
 *  车道软件使用此函数库说明：
 *
 *  1. 初始化：
 *	   	LPNR_Init 参数为LPNR设备的IP地址字串。返回0表示成功。-1 表示失败 (IP地址错误，或是该IP没有响应UDP询问)
 *		  
 *		 
 *	2. 设置回调函数(linux)，或是消息接收窗口(Windows)
 *		 LPNR_SetCallBack -- for linux and/or Windows
 *		 LPNR_SetWinMsg   -- for windows
 *			初始化成功后立刻设置。
 *
 *	3. 获取车牌号码：
 *		 LPNR_GetPlateNumber - 参数为调用函数储存车牌号码字串的指针，长度必须足够。车牌号码存放为
 *       <颜色><汉字><数字>，例如：蓝苏A123456. 如果没有辨识到车牌，返回字串"整牌拒识"。
 *
 *	4. 获取车牌彩色小图
 *		 LPNR_GetPlateColorImage - 参数为储存图片的buffer指针，图片内容为bmp格式。直接储存整个buffer到一个
 *       文件就是一个bmp图档。函数返回值是整个buffer的长度。应用程序需保证buffer长度足够，需要的长度为
 *			 车牌小图长度x宽度+54。因为车牌图片尺寸每次处理都不相同，必须提供一个最大可能的尺寸。
 * 
 *	4. 获取车牌二值化小图
 *		 LPNR_GetPlateBinaryImage - 参数为储存图片的buffer指针，图片内容为bmp格式。直接储存整个buffer到一个
 *       文件就是一个bmp图档。函数返回值是整个buffer的长度。应用程序需保证buffer长度足够，需要的长度为
 *			 车牌小图长度x宽度+54。因为车牌图片尺寸每次处理都不相同，必须提供一个最大可能的尺寸。
 *
 *	5. 获取抓拍整图 （进行车牌识别的摄像机输入图片）
 *		 LPNR_GetCapturedImage - 参数为储存图片的buffer指针，图片内容为jpg格式。直接储存整个buffer到一个
 *       文件就是一个jpeg图档。函数返回值是整个buffer的长度。应用程序需保证buffer长度足够，需要的长度概算为
 *       摄像机解像度 x factor. factor根据设置的JPEG压缩质量大约为0.1~0.5。
 *
 *  6. 获取当前实时图像帧 （开始分析后实时图像会暂停，直到当前图像分析完成才继续更新）
 *		 LPNR_GetLiveFrame - 参数和LPNR_GetCapturedImage相同，也是一个jpeg帧的内容。默认是使能发送实时图像。
 *		 可以调用LPNR_EnableLiveFrame禁能实时图像发送，减小网络负荷。
 *
 *  7. 询问状态的函数
 *		LPNR_IsOnline - 是否连线，返回布林值
 *		LPNR_IsIdle - 识别机是否进行识别处理中，返回布林值
 *     LPNR_GetPlateColorImageSize - 返回车牌彩色小图大小
 *     LPNR_GetPlateBinaryImageSize - 返回车牌二值化小图大小
 *     LPNR_GetCapturedImageSize - 返回抓拍大图大小（# of bytes)
 *     LPNR_GetHeadImageSize - 获取车头图大小（如有使能发送车头图）
 *     LPNR_GetQuadImageSize - 获取1/4图图片大小（如有使能发送抓拍小图）
 *	   LPNR_GetTiming - 获取识别机完成当前识别处理消耗的时间 （总过程时间和总处理时间）
 *
 *  8. 释放处理结果数据和图片(清除数据)
 *		 LPNR_ReleaseData
 *
 *  9. 软触发抓拍识别
 *		 LPNR_SoftTrigger - 抓拍 + 识别
 *	     LPNR_TakeSnapFrame - 抓拍一张图，但不进行识别
 *
 *	10. 结束
 *		 LPNR_Terminate - 调用此函数结束使用LPNR，动态库内部工作线程结束并关闭网络连接。
 *		
 *  11. 其他辅助函数
 *		LPNR_SyncTime - 同步识别机时间和计算机时间 （下发对时命令）
 *		LPNR_EnableLiveFrame - 使能或是禁能识别机发送实时图像帧
 *      LPNR_ReleaseData - 释放当前识别结果动态分配的数据
 *		LPNR_Lock/LPNR_Unlock - 上锁，解锁数据（获取实时帧或是任何识别结果数据之前先加锁，获取完后解锁，避免获取过程中被工作线程更改。
 *		LPNR_GetPlateAttribute - 获取车牌/车辆额外信息
 *		LPNR_GetExtDI - 获取一体机的扩展DI状态
 *		LPNR_GetMachineIP - 获取识别机IP字串
 *		LPNR_GetCameraLabel - 获取识别机标签（名字）
 *
 *	事件编号：
 *	回调函数只有一个参数就是事件编号。对于windows，这个编号在消息通知的 LPARAM里面，WPARAM为发出事件的摄像机
 *  句柄 (由LPNR_Init返回)。
 *    事件编号1：LPNR连线。
 *    事件编号2：LPNR离线
 *    事件编号3：开始进行识别处理
 *    事件编号4：识别处理结束可以获取结果。
 *	  事件编号5：实时图像帧更新，可以获取，更新画面。
 *    事件编号6：车辆进入虚拟线圈检测区（上位机要等收到事件编号4后才可以去获取车牌信息和图片）
 *    事件编号7：车辆离开虚拟线圈检测区
 *    事件编号8：扩展DI点状态有变化（200D, 200P, 500才有扩展IO）
 *
 *  NOTE - ENABLERS
 *  1. ENABLE_LOG	- define 后，程序会记录日志在固定的文件中
 *  2. ENABLE_TESTCODE - define 后，测试程序使能，可以编译出可执行程序测试（linux版本才有）
 *
 *  修改：
 *  A. 2015-10-01：
 *     1. 新增函数 LPNR_GetHeadImageSize, LPNR_GetHeadImage
 *         功能：获取车头图像，图像大小为CIF （PAL为720x576， 以车牌位置为中心点， 没有车牌则以识别区为中心的。
 *     2. 新增函数 LPNR_GetQuadImageSize, LPNR_GetQuadImage
 *         功能：获取1/4解像度图像。
 *     3. 新增函数 LPNR_GetHeadImageSize, LPNR_GetHeadImage获取车头图。图片大小为CIF
 *	   以上两种图像没有字符叠加信息需要配合识别机版本 1.3.1.21 （含）之后才有。
 *
 * B 2016-01-15
 *		1. 新增事件编号6，7，8  需配合识别机程序版本1.3.2.3之后（含）版本。
 *      2. 新增函数 LPNR_GetExtDI， LPNR_GetMachineIP，LPNR_GetPlateAttribute
 *	    3. 初始化识别机时，先测试指定IP的识别机有没有响应UDP查询请求。没有的话表示该IP没有识别机、没有连线或是程序没有运行。
 *          一样返回错误。这样可以在初始化时候立刻知道识别机是不是备妥。不用等超时没有连线。
 *		4. 日志保存位置修改 
 *			- linux: /var/log/lpnr_<ip>.log （/var/log必须存在）
 *          - Windows: ./LOG/LPNR-<IP>.log (注意，程序不会创建LOG文件夹，必须手工创建）
 *
 * C 2016-04-13
 *		1. 新增接口 LPNR_GetCameraLabel
 *
 * D 2016-12-16
 *     1. 新增接口 - LPNR_QueryPlate: 在不先建立TCP连接的情况下，简单的一个调用实现UDP创建，查询车牌号码，关闭UDP，返回结果。
 *          此函数用于对车位相机的轮询。上位机不需要建立大量连接的情况下，一一查询每个车位的当前车牌号码。当车牌号码内容是
 *          “整牌拒识”, 表示没有车辆（或是有车没有挂车牌）。这个函数是给上位机查询停车场车位相机当前车位上车辆的车牌号码。
 *
 * E 2017-06-07
 *     1. 新增接口 
 *			- LPNR_LightCtrl - 控制识别机照明灯（在200A机型，可以使用照明灯ON、OFF讯号控制栏杆机抬落杆。
 *			- LPNR_SetExtDO - 控制扩展DO（需具备扩展IO的机型才有效果)
 * F 2017-07-11
 *     1. 新增接口
 *		  - DLLAPI BOOL CALLTYPE LPNR_SetOSDTimeStamp(HANDLE h, BOOL bEnable, int x, int y);
 *
 * G 2017-09-29
 *		1. 新增识别机脉冲讯号输出控制接口
 *		  - DLLAPI BOOL CALLTYPE LPNR_PulseOut(HANDLE h, int pin, int period, int count);
 *		2. 新增串口透传功能
 *		  - DLLAPI BOOL CALLTYPE LPNR_COM_init(HANDLE h, BOOL bEnable, int Speed);
 *		  - DLLAPI BOOL CALLTYPE LPNR_COM_aync(HANDLE h, BOOL bEnable);
 *		  - DLLAPI BOOL CALLTYPE LPNR_COM_send(HANDLE h, BYTE *data, int len);
 *		  - DLLAPI BOOL CALLTYPE LPNR_COM_iqueue(HANDLE h);
 *		  - DLLAPI BOOL CALLTYPE LPNR_COM_peek(HANDLE h, BYTE *RxData, int size);
 *		  - DLLAPI BOOL CALLTYPE LPNR_COM_read(HANDLE h, BYTE *RxData, int size);
 *		  - DLLAPI int CALLTYPE LPNR_COM_remove(HANDLE h, int size);
 *		  - DLLAPI int CALLTYPE LPNR_COM_clear(HANDLE h);
 * 
 * H 2018-03-07
 *    1. LPNR_GetPlateAttribute 函数中，参数attr数组内容增加
 *		  attr[5]：车牌颜色代码 (PLATE_COLOR_E)
 *        attr[6]:  车牌类型(PLATE_TYPE_E)
 *    2. 黄绿色车牌的车牌颜色汉字改为"秋", 例如：秋苏E12345. 以便和绿色渐变车牌区别
 *    注： 事件EVT_PLATENUM是提前发送的车牌号码，识别机版本1.3.10.3开始，黄绿色才会发送“秋”，否则还是发送“绿”。
 *            收到EVT_DONE后去获取的车牌号码，则只要是支持新能源车的识别机版本，黄绿色牌都会给“秋”字。因为这是
 *            动态库根据车牌颜色代码给的颜色汉字。
 *
 *  I 2018-03-26
 *    1. 配合线圈触发双向功能（车头，车尾，车头+车尾），识别机提前上报的车牌号码如果是车尾识别，会在车牌号码后面加上后缀
 *        (车尾). 动态库会把这个后缀删除，并且设置属性 attr[1]设置为车尾。在收到EVT_PLATENUM事件后，可以获取车牌号码（LPNR_GetPlateNumber）
 *       以及车头车尾信息（LPNR_GetPlateAttribute），但是此时获取的attribute只有attr[1]是有意义的。其他的成员需要等收到EVT_DONE事件后再调用
 *       一次LPNR_GetPlateAttribute获取。
 *    2. 增加一个事件 EVT_ACK. 识别机收到下行控制帧(DO, PULSE)后，会发送应答帧。DataType是DTYP_ACK, DataId是下行控制帧的控制码 ctrlAttr.code
 *
 * J  2018-03-31
 *    1. LPNR_GetExtDI接口（获取DI值）改为 LPNR_GetExtDIO (获取当前DI和DO的值)
 * 
 * K 2018-05-xx
 *   1. 增加视频叠加接口
 *       - LPNR_SetOSDTimeStamp
 *       - LPNR_SetOSDLabel
 *       - LPNR_SetOSDLogo
 *       - LPNR_SetOSDROI
 *       - LPNR_SetOSDPlate
 *       - LPNR_UserOSDOff
 *       - LPNR_UserOSDOn
 *   2. 增加视频流使能禁能接口
 *       - LPNR_SetStream
 *   3. 增加抓拍图片上报使能禁能接口
 *       - LPNR_SetCaptureImage
 *
 * L  2018-07-04
 *   1. 修改瑕疵，原来工作线程没有处理select返回-1的情况，现在加上
 *   2  增加二级图片和车牌buffer, 收到DID_BEGIN后，把前一辆车的数据搬到二级buffer,　然后清除一级buffer. 上位机要数据和图片时候，由二级buffer去获取。
 *       这样可以避免车牌识别连续触发两次，上位机要获取第一次的结果时，获取不到（第二次触发时把数据清除）。
 *   3  增加接口 LPNR_GetSelectedImage/LPNR_GetSelectedImageSize。可以替代之前个别各个不同图片的接口。但为了向后兼容，原来的接口还在
 *       可以获取所有图片（大图，中图，小图，微图，车牌彩色图，车牌二值图）
 *   4. LPNR_SetCaptureImage接口修改，增加tiny image使能的参数（多一个参数）。抓拍机需要支持tiny image抓拍叠加输出的版本 (1.3.13.12以及其后的版本)
 *   5. 增加LPNR_GetPlateNumberAdvance接口，收到EVT_PLATENUM事件后要调用这个接口才能获取到提前发送的车牌号，如果在收到EVT_DONE之前调用
 *	     LPNR_GetPlateNumber，获取到的是上一个车牌的号码。EVT_PLATENUM会比EVT_DONE提早发送100~200毫秒。这个时间差主要是抓拍图片字符叠加，
 *	     图片压缩，图发送消耗的时间。使能输出的图片越多，延时越长。
 *
 * M 2018-07-19
 *   1. 增加LOG操作函数:
 *	     - LPNR_EnableLog - 临时使能/禁能日志
 *      - LPNR_SetLogPath - 设置日志保存文件夹（默认文件夹看mtrace.c里面的宏定义）
 *      - LPNR_SetLogSizeAndCount - 设置每个日志文件的大小，和滚动备份的文件数（默认大小和滚动备份数量看mtrace.c里面的宏定义）
 *	    - LPNR_UserLog - 用户写日志到LPNR的日志文件里面。例如以本函数库加以封装的客户指定动态库程序，可以将日志写到
 *                                    LPNR的内部日志文件里面，统一记录容易查问题。
 *   2. 增加proxy操做功能
 *       proxy是让车道程序能代为建立和识别机的TCP连接，收费系统一般摄像机是在车道一级的内网，站级以及更高级的上位机是无法直接访问的。
 *      车道程序的动态库可以利用这个功能打开proxy功能，于是，上位机可以搜索到，也可以连接到车道计算机下属的摄像机。这样就可以允许维修人员
 *      进行远程连接，设置，更新等。
 *      - Proxy_Init  - 使能并初始化proxy功能
 *      - Proxy_Terminate - 禁能并结束Proxy功能，所有已经连接的远程客户端都会被断开，并且无法再连接
 *      - Proxy_GetClients - 获取当前利用proxy功能连接到识别机的客户端数目以及它们的IP地址
 *   3. 使用common函数库winnet.c 和 mtrace.c，不再使用自带的函数。
 *
 *  N 2018-09-30
 *    1. 配合抓拍机新增功能，获取一帧并裁剪指定区域后发送。最多指定10个裁剪区域（目前使用手工编译配置文件放置在抓拍机里面/home/usercrop.conf)
 *        也可以发送抓拍裁剪命令指定一个动态区域裁剪（不需要配置，或是不在配置里面的区域）
 *    2. 收到裁剪的抓拍图片后，动态库发送消息EVT_CROPIMG，这个消息的高16位是裁剪区编号(1..10)
 *    3. 添加一个接口LPNR_TakeSnapFrameEx，功能是LPNR_TakeSnapFrame的扩展，可以指定抓拍哪个解像度的图。
 *        动态库收到需要的抓拍帧后，一样发送消息EVT_SNAP，但是高16位是解像度编号 [1..3]
 *    4. 修改两处BUG，一处瑕疵。BUG是在IID2Index函数里面，IID_HEX转我们的image index时错误的给了IMGID_CAPSMALL(1/4图), 应该给IMGID_CAPTINY(1/16)
 *    5. 瑕疵是原来连接后，句柄里的tickLastHeared成员是设置为0，这样的话，如果客户端连接后一直没有发送东西，超时断开就不会起到作用。因此现在是
 *        建立连接后就把这个成员值设置为当前时间。让超时断开从连接后就起作用，而不是要收到一个帧后才起作用。不过，这个瑕疵几乎没有影响，因为识别机
 *        在连接建立就就会发送好多当前设置，版本号，本地时钟等讯息。
 *   NOTE - 使用本函数当一般源码编译为自己的动态库或是应用程序，需要在项目里面或是linux的Makefile里面自己定义是否要使能日志
 *                 功能。定义ENABLE_LOG这个编译定义项，即可使能日志功能。默认是没有定义的。源代码里面已经没有定义。
 *              
 * O 2019-06-11
 *     1. Proxy 功能改为通用函数，写入utils_proxy.c. 
 *     2. 增加telnet, ftp 的代理功能，端口为 8023， 8021
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#ifdef linux
#include <error.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include <errno.h>
#include "lpnrprotocol.h"
#include "../common/longtime.h"

// enabler
#define ENABLE_LOG		// 放到项目的编译定义里面
#define ENABLE_TRACE_FUNCTION  // 要ENABLE_LOG的前提，这个定义才有用。不需要的话注释掉。

#define IsValidHeader( hdr)  ( ((hdr).DataType & DTYP_MASK) == DTYP_MAGIC )

#define FREE_BUFFER(buf)	\
	if ( buf ) \
	{ \
		free( buf ); \
		buf = NULL; \
	}

#define YESNO(b)		( (b) ? "yes" : "no" )

#ifdef linux 
// -------------------------- [LINUX] -------------------------
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../utils/utils_net.h"
#include "../utils/utils_mtrace.h"

// nutalize or mimic windows specific functions
#define WINSOCK_START()
#define WINSOCK_STOP()
#define closesocket(fd)	close(fd)
#define WSAGetLastError()	( errno )
// map other windows functions to linux equivalent
#define Sleep(n)		usleep((n)*1000)
#define ASSERT			assert
// nutral macro functions with common format for both windows and linux
#define Mutex_Lock(h)		pthread_mutex_lock(&(h)->hMutex )
#define Mutex_Unlock(h)	pthread_mutex_unlock(&(h)->hMutex )
#define Ring_Lock(h)		pthread_mutex_lock(&(h)->hMutexRing )
#define Ring_Unlock(h)	pthread_mutex_unlock(&(h)->hMutexRing )
#define Proxy_Lock(h)		pthread_mutex_lock(&(h)->hMutexProxy )
#define Proxy_Unlock(h)	pthread_mutex_unlock(&(h)->hMutexProxy )
#define DeleteObject(h)		pthread_mutex_destroy(&h)
//#define Mutex_Lock(h)	
//#define Mutex_Unlock(h)	
#define TRACE_LOG(hOBJ,fmt...)							MTRACE_LOG(hOBJ->hLog, fmt)
#ifdef ENABLE_TRACE_FUNCTION
#define TRACE_FUNCTION(hOBJ)						MTRACE_LOG(hOBJ->hLog,"【%s】\n", __FUNCTION__)
#define TRACE_FUNCTION_FMT(hOBJ,fmt,...)	MTRACE_LOG(hOBJ->hLog, "【%s】- "fmt, __FUNCTION__, ##__VA_ARGS__)
#else
TRACE_FUNCTION(hOBJ)
#define TRACE_FUNCTION_FMT(hOBJ,fmt...)
#endif
#define THREADCALL	void *
#define THREADRET		NULL

unsigned long GetTickCount()
{
	static signed long long begin_time = 0;
	static signed long long now_time;
	struct timespec tp;
	unsigned long tmsec = 0;
	if ( clock_gettime(CLOCK_MONOTONIC, &tp) != -1 )
	{
		now_time = tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
	}
	if ( begin_time == 0 )
		begin_time = now_time;
	tmsec = (unsigned long)(now_time - begin_time);
	return tmsec;
}

#define max(a,b)			( (a) > (b) ? (a) : (b) )

#else	
// -------------------------- [WINDOWS] -------------------------
#include "../common/winnet.h"
#include "../common/mtrace.h"
#include <windows.h>
#include <io.h>
#include <direct.h>
#include <fcntl.h>
#include <sys/stat.h>
#pragma comment(lib,"User32.lib")
// common enabler - better put them in VS project setting
//#define _CRT_SECURE_NO_WARNINGS
//#define _CRT_SECURE_NO_DEPRECATE

#define WINSOCK_START()		winsock_startup()
#define WINSOCK_STOP()		winsock_cleanup()
// nutral macro functions with common format for both windows and linux
#define Mutex_Lock(h)			WaitForSingleObject((h)->hMutex, INFINITE )
#define Mutex_Unlock(h)		ReleaseMutex(  (h)->hMutex )
#define Ring_Lock(h)			WaitForSingleObject((h)->hMutexRing, INFINITE )
#define Ring_Unlock(h)		ReleaseMutex(  (h)->hMutexRing )
#define Proxy_Lock(h)			WaitForSingleObject((h)->hMutexProxy, INFINITE )
#define Proxy_Unlock(h)		ReleaseMutex(  (h)->hMutexProxy )
static BOOL m_nWinSockRef = 0;
#define TRACE_LOG(hOBJ,...)									MTRACE_LOG(hOBJ->hLog, __VA_ARGS__)
#ifdef ENABLE_TRACE_FUNCTION
#define TRACE_FUNCTION(hOBJ)							MTRACE_LOG(hOBJ->hLog,"【"__FUNCTION__"】\n")
#define TRACE_FUNCTION_FMT(hOBJ,fmt,...)		MTRACE_LOG(hOBJ->hLog, "【"__FUNCTION__"】- "fmt, __VA_ARGS__)
#else
TRACE_FUNCTION(hOBJ)
#define TRACE_FUNCTION_FMT(hOBJ,fmt,...)
#endif

/* 'strip' is IP string, 'sadd_in' is struct sockaddr_in */
#define inet_aton( strip, saddr_in )	( ( (saddr_in) ->s_addr = inet_addr( (strip) ) ) != INADDR_NONE )  

#define THREADCALL	DWORD WINAPI
#define THREADRET		0

#endif

#include "rwlpnrapi.h"


// -------------------------- [END] -------------------------

#define LPNR_PORT			6008
#define MAGIC_NUM			0xaabbccdd

#define STAT_RUN				1		// work thread running
#define STAT_END			2		// work thread end loop
#define STAT_EXIT			3		// work thread exit

#define MAX_RING_SIZE		1024

typedef enum {
	UNKNOWN=0,
	NORMAL,
	DISCONNECT,
	RECONNECT,
}  LINK_STAT_E;

typedef enum {
	OP_IDLE=0,
	OP_PROCESS,
	OP_RREPORT,
} OP_STAT_E;

struct RemoteClient {
	char strIP[16];
	int		port;
	SOCKET fd_pair[2];		// 0 - me and remote, 1 - me and camera
#ifdef linux
	pthread_t		hThread;
#else	
	HANDLE	hThread;
#endif	
	struct RemoteClient *prev;		// previous node
	struct RemoteClient *next;		// next node
};


typedef struct tagLPNRObject
{
	DWORD	dwMagicNum;
	SOCKET sock;
	char  strIP[16];
	int		status;
	char label[SZ_LABEL];
	LINK_STAT_E	enLink;
	OP_STAT_E		enOper;
	DWORD tickLastHeared;
	int		acked_ctrl_id;			// 收到的下行控制命令的ACK帧DataId, 这个内容就是下行控制帧的控制命令字 header.ctrlAttr.code
#ifdef linux
	pthread_t		hThread;
	pthread_mutex_t		hMutex;
#else	
	HANDLE	hThread;
	HANDLE	hMutex;
	HWND		hWnd;
	int				nMsgNo;
#endif	
	LPNR_callback  cbfxc;
	// 一级图片接收buffer 
	PVOID		pCapImage[IMGID_BUTT];			// input image in jpeg
	int				szCapImage[IMGID_BUTT];			// input image size
	// 一级车牌信息buffer和处理时间
	char			strPlate[12];
	PlateInfo	plateInfo;
	int				process_time;		// in msec
	int				elapsed_time;		// in msec
	// 二级图片接收buffer
	PVOID		pCapImage2[IMGID_BUTT];			// input image in jpeg
	int				szCapImage2[IMGID_BUTT];			// input image size
	// 二级车牌信息buffer和处理时间
	char			strPlate2[12];
	PlateInfo	plateInfo2;
	int				process_time2;		// in msec
	int				elapsed_time2;		// in msec
	// 裁剪的抓拍图片保存位置 - 被获取后立刻释放内存，所以不能重复获取。除非又收到新的
	PVOID		pCropImage[10];		
	int				szCropImage[10];
	int				szCropBuf[10];			// crop image buffer  size allocated
	// 实时帧buffer
	PVOID		pLiveImage;
	int				nLiveSize;
	int				nLiveAlloc;
	// 提前发送的车牌号码保存在这里, 这个buffer内容不清除，每次识别出车牌提前发送后更新
	char			strPlateAdv[12];
	// 触发识别原因
	TRIG_SRC_E enTriggerSource;		// TRIG_SRC_E 枚举类型的 int值 
	//	for model with extended GPIO 
	int				diParam;				// 高16位是last DI value，低16位是current DI value
	short			dio_val[2];			// [0]是DI，[1]是DO. 这是抓拍机在客户端连接或是下命令CTRL_READEXTDIO上报的
											// dio_val[0]应该永远和diParam的低16位相同。
	
	// 版本讯息
	VerAttr		verAttr;
	// 时间讯息，当连接上识别机后，识别机会发送它的系统时间
	CTM			camTime;
	_longtime  camLongTime;		// 时间信息的longtime 格式
	int				secDiff;					// PC系统时间 - 识别机系统时间的差 (单位秒)
	// 解像度信息
	SIZE		szImage[3];		// 0: full, 1: 1/4, 2: 1/16
	// Logger
	BOOL	bLogEnable;
	HANDLE	hLog;
	// 摄像机配置讯息
	ParamConf	paramConf;
	ExtParamConf extParamConf;
	DataAttr		dataCfg;
	H264Conf	h264Conf;
	OSDConf	osdConf;
	OSDPayload osdPayload;
	OSDFixText osdFixText[4];

	// 透明串口数据接收ring buffer (for model with external serial port)
#ifdef linux
	pthread_mutex_t		hMutexRing;
#else	
	HANDLE	hMutexRing;
#endif
	int		ring_head, ring_tail;
	BYTE	ring_buf[MAX_RING_SIZE];

	// proxy
	char		   *proxy_host_ip;
	SOCKET  proxy_listen;
	SOCKET  proxy_udp;
	BOOL	   bProxyEnable;
	int			   nProxyListCount;
	struct RemoteClient *list_head;
#ifdef linux
	pthread_t		hThreadProxy;
	pthread_mutex_t		hMutexProxy;
#else	
	HANDLE	hThreadProxy;
	HANDLE	hMutexProxy;
#endif

} HVOBJ,		*PHVOBJ;

static BOOL __gbEnableLog = TRUE;
#define IS_VALID_OBJ(pobj)	( (pobj) && (pobj)->dwMagicNum==MAGIC_NUM )

#define NextTailPosit(h)		( (h)->ring_tail==MAX_RING_SIZE-1 ? 0 : (h)->ring_tail+1 )
#define NextHeadPosit(h)	( (h)->ring_head==MAX_RING_SIZE-1 ? 0 : (h)->ring_head+1 )
#define IsRingEmpty(h)		( (h)->ring_head==(h)->ring_tail )
#define IsRingFull(h)			( NextTailPosit(h) == (h)->ring_head )
#define RingElements(h)		(((h)->ring_tail >= (h)->ring_head) ? ((h)->ring_tail - (h)->ring_head) : (MAX_RING_SIZE - (h)->ring_head + (h)->ring_tail) )
#define	PrevPosit(h,n)		( (n)==0 ? MAX_RING_SIZE-1 : (n)-1 )
#define	NextPosit(h,n)		( (n)==MAX_RING_SIZE-1 ? 0 : (n)+1 )
#define RingBufSize(h)		( MAX_RING_SIZE-1 )
// pos 是在ring_buf里的位置，返回是第几个数据（由head开始算是0）
#define PositIndex(h,pos)		( (pos)==(h)->ring_tail ? -1 : ((pos)>=(h)->ring_head ? (pos)-(h)->ring_head : ((MAX_RING_SIZE-(h)->ring_head-1) + (pos))) )
// PositIndex的反操作，idx是由head开始的第几个数据（head是0），返回值是在ring_buffer里的位置。
#define IndexPoist(h,idx)	(  (idx)+(h)->ring_head<MAX_RING_SIZE ? (idx)+(h)->ring_head : (idx+(h)->ring_head+1-MAX_RING_SIZE) )
#define GetRingData(h,pos)		((h)->ring_buf[pos])
#define SetRingData(h,pos, b)		(h)->ring_buf[pos] = (BYTE)(b);

// forward reference
THREADCALL lpnr_workthread_fxc(PVOID lpParameter);
THREADCALL proxy_masterthread_fxc(PVOID lpParameter);
THREADCALL proxy_workthread_fxc(PVOID lpParameter);

void proxy_destroy_clients(PHVOBJ hObj);
int proxy_get_clients(PHVOBJ hObj, char strIP[][16], int size_array);

///////////////////////////////////////////////////////////////
// LPNR API
static const char *GetEventText( int evnt )
{
	switch (evnt)
	{
	case EVT_ONLINE	:	return "车牌识别机连线";
	case EVT_OFFLINE:	return "车牌识别机离线";
	case EVT_FIRED:		return "开始识别处理";
	case EVT_DONE:		return "识别结束，数据已接收完成";
	case EVT_LIVE:		return "实时帧更新";
	case EVT_VLDIN:		return "车辆进入虚拟线圈检测区";
	case EVT_VLDOUT:	return "车辆离开虚拟线圈检测区";
	case EVT_EXTDI:		return "扩展DI点状态变化";
	case EVT_SNAP:		return "接收到抓拍帧";
	case EVT_ASYNRX:	return "接收到透传串口输入数据";
	case EVT_PLATENUM: return "接收到提前发送的车牌号";
	case EVT_VERINFO:	return "收到识别机的软件版本讯息";
	case EVT_ACK:			return "收到下行控制帧的ACK回报";
	case EVT_NEWCLIENT: return "新的远程客户端连接摄像机";
	case EVT_CLIENTGO:	return "远程客户端连接断开";
	}
	return "未知事件编号";
}

static const char *GetTriggerSourceText(TRIG_SRC_E enTrig)
{
	switch(enTrig)
	{
	case IMG_UPLOAD:
		return "上位机返送图片识别";
	case IMG_HOSTTRIGGER:
		return "上位机软触发";
	case IMG_LOOPTRIGGER:
		return "定时自动触发";
	case IMG_AUTOTRIG:
		return "定时自动触发";
	case IMG_VLOOPTRG:
		return "虚拟线圈触发";
	case IMG_OVRSPEED:
		return "超速触发";
	}
	return "Unknown Trigger Source";
}

static const char *GetImageName(int index)
{
	const char *img_name[] = { "车牌二值图", "车牌彩色图", "抓拍全图", "抓拍中图", "抓拍小图", "抓拍微图", "抓拍车头图" };
	if ( 0 <= index && index < sizeof(img_name)/sizeof(img_name[0]) )
		return img_name[index];
	return "unknown";
}

static void NoticeHostEvent( PHVOBJ hObj, int evnt  )
{
	LPARAM lParam = evnt;
	int param = evnt >> 16;
	evnt &= 0xffff;
	if ( evnt != EVT_LIVE )
	{

		TRACE_LOG(hObj, "发送事件编号 %d  (%s)  - param=%d 给应用程序.\n", evnt, GetEventText(evnt), param );
	}
	if ( hObj->cbfxc )  hObj->cbfxc((HANDLE)hObj, lParam);
#ifdef WIN32
	if ( hObj->hWnd )
		PostMessage( hObj->hWnd, hObj->nMsgNo, (DWORD)hObj, lParam );
#endif
}

// nWhich 1: 一级buffer, 2: 二级buffer, 4: crop image
static void ReleaseData(PHVOBJ pHvObj, int nWhich)
{
	int i;
	// ReleaseData
	Mutex_Lock(pHvObj);
	if ( nWhich & 0x02 )
	{
		for(i=0; i<IMGID_BUTT; i++)
		{
			FREE_BUFFER(pHvObj->pCapImage2[i]);
			pHvObj->szCapImage2[i] = 0;
		}
		pHvObj->strPlate2[0] = '\0';
		memset( &pHvObj->plateInfo2, 0, sizeof(PlateInfo) );
	}
	if ( nWhich & 0x01 )
	{
		for(i=0; i<IMGID_BUTT; i++)
		{
			FREE_BUFFER(pHvObj->pCapImage[i]);
			pHvObj->szCapImage[i] = 0;
		}
		pHvObj->strPlate[0] = '\0';
		memset( &pHvObj->plateInfo, 0, sizeof(PlateInfo) );
	}
	if ( nWhich & 0x4 )
	{
		int i;
		for(i=0; i<10; i++)
		{
			if ( pHvObj->szCropBuf[i] )
				free( pHvObj->pCropImage[i]);
			pHvObj->szCropBuf[i] = pHvObj->szCropImage[i] = 0;
			pHvObj->pCropImage[i] = NULL;
		}	
	}
	Mutex_Unlock(pHvObj);
}

static void RolloverData(PHVOBJ pHvObj)
{
	int i;
	ReleaseData(pHvObj,2);
	for(i=0; i<IMGID_BUTT; i++)
	{
		pHvObj->pCapImage2[i] = pHvObj->pCapImage[i];
		pHvObj->pCapImage[i] = NULL;
		pHvObj->szCapImage2[i] = pHvObj->szCapImage[i];
		pHvObj->szCapImage[i] = 0;
		pHvObj->process_time2 = pHvObj->process_time;		// in msec
		pHvObj->elapsed_time2 = pHvObj->elapsed_time;		// in msec
	}
	strcpy(pHvObj->strPlate2, pHvObj->strPlate);
}

static int IID2Index(int IID)
{
	switch(IID)
	{
	case IID_CAP:
		return IMGID_CAPFULL;
	case IID_HEAD:
		return IMGID_CAPHEAD;
	case IID_QUAD:
		return IMGID_CAPSMALL;
	case IID_T3RD:
		return IMGID_CAPMIDDLE;
	case IID_HEX:
		return IMGID_CAPTINY;
	case IID_PLRGB:
		return IMGID_PLATECOLOR;
	case IID_PLBIN:
		return IMGID_PLATEBIN;
	}
	return -1;
}

static BOOL TestCameraReady( DWORD dwIP )
{
	DataHeader header;
	int len;
	BOOL bFound = FALSE;
	SOCKET sock = sock_udp_open();

	sock_udp_timeout( sock, 500 );
	SEARCH_HEADER_INIT( header );
	len = sock_udp_send0( sock, dwIP, PORT_SEARCH, (char *)&header, sizeof(DataHeader) );
	if ( (len=sock_udp_recv( sock, (char *)&header, sizeof(DataHeader), &dwIP)) == sizeof(DataHeader) && 
		header.DataId == DID_ANSWER )
		bFound = TRUE;
	sock_close( sock );
	return bFound;
}

#if defined _WIN32 || defined _WIN64
DLLAPI BOOL CALLTYPE LPNR_WinsockStart( BOOL bStart )
{
	if ( bStart )
		WINSOCK_START();
	else
		WINSOCK_STOP();
	return TRUE;		// 假设永远会成功
}
#endif

DLLAPI HANDLE CALLTYPE LPNR_Init( const char *strIP )
{
	PHVOBJ hObj = NULL;
#ifdef linux
	struct sockaddr_in 	my_addr;
	if ( inet_aton( strIP, (struct in_addr *)&my_addr)==0 )
#else
	DWORD threadId;

	if ( inet_addr(strIP) == INADDR_NONE )
#endif
	{
		return NULL;
	}

	WINSOCK_START();

#ifdef PROBE_BEFORE_CONNECT
	if ( !TestCameraReady(inet_addr(strIP)) )
	{
		return NULL;
	}
#endif
	// 1. 创建对象
	hObj = (PHVOBJ)malloc(sizeof(HVOBJ));
	memset( hObj, 0, sizeof(HVOBJ) );
	strcpy( hObj->strIP, strIP );
	hObj->strIP[15] = '\0';
	hObj->sock = INVALID_SOCKET;
	hObj->dwMagicNum = MAGIC_NUM;
	hObj->bLogEnable = __gbEnableLog;
	if ( hObj->bLogEnable )
	{
		char log_name[64] = "LPNRDLL-";
		strcat(log_name, strIP);
		hObj->hLog = MLOG_INIT(NULL, log_name);
	}
	// 2. 创建Mutex锁 and 启动工作线程

#ifdef linux
	pthread_mutex_init(&hObj->hMutex, NULL );
	pthread_mutex_init(&hObj->hMutexRing, NULL );
	pthread_create( &hObj->hThread, NULL, lpnr_workthread_fxc, (void *)hObj);
#else
	hObj->hMutex = CreateMutex(
							NULL,				// default security attributes
							FALSE,			// initially not owned
							NULL);				// mutex name (NULL for unname mutex)

	hObj->hMutexRing = CreateMutex(
							NULL,				// default security attributes
							FALSE,			// initially not owned
							NULL);				// mutex name (NULL for unname mutex)

	hObj->hThread = CreateThread(
						NULL,						// default security attributes
						0,								// use default stack size
(LPTHREAD_START_ROUTINE)lpnr_workthread_fxc,		// thread function
						hObj,						// argument to thread function
						0,								// thread will be suspended after created.
						&threadId);				// returns the thread identifier
#endif
	TRACE_FUNCTION_FMT(hObj,"%s - OK\n", strIP );

	// proxy member initiialize
	hObj->proxy_listen = hObj->proxy_udp = INVALID_SOCKET;
	return hObj;
}

DLLAPI BOOL CALLTYPE LPNR_Terminate(HANDLE h)
{
	//int i;
	PHVOBJ pHvObj = (PHVOBJ)h;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
		
	TRACE_FUNCTION(pHvObj);
	if ( pHvObj->hThread )
	{
		pHvObj->status = STAT_END;
#ifndef linux		
		WaitForSingleObject(pHvObj->hThread, 100);
		if (  pHvObj->hThread )
		{
			// work thread not terminated. kill it
			TerminateThread( pHvObj->hThread, 0 );
		}
		CloseHandle(pHvObj->hMutex);
		CloseHandle(pHvObj->hMutexRing);
#else
		pthread_cancel( pHvObj->hThread );
		pthread_join(pHvObj->hThread, NULL);
		pthread_mutex_destroy(&pHvObj->hMutex );
		pthread_mutex_destroy(&pHvObj->hMutexRing );
#endif		
	}
	ReleaseData(pHvObj, 7);
	//DeleteObject(pHvObj->hMutex);
	if ( pHvObj->sock != INVALID_SOCKET )
 		closesocket( pHvObj->sock);

	if ( pHvObj->bProxyEnable )
		Proxy_Terminate(pHvObj);

	free( pHvObj );	
	return TRUE;
}

DLLAPI BOOL CALLTYPE Proxy_Init(HANDLE h, const char *HostIP)
 {
	PHVOBJ pHvObj = (PHVOBJ)h;
	DWORD threadId;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_FUNCTION_FMT(pHvObj, "HostIP=%s\n", HostIP);
	if (HostIP)
	{
		if ( inet_addr(HostIP)==INADDR_NONE  )
		{
			TRACE_LOG(pHvObj,"--> Invalid Host IP address!\n");
			return FALSE;
		}
		pHvObj->proxy_host_ip = strdup(HostIP);
	}
	// create TCP listen socket and a UDP socket on-behalf of camera
	pHvObj->proxy_listen = sock_listen(PORT_LISTEN,HostIP,5);
	if ( pHvObj->proxy_listen == INVALID_SOCKET )
	{
		TRACE_LOG(pHvObj,"--> Failed to listen TCP port %d (error=%d)!!!\n", PORT_LISTEN, WSAGetLastError());
		goto proxy_error;
	}
	pHvObj->proxy_udp = sock_udp_bindLocalIP(HostIP ? inet_addr(HostIP) : INADDR_ANY, PORT_SEARCH);
	if ( pHvObj->proxy_udp==INVALID_SOCKET)
	{
		TRACE_LOG(pHvObj,"--> Failed to bind UDP port %d (error=%d)!!!\n", PORT_SEARCH, WSAGetLastError());
		goto proxy_error;
	}
	// sock_udp_broadcast(pHvObj->proxy_udp, TRUE);
	// create mutex and working thread
	pHvObj->bProxyEnable = TRUE;
#ifdef linux
	pthread_mutex_init(&pHvObj->hMutexProxy, NULL );
	pthread_create( &pHvObj->hThreadProxy, NULL, proxy_masterthread_fxc, (void *)pHvObj);
#else
	pHvObj->hMutexProxy = CreateMutex(
							NULL,				// default security attributes
							FALSE,			// initially not owned
							NULL);				// mutex name (NULL for unname mutex)
	if ( pHvObj->hMutexProxy==NULL )	
		goto proxy_error;

	pHvObj->hThreadProxy = CreateThread(
						NULL,						// default security attributes
						0,								// use default stack size
(LPTHREAD_START_ROUTINE)proxy_masterthread_fxc,		// thread function
						pHvObj,						// argument to thread function
						0,								// thread will be suspended after created.
						&threadId);				// returns the thread identifier
	if ( pHvObj->hThreadProxy==NULL )	
		goto proxy_error;
#endif
	return TRUE;

proxy_error:
	if ( pHvObj->proxy_host_ip )  free(pHvObj->proxy_host_ip);
	if ( pHvObj->proxy_listen != INVALID_SOCKET )
		closesocket(pHvObj->proxy_listen);
	if ( pHvObj->proxy_udp != INVALID_SOCKET )
		closesocket(pHvObj->proxy_udp);
#ifdef linux
		pthread_mutex_destroy(&pHvObj->hMutexProxy );
#else
	if ( pHvObj->hMutexProxy )
		CloseHandle(pHvObj->hMutexProxy);
#endif
	return FALSE;
 }

DLLAPI BOOL CALLTYPE Proxy_Terminate(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	if ( pHvObj->bProxyEnable )
	{
		pHvObj->bProxyEnable  = FALSE;
#ifndef linux		
		WaitForSingleObject(pHvObj->hThreadProxy, 100);
		if (  pHvObj->hThreadProxy )
		{
			// work thread not terminated. kill it
			TerminateThread( pHvObj->hThreadProxy, 0 );
		}
		CloseHandle(pHvObj->hMutexProxy);
#else
		pthread_cancel( pHvObj->hThreadProxy );
		pthread_join(pHvObj->hThreadProxy, NULL);
#endif		
		if ( pHvObj->proxy_host_ip )  free(pHvObj->proxy_host_ip);
		if (pHvObj->proxy_listen != INVALID_SOCKET )
		{
			closesocket(pHvObj->proxy_listen);
			pHvObj->proxy_listen = INVALID_SOCKET;
		}
		if (pHvObj->proxy_udp != INVALID_SOCKET )
		{
			closesocket(pHvObj->proxy_udp);
			pHvObj->proxy_udp = INVALID_SOCKET;
		}
	}

	if ( pHvObj->nProxyListCount > 0 )
		proxy_destroy_clients(pHvObj);
#ifndef linux
	CloseHandle(pHvObj->hMutexProxy);
#else
	pthread_mutex_destroy(&pHvObj->hMutexProxy);
#endif
	return TRUE;
}

DLLAPI int	 CALLTYPE Proxy_GetClients(HANDLE h, char strIP[][16], int ArraySize)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;

	TRACE_FUNCTION_FMT(pHvObj, "Proxy now is %s\n", pHvObj->bProxyEnable ? "ENABLED" : "DISABLED");
	if ( pHvObj->bProxyEnable && pHvObj->nProxyListCount > 0 )
	{
		return proxy_get_clients(pHvObj, strIP, ArraySize);
	}
	return 0;
}


// 设置应用程序的事件回调函数
DLLAPI BOOL CALLTYPE LPNR_SetCallBack(HANDLE h, LPNR_callback mycbfxc )
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
		
	TRACE_FUNCTION(pHvObj);
	pHvObj->cbfxc = mycbfxc;
	return TRUE;
}
#if defined _WIN32 || defined _WIN64
// 设置应用程序接收消息的窗口句柄和消息编号
DLLAPI BOOL CALLTYPE LPNR_SetWinMsg( HANDLE h, HWND hwnd, int msgno)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj,"msgno = %d\n", msgno);
	pHvObj->hWnd = hwnd;
	pHvObj->nMsgNo = msgno;
	return TRUE;
}
#endif

DLLAPI BOOL CALLTYPE LPNR_GetPlateNumber(HANDLE h, char *buf)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_FUNCTION_FMT(pHvObj, "应用程序获取车牌号码 (%s)\n", pHvObj->strPlate);
	// NOTE:
	// let API user program decide to lock or not to prevent dead lock (User invoke LPNR_Lock then call this function will cause dead lock in Linux 
	// as we dont use recusive mutex. application try to lock a mutex which already owned by itself will cause self-deadlock.
	// same reasone for other functions that comment out the Mutex_Lock/Mutex_Unlock
	// It is OK in Windows as Windows's mutex is recursively.
	//Mutex_Lock(pHvObj);		
	strcpy(buf, pHvObj->strPlate );
	//Mutex_Unlock(pHvObj);
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_GetPlateNumberAdvance(HANDLE h, char *buf)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_FUNCTION_FMT(pHvObj, "应用程序获取提前发送的车牌号码 (%s)\n", pHvObj->strPlateAdv);
	strcpy(buf, pHvObj->strPlateAdv );
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_GetPlateAttribute(HANDLE h, BYTE *attr)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_FUNCTION_FMT(pHvObj, "应用程序获取车牌号码 (%s)\n", pHvObj->strPlate);
	memcpy(attr, pHvObj->plateInfo.MatchRate, sizeof(pHvObj->plateInfo.MatchRate));
	return TRUE;
}

DLLAPI int CALLTYPE LPNR_GetPlateColorImageSize(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	TRACE_FUNCTION_FMT(pHvObj,"- size=%d\n", pHvObj->szCapImage2[IMGID_PLATECOLOR]);
	return pHvObj->szCapImage2[IMGID_PLATECOLOR];
}

DLLAPI int CALLTYPE LPNR_GetPlateColorImage(HANDLE h, void *buf)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_FUNCTION_FMT(pHvObj," 应用程序获取车牌彩色图片 size=%d\n", pHvObj->szCapImage2[IMGID_PLATECOLOR] );
	//Mutex_Lock(pHvObj);
	if ( pHvObj->szCapImage2[IMGID_PLATECOLOR] > 0 )
		memcpy(buf, pHvObj->pCapImage2[IMGID_PLATECOLOR], pHvObj->szCapImage2[IMGID_PLATECOLOR] );
	//Mutex_Unlock(pHvObj);
	return pHvObj->szCapImage2[IMGID_PLATECOLOR];
}

DLLAPI int CALLTYPE LPNR_GetPlateBinaryImageSize(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;

	TRACE_FUNCTION_FMT(pHvObj, "size=%d\n",  pHvObj->szCapImage2[IMGID_PLATEBIN]);
	return pHvObj->szCapImage2[IMGID_PLATEBIN];
}

DLLAPI int CALLTYPE LPNR_GetPlateBinaryImage(HANDLE h, void *buf)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_FUNCTION_FMT(pHvObj,"应用程序获取车牌二值图片 size=%d\n", pHvObj->szCapImage2[IMGID_PLATEBIN] );
	//Mutex_Lock(pHvObj);
	if ( pHvObj->szCapImage2[IMGID_PLATEBIN]  > 0 )
		memcpy(buf, pHvObj->pCapImage2[IMGID_PLATEBIN] , pHvObj->szCapImage2[IMGID_PLATEBIN]  );
	//Mutex_Unlock(pHvObj);
	return pHvObj->szCapImage2[IMGID_PLATEBIN] ;
}

DLLAPI int CALLTYPE LPNR_GetLiveFrameSize(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	return pHvObj->nLiveSize;
}

DLLAPI int CALLTYPE LPNR_GetLiveFrame(HANDLE h, void *buf)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	//Mutex_Lock(pHvObj);
	if ( pHvObj->nLiveSize > 0 )
		memcpy(buf, pHvObj->pLiveImage, pHvObj->nLiveSize );
	//Mutex_Unlock(pHvObj);
	return pHvObj->nLiveSize;
}

DLLAPI BOOL CALLTYPE LPNR_IsOnline(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	//TRACE_FUNCTION_FMT(pHvObj,"%s\n", pHvObj->enLink == NORMAL ? "yes" : "no");
	return pHvObj->enLink == NORMAL;
}


DLLAPI BOOL CALLTYPE LPNR_ReleaseData(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_FUNCTION(pHvObj);
	// Release 二级buffer
	ReleaseData(pHvObj, 2);
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_SoftTrigger(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_LOG(pHvObj,"【LPNR_SoftTrigger】\n");
	if ( pHvObj->enLink != NORMAL )
	{
		TRACE_LOG(pHvObj,"\t - 抓拍机离线!\n");
		return FALSE;
	}
	if ( pHvObj->enOper != OP_IDLE )
	{
		TRACE_LOG(pHvObj,"\t - 抓拍机忙，当前分析尚未完成!\n");
		return FALSE;
	}
	CTRL_HEADER_INIT( header, CTRL_TRIGGER, 0 );
	sock_write( pHvObj->sock, (const char*)&header, sizeof(header) );
	return TRUE;
}


DLLAPI BOOL CALLTYPE LPNR_SoftTriggerEx(HANDLE h, int Id)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj,"Id=%d\n", Id);
	if ( pHvObj->enLink != NORMAL )
	{
		MTRACE_LOG(pHvObj->hLog,"\t - Camera offline!\n");
		return FALSE;
	}
/*	
	if ( pHvObj->enOper != OP_IDLE )
	{
		MTRACE_LOG(pHvObj->hLog,"\t - Camera busy, current recognition has not finished yet!\n");
		return FALSE;
	}
*/	
	CTRL_HEADER_INIT( header, CTRL_TRIGGER, Id );
	sock_write( pHvObj->sock, (const char*)&header, sizeof(header) );
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_IsIdle(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	//	TRACE_FUNCTION(pHvObj);   application may call this frequently, so don't log this
	if ( pHvObj->enLink != NORMAL )
	{
		//TRACE_LOG(pHvObj,"\t - 抓拍机离线!\n");
		return FALSE;
	}
	return ( pHvObj->enOper == OP_IDLE );
}

DLLAPI BOOL CALLTYPE LPNR_GetTiming(HANDLE h, int *elaped, int *processed )
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_FUNCTION(pHvObj);
	if ( pHvObj->enLink != NORMAL )
	{
		TRACE_LOG(pHvObj," - 抓拍机离线!\n");
		return FALSE;
	}
	*elaped = pHvObj->elapsed_time2;
	*processed = pHvObj->process_time2;
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_EnableLiveFrame(HANDLE h, int nSizeCode)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_FUNCTION_FMT(pHvObj,"Live frame size code=%d\n", nSizeCode );
	if ( pHvObj->enLink != NORMAL )
	{
		TRACE_LOG(pHvObj,"\t - 抓拍机离线!\n");
		return FALSE;
	}
	CTRL_HEADER_INIT( header, CTRL_LIVEFEED, nSizeCode );
	sock_write( pHvObj->sock, (const char*)&header, sizeof(header) );
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_TakeSnapFrame(HANDLE h, BOOL bFlashLight)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_FUNCTION_FMT(pHvObj,"Enable Lighting=%s\n", bFlashLight ? "Yes" : "No" );
	if ( pHvObj->enLink != NORMAL )
	{
		TRACE_LOG(pHvObj,"\t-  失败，识别机离线!\n");
		return FALSE;
	}
	// 清除数据
	CTRL_HEADER_INIT(header, CTRL_SNAPCAP, bFlashLight ? 1 : 0 );
	sock_write( pHvObj->sock, (char *)&header, sizeof(header) );
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_TakeSnapFrameEx(HANDLE h, BOOL bFlashLight, int nSizeCode)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	int param;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_FUNCTION_FMT(pHvObj,"Enable Lighting=%s，nSizeCode=%d \n", bFlashLight ? "Yes" : "No", nSizeCode );
	if ( pHvObj->enLink != NORMAL )
	{
		TRACE_LOG(pHvObj,"\t-  失败，识别机离线!\n");
		return FALSE;
	}
	if ( nSizeCode < 1 || 3 < nSizeCode )
	{
		TRACE_LOG(pHvObj,"\t- 参数错误 nSizeCode 取值范围必须为[1..3]\n");
		return FALSE;
	}
	param = bFlashLight ? 1 : 0;
	param |= nSizeCode << 16;
	CTRL_HEADER_INIT(header, CTRL_SNAPCAP, param);
	sock_write( pHvObj->sock, (char *)&header, sizeof(header) );
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_TakeCropFrame(HANDLE h, int nCropArea, HI_RECT *rect)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_FUNCTION_FMT(pHvObj,"nCropArea=%d\n",  nCropArea);
	if ( pHvObj->enLink != NORMAL )
	{
		TRACE_LOG(pHvObj,"\t-  失败，识别机离线!\n");
		return FALSE;
	}
	if (rect!=NULL)
	{
		CTRL_HEADER_INIT(header, CTRL_CROPVIN, 0 );
		memcpy(header.ctrlAttr.option, &nCropArea, 4);
		memcpy(header.ctrlAttr.option+4, rect, sizeof(int)*4);
	}
	else
		CTRL_HEADER_INIT(header, CTRL_CROPVIN, nCropArea );
	sock_write( pHvObj->sock, (char *)&header, sizeof(header) );
	return TRUE;
}

BOOL CALLTYPE LPNR_DownloadImage(HANDLE h, BYTE *image, int size, int format)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj,"nimage size=%d, format=%d\n",  size, format);
	if ( pHvObj->enLink != NORMAL )
	{
		TRACE_LOG(pHvObj,"\t-  失败，识别机离线!\n");
		return FALSE;
	}
	IMAGE_HEADER_INITEX( header, IID_CAP, 0, 0, format, 0, "download", size );
	sock_write( pHvObj->sock, (char *)&header, sizeof(header) );
	// image body
	sock_write( pHvObj->sock, (char *)image, size );
	return TRUE;
}

BOOL CALLTYPE LPNR_DownloadImageFile(HANDLE h, LPCTSTR strFile)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	struct stat st;
	int fd;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj,"file=%s, format=%d\n",  strFile);
	if ( pHvObj->enLink != NORMAL )
	{
		TRACE_LOG(pHvObj,"\t-  失败，识别机离线!\n");
		return FALSE;
	} 
#if defined WIN32 || defined _WIN64
	if ( strlen(strFile)>0 && stat(strFile,&st)==0 &&  st.st_size>0 && (fd=open( strFile, O_RDONLY|O_BINARY)) != -1 )
#else
	if ( strlen(strFile)>0 && stat(strFile,&st)==0 &&  st.st_size>0 && (fd=open( strFile, O_RDONLY )) != -1 )
#endif
	{
		DataHeader hdr;
		char fname[_MAX_FNAME];
		char extName[_MAX_EXT];
		char buffer[1024];
		int len;
		long  lsize = (long)st.st_size;
		int format;
		_splitpath(strFile, NULL, NULL, fname, extName);
		strcpy(buffer,fname);
		strcat(buffer,extName);
		if ( stricmp(extName,".bmp")==0 )
			format = IFMT_BMP;
		else if ( stricmp(extName, ".jpg")==0 || stricmp(extName, ".jpeg")==0 )
			format = IFMT_JPEG;
		else if ( stricmp(extName, ".yuv")==0 )		// LPNRTool会将原始yuv保存为BMP，所以这个格式其实对下载其实不存在的
			format = IFMT_YUV;						// 车牌识别程序需要根据帧长度判断图的宽度和高度，如果不符合任何合法比例，不进行处理
		IMAGE_HEADER_INITEX( hdr, IID_CAP, 0, 0, format, 0, (LPCTSTR)buffer, lsize );
		sock_write( pHvObj->sock, (char *)&hdr, sizeof(hdr) );
		// image body
		while ( (len=read( fd, buffer, sizeof(buffer))) > 0 )
		{
			sock_write( pHvObj->sock, buffer, len );
			lsize -= len;
		}
		close(fd);
	}
	return TRUE;
}
DLLAPI BOOL CALLTYPE LPNR_Lock(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_FUNCTION(pHvObj);
	//Mutex_Lock(pHvObj);
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_Unlock(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_FUNCTION(pHvObj);
	//Mutex_Unlock(pHvObj);
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_SyncTime(HANDLE h)
{
	DataHeader header;
#ifdef WIN32
	SYSTEMTIME tm;
#else
	struct tm *ptm;
	struct timeval tv;
#endif
	PHVOBJ pHvObj = (PHVOBJ)h;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION(pHvObj);

	if ( pHvObj->enLink != NORMAL )
	{
		TRACE_LOG(pHvObj,"\t- 失败，识别机离线!\n");
		return FALSE;
	}
	// set camera system time
#ifdef WIN32
	// set camera system time
	GetLocalTime( &tm );
	TIME_HEADER_INIT( header, tm.wYear, tm.wMonth, tm.wDay, tm.wHour, tm.wMinute, tm.wSecond, tm.wMilliseconds );
#else
	gettimeofday( &tv, NULL );
	// set camera system time
	ptm = localtime(&tv.tv_sec);
	TIME_HEADER_INIT( header, ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
						 ptm->tm_hour, ptm->tm_min, ptm->tm_sec, tv.tv_usec/1000 );
#endif
	sock_write( pHvObj->sock, (char *)&header, sizeof(header) );	
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_ResetHeartBeat(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION(pHvObj);
	pHvObj->tickLastHeared = 0;
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_GetMachineIP(HANDLE h, LPSTR strIP)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION(pHvObj);

	strcpy( strIP, pHvObj->strIP	);
	return TRUE;
}


DLLAPI LPCTSTR CALLTYPE LPNR_GetCameraLabel(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
			return "";
	TRACE_FUNCTION_FMT(pHvObj, "label = %s\n", pHvObj->label);

	return (LPCTSTR)pHvObj->label;
}

DLLAPI BOOL CALLTYPE LPNR_GetModelAndSensor(HANDLE h, int *modelId, int *sensorId)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj, "model = %d, sensor=%d\n", pHvObj->verAttr.param[1] >> 16, pHvObj->verAttr.param[1] & 0x0000ffff);

	*modelId = pHvObj->verAttr.param[1] >> 16;
	*sensorId = pHvObj->verAttr.param[1] & 0x0000ffff;
	return TRUE;
}
DLLAPI BOOL CALLTYPE LPNR_GetVersion(HANDLE h, DWORD *version, int *algver)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj, "version = 0x%x, algorithm verion=%d:%d\n", pHvObj->verAttr.param[0], pHvObj->verAttr.algver/256, pHvObj->verAttr.algver &0xff);

	*version = pHvObj->verAttr.param[0];
	*algver = pHvObj->verAttr.algver;
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_GetImageResolution(HANDLE h, int nSizeCode, int *cx, int *cy)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj, "nSizeCode = %d\n", nSizeCode);
	if ( nSizeCode <= 0 || 3 < nSizeCode )
	{
		TRACE_LOG(pHvObj, "--> Invalid Size Code %d!\n", nSizeCode);
		return FALSE;
	}
	nSizeCode--;
	*cx = pHvObj->szImage[nSizeCode].cx;
	*cy =  pHvObj->szImage[nSizeCode].cy;
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_GetCameraTime(HANDLE h, INT64 *i64time)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION(pHvObj);
	*i64time = pHvObj->camLongTime;
	return TRUE;
}

DLLAPI int CALLTYPE LPNR_GetCameraTimeDiff(HANDLE h, int *sec)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION(pHvObj);
	*sec = pHvObj->secDiff;
	return TRUE;
}


DLLAPI BOOL CALLTYPE LPNR_GetCapability(HANDLE h, DWORD *cap)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	*cap = pHvObj->verAttr.param[2];
	return TRUE;
}

DLLAPI int	 CALLTYPE LPNR_GetTriggerSource(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;

	if ( !IS_VALID_OBJ(pHvObj) )
		return -1;

	TRACE_FUNCTION(pHvObj);

	return (int)pHvObj->enTriggerSource;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UDP operation
DLLAPI BOOL CALLTYPE LPNR_QueryPlate( IN LPCTSTR strIP, OUT LPSTR strPlate, int tout )
{
	DataHeader header, rplyHeader;
	int len;
	BOOL bFound = FALSE;
	SOCKET sock;
	DWORD dwIP;

	if ( (dwIP=inet_addr(strIP)) == INADDR_NONE )
	{
		return FALSE;
	}
	sock = sock_udp_open();
	if ( sock == INVALID_SOCKET )
	{
		return FALSE;
	}
	//sock_udp_timeout( sock, tout );
	QUERY_HEADER_INIT( header, QID_PKPLATE );
	len = sock_udp_send0( sock, dwIP, PORT_SEARCH, (char *)&header, sizeof(DataHeader) );
	if ( sock_dataready(sock, tout) &&
	    (len=sock_udp_recv( sock, (char *)&rplyHeader, sizeof(DataHeader), &dwIP)) == sizeof(DataHeader) )
	{
		if ( rplyHeader.DataType == DTYP_REPLY && (rplyHeader.DataId & 0xffff) == QID_PKPLATE &&  (rplyHeader.DataId>>16)==0 )
		{
			strcpy(strPlate, rplyHeader.plateInfo.chNum);
			bFound = TRUE;
		}
		else
		{
			; // TRACE_LOG(NULL, "车位相机应答帧类型(0x%x)或是ID(%d)错误！\n", rplyHeader.DataType, rplyHeader.DataId);
		}
	}
	else
	{
		; // TRACE_LOG(NULL, "--> 车位相机应答超时!\n");
	}

	sock_close( sock );
	return bFound;
}

int CALLTYPE LPNR_QueryPicture(IN LPCTSTR strIP, OUT BYTE *imgbuf, int size, int imgid, int format, int tout)
{
	DataHeader header, rplyHeader;
	int len, total_len=0;
	BOOL bFound = FALSE;
	SOCKET sock;
	DWORD dwIP;
	DWORD tick_tout;
	int   block_get = 0;
	int   total_block = 1000000;
	char  udp_buffer[65536];

	if ( (dwIP=inet_addr(strIP)) == INADDR_NONE )
	{
		return -1;
	}
	if ( imgid < IID_CAP || IID_HEX < imgid )
	{
		return -1;
	}
	if ( format != IFMT_JPEG && format != IFMT_BMP )
		return -1;
	if ( tout < 1000 )  tout = 1000;		// 最少给1秒

	sock = sock_udp_open();
	if ( sock == INVALID_SOCKET )
	{
		return -1;
	}
	//sock_udp_timeout( sock, tout );
	QUERY_HEADER_INIT( header, QID_PICTURE );
	header.dataAttr.val[0] = imgid;
	header.dataAttr.val[1] = format;

	len = sock_udp_send0( sock, dwIP, PORT_SEARCH, (char *)&header, sizeof(DataHeader) );
	tick_tout = GetTickCount() + tout;
	while ( block_get < total_block && GetTickCount() < tick_tout )
	{
		if ( sock_dataready(sock, 100) )
		{
			len = sock_udp_recv( sock, udp_buffer, sizeof(udp_buffer), &dwIP);
			if ( len > sizeof(DataHeader) )
			{
				int offset;
				memcpy(&rplyHeader, udp_buffer, sizeof(DataHeader) );
				offset = rplyHeader.dataAttr.val[3];
				total_len = rplyHeader.dataAttr.val[2];
				total_block = rplyHeader.dataAttr.val[0];
				// 接收image的buffer不够大或是数据错误，或是发送的UDP包超过64K
				if ( total_len > size || offset+len > total_len || len != sizeof(DataHeader)+rplyHeader.size )		
					break;
				memcpy(imgbuf+offset, udp_buffer+sizeof(DataHeader), rplyHeader.size);
				block_get++;
			}
		}
	}
	sock_close( sock );
	return block_get==total_block ? total_len : 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Ext. GPIO operations
DLLAPI BOOL CALLTYPE LPNR_GetExtDIO(HANDLE h, short dio_val[])
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION(pHvObj);

	dio_val[0] = pHvObj->dio_val[0];
	dio_val[1] = pHvObj->dio_val[1];
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_SetExtDO(HANDLE h, int pin, int value)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader ctrlHdr;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) ||  pin < 0 || pin >= 4 )
		return FALSE;
	
	TRACE_FUNCTION_FMT(pHvObj, "pin=%d, value=%d\n", pin, value);

	CTRL_HEADER_INIT( ctrlHdr, CTRL_WRITEEXTDO, 1);
	ctrlHdr.ctrlAttr.option[0] = pin;
	ctrlHdr.ctrlAttr.option[1] = value;
	pHvObj->acked_ctrl_id = 0;
	len = sock_write( pHvObj->sock, (const char*)&ctrlHdr, sizeof(ctrlHdr) );
	return len==sizeof(DataHeader);
}

DLLAPI BOOL CALLTYPE LPNR_PulseOut(HANDLE h, int pin, int period, int count)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader ctrlHdr;
	int len, param;

	if ( !IS_VALID_OBJ(pHvObj) ||  pin < 0 || pin >= 4 )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj, "pin=%d, period=%d, count=%d\n", pin, period, count);

	param = pin + ((count & 0xff) << 8) + ((period & 0xffff) << 16);
	pHvObj->acked_ctrl_id = 0;
	CTRL_HEADER_INIT( ctrlHdr, CTRL_DOPULSE, param);
	len = sock_write( pHvObj->sock, (const char*)&ctrlHdr, sizeof(ctrlHdr) );
	return len==sizeof(DataHeader);
}

DLLAPI BOOL CALLTYPE LPNR_LightCtrl(HANDLE h, int onoff, int msec)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader ctrlHdr;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj, "OnOff=%d, duration=%d (msec)\n", onoff, msec);

	CTRL_HEADER_INIT( ctrlHdr, CTRL_LIGHT, onoff);
	memcpy(ctrlHdr.ctrlAttr.option, &msec, 4);
	len = sock_write( pHvObj->sock, (const char*)&ctrlHdr, sizeof(ctrlHdr) );
	return len==sizeof(DataHeader);
}

DLLAPI BOOL CALLTYPE LPNR_IRCut(HANDLE h, int onoff)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader ctrlHdr;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj, "OnOff=%d\n", onoff);

	CTRL_HEADER_INIT( ctrlHdr, CTRL_IRCUT, onoff);
	len = sock_write( pHvObj->sock, (const char*)&ctrlHdr, sizeof(ctrlHdr) );
	return len==sizeof(DataHeader);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Video OSD and stream controls
DLLAPI BOOL CALLTYPE LPNR_SetOSDTimeStamp(HANDLE h, BOOL bEnable, int x, int y)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj, "Enable=%s, x=%d, y=%d\n", YESNO(bEnable), x, y);

	if ( bEnable )
		pHvObj->osdConf.enabler |= OSD_EN_TIMESTAMP;
	else
		pHvObj->osdConf.enabler &= ~OSD_EN_TIMESTAMP;
	OSD_HEADER_INIT(header, &pHvObj->osdConf);
	header.size = sizeof(OSDPayload);
	if ( x!=0 && y!=0 )
		pHvObj->osdPayload.enabler |= OSD_EN_TIMESTAMP;
	else
		pHvObj->osdPayload.enabler &= ~OSD_EN_TIMESTAMP;
	pHvObj->osdPayload.x[OSDID_TIMESTAMP] = x;
	pHvObj->osdPayload.y[OSDID_TIMESTAMP] = y;
	len = sock_write( pHvObj->sock, (const char*)&header, sizeof(header) );
	len += sock_write( pHvObj->sock, (const char*)&pHvObj->osdPayload, sizeof(OSDPayload) );
	return len==sizeof(DataHeader) + sizeof(OSDPayload);
}

DLLAPI BOOL CALLTYPE LPNR_SetOSDLabel(HANDLE h, BOOL bEnable, const char *label)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj, "Enable=%s, label=%s\n", YESNO(bEnable), label);

	if ( label != NULL )
	{
		memset( &header, 0, sizeof(header) );
		header.DataType = DTYP_EXCONF;
		header.DataId = 0;
		memcpy(pHvObj->extParamConf.label, label, SZ_LABEL);
		pHvObj->extParamConf.label[SZ_LABEL-1] = '\0';
		memcpy( &header.extParamConf, &pHvObj->extParamConf, sizeof(ExtParamConf) );
		sock_write (pHvObj->sock, (char *)&header, sizeof(header) );
	}

	if ( bEnable )
		pHvObj->osdConf.enabler |= OSD_EN_LABEL;
	else
		pHvObj->osdConf.enabler &= ~OSD_EN_LABEL;
	OSD_HEADER_INIT(header, &pHvObj->osdConf);
	len = sock_write( pHvObj->sock, (const char*)&header, sizeof(header) );
	return len > 0;
}

DLLAPI BOOL CALLTYPE LPNR_SetOSDLogo(HANDLE h, BOOL bEnable)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj, "Enable=%s\n", bEnable ? "yes" : "no");

	if ( bEnable )
		pHvObj->osdConf.enabler |= OSD_EN_LOGO;
	else
		pHvObj->osdConf.enabler &= ~OSD_EN_LOGO;
	OSD_HEADER_INIT(header, &pHvObj->osdConf);
	len = sock_write( pHvObj->sock, (const char*)&header, sizeof(header) );
	return len == sizeof(header);
}

DLLAPI BOOL CALLTYPE LPNR_SetOSDROI(HANDLE h, BOOL bEnable)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj, "Enable=%s\n", bEnable ? "yes" : "no");

	if ( bEnable )
		pHvObj->osdConf.enabler |= OSD_EN_ROI;
	else
		pHvObj->osdConf.enabler &= ~OSD_EN_ROI;
	OSD_HEADER_INIT(header, &pHvObj->osdConf);
	len = sock_write( pHvObj->sock, (const char*)&header, sizeof(header) );
	return len == sizeof(header);
}

DLLAPI BOOL CALLTYPE LPNR_SetOSDPlate(HANDLE h, BOOL bEnable, int loc, int dwell, BOOL bFadeOut)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	
	TRACE_FUNCTION_FMT(pHvObj, "Enable=%s, loc=%d, dwell=%d (sec), FadeOut=%s\n", YESNO(bEnable), dwell, YESNO(bFadeOut));

	if ( bEnable )
	{
		pHvObj->osdConf.enabler |= OSD_EN_PLATE;
		if ( loc != 1 && loc != 2 && loc != 3 ) loc = 2;		// 默认是中下
		pHvObj->osdConf.x = -loc - 6;		// 1~3 --> -7 ~ -9
		pHvObj->osdConf.param[OSD_PARM_DWELL] = dwell;
		pHvObj->osdConf.param[OSD_PARM_FADEOUT] = bFadeOut ? 1 : 0;
	}
	else
		pHvObj->osdConf.enabler &= ~OSD_EN_PLATE;
	OSD_HEADER_INIT(header, &pHvObj->osdConf);
	len = sock_write( pHvObj->sock, (const char*)&header, sizeof(header) );
	return len == sizeof(header);
}

DLLAPI BOOL CALLTYPE LPNR_COM_init(HANDLE h, int Speed)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader ctrlHdr;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj, "speed=%d\n", Speed);

	CTRL_HEADER_INIT( ctrlHdr, CTRL_COMSET, Speed);
	len = sock_write( pHvObj->sock, (const char*)&ctrlHdr, sizeof(ctrlHdr) );
	return len==sizeof(DataHeader);
}

DLLAPI BOOL CALLTYPE LPNR_UserOSDOn(HANDLE h, int x, int y, int align, int fontsz, int text_color, int opacity, const char *text)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj, "x=%d, x=%d, align=%d, fontsz=%d, text_color=0x%x, opacity=%d, text=%s\n", x, y, align, fontsz, opacity, text);

	OSDTEXT_HEADER_INIT(header, NULL);
	header.osdText.x  = x;
	header.osdText.y  = y;
	header.osdText.nFontId = 0;		// 宋体
	if ( fontsz==24 || fontsz==32 || fontsz==40 || fontsz==48 || fontsz==56 || fontsz==64 || fontsz==80 || fontsz==96 )
		header.osdText.nFontSize = fontsz;
	else
		header.osdText.nFontSize = 32;
	header.osdText.RGBForgrund = text_color;
	header.osdText.alpha[0] = opacity * 128 / 100;		// 0~100 --> 0 ~ 128
	header.osdText.alpha[1] = 0;
	if ( 0 < align && align < 4 )
		header.osdText.param[OSD_PARM_ALIGN] = align - 1;		// 1 ~ 3 --> 0 ~ 2
	header.osdText.param[OSD_PARM_SCALE] = 1;
	header.size = strlen(text) + 1;
	len = sock_write( pHvObj->sock, (const char *)&header, sizeof(header) );
	len += sock_write(pHvObj->sock, text, strlen(text)+1);
	return len == sizeof(header) + header.size;
}

DLLAPI BOOL CALLTYPE LPNR_UserOSDOff(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader ctrlHdr;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION(pHvObj);

	OSDTEXT_HEADER_INIT(ctrlHdr, NULL);
	sock_write( pHvObj->sock, (const char *)&ctrlHdr, sizeof(ctrlHdr) );
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_SetStream(HANDLE h,  int encoder, BOOL bSmallMajor, BOOL bSmallMinor)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	int  nStreamSize = 0;
	int  len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj, "encoder=%s, small-major=%s, small-minor=%s\n", encoder?"H265":"H264", YESNO(bSmallMajor), YESNO(bSmallMinor));

	if ( bSmallMinor )
		nStreamSize = 1;
	if ( bSmallMajor )
		nStreamSize |= 0x02;

	pHvObj->h264Conf.u8Param[1] = nStreamSize;
	pHvObj->h264Conf.u8Param[2] = encoder;

	H264_HEADER_INIT( header, pHvObj->h264Conf);
	len = sock_write( pHvObj->sock, (const char *)&header, sizeof(header) );
	return len==sizeof(header);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Captured image operations
DLLAPI int CALLTYPE LPNR_GetCapturedImageSize(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	TRACE_FUNCTION_FMT(pHvObj, "size=%d\n", pHvObj->szCapImage2[IMGID_CAPFULL]);
	return pHvObj->szCapImage2[IMGID_CAPFULL];
}

DLLAPI int CALLTYPE LPNR_GetCapturedImage(HANDLE h, void *buf)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	TRACE_FUNCTION_FMT(pHvObj,"应用程序获取抓拍图片 size=%d\n", pHvObj->szCapImage2[IMGID_CAPFULL] );
	//Mutex_Lock(pHvObj);
	if ( pHvObj->szCapImage2[IMGID_CAPFULL] > 0 )
		memcpy(buf, pHvObj->pCapImage2[IMGID_CAPFULL], pHvObj->szCapImage2[IMGID_CAPFULL] );
	//Mutex_Unlock(pHvObj);
	return pHvObj->szCapImage2[IMGID_CAPFULL];
}


DLLAPI int CALLTYPE LPNR_GetHeadImageSize(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	TRACE_FUNCTION_FMT(pHvObj, "size=%d\n", pHvObj->szCapImage2[IMGID_CAPHEAD]);
	return pHvObj->szCapImage2[IMGID_CAPHEAD];
}

DLLAPI int CALLTYPE LPNR_GetHeadImage(HANDLE h, void *buf)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	TRACE_FUNCTION_FMT(pHvObj,"应用程序获取车头图片 size=%d\n", pHvObj->szCapImage2[IMGID_CAPHEAD] );
	//Mutex_Lock(pHvObj);
	if ( pHvObj->szCapImage2[IMGID_CAPHEAD]  > 0 )
		memcpy(buf, pHvObj->pCapImage2[IMGID_CAPHEAD] , pHvObj->szCapImage2[IMGID_CAPHEAD]  );
	//Mutex_Unlock(pHvObj);
	return pHvObj->szCapImage2[IMGID_CAPHEAD] ;
}


DLLAPI int CALLTYPE LPNR_GetMiddleImageSize(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	TRACE_FUNCTION_FMT(pHvObj, "size=%d\n", pHvObj->szCapImage2[IMGID_CAPMIDDLE]);
	return pHvObj->szCapImage2[IMGID_CAPMIDDLE];
}

DLLAPI int CALLTYPE LPNR_GetMiddleImage(HANDLE h, void *buf)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	TRACE_FUNCTION_FMT(pHvObj,"应用程序获取4/9图片 size=%d\n", pHvObj->szCapImage2[IMGID_CAPMIDDLE] );
	//Mutex_Lock(pHvObj);
	if ( pHvObj->szCapImage2[IMGID_CAPMIDDLE] > 0 )
		memcpy(buf, pHvObj->pCapImage2[IMGID_CAPMIDDLE] , pHvObj->szCapImage2[IMGID_CAPMIDDLE]  );
	//Mutex_Unlock(pHvObj);
	return pHvObj->szCapImage2[IMGID_CAPMIDDLE] ;
}


DLLAPI int CALLTYPE LPNR_GetQuadImageSize(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	TRACE_FUNCTION_FMT(pHvObj, "size=%d\n", pHvObj->szCapImage2[IMGID_CAPSMALL]);
	return pHvObj->szCapImage2[IMGID_CAPSMALL];
}

DLLAPI int CALLTYPE LPNR_GetQuadImage(HANDLE h, void *buf)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	TRACE_FUNCTION_FMT(pHvObj,"应用程序获取1/4图片 size=%d\n", pHvObj->szCapImage2[IMGID_CAPSMALL]  );
	//Mutex_Lock(pHvObj);
	if ( pHvObj->szCapImage2[IMGID_CAPSMALL]  > 0 )
		memcpy(buf, pHvObj->pCapImage2[IMGID_CAPSMALL] , pHvObj->szCapImage2[IMGID_CAPSMALL]  );
	//Mutex_Unlock(pHvObj);
	return pHvObj->szCapImage2[IMGID_CAPSMALL] ;
}


DLLAPI BOOL CALLTYPE LPNR_SetCaptureImage(HANDLE h, BOOL bDisFull, BOOL bEnMidlle, BOOL bEnSmall, BOOL bEnHead, BOOL bEnTiny)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj, "Disable-full=%s, Enable-middle=%s, Enable-Small=%s, Enable-Head=%s, Enable-Tiny=%s\n",
				YESNO(bDisFull), YESNO(bEnMidlle), YESNO(bEnSmall), YESNO(bEnHead), YESNO(bEnTiny) );

	memset( &header, 0, sizeof(header) );
	header.DataType = DTYP_CONF;
	header.DataId = 0;
	memcpy( &header.paramConf, &pHvObj->paramConf, sizeof(ParamConf) );

	if (bDisFull)
		header.paramConf.enabler |= PARM_DE_FULLIMG;
	else
		header.paramConf.enabler &= ~PARM_DE_FULLIMG;

	if ( bEnMidlle )
			header.paramConf.enabler |= PARM_EN_T3RDIMG;
	else
			header.paramConf.enabler &= ~PARM_EN_T3RDIMG;

	if ( bEnHead )
		header.paramConf.enabler  |= PARM_EN_HEADIMG;
	else
		header.paramConf.enabler  &= ~PARM_EN_HEADIMG;

	if ( bEnSmall )
			header.paramConf.enabler |= PARM_EN_QUADIMG;
	else
			header.paramConf.enabler &= ~PARM_EN_QUADIMG;

	if ( bEnTiny )
			header.paramConf.enabler |= PARM_EN_HEXIMG;
	else
			header.paramConf.enabler &= ~PARM_EN_HEXIMG;

	len = sock_write (pHvObj->sock, (char *)&header, sizeof(header) );

	return len==sizeof(header);
}

DLLAPI BOOL CALLTYPE LPNR_GetCaptureImage(HANDLE h, BOOL *bDisFull, BOOL *bEnMidlle, BOOL *bEnSmall, BOOL *bEnHead, BOOL *bEnTiny)
{
	PHVOBJ pHvObj = (PHVOBJ)h;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	*bDisFull = (pHvObj->paramConf.enabler & PARM_DE_FULLIMG) != 0;
	*bEnMidlle = (pHvObj->paramConf.enabler & PARM_EN_T3RDIMG) != 0;
	*bEnHead =  (pHvObj->paramConf.enabler & PARM_EN_HEADIMG) != 0;
	*bEnSmall =  (pHvObj->paramConf.enabler & PARM_EN_QUADIMG) != 0;
	*bEnTiny = (pHvObj->paramConf.enabler & PARM_EN_HEXIMG) != 0;
	return TRUE;
}

static int GetMaxSizeImage(PHVOBJ pHvObj)
{
	int max_size = 0;
	int i, index=-1;
	for(i=IMGID_CAPFULL; i<IMGID_BUTT; i++)
		if ( pHvObj->szCapImage2[i] > max_size )
		{
			max_size = pHvObj->szCapImage2[i];
			index = i;
		}
	return index;
}

static int GetMinSizeImage(PHVOBJ pHvObj)
{
	int min_size = 0x7fffffff;
	int i, index=-1;
	for(i=IMGID_CAPFULL; i<IMGID_BUTT; i++)
		if ( pHvObj->szCapImage2[i]>0 && pHvObj->szCapImage2[i] < min_size )
		{
			min_size = pHvObj->szCapImage2[i];
			index = i;
		}
	return index;
}

DLLAPI int CALLTYPE LPNR_GetSelectedImageSize(HANDLE h, int imageId)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	int  size = 0;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;

	TRACE_FUNCTION_FMT(pHvObj,"Image-Id=%d\n", imageId);

	if ( imageId==IMGID_CAPLARGEST )
		imageId = GetMaxSizeImage(pHvObj);
	else if ( imageId==IMGID_CAPSMALLEST )
		imageId = GetMinSizeImage(pHvObj);
	else if ( IMGID_CROPIMAGE(1) <= imageId && imageId <= IMGID_CROPIMAGE(10) )
	{
		int index = imageId - IMGID_CROPIMAGE(1);
		size = pHvObj->szCropImage[index];
	}
	else if ( 0 <= imageId &&  imageId < IMGID_BUTT )
		size = pHvObj->szCapImage2[imageId];
	return size;
}

DLLAPI int CALLTYPE LPNR_GetSelectedImage(HANDLE h, int imageId, void *buf)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	int  size=0;

	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;

	TRACE_FUNCTION_FMT(pHvObj,"Image-Id=%d\n", imageId);

	if ( imageId==IMGID_CAPLARGEST )
	{
		imageId = GetMaxSizeImage(pHvObj);
		if ( imageId != -1 )
		{
			size = pHvObj->szCapImage2[imageId];
			memcpy(buf, pHvObj->pCapImage2[imageId], size );
		}
	}
	else if ( imageId==IMGID_CAPSMALLEST )
	{
		imageId = GetMinSizeImage(pHvObj);
		if ( imageId != -1 )
		{
			size = pHvObj->szCapImage2[imageId];
			memcpy(buf, pHvObj->pCapImage2[imageId], size );
		}
	}
	else if ( IMGID_CROPIMAGE(1) <= imageId && imageId <= IMGID_CROPIMAGE(10) )
	{
		int index = imageId - IMGID_CROPIMAGE(1);
		if ( pHvObj->pCropImage[index] && pHvObj->szCropImage[index] > 0 )
		{
			memcpy(buf, pHvObj->pCropImage[index], pHvObj->szCropImage[index] );
			//FREE_BUFFER(pHvObj->pCropImage[index]);
			//size = pHvObj->szCropImage[index];
			//pHvObj->szCropImage[index] = 0;
		}
	}
	else if ( 0 <= imageId && imageId < IMGID_BUTT )
	{
		memcpy(buf, pHvObj->pCapImage2[imageId], pHvObj->szCapImage2[imageId]);
		size = pHvObj->szCapImage2[imageId];
	}
	return size;
}

DLLAPI const void * CALLTYPE LPNR_GetSelectedImagePtr(HANDLE h, int imageId, int *imgsize)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	int  size=0;
	const void *imgptr = NULL;

	if ( !IS_VALID_OBJ(pHvObj) )
		return NULL;

	TRACE_FUNCTION_FMT(pHvObj,"Image-Id=%d\n", imageId);

	if ( imageId==IMGID_CAPLARGEST )
	{
		imageId = GetMaxSizeImage(pHvObj);
		if ( imageId != -1 )
		{
			imgptr = pHvObj->pCapImage2[imageId];
			size = pHvObj->szCapImage2[imageId];
		}
	}
	else if ( imageId==IMGID_CAPSMALLEST )
	{
		imageId = GetMinSizeImage(pHvObj);
		if ( imageId != -1 )
		{
			imgptr = pHvObj->pCapImage2[imageId];
			size = pHvObj->szCapImage2[imageId];
		}
	}
	else if ( IMGID_CROPIMAGE(1) <= imageId && imageId <= IMGID_CROPIMAGE(10) )
	{
		int index = imageId - IMGID_CROPIMAGE(1);
		imgptr = pHvObj->pCropImage[index];
		size = pHvObj->szCropImage[index];
	}
	if ( imgsize ) *imgsize = size;
	return imgptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Transparent COM port operations
DLLAPI BOOL CALLTYPE LPNR_COM_aync(HANDLE h, BOOL bEnable)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader ctrlHdr;
	int len, param = bEnable ? 1 : 0;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_FUNCTION_FMT(pHvObj, "Enable=%s\n", YESNO(bEnable));

	CTRL_HEADER_INIT( ctrlHdr, CTRL_COMASYN, param);
	len = sock_write( pHvObj->sock, (const char*)&ctrlHdr, sizeof(ctrlHdr) );
	return len==sizeof(DataHeader);
}

DLLAPI BOOL CALLTYPE LPNR_COM_send(HANDLE h, const char *data, int size)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader ctrlHdr;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	CTRL_HEADER_INIT( ctrlHdr, CTRL_COMTX, 0);
	ctrlHdr.size = size;
	len = sock_write( pHvObj->sock, (const char*)&ctrlHdr, sizeof(ctrlHdr) );
	len += sock_write(pHvObj->sock, data, size);
	return len==sizeof(DataHeader);
}

DLLAPI int CALLTYPE LPNR_COM_iqueue(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;

	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	return RingElements(pHvObj);
}

DLLAPI int CALLTYPE LPNR_COM_peek(HANDLE h, char *RxData, int size)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	int  nb, i, pos;

	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;

	Ring_Lock(pHvObj);
	nb = RingElements(pHvObj);
	if ( size > nb )
		size = nb;
	pos = pHvObj->ring_head;
	for(i=0; i<nb; i++)
	{
		RxData[i] = pHvObj->ring_buf[pos];
		pos = NextPosit(pHvObj,pos);
	}
	Ring_Unlock(pHvObj);
	return nb;
}

DLLAPI int CALLTYPE LPNR_COM_read(HANDLE h, char *RxData, int size)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	int  nb, i;

	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;

	Ring_Lock(pHvObj);
	nb = RingElements(pHvObj);
	if ( size > nb )
		size = nb;
	for(i=0; i<nb; i++)
	{
		RxData[i] = pHvObj->ring_buf[pHvObj->ring_head];
		pHvObj->ring_head = NextHeadPosit(pHvObj);
	}
	Ring_Unlock(pHvObj);
	return nb;
}

DLLAPI int CALLTYPE LPNR_COM_remove(HANDLE h, int size)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	int  nb, i;

	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;

	Ring_Lock(pHvObj);
	nb = RingElements(pHvObj);
	if ( size > nb )
		size = nb;
	for(i=0; i<nb; i++)
		pHvObj->ring_head = NextHeadPosit(pHvObj);
	Ring_Unlock(pHvObj);
	return nb;
}

DLLAPI BOOL CALLTYPE LPNR_COM_clear(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	Ring_Lock(pHvObj);
	pHvObj->ring_head = pHvObj->ring_tail = 0;
	Ring_Unlock(pHvObj);
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LOG operation
DLLAPI BOOL CALLTYPE LPNR_EnableLog(HANDLE h, BOOL bEnable)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( h==NULL )
		__gbEnableLog = bEnable;
	else if ( IS_VALID_OBJ(pHvObj) )
	{
		pHvObj->bLogEnable = bEnable;
		TRACE_FUNCTION_FMT(pHvObj, "Enable=%s\n", YESNO(bEnable));
		MLOG_ENABLE(pHvObj->hLog, bEnable);
	}
	else
		return FALSE;
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_SetLogPath(HANDLE h, const char *Dir)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( h==NULL )
		MLOG_SETPATH(NULL, Dir);
	else if ( IS_VALID_OBJ(pHvObj) )
	{
		TRACE_FUNCTION_FMT(pHvObj, "Dir=%s\n", Dir);
		MLOG_SETPATH(pHvObj->hLog, Dir);
	}
	else
		return FALSE;
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_SetLogSizeAndCount(HANDLE h, int size, int count)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	
	TRACE_FUNCTION_FMT(pHvObj, "size=%d (KB), count=%d\n", size, count);

	MLOG_SETLIMITCNT(pHvObj->hLog, size, count);
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_UserLog(HANDLE h, const char *fmt,...)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	va_list va;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	va_start( va, fmt );
#ifdef ENABLE_LOG		// don't know why, if not enabled, this macro expansion will cause compiler error
	MTRACE_VLOG(pHvObj->hLog, fmt, va);
#endif
	va_end( va );

	return TRUE;
}

static int file_type(int fd)
{
	unsigned char header[10];
	if (fd < 0)
		return IFMT_JPEG;
	read(fd, header, sizeof(header));
	if (header[0] == 'B' && header[1] == 'M')
		return IFMT_BMP;
	return IFMT_JPEG;
}

DLLAPI BOOL CALLTYPE LPNR_UploadRecognizeImage(HANDLE h, const char *strFile)
{  
	int fd;
	struct stat st;
	long lsize;
	int imgType = IFMT_JPEG;

	PHVOBJ pHvObj = (PHVOBJ)h; 
	if (!IS_VALID_OBJ(pHvObj))
		return FALSE;
	int m_sock = pHvObj->sock;
#if defined WIN32 || defined _WIN64
	if (stat(strFile, &st) == 0 && (lsize = st.st_size)>0 && (fd = open(strFile, O_RDONLY | O_BINARY)) != -1)
#else 
	if (stat(strFile, &st) == 0 && (lsize = st.st_size)>0 && (fd = open(strFile, O_RDONLY )) != -1)
#endif
	{ 
		DataHeader hdr;
		char fname[_MAX_FNAME];
		char extName[_MAX_EXT];
		char buffer[1024];
		int len;
		_splitpath(strFile, NULL, NULL, fname, extName);
		strcpy(buffer, fname);
		strcat(buffer, extName);
		imgType = file_type(fd);
		IMAGE_HEADER_INITEX(hdr, IID_CAP, 0, 0, imgType, 0, (LPCTSTR)buffer, lsize);
		lseek(fd, 0, 0);
		sock_write(m_sock, (char *)&hdr, sizeof(hdr)); 
		while ((len = read(fd, buffer, sizeof(buffer))) > 0)
		{
			sock_write(m_sock, buffer, len);
			lsize -= len;
		}
		close(fd);
		return TRUE;
	} 
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WORKING THREAD
#define IMG_BUFSIZE	(1920*1080*3)		// 6MB

void Handle_Socket_Error(PHVOBJ pHvObj)
{
	sock_close( pHvObj->sock );
	pHvObj->sock = INVALID_SOCKET;
	pHvObj->enLink = DISCONNECT;
	pHvObj->enOper = OP_IDLE;
	ReleaseData(pHvObj,3);
	NoticeHostEvent( pHvObj, EVT_OFFLINE );
}

static void ParseCameraTimeHeader(PHVOBJ pHvObj, TimeAttr *pTimeAttr)
{
	_longtime lt_now = timeGetLongTime();

	pHvObj->camTime.year = pTimeAttr->year;
	pHvObj->camTime.month = pTimeAttr->month;
	pHvObj->camTime.day = pTimeAttr->day;
	pHvObj->camTime.hour = pTimeAttr->hour;
	pHvObj->camTime.minute = pTimeAttr->minute;
	pHvObj->camTime.second = pTimeAttr->second;
	pHvObj->camTime.msec = pTimeAttr->msec;
	pHvObj->camLongTime = timeLongTimeFromCTM(&pHvObj->camTime);
	pHvObj->secDiff = (int)((lt_now - pHvObj->camLongTime)/1000);
	pHvObj->szImage[0].cx = pTimeAttr->resv[0];
	pHvObj->szImage[0].cy = pTimeAttr->resv[1];
	pHvObj->szImage[1].cx = pTimeAttr->resv[2];
	pHvObj->szImage[1].cy = pTimeAttr->resv[3];
	pHvObj->szImage[2].cx = pTimeAttr->resv[4];
	pHvObj->szImage[2].cy = pTimeAttr->resv[5];
}

THREADCALL lpnr_workthread_fxc(PVOID lpParameter)
{
	PHVOBJ pHvObj = (PHVOBJ) lpParameter;
	PVOID payload = malloc( IMG_BUFSIZE );
	int	   rlen;
	long   imgWidth, imgHeight, imgType;
	fd_set set;
	struct timeval val;
	int ret;
	DataHeader  dataHeader;
	DataHeader header;
	SOCKET m_sock = INVALID_SOCKET;
	BYTE soh[3] = { 0xaa, 0xbb, 0xcc };
	char *liveFrame = NULL;
	int	  nLiveSize = 0;
	int	  nLiveAlloc = 0;
	DWORD tickHeartBeat=0;
	DWORD tickStartProc = 0;

	TRACE_LOG(pHvObj,"<<< ===================   W O R K   T H R E A D   S T A R T   =====================>>>\n");
	pHvObj->status = STAT_RUN;
	pHvObj->tickLastHeared = 0;
	for(; pHvObj->status==STAT_RUN; )
	{
		if ( m_sock == INVALID_SOCKET )
		{
			TRACE_LOG(pHvObj,"reconnect camera IP %s port 6008...\n", pHvObj->strIP );
			pHvObj->enLink = RECONNECT;
			m_sock = sock_connect( pHvObj->strIP, 6008 );
			if ( m_sock != INVALID_SOCKET )
			{
				DataHeader header;
				pHvObj->enLink = NORMAL;
				pHvObj->sock = m_sock;
				LPNR_SyncTime((HANDLE)pHvObj);
				// disable intermidiate files
				CTRL_HEADER_INIT( header, CTRL_IMLEVEL, 0  );
				sock_write( m_sock, (char *)&header, sizeof(header) );
				TRACE_LOG(pHvObj,"Connection established. download system time to camera.\n");
				NoticeHostEvent(pHvObj, EVT_ONLINE ); 
				pHvObj->tickLastHeared = GetTickCount();			// reset last heared time
				pHvObj->nLiveSize = 0;
			}
			else
			{
				Sleep(1000);
				continue;
			}
		}

		// check for sending heart-beat packet
		if ( GetTickCount() >= tickHeartBeat )
		{
			DataHeader hbeat;
			HBEAT_HEADER_INIT(hbeat);
			//TRACE_LOG(pHvObj,"send heartbeat packet to camera.\n");
			sock_write( m_sock, (const char *)&hbeat, sizeof(hbeat) );
			tickHeartBeat = GetTickCount() + 1000;
		}

		// check for input from camera
		FD_ZERO( &set );
		FD_SET( m_sock, &set );
		val.tv_sec = 0;
		val.tv_usec = 10000;
		ret = select( m_sock+1, &set, NULL, NULL, &val );
		if ( ret < 0 )
		{
			TRACE_LOG(pHvObj,"Select error, close and reconnect...wsaerror=%d\n", WSAGetLastError() );
			Handle_Socket_Error( pHvObj );
			m_sock = INVALID_SOCKET;
		}
		else if ( ret > 0 && FD_ISSET(m_sock, &set) )
		{
			int rc;
			if ( (rc=sock_read_n_bytes( m_sock, (char *)&dataHeader, sizeof(dataHeader) )) != sizeof(dataHeader) )
			{
				if ( rc<=0 )
				{
					TRACE_LOG(pHvObj,"Socket broken, close and reconnect...wsaerror=%d\n", WSAGetLastError() );
					Handle_Socket_Error( pHvObj );
					m_sock = INVALID_SOCKET;
					continue;
				}
				else
				{
					int nskip;
					TRACE_LOG(pHvObj, "读取数据帧头获得的长度是%d, 需要%d字节。重新同步到下一个帧头！\n",
							rc, sizeof(dataHeader) );
					nskip = sock_drain_until( m_sock, soh, 3 );
					TRACE_LOG(pHvObj,"--> 抛弃 %d 字节!\n", nskip);
				}
			}
			pHvObj->tickLastHeared = GetTickCount();
			if ( !IsValidHeader(dataHeader) )
			{
				int nskip;
				TRACE_LOG(pHvObj,"Invalid packet header received 0x%08X, re-sync to next header\n", dataHeader.DataType );
				nskip = sock_drain_until( m_sock, soh, 3 );
				TRACE_LOG(pHvObj,"--> skip %d bytes\n", nskip);
				continue;
			}	
			// read payload after header (if any)
			if ( dataHeader.size > 0 )
			{
				if ( dataHeader.size > IMG_BUFSIZE )
				{
					int nskip;
					// image size over buffer size ignore
					TRACE_LOG(pHvObj, "payload size %d is way too large. re-sync to next header\n", dataHeader.size);
					nskip = sock_drain_until( m_sock, soh, 3 );
					TRACE_LOG(pHvObj, "--> skip %d bytes\n", nskip);
					continue;
				}
				if ( (rlen=sock_read_n_bytes( m_sock, payload, dataHeader.size )) != dataHeader.size  )
				{
					int nskip;
					TRACE_LOG(pHvObj, "read payload DataType=0x%x, DataId=%d, expect %d bytes, only get %d bytes. --> ignored!\n", 
							dataHeader.DataType, dataHeader.DataId, dataHeader.size, rlen );
					nskip = sock_drain_until( m_sock, soh, 3 );
					TRACE_LOG(pHvObj,"--> drop this packet, skip %d bytes\n", nskip);
					//sock_close( m_sock );
					//m_sock = pHvObj->sock = INVALID_SOCKET;
					//pHvObj->enOper = OP_IDLE;
					continue;
				}
			}
			// receive a image file (JPEG or BMP)
			if ( dataHeader.DataType == DTYP_IMAGE )
			{
				// acknowledge it
				ACK_HEADER_INIT( header );
				sock_write( m_sock, (const char *)&header, sizeof(header) );
				// then process
				imgWidth = dataHeader.imgAttr.width;
				imgHeight = dataHeader.imgAttr.height;
				imgType = dataHeader.imgAttr.format;
				// IMAGE is a live frame
				if (dataHeader.DataId == IID_LIVE )
				{
					Mutex_Lock(pHvObj);
					if ( dataHeader.size > pHvObj->nLiveAlloc )
					{
						pHvObj->nLiveAlloc = (dataHeader.size + 1023)/1024 * 10240;		// roundup to 10K boundry
						pHvObj->pLiveImage = realloc( pHvObj->pLiveImage, pHvObj->nLiveAlloc );
					}
					pHvObj->nLiveSize = dataHeader.size;
					memcpy( pHvObj->pLiveImage, payload, dataHeader.size );
					Mutex_Unlock(pHvObj);
					NoticeHostEvent(pHvObj, EVT_LIVE );
				}
				else if ( dataHeader.DataId==IID_CROP )
				{
					int index = dataHeader.imgAttr.index;
					if ( 0 < index && index <= 10 )
					{
						Mutex_Lock(pHvObj);
						if ( pHvObj->szCropBuf[index-1] < dataHeader.size )
						{
							pHvObj->pCropImage[index-1] = realloc(pHvObj->pCropImage[index-1], dataHeader.size);
							pHvObj->szCropBuf[index-1] = dataHeader.size;
						}
						pHvObj->szCropImage[index-1] = dataHeader.size;
						memcpy( pHvObj->pCropImage[index-1], payload, dataHeader.size );
						Mutex_Unlock(pHvObj);
						TRACE_LOG(pHvObj,"==> 接收到裁剪抓拍图%d - %s (%d bytes)\n", index, dataHeader.imgAttr.basename, dataHeader.size);
						NoticeHostEvent(pHvObj, EVT_CROPIMG|(index<<16) );
					}
				}
				else // if (dataHeader.DataId != IID_LIVE ) - other output images
				{	
					// Processed image
					// printf("received processed image: Id=%d, size=%d\r\n", dataHeader.DataId, dataHeader.size );
					int index = IID2Index(dataHeader.DataId);
					if ( index != -1 && dataHeader.size > 0)
					{
						pHvObj->szCapImage[index] = dataHeader.size;
						pHvObj->pCapImage[index] = malloc( dataHeader.size );
						memcpy( pHvObj->pCapImage[index], payload, dataHeader.size );
						TRACE_LOG(pHvObj,"==> 接收到%s - %s (%d bytes) Image DataId=%d, Index=%d - ImageId=%d\n", 
									GetImageName(index), dataHeader.imgAttr.basename, dataHeader.size, dataHeader.DataId,dataHeader.imgAttr.index, index);
						if ( strcmp(dataHeader.imgAttr.basename,"vsnap.jpg") == 0 )
						{
							//  对于snap capture, 用户收到消息后就会来获取，我们应该直接滚动到szCapImage2和pCapImage2
							FREE_BUFFER(pHvObj->pCapImage2[index]);
							pHvObj->pCapImage2[index] = pHvObj->pCapImage[index];
							pHvObj->pCapImage[index] = NULL;
							pHvObj->szCapImage2[index] = pHvObj->szCapImage[index];
							pHvObj->szCapImage[index] = 0;
							NoticeHostEvent(pHvObj, (dataHeader.imgAttr.index<<16)|EVT_SNAP );
						}
					}
					else
					{
						TRACE_LOG(pHvObj,"==> 忽略识别机发送的图%s (%d bytes)\n", dataHeader.imgAttr.basename, dataHeader.size);
					}
				}  // else if not live image
			}
			else if ( dataHeader.DataType == DTYP_DATA )
			{
				switch (dataHeader.DataId )
				{
				case DID_BEGIN:
					memset( &pHvObj->plateInfo, 0, sizeof(PlateInfo) );
					strcpy( pHvObj->strPlate, "  整牌拒识" );
					pHvObj->enOper = OP_RREPORT;
					break;
				case DID_END:
					pHvObj->enOper = OP_IDLE;
					tickStartProc = 0;
					// 设置其他车牌结果属性值
					// 0 为 信心度 0~100
					// 1 为 车头车尾信息 1 车头，0xff 车尾，0 未知
					// 2 为车身颜色代码 （不大可靠）
					pHvObj->plateInfo.MatchRate[3] = (BYTE)dataHeader.dataAttr.val[2];	// 车速
					pHvObj->plateInfo.MatchRate[4] = (BYTE)dataHeader.dataAttr.val[1];	// 触发源
					// 将车牌数据和图片由一级buffer转到二级buffer, 上位机获取图片和车牌信息都由二级buffer获取
					RolloverData(pHvObj);
					TRACE_LOG(pHvObj, "==>识别结束，数据接收完成!\n");
					NoticeHostEvent(pHvObj, EVT_DONE );
					break;
				case DID_PLATE:
					Mutex_Lock(pHvObj);
					memcpy( &pHvObj->plateInfo, &dataHeader.plateInfo, sizeof(PlateInfo) );
					switch( pHvObj->plateInfo.plateCode & 0x00ff )
					{
					case PLC_BLUE:
						strcpy( pHvObj->strPlate, "蓝" );
						break;
					case PLC_YELLOW:
						strcpy( pHvObj->strPlate, "黄" );
						break;
					case PLC_WHITE:
						strcpy( pHvObj->strPlate, "白" );
						break;
					case PLC_BLACK:
						strcpy( pHvObj->strPlate, "黑" );
						break;
					case PLC_GREEN:
						strcpy( pHvObj->strPlate, "绿" );
						break;
					case PLC_YELGREEN:
						strcpy( pHvObj->strPlate, "秋" );
						break;
					default:
						strcpy( pHvObj->strPlate, "蓝" );
					}
					strcat( pHvObj->strPlate, pHvObj->plateInfo.chNum );
					TRACE_LOG(pHvObj, "==> 接收到识别的车牌 【%s】\n", pHvObj->strPlate );
					pHvObj->plateInfo.MatchRate[5] = (BYTE)pHvObj->plateInfo.plateCode & 0x00ff;	// 车牌颜色代码
					pHvObj->plateInfo.MatchRate[6] = (BYTE)((pHvObj->plateInfo.plateCode>>8) & 0xff);	// 车牌类型代码
					Mutex_Unlock(pHvObj);
					break;
				case DID_TIMING:
					pHvObj->process_time = dataHeader.timeInfo.totalProcess;
					pHvObj->elapsed_time = dataHeader.timeInfo.totalElapsed;
					break;
				case DID_COMDATA:
					if ( dataHeader.size > 0 )
					{
						int  i=0;
						BYTE *rx_data = (BYTE *)payload;
						TRACE_LOG(pHvObj, "Receive COM port RX data (%d bytes) - save to ring buffer. Current ring elements=%d\n", dataHeader.size, RingElements(pHvObj));
						Ring_Lock(pHvObj);
						for( ; !IsRingFull(pHvObj) && dataHeader.size > 0; )
						{
							pHvObj->ring_buf[pHvObj->ring_tail] = rx_data[i++];
							pHvObj->ring_tail = NextTailPosit(pHvObj);
							dataHeader.size--;
						}
						Ring_Unlock(pHvObj);
					}
					break;
				case DID_EXTDIO:
					pHvObj->dio_val[0] = dataHeader.dataAttr.val[0] & 0xffff;
					pHvObj->dio_val[1] = dataHeader.dataAttr.val[1] & 0xffff;
					break;
				case DID_VERSION:
					memcpy(&pHvObj->verAttr, &dataHeader.verAttr, sizeof(VerAttr));
					NoticeHostEvent(pHvObj, EVT_VERINFO );
					break;
				case DID_CFGDATA:
					memcpy(&pHvObj->dataCfg, &dataHeader.dataAttr, sizeof(DataAttr));
					break;
				}	// switch (dataHeader.DataId )
			}	// else if ( dataHeader.DataType == DTYP_DATA )
			else if ( dataHeader.DataType == DTYP_EVENT )
			{
				switch(dataHeader.DataId)
				{
				case EID_TRIGGER:
					pHvObj->enOper = OP_PROCESS;
					tickStartProc = GetTickCount();
					pHvObj->enTriggerSource = (TRIG_SRC_E)dataHeader.evtAttr.param;
					TRACE_LOG(pHvObj, "==> 识别机触发识别 (%s)，开始处理!\n", GetTriggerSourceText(pHvObj->enTriggerSource) );
					NoticeHostEvent(pHvObj, EVT_FIRED );
					break;
				case EID_VLDIN:
					TRACE_LOG(pHvObj, "==> 车辆进入虚拟线圈识别区!\n");
					NoticeHostEvent(pHvObj, EVT_VLDIN );
					break;
				case EID_VLDOUT:
					TRACE_LOG(pHvObj, "==> 车辆离开虚拟线圈识别区!\n");
					NoticeHostEvent(pHvObj, EVT_VLDOUT );
					break;
				case EID_EXTDI:
					pHvObj->diParam = dataHeader.evtAttr.param;
					pHvObj->dio_val[0] = pHvObj->diParam & 0xffff;
					TRACE_LOG(pHvObj, "==> EXT DI 状态变化 0x%x -> 0x%x\n", 
							(pHvObj->diParam >> 16) & 0xffff, (pHvObj->diParam & 0xffff));
					NoticeHostEvent(pHvObj, EVT_EXTDI );
					break;
				}
			}
			else 	if ( dataHeader.DataType == DTYP_CONF )
			{
				memcpy(&pHvObj->paramConf, &dataHeader.paramConf, sizeof(ParamConf));
			}
			else 	if ( dataHeader.DataType == DTYP_EXCONF )
			{
				memcpy(&pHvObj->extParamConf, &dataHeader.extParamConf, sizeof(ExtParamConf));
				memcpy( pHvObj->label, &dataHeader.extParamConf.label, SZ_LABEL);
				pHvObj->label[SZ_LABEL-1] = '\0';
				TRACE_LOG(pHvObj, "收到扩展配置参数 - label=%s\n", pHvObj->label);
			}
			else if ( dataHeader.DataType == DTYP_H264CONF )
			{
				memcpy( &pHvObj->h264Conf, &dataHeader.h264Conf, sizeof(H264Conf) );
			}
			else if ( dataHeader.DataType == DTYP_OSDCONF )
			{
				memcpy( &pHvObj->osdConf, &dataHeader.osdConf, sizeof(OSDConf) );
				if ( dataHeader.size > 0 )
				{
					if ( dataHeader.size >= sizeof(OSDPayload) )
						memcpy(&pHvObj->osdPayload, payload, sizeof(OSDPayload) );
					if ( dataHeader.size >= sizeof(OSDPayload) + sizeof(pHvObj->osdFixText) )
						memcpy(pHvObj->osdFixText, (char *)payload+sizeof(OSDPayload), sizeof(pHvObj->osdFixText) );
				}
			}
			else if ( dataHeader.DataType==DTYP_TEXT &&  dataHeader.DataId==TID_PLATENUM )
			{
				// 收到提前发送的车牌号码
				char *ptr;
				// 如果车牌号码有后缀 (车尾)，例如："蓝苏E12345(车尾)"，需要把后缀砍掉
				if ( (ptr=strchr(dataHeader.textAttr.text,'('))!=NULL)
				{
					*ptr = '\0';
					pHvObj->plateInfo.MatchRate[1] = 0xff;
				}
				strcpy(pHvObj->strPlateAdv ,dataHeader.textAttr.text);
				NoticeHostEvent(pHvObj, EVT_PLATENUM );
				TRACE_LOG(pHvObj, "收到提前发送的车牌号码: %s\n", pHvObj->strPlateAdv);
			}
			else if ( dataHeader.DataType==DTYP_TIME )
			{
				ParseCameraTimeHeader(pHvObj,&dataHeader.timeAttr);
			}
			else if ( dataHeader.DataType==DTYP_ACK && dataHeader.DataId!=0 )
			{
				// 收到控制应答帧
				pHvObj->acked_ctrl_id = dataHeader.DataId;
				NoticeHostEvent(pHvObj, EVT_ACK );
				TRACE_LOG(pHvObj, "收到控制命令 %d的应答帧!\n", pHvObj->acked_ctrl_id);
			}
		}	// if ( ret > 0 && FD_ISSET(pHvObj->sock, &set) )
		if ( (GetTickCount() - pHvObj->tickLastHeared > 10000) )
		{
			// over 10 seconds not heared from LPNR camera. consider socket is broken 
			TRACE_LOG(pHvObj,"Camera heart-beat not heared over 10 sec. reconnect.\n");
			Handle_Socket_Error( pHvObj );
			m_sock = INVALID_SOCKET;
		}
		if ( pHvObj->enOper==OP_PROCESS && (GetTickCount() - tickStartProc) > 2000 )
		{
			TRACE_LOG(pHvObj,"LPNR 分析图片时间超过2秒，复位状态到闲置!\n");
			pHvObj->enOper = OP_IDLE;
		}
	}
	pHvObj->status = STAT_EXIT;
	closesocket( m_sock );
	m_sock = pHvObj->sock = INVALID_SOCKET;
	pHvObj->enLink = DISCONNECT;
	pHvObj->hThread = 0;
	FREE_BUFFER(pHvObj->pLiveImage);
	ReleaseData(pHvObj,7);
	free(payload);
	TRACE_LOG(pHvObj,"-----------------    W O R K    T H R E A D   E X I T   ------------------\n");
	return THREADRET;
 }
 
 // 仅Windows模式下支持Proxy功能，因为有些API仅在Windows下实现并且测试过
#if defined _WIN32 || defined _WIN64
 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // PROXY working thread and functions
 void proxy_destroy_clients(PHVOBJ pHvObj)
 {
	 struct RemoteClient *client;
	 Proxy_Lock(pHvObj);
	 client = pHvObj->list_head;
	 while ( client != NULL )
	 {
		 struct RemoteClient *client_del = client;
		 closesocket(client->fd_pair[0]);
		 closesocket(client->fd_pair[1]);
		 client = client->next;
		 free(client_del);
	 }
	 pHvObj->list_head = NULL;
	 pHvObj->nProxyListCount = 0;
	 Proxy_Unlock(pHvObj);
 }

 int proxy_get_clients(PHVOBJ pHvObj, char strIP[][16], int size_array)
 {
	struct RemoteClient *client;
	 int i=0;
	 Proxy_Lock(pHvObj);
	 client = pHvObj->list_head;
	 while ( client != NULL && i<size_array)
	 {
		 strcpy(strIP[i++], client->strIP);
		 client = client->next;
	 }
	 Proxy_Unlock(pHvObj);
	 return i;
 }



 BOOL proxy_create_client(PHVOBJ pHvObj, SOCKET fd_client, SOCKET fd_camera)
 {
	 struct RemoteClient *client = ( struct RemoteClient *)calloc(1,sizeof(struct RemoteClient));
	 struct RemoteClient *client_tail;
	DWORD threadId;

	 strcpy(client->strIP, sock_getpeername(fd_client, NULL));
	 client->fd_pair[0] = fd_client;
	 client->fd_pair[1] = fd_camera;
	 Proxy_Lock(pHvObj);
	 client_tail = pHvObj->list_head;
	 if ( client_tail == NULL )
		 pHvObj->list_head = client;
	 else
	 {
		 while ( client_tail->next != NULL )
			 client_tail = client_tail->next;
		 client_tail->next = client;
		 client->prev = client_tail;
	 }
	 pHvObj->nProxyListCount++;
	TRACE_LOG(pHvObj, "PROXY CLIENT: %s connected, total clients now=%d\n", client->strIP, pHvObj->nProxyListCount);
	 Proxy_Unlock(pHvObj);
	 // create proxy working thread
#ifdef linux
	pthread_create( &client->hThread, NULL, proxy_workthread_fxc, (void *)client);
#else
	client->hThread = CreateThread(
						NULL,						// default security attributes
						0,								// use default stack size
(LPTHREAD_START_ROUTINE)proxy_workthread_fxc,		// thread function
						client,						// argument to thread function
						0,								// thread will be suspended after created.
						&threadId);				// returns the thread identifier
#endif 
	NoticeHostEvent(pHvObj, EVT_NEWCLIENT);
	return TRUE;
}

#ifdef linux
BOOL thread_alive(pthread_t thread)
{
	if ( pthread_kill(thread, 0)==0 )
		return TRUE;
	return FALSE;
}
#else
BOOL thread_alive(HANDLE hThread)
{
	DWORD exitCode=0;

	return ( GetExitCodeThread(hThread,&exitCode)==TRUE && exitCode==STILL_ACTIVE );
}
#endif

void proxy_remove_deadclient(PHVOBJ pHvObj)
{
	struct RemoteClient *client;
	BOOL bRemoved = FALSE;

	Proxy_Lock(pHvObj);
	client = pHvObj->list_head;
	while (client != NULL)
	{
		struct RemoteClient *client_this = client;
		client = client->next;
		if ( ! thread_alive(client_this->hThread) )
		{
			if ( client )
				client->prev = client_this->prev;
			if ( client_this->prev )
				client_this->prev->next = client;
			if ( client_this == pHvObj->list_head )
				pHvObj->list_head = client;
			pHvObj->nProxyListCount--;
			TRACE_LOG(pHvObj, "Proxy Client %s is gone. Client left = %d\n", client_this->strIP, pHvObj->nProxyListCount);
			free(client_this);
			bRemoved = TRUE;
		}
	}
	Proxy_Unlock(pHvObj);
	if ( bRemoved )
		NoticeHostEvent(pHvObj, EVT_CLIENTGO);
}

THREADCALL proxy_workthread_fxc(void *param)
{
	struct RemoteClient *client = (struct RemoteClient *)param;
	fd_set set;
	struct timeval tv;
	int ret, len;
	int  fd_max, fd[2];
	char  buffer[4096];

	fd[0] = client->fd_pair[0];
	fd[1] = client->fd_pair[1];
	fd_max = max(fd[0],fd[1]);
	// from now on, do not reference pointer client, it may gone any time
	for(;;)
	{
		FD_ZERO( & set );
		FD_SET( fd[0], & set );
		FD_SET( fd[1], & set );
		tv.tv_sec = 0;
		tv.tv_usec = 10000;
		ret = select(fd_max+1, &set, NULL, NULL, &tv);
		if ( ret < 0 )
			break;
		else if ( ret > 0 && FD_ISSET(fd[0], &set) )
		{
			len = sock_read(fd[0], buffer, sizeof(buffer) );
			if ( len <= 0 )
			{
				// socket broken, close another side
				closesocket(fd[1]);
				break;
			}
			// forward to another end
			sock_write(fd[1], buffer, len);
		}
		if ( ret > 0 && FD_ISSET(fd[1], &set) )
		{
			len = sock_read(fd[1], buffer, sizeof(buffer) );
			if ( len <= 0 )
			{
				// socket broken, close another side
				closesocket(fd[0]);
				break;
			}
			// forward to another end
			sock_write(fd[0], buffer, len);
		}
	}
	return THREADRET;
}

static int proxy_get_interface(const char *host, IF_ATTR4 ifa[], int size)
{
	int i, num_if = get_all_interface(ifa, size);
	if ( num_if > 1 && host != NULL )
	{
		DWORD host_addr = INET_ATON(host);
		for(i=0; i<num_if; i++)
			if ( host_addr==ifa[i].addr.s_addr )
				break;
		if ( i < num_if )
		{
			if ( i > 0 )
				memcpy(ifa, ifa+i, sizeof(IF_ATTR4));
			num_if = 1;
		}
	}
	return num_if;
}

static void proxy_log_interface(PHVOBJ pHvObj, IF_ATTR4 ifa[], int size)
{
	int i;
	char strIP[16], strMask[16], strBcast[16];
	TRACE_LOG(pHvObj, "Number of host interface IP=%d\n", size);
	for(i=0; i<size; i++)
		TRACE_LOG(pHvObj, "- [%d]  if_name=%s, IP=%s, netmask=%s, broadcast-addr=%s\n",
				i+1, ifa[i].ifa_name, 
				INET_NTOA2(ifa[i].addr.s_addr, strIP), 
				INET_NTOA2(ifa[i].mask.s_addr, strMask), 
				INET_NTOA2(ifa[i].broadcast.s_addr, strBcast) );
}

// select one in same net-segment. if not, return -1
static int proxy_best_if(SOCKADDR_IN *sender,  IF_ATTR4 *ifa, int num_if)
{
	int i;
	for(i=0; i<num_if; i++)
		if ( (sender->sin_addr.s_addr & ifa[i].mask.s_addr)==(ifa[i].addr.s_addr & ifa[i].mask.s_addr) )
			return i;
	return -1;
}


THREADCALL proxy_masterthread_fxc(void *param)
{
#define MAX_HOST_IP  10
	PHVOBJ pHvObj = (PHVOBJ)param;
	IF_ATTR4 ifa_attr[MAX_HOST_IP];
	int		num_net;
	DWORD poll_clock=0;
	SOCKADDR_IN  poll_from;
	fd_set  set;
	struct timeval tv;
	int nsel;
	int fd_max;

	
	TRACE_LOG(pHvObj, "=====【PROXY MASTER THREAD BEGIN】=====\n");
	num_net = proxy_get_interface(pHvObj->proxy_host_ip, ifa_attr, MAX_HOST_IP);
	proxy_log_interface(pHvObj, ifa_attr, num_net);
	fd_max = max(pHvObj->proxy_listen,pHvObj->proxy_udp);
	memset(&poll_from,0,sizeof(poll_from));
	pHvObj->bProxyEnable = TRUE;

	for(;pHvObj->bProxyEnable;)
	{
		// remove any dead connection
		proxy_remove_deadclient(pHvObj);
		// poll for any search or connection reques
		FD_ZERO(&set);
		FD_SET(pHvObj->proxy_listen, &set);
		FD_SET(pHvObj->proxy_udp, &set);
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		nsel  = select(fd_max+1, &set,NULL,NULL,&tv);
		if ( nsel < 0 )
		{
			TRACE_LOG(pHvObj, "PROXY MASTER - select error, terminate (error=%d)!!!\n", WSAGetLastError());
			pHvObj->bProxyEnable = FALSE;
		}
		else if ( nsel > 0 && FD_ISSET(pHvObj->proxy_listen,&set) )
		{
			int fd_client = sock_accept(pHvObj->proxy_listen);
			int fd_camera;
			if ( fd_client == INVALID_SOCKET )
			{
				TRACE_LOG(pHvObj,"PROXY MASTER - Failed to accept a connection, listen socket broken, terminated!!!\n");
				pHvObj->bProxyEnable = FALSE;
			}
			fd_camera = sock_connect(pHvObj->strIP, PORT_LISTEN);
			if ( fd_camera==INVALID_SOCKET )
			{
				TRACE_LOG(pHvObj,"PROXY MASTER - Failed to connect camera, client connection refused!!!\n");
				closesocket(fd_client);
			}
			else
			{
				// new connection pair created
				proxy_create_client(pHvObj, fd_client, fd_camera);
			}
		}
		// UDP search
		if ( nsel > 0 && FD_ISSET(pHvObj->proxy_udp,&set) )
		{
			DataHeader header;
			SOCKADDR_IN sender;
			int i, i0, iend, idx;
			const char *dst_ip = NULL;
			int len = sock_udp_recv0( pHvObj->proxy_udp, (char *)&header, sizeof(header), &sender);
			TRACE_LOG(pHvObj,"PROXY - receive UDP packet (len=%d), type=0x%x, DataId=%d, sender=%s:%d...\n", 
					len, header.DataType, header.DataId, INET_NTOA(sender.sin_addr.s_addr), ntohs(sender.sin_port) );
			if ( len==sizeof(header) && header.DataType==DTYP_UDP && header.DataId==DID_SEARCH )
			{
				if ( memcmp(&poll_from, &sender, sizeof(SOCKADDR_IN))==0 && GetTickCount()-poll_clock<3000 )
				{
					// search message received from same sender within 3 sec. ignored
					continue;
				}
				memcpy(&poll_from, &sender, sizeof(SOCKADDR_IN));
				poll_clock = GetTickCount();
				idx = proxy_best_if(&sender, ifa_attr, num_net);
				if ( idx==-1 )
				{
					TRACE_LOG(pHvObj, "Proxy - cannot find best match interface, send answer to all interface...\n");
					i0 = 0; iend = num_net;
				}
				else
				{
					TRACE_LOG(pHvObj, "Proxy - best match interface, send answer via interface ifa[%d]=%s...\n", idx, ifa_attr[idx].ifa_name );
					i0 = idx;
					iend = idx+1;
				}
				memset(&header, 0, sizeof(header));
				header.DataId = DID_ANSWER;
				header.dataAttr.s16Val[0] = (HI_S16)PORT_LISTEN;
				header.dataAttr.s16Val[1] = pHvObj->verAttr.param[1] >> 16;
				header.dataAttr.val[1] = pHvObj->verAttr.param[0];
				header.dataAttr.val[2] = 1;		// 经过PROXY 连接的
				memcpy(&header.dataAttr.s8Val[32-SZ_LABEL], pHvObj->label, SZ_LABEL);
				for(i=i0; i<iend; i++)
				{
					SOCKET fd = sock_udp_bindLocalIP(ifa_attr[i].addr.s_addr,PORT_SEARCH+1);
					if ( fd==INVALID_SOCKET ) 
					{
						TRACE_LOG(pHvObj, "Proxy failed to open UDP reply socket to answer SEARCH packet (errno=%d)!\n",  WSAGetLastError());
						continue;
					}
					if ( (sender.sin_addr.s_addr & ifa_attr[i].mask.s_addr) == (ifa_attr[i].addr.s_addr &ifa_attr[i].mask.s_addr) )
					{
						short port = sender.sin_port;

						TRACE_LOG(pHvObj, "--> in same network segment, directly send UDP reply to %s:%d\n", INET_NTOA(sender.sin_addr.s_addr), ntohs(port));
						sock_udp_sendX(fd, &sender, 1, &header, sizeof(header));
					}
					else
					{
						short port = sender.sin_port;
						TRACE_LOG(pHvObj, "--> in different network segment, broadcast UDP reply to port %d\n", ntohs(port));
						sock_udp_broadcast(fd, 1);
						sender.sin_addr.s_addr = INADDR_BROADCAST;
						sock_udp_sendX(fd, &sender, 1, &header, sizeof(header));
					}
					closesocket(fd);
				}
			}
		}
	}
	TRACE_LOG(pHvObj, "=====【PROXY MASTER THREAD EXIT】=====\n");
	closesocket(pHvObj->proxy_listen);
	closesocket(pHvObj->proxy_udp);
	pHvObj->proxy_listen = INVALID_SOCKET;
	pHvObj->proxy_udp = INVALID_SOCKET;
	// 已经建立的代理连接还可以继续存活，直到自己断开或是Proxy_Terminate被调用
	return THREADRET;
}
#endif

 //=============================================================================
#if defined linux && defined ENABLE_LPNR_TESTCODE
#include <termios.h>

static struct termios tios_save;

static int ttysetraw(int fd)
{
	struct termios ttyios;
	if ( tcgetattr (fd, &tios_save) != 0 )
		return -1;
	memcpy(&ttyios, &tios_save, sizeof(ttyios) );
	ttyios.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
                      |INLCR|IGNCR|ICRNL|IXON);
	ttyios.c_oflag &= ~(OPOST|OLCUC|ONLCR|OCRNL|ONOCR|ONLRET);
	ttyios.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
	ttyios.c_cflag &= ~(CSIZE|PARENB);
	ttyios.c_cflag |= CS8;
	ttyios.c_cc[VMIN] = 1;
	ttyios.c_cc[VTIME] = 0;
	return tcsetattr (fd, TCSANOW, &ttyios);
}

static int ttyrestore(int fd)
{
	return tcsetattr(fd, TCSANOW, &tios_save);
}

int tty_ready( int fd )
{
	int	n=0;
 	fd_set	set;
	struct timeval tval = {0, 10000};		// 10 msec timed out

	FD_ZERO(& set);
	FD_SET(fd, &set);
	n = select( fd+1, &set, NULL, NULL, &tval );

	return n;
}

void lpnr_event_handle( HANDLE h, int evnt )
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	int i, n, size, model, sensor;
	DWORD	  version;
	int				  algver;
	char *buf;
	char chIP[16];
	char rx_buf[1024];
	char strClient[10][16];
	int    num_client;
	int	    param = evnt >> 16;

	evnt &= 0xffff;
	LPNR_GetMachineIP(h, chIP);
	switch (evnt)
	{
	case EVT_ONLINE:
		printf("LPNR camera (IP %s) goes ONLINE.\r\n", chIP );
		break;
	case EVT_VERINFO:
		LPNR_GetModelAndSensor(h, &model, &sensor);
		printf("Mode=%d, sensor=%d\r\n", model, sensor);
		LPNR_GetVersion(h, &version, &algver);
		printf("version=0x%x, algorithm version=%d.%d\r\n", version, algver>>16, algver&0xffff);
		break;
	case EVT_OFFLINE:
		printf("LPNR camera IP %s goes OFFLINE.\r\n", pHvObj->strIP );
		break;
	case EVT_FIRED:
		printf("LPNR camera IP %s FIRED a image process.\r\n", chIP );
		printf("trigger source=%d\r\n", LPNR_GetTriggerSource(h) );
		break;
	case EVT_DONE:
		printf("LPNR camera IP %s processing DONE\r\n", pHvObj->strIP );
		for(i=0; i<IMGID_BUTT; i++)
		{
			size = LPNR_GetSelectedImageSize(h,i);
			printf("image Id=%d, size=%d\r\n", i, size);
		}
		LPNR_GetPlateNumber(h, rx_buf);
		printf("\t plate number = %s\r\n", rx_buf );
		LPNR_ReleaseData((HANDLE)h);
		break;
	case EVT_LIVE:
		LPNR_Lock(h);
		size = LPNR_GetLiveFrameSize(h);
		buf = malloc(size);
		LPNR_GetLiveFrame(h, buf);
		LPNR_Unlock(h);
		// do some thing for live frame (render on screen for example)., but do not do it in this thread context
		// let other render thread do the job. We don't want to block LPNR working thread for too long.
		printf("reveive a live frame size=%d bytes\r\n", size );
		free(buf);
		break;
	case EVT_SNAP:
		printf("snap image reveived, size=%d bytes\r\n", LPNR_GetCapturedImageSize(h));
		break;
	case EVT_ASYNRX:
		printf("Serial port data Rx (%d bytes) -- ", (n=LPNR_COM_iqueue(h)));
		LPNR_COM_read(h, rx_buf, sizeof(rx_buf));
		rx_buf[n] = '\0';
		printf("%s\r\n", rx_buf);
		break;
	case EVT_PLATENUM:
		LPNR_GetPlateNumberAdvance(h,rx_buf);
		printf("Plate number send in advance=%s\r\n", rx_buf);
		break;
	case EVT_NEWCLIENT:
	case EVT_CLIENTGO:
		num_client = Proxy_GetClients(h, strClient, 10);
		printf("Proxy Client %s, client now = %d\r\n", evnt==EVT_NEWCLIENT ? "Added" : "Removed", num_client);
		for(i=0; i<num_client; i++)
			printf("\t- Client[%d] - %s\r\n", i+1, strClient[i]);
		break;
	}
}

/*
 *  after launched, user use keyboard to control the opeation. stdin is set as raw mode.
 *  therefore, any keystroke will be catched immediately without have to press <enter> key.
 *  'q' - quit
 *  'l' - soft trigger.
 */
int generate_random_string(char buf[], int size)
{
	int i;
	for(i=0; i<size; i++)
	{
		buf[i] = 32 + (random() % 95);
	}
	return size;
}

void show_help()
{
	printf("Single key operation command:\r\n"
		"- ?: show this message\r\n"
		"- 0,1,2: select DO pin to be operate or live feed frame size (n+1)\r\n"
		"- a: initial Proxy connection for remote client!\r\n"
		"- A: terminate Proxy connection for remote client!\r\n"
		"- d: disable live feed\r\n"
		"- e: enable live feed for selected frame size by previous '0'~'2' command!\r\n"
		"- f:  toggle lighting on/off\r\r"
		"- F: toggle IR cut on/off\r\n"
		"- i: initial tranparent COM port @ 9600\r\n"
		"- r: enable COM port synchronus Rx (must input 'i' command once)\r\n"
		"- l: manual trigger a capture and recognition\r\n"
		"- L: toggle enable/disable log function!\n"
		"- m: show camera model and sensor Id\r\n"
		"- n: show camera label\r\n"
		"- o: toggle DO pin output value, pin number was selected by '0'~'2' command\r\n"
		"- p: output 250 ms period pulse on pin (number was selected by '0'~'2' command)\r\n"
		"- P: output 500 ms period pulse on pin (number was selected by '0'~'2' command)\r\n"
		"- q: quit this test program\r\n"
		"- r: enable the COM port asynchrous Rx\r\n"
		"- R: disable COM port synchronus Rx\r\n"
		"- s: take a snapshot w/o lighting\r\n"
		"- S: take a snapshot w/ lighting\r\n"
		"- t: time-sync. make camera time same as computer time\r\n"
		"- T: toggle OSD time-stamp on/off\r\n"
		"- u: turn off user text OSD\r\n"
		"- U: turn on user text OSD with fixed text built-in test code\r\n"
		"- v: show camera software and algorithm version\r\n"
		"- x: generate a random ASCII string Tx to COM port (must input 'i' command once)\r\n"
		);
}

int main( int argc, char *const argv[] )
{
	int ch;
	BOOL bQuit = FALSE;
	BOOL bOnline = FALSE;
	BOOL bLogEnable = TRUE;
	int		 lighton = 0;
	int     IRon = 0;
	int		fd;
	HANDLE hLPNR;
	int    n, pin=0, value=0, period;
	char tx_buf[64];
	BOOL bTimeStamp = TRUE;
	int softver, algver;
	int model, sensor;

	if ( argc != 2 )
	{
		fprintf(stderr, "USUAGE: %s <ip addr>\n", argv[0] );
		return -1;
	}
	fd = 0;		// stdin
	ttysetraw(fd);
	if ( (hLPNR=LPNR_Init(argv[1])) == NULL )
	{
		ttyrestore(fd);
		fprintf(stderr, "Invalid LPNR IP address: %s\r\n", argv[1] );
		return -1;
	}
	LPNR_SetCallBack( hLPNR, lpnr_event_handle );

	srandom(time(NULL));

	for(;!bQuit;)
	{
		BOOL bOn = LPNR_IsOnline(hLPNR);
		if ( bOn != bOnline )
		{
			bOnline = bOn;
			printf("camera become %s\r\n", bOn ? "online" : "offline" );
		}
		if ( tty_ready(fd) && read(fd, &ch, 1)==1 )
		{
			ch &= 0xff;
			switch (ch)
			{
			case '?':
				show_help();
				break;
			case '0':
			case '1':
			case '2':
				pin = ch - '0';
				printf("choose pin number %d\r\n", pin);
				break;
			case 'a':
				printf("initial PROXY feature!\r\n");
				Proxy_Init(hLPNR, NULL);
				break;
			case 'A':
				printf("terminate PROXY feature!\r\n");
				Proxy_Terminate(hLPNR);
				break;
			case 'd':	// disable live
				printf("disable live feed.\r\n");
				LPNR_EnableLiveFrame( hLPNR, 0 );
				break;
			case 'e':	// enable live
				printf("enable live feed of size %d\r\n", pin+1);
				LPNR_EnableLiveFrame( hLPNR, pin+1 );
				break;
			case 'f':		// 'f' - flash light
				lighton = 1 - lighton;
				printf("light control, on/off=%d\r\n", lighton);
				LPNR_LightCtrl(hLPNR, lighton, 0);
				break;
			case 'F':		// IR cut
				IRon = 1 - IRon;
				printf("IR-cut control, on/off=%d\r\n", IRon);
				LPNR_IRCut(hLPNR, IRon);
				break;
			case 'i':		// initial COM port
				printf("initial transparent COM port!\r\n");
				LPNR_COM_init(hLPNR,9600);
				break;
			case 'l':
				printf("manual trigger a recognition...\r\n");
				LPNR_SoftTrigger( hLPNR );
				break;
			case 'L':
				bLogEnable = !bLogEnable;
				printf("log function now is %s\r\n", bLogEnable ? "ENABLE" : "DISABLE");
				break;
			case 'm':
				LPNR_GetModelAndSensor(hLPNR, &model, &sensor);
				printf("camera model number = %d, sensor type=%d\r\n", model, sensor);
				break;
			case 'n':
				printf("camera name (label) = %s\r\n",  LPNR_GetCameraLabel(hLPNR));
				break;
			case 'o':		// Ext DO
				value = 1 - value;
				printf("outpput DO pin=%d, value=%d\r\n", pin, value);
				LPNR_SetExtDO(hLPNR, pin, value);
				break;
			case 'p':		// pulse
			case 'P':
				n = random() % 10 + 1;
				period = ch=='p' ? 250 : 500;
				printf("output pulse at %d msec period for %d times on pin %d.\r\n",  period, n,  pin);
				LPNR_PulseOut(hLPNR, pin, period, n);
				break;
			case 'q':
				printf("Quit test program...\r\n");
				bQuit = TRUE;
				break;
			case 'r':		// must issue 'i' command first (once only)
				printf("enable COM port async Rx\r\n");
				LPNR_COM_aync(hLPNR,TRUE);
				break;
			case 'R':
				printf("disable COM port async Rx\r\n");
				LPNR_COM_aync(hLPNR,FALSE);
				break;
			case 's':
			case 'S':
				printf("take s snap frame...\r\n");
				LPNR_TakeSnapFrame(hLPNR,ch=='S');
				break;
			case 't':
				printf("time sync with camera.\r\n");
				LPNR_SyncTime(hLPNR);
				break;
			case 'T':
				bTimeStamp = !bTimeStamp;
				printf("toggle timestamp OSD - turn it %s\n", bTimeStamp ? "ON" : "OFF");
				LPNR_SetOSDTimeStamp(hLPNR,TRUE,0,0);
				break;
			case 'u':	// turn off user OSD
				LPNR_UserOSDOff(hLPNR);
				break;
			case 'U':	// turn on user OSD
				printf("turn on usse OSD text!\r\n");
				LPNR_UserOSDOn(hLPNR,-5,0,2,40,0xFFFF00,80,"Overlay Line 1\nMiddle Line\nLast Line is the longest");
				break;
			case 'v':
				LPNR_GetVersion(hLPNR, (DWORD *)&softver, &algver);
				printf("camera firmware version=0x%x, algorithm version: %d:%d\r\n", softver, algver>>16, algver & 0xffff);
				break;
			case 'x':
				// generate random string Tx to COM
				n = generate_random_string(tx_buf, random()%60+1);
				tx_buf[n] = '\0';
				printf("Tx %d bytes: %s\r\n",  n, tx_buf);
				LPNR_COM_send(hLPNR, tx_buf, n);
				break;
			default:
				printf("ch=%c (%d), unknown command code, ignored.\r\n", ch, ch );
				break;
			}
		}
	}	
	printf("terminate working thread and destruct hLPNR...\r\n");
	LPNR_Terminate(hLPNR);
	ttyrestore(fd);
	return 0;
}

#endif
