#ifndef _RWLPNRAPI_INCLDED_
#define _RWLPNRAPI_INCLDED_

#define _WINDLL

#if defined WIN32 || defined _WIN64
#ifdef _WINDLL					// to buid a DLL
#define DLLAPI		__declspec(dllexport)
#define CALLTYPE	__stdcall
#elif defined DLL_IMPORT
#define DLLAPI		__declspec(dllimport)
#define CALLTYPE	__stdcall
#else  
#define DLLAPI		
#define CALLTYPE
#endif
#else  // [linux]
#include "wintype.h"
 
#define DLLAPI
#define CALLTYPE
#define	IN
#define OUT

#define _MAX_FNAME 256
#define _MAX_EXT 64

typedef struct
{
   int cx;
   int cy;
}SIZE;

#endif

// ʶ�����̬�ⷢ�͵���Ϣ
#define EVT_ONLINE			1		// ʶ�������
#define EVT_OFFLINE			2		// ʶ�������
#define EVT_FIRED			3		// ʶ�𴥷�
#define EVT_DONE			4		// ʶ�������������á��յ������Ϣ����Ի�ȡʶ����ѶϢ�ͻ�ȡץ��ͼƬ��
#define EVT_LIVE			5		// ʵʱͼ�����
#define EVT_VLDIN			6		// �г�����������Ȧ
#define EVT_VLDOUT			7		// �����뿪������Ȧʶ����
#define EVT_EXTDI			8		// DI ״̬�仯 ��һ����Ĳ����ã�����չDI�㣩
#define EVT_SNAP			9		// �յ�ץ�� ͼƬ��ʹ��LPNR_TakeSnapFrame//LPNR_TakeSnapFrameEx��ȡץ��ͼƬ�Ż��յ�����Ϣ)
												// ��Ϣ�ĸ�16λ���ĸ��ߴ��ץ��ͼ(1 ȫ�����, 2: 1/4, 3: 1/16). �û���Ҫ��ȡ��Ӧ��ץ��ͼ��ָ��ץ�Ľ����
												// ��ʶ���1.3.15.2�汾��ʼ���еĹ��ܣ�֮ǰ�汾ֻ֧��ץ��ȫ�����ͼ�������ϰ汾����16λ��0.
#define EVT_ASYNRX		10		// �յ�͸���������� ��ʶ���1.3.6.3�汾��ʼ��֧��͸�����ڣ�ʶ�����Ҫ�еĻ��Ͳ����ã�
#define EVT_PLATENUM	11		// �յ���ǰ���͵ĳ��ƺ��� ���ڸ���ͼƬ�����������֮ǰ�ȷ��ͣ�����λ�������ȡ����
#define EVT_VERINFO		12		// �����Ժ��Ѿ��յ��汾ѶϢ (�յ������Ϣ��ſ���ȥ��ȡʶ����İ汾��Ϣ���ͺŵȣ�
#define EVT_ACK			13		// DO��������������ACK֡�յ���Ҳ����ʶ����Ѿ�ִ����DO/�������
#define EVT_NEWCLIENT	14		// һ���µ�Զ�̿ͻ������ӵ������������PROXY���ƣ�
#define EVT_CLIENTGO	15		// һ��Զ�̿ͻ��˶Ͽ�����, �������������Ǳ���.
#define EVT_CROPIMG		16		// �յ��ü���ץ��ͼƬ ��event number�ĸ�16λ�ᱣ��Ĳü�������0~10��ʶ���1.3.15.2�汾����

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
   int	x;
   int	y;
   int	width;
   int	height;
} HI_RECT, *PHI_RECT;

typedef void(CALLTYPE *LPNR_callback)(void *, int);

// �������������TCP���ӣ����������̡߳�
DLLAPI HANDLE CALLTYPE LPNR_Init( const char *strIP );
// �ر����ӣ����������߳�
DLLAPI BOOL CALLTYPE LPNR_Terminate(HANDLE h);
// ���ûص����� ��for Windows, LPNR_SetWinMsg is recommended��
DLLAPI BOOL CALLTYPE LPNR_SetCallBack(HANDLE h, LPNR_callback mycbfxc );
#if defined _WIN32 || defined _WIN64
DLLAPI BOOL CALLTYPE LPNR_SetWinMsg( HANDLE h, HWND hwnd, int msgno);
#endif
// ѯ��������Ƿ�online
DLLAPI BOOL CALLTYPE LPNR_IsOnline(HANDLE h);
// ��ȡԤ�ȷ��͵ĳ��ƺ��루���յ�EVT_PLATENUM֮���ȡ��
DLLAPI BOOL CALLTYPE LPNR_GetPlateNumberAdvance(HANDLE h, char *buf);
// ��ȡ���ƺ��룬������յ�EVT_DONE����òſ��Ի�ȡ����ʶ��ĳ��ƺ��롣
DLLAPI BOOL CALLTYPE LPNR_GetPlateNumber(HANDLE h, char *buf);
DLLAPI BOOL CALLTYPE LPNR_GetPlateAttribute(HANDLE h, BYTE *attr);
/* LPNR_GetPlateAttribute: ��ȡ����������Ϣ��ͬ����Ҫ���յ�EVT_DONE�����
 * plate attribute:
 * attr������8���ֽڡ�ÿ���ֽ�����Ϊ��
 * 0�� ���Ŷ� ��0~100����reserved��
 * 1�� ��ͷ���ǳ�β�ĳ��ƣ�reserved��1����ͷ��0xff ��β��0 δ֪��
 * 2��������ɫ�� ��reserved��
 * 3������ ��ֻ�г��ٴ���ʱ���ٶ�ֵ�������壩
 * 4������Դ (0��ͼƬ���ͣ�1����λ��������2����Ȧ������3����ʱ������4��������Ȧ������5�����ٴ���)
 * 5��������ɫ���� (PLATE_COLOR_E)
 * 6:  ��������(PLATE_TYPE_E)
 * 7������
 */
// ��ȡlive jpeg֡���յ�EVT_LIVE�¼�����á�buf������������һ��JPEG��ʽ��ͼƬ���ݡ�buf��С������ڵ���LPNR_GetLiveFrameSize���ص�ֵ��
DLLAPI int CALLTYPE LPNR_GetLiveFrameSize(HANDLE h);
DLLAPI int CALLTYPE LPNR_GetLiveFrame(HANDLE h, void *buf);
// ���º�����ȡץ��ͼƬ��һ�����յ�EVT_DONE�����
// ��ȡ���Ʋ�ɫͼ��buf���ݾ���������һ��bmp��ʽ��ͼƬ������JPEG��������ó��Ʋ�ɫͼʹ��JPEG��ʽʱ������С������ڵ���LPNR_GetPlateColorImageSize���ص�ֵ
DLLAPI int CALLTYPE LPNR_GetPlateColorImageSize(HANDLE h);
DLLAPI int CALLTYPE LPNR_GetPlateColorImage(HANDLE h, void *buf);
// ��ȡ���ƶ�ָͼ��binary image����buf���ݾ���������һ��bmp��ʽ��ͼƬ����С������ڵ���LPNR_GetPlateBinaryImageSize���ص�ֵ
DLLAPI int CALLTYPE LPNR_GetPlateBinaryImageSize(HANDLE h);
DLLAPI int CALLTYPE LPNR_GetPlateBinaryImage(HANDLE h, void *buf);
// ��ȡץ��ȫͼ�������������ƺ����ͼƬ����buf���ݾ��Ǹ�ͼƬjpeg��ʽ���������ݣ���С������ڵ���LPNR_GetCapturedImageSize���ص�ֵ
// ���������û�н�ֹ����ץ��ȫͼ�Ż��յ����ͼƬ
DLLAPI int CALLTYPE LPNR_GetCapturedImageSize(HANDLE h);
DLLAPI int CALLTYPE LPNR_GetCapturedImage(HANDLE h, void *buf);
// ��ȡץ��ͼƬ�ĳ�ͷͼ�������������ƺ����ͼƬ����buf���ݾ��Ǹ�ͼƬjpeg��ʽ���������ݣ���С������ڵ���LPNR_GetHeadImageSize���ص�ֵ
// ���������ʹ�ܷ��ͳ�ͷͼ�Ż��ڷ����공�ƺ��յ����ͼƬ
DLLAPI int CALLTYPE LPNR_GetHeadImageSize(HANDLE h);
DLLAPI int CALLTYPE LPNR_GetHeadImage(HANDLE h, void *buf);
// ��ȡץ��ͼƬ4/9����ȵ�ͼ�������������ƺ����ͼƬ����buf���ݾ��Ǹ�ͼƬjpeg��ʽ���������ݣ���С������ڵ���LPNR_GetMiddlemageSize���ص�ֵ
// ���������ʹ�ܷ���4/9�����ͼ�Ż��ڷ����공�ƺ��յ����ͼƬ
DLLAPI int CALLTYPE LPNR_GetMiddleImageSize(HANDLE h);
DLLAPI int CALLTYPE LPNR_GetMiddleImage(HANDLE h, void *buf);
// ��ȡץ��ͼƬ1/4����ȵ�ͼ�������������ƺ����ͼƬ����buf���ݾ��Ǹ�ͼƬjpeg��ʽ���������ݣ���С������ڵ���LPNR_GetQuadImageSize���ص�ֵ
// ���������ʹ�ܷ���1/4�����ͼ�Ż��ڷ����공�ƺ��յ����ͼƬ
DLLAPI int CALLTYPE LPNR_GetQuadImageSize(HANDLE h);
DLLAPI int CALLTYPE LPNR_GetQuadImage(HANDLE h, void *buf);
// ͬ�������ʱ�䣬�ú����·������ʱ�����������������������ϵͳʱ�䡣
DLLAPI BOOL CALLTYPE LPNR_SyncTime(HANDLE h);
// ������������֡��ʱ������λ�����ĵ��Ե�ϵͳʱ�����Ҫ����������������⹤���߳�����Ϊ��ʱ��δ�յ�����֡���ر����ӡ�
DLLAPI BOOL CALLTYPE LPNR_ResetHeartBeat(HANDLE h);
// ��ȡ���������ݺ��ͷű����ͼƬ���ݡ����Բ����á������߳����´ο�ʼʶ�����ʱ�����Զ��ͷš�
DLLAPI BOOL CALLTYPE LPNR_ReleaseData(HANDLE h);
// ��λ������һ��ʶ������������������������idle״̬��û�����ڷ��������У�
// LPNR_SoftTriggerEx ��һ������Id������һ����ţ�ÿ�δ��������������ʹ��ͬһ��Id�Ŷ�η��ͣ�ֻ�ᴥ��һ�Ρ����������粻�ȶ�����η���ȷ�����Դ���ʱʹ�á�
DLLAPI BOOL CALLTYPE LPNR_SoftTrigger(HANDLE h);
DLLAPI BOOL CALLTYPE LPNR_SoftTriggerEx(HANDLE h, int Id);
// �������������߳�״̬�Ƿ������ã��ȴ��´δ�����
DLLAPI BOOL CALLTYPE LPNR_IsIdle(HANDLE h);
// ��ȡ���һ�γ��Ʒ����Ĵ���ʱ�䡣elaped time��wall clock��processed time��CPU time. ��λ��msec
DLLAPI BOOL CALLTYPE LPNR_GetTiming(HANDLE h, int *elaped, int *processed );
// ����ʵʱJPEG֡��nSizeCodeȡֵ��Χ 0~3
// 0 - �ر�ʵʱ֡����
// 1 - ����ȫ�����
// 2 - ����1/4�����
// 3 - ����1/16�����
// ע�ͣ���Ҫ���ʶ�������汾 1.3.5.18 �����ϵİ汾���;ɰ汾���ӵĻ���nSIzeCode 0 һ���ǹرգ�!=0 ����������ȸ��ݵ�ǰ���õĲ������͡�
// 1.3.5.18 ��֮��İ汾��ÿ���ͻ��˿��������Լ���Ҫ�Ľ���ȣ����ụ��Ӱ�졣�ϵİ汾���ʵʱ֡�Ľ������ȫ�ֵĲ������á�
// 1.3.5.18�汾֮�ϣ�֡�ʻ��ܶࡣ������1/4, 1/16�����������ܵ���֡�ʣ�25���ϣ�
// ���ڲ���Ҫlive jpeg֡��application, ����ر�live jpeg, ���Խ�ʡ�������������ÿ���յ�EVT_ONLINE��Ϣ�󣬾͵���
// LPNR_EnableLiveFrame(h,0)���Ա�֤�ر�live jpeg. ��������LPNR_Init�����̵��ã���Ϊ��ʱ��TCP���ӻ�û������
DLLAPI BOOL CALLTYPE LPNR_EnableLiveFrame(HANDLE h, int nSizeCode);
// ��ȡһ��ץ��֡�����ǲ����г��Ʒ������������ֻ�Ǵ���һ��ץ�ģ�ͼƬҪ���յ�EVT_SNAP��Ϣ���¼����󣬵���LPNR_GetCapturedImage��ȡ
// bFlashLight TRUE��ץ��ʱ���⣬FALSE�ǲ�Ҫ���⡣
// LPNR_TakeSnapFrameEx ����ָ��Ҫץ���Ǹ�����ȵ�ͼ����Ҫʶ����汾��1.3.15.2���ϰ汾�Ļ�������nSizeCode�����ݣ�һ����ȫ�����ץ�ġ�
DLLAPI BOOL CALLTYPE LPNR_TakeSnapFrame(HANDLE h, BOOL bFlashLight);
DLLAPI BOOL CALLTYPE LPNR_TakeSnapFrameEx(HANDLE h, BOOL bFlashLight, int nSizeCode);
// LPNR_Lock/LPNR_Unlock - ��ͼƬ���ݺͳ������ݼ���/�����������ڶ�ȡ�����б������߳�ˢ�����ݡ�
// �ж�ȡlive jpeg frameʱ����Ҫ�õ����������������п����ڶ�ȡ�Ĺ����У��µ�һ��ʵʱ֡��ˢ��buffer����ɶ��������ݴ���
// ����ͼƬ���Բ���������Ϊ����ֻ������ʶ����ʱ�Żᱻˢ�¡�
DLLAPI BOOL CALLTYPE LPNR_Lock( HANDLE h );
DLLAPI BOOL CALLTYPE LPNR_Unlock(HANDLE h);
// capture cropped image -
// nCropArea ����ץ�Ļ��������úõĲü�������[1..10 max], �����0�Ļ�����ʾ��Ҫ��̬�ü�ĳ������
// rect - ���ָ�����ݱ���ľ���һ��HI_RECT�Ľṹ { int x, int y, int width, int height} ��ʾҪ�ü��ķ�Χ����rect��NULLʱ����ʾ�ü�����Χ
// �Ѿ���ʶ����������ļ� "/home/usercrop.conf"���涨�塣�������NULL�����ǲü�ָ����Χ�����ҷ��ص�ͼƬ�����'nCropArea'[1..10]ָ���Ļ�������
// �û��յ���ϢEVT_CROPIMGʱ����16λ����nCropArea���Ժ���LPNR_GetSelectedImageSize+LPNR_GetSelectedImage, ����LPNR_GetSelectedImagePtr��ȡͼƬ��
// ͼƬ��ȡ��������BMP��ʽ���ͼ����棬��˲�Ҫ��ȡ̫�������������500�����ػ��͡���ȡ̫�������������ʶ����ڴ治�㣬��ϵͳKill����
DLLAPI BOOL CALLTYPE LPNR_TakeCropFrame(HANDLE h, int nCropArea, HI_RECT *rect);

// ����ͼƬʶ��
// image
DLLAPI BOOL CALLTYPE LPNR_DownloadImage(HANDLE h, BYTE *image, int size, int format);
DLLAPI BOOL CALLTYPE LPNR_DownloadImageFile(HANDLE h, LPCTSTR strPath);

// helper functions
// ��ȡ�����IP�����Ǹ�һ��application���Ӷ��ʶ���ʱ�����յ�һ����Ϣ��Ҫ֪�������Ϣ������̨ʶ������͵�ʱ�򣬿��Ե������������
DLLAPI BOOL CALLTYPE LPNR_GetMachineIP(HANDLE h, LPSTR strIP);
// ��ȡ�����label��name of camera configured��
DLLAPI LPCTSTR CALLTYPE LPNR_GetCameraLabel(HANDLE h);
// new functions to obtain version and model
DLLAPI BOOL CALLTYPE LPNR_GetModelAndSensor(HANDLE h, int *modelId, int *sensorId);
DLLAPI BOOL CALLTYPE LPNR_GetVersion(HANDLE h, DWORD *version, int *algver);
DLLAPI BOOL CALLTYPE LPNR_GetCapability(HANDLE h, DWORD *cap);
// ��ȡʶ���������ȵĿ��(cx)�͸߶�(cy)����λ�����أ�
DLLAPI BOOL CALLTYPE LPNR_GetImageResolution(HANDLE h, int nSizeCode, int *cx, int *cy);
// �յ�EVT_ONLINE�󣬵�������������������֪����ǰ�����ϵͳʱ�䣬�Լ������ϵͳʱ��ͼ����ϵͳʱ���(��λsec)
// ע�⣬���ʶ�����ʱ���ֻ�����ߵĹ��̻��յ�һ�Σ����Բ���һֱ���µġ������յ�������Ϣ�󼴿̴����ж��Ƿ���Ҫ��ʱ��
DLLAPI BOOL CALLTYPE LPNR_GetCameraTime(HANDLE h, INT64 *longtime);
DLLAPI BOOL CALLTYPE LPNR_GetCameraTimeDiff(HANDLE h, int *sec);

// control camera's lighting control output. 
// onoff - 1 ON, 0 OFF
// msec - if 'onoff'==1, this argument set 'msec' to turn on. if 'msec' is 0 means turn on until explicitly turned off.
DLLAPI BOOL CALLTYPE LPNR_LightCtrl(HANDLE h, int onoff, int msec);
// control camaera's IR-cut lens.  'onoff' 1 apply IR-cut filter, 0 turn off IR-cut filter ����������������˹�Ƭ���أ�һ�㲻���õ���
DLLAPI BOOL CALLTYPE LPNR_IRCut(HANDLE h, int onoff);

// query function (use UDP, ����Ҫ�Ƚ�������) - �����������Ƹ�һ��������ϵͳ�������м��ٸ������ʹ�õġ�����Ҫ����
// ��ÿ̨���߻���TCP���ӣ��������ÿ̨�������ѯ��
// ��strIP' ��Ҫ��ѯ���������IP��ַ��'strPlate'����������һ��ʶ��ĳ��ƺţ�'tout' is response timed out setting in msec 
// ����FALSE��ʾ�������û����ָ��ʱ����Ӧ��
DLLAPI BOOL CALLTYPE LPNR_QueryPlate( IN LPCTSTR strIP, OUT LPSTR strPlate, int tout );
// ʹ��UDP��ȡ�����ͼƬ����ȡ����ͼƬ������BMP����JPEG
// ���� strIP�ǳ�λ�����IP, imgbuf�ǽ���ͼƬ�ĵ�ַ, size��imgbuf�ĳ���(�ֽ�), imgid���ĸ�ͼ��(IID_CAP~IID_HEX) format����Ҫ��ͼƬ��ʽ(0 ԭʼYUV��ʽ, 1 JPEG, 2 BMP)
// tout�ǳ�ʱʱ�䣨��λmsec�����������ʱ�仹û�յ��������꣬���ش���
// ����ֵ���յ���ͼƬ���ȣ����������ͬ������ʽ�ġ��ɹ�ʱ�����յ���ͼƬ���ȣ�-1��ʾʧ�ܣ�ץ�Ļ�û����Ӧ��0��ʾ���ղ���ͼƬ����ͼƬû�н�������
// ����ʹ��UDP���͡���Ҫ���UDP��ƴ�ӳ�һ��ͼƬ�����粻��������ܾ�������������ʱ�����JPEG��ʽ�����Լ���������߳ɹ���
// NOTE: ��λ��� AR130 ֻ�ᷢ��ȫ������ͼ������imgid������򲻹ܣ�ȫ������ȫ������(130������)
DLLAPI int CALLTYPE LPNR_QueryPicture(IN LPCTSTR strIP, OUT BYTE *imgbuf, int size, int imgid, int format, int tout);
#if defined _WIN32 || defined _WIN64
// ���APû���Լ�����Winsock��ϵͳ��һ��ĸ߽׿�����ܶ����Զ�����winsock��������Ҫʹ�������������Winsock��̬��ſ��������������������ֻ��Ҫ��һ�μ��ɡ�
// ����ʹ�ö���ʶ�����
DLLAPI BOOL CALLTYPE LPNR_WinsockStart( BOOL bStart );
#endif

// extended DI/DO functions
// get current ext. DI (dio_val[0]) and DO (dio_val[1]) value, one bit represent one I/O point status. Bit set means corresponding DIO is ON
DLLAPI BOOL CALLTYPE LPNR_GetExtDIO(HANDLE h, short dio_val[]);
DLLAPI BOOL CALLTYPE LPNR_SetExtDO(HANDLE h, int pin, int value);
// output 50% duty cycle pulse train with 'period' (msec). count is pulse count. if count==0 means forever until stop explicitly (period 0 means stop).
DLLAPI BOOL CALLTYPE LPNR_PulseOut(HANDLE h, int pin, int period, int count);		

// [��Ƶ���ӿ��ƣ�������H264/H265�������Ƶ�ϵ����ַ���ͼƬ]
// Video OSD (On Screen Display) overlay functions
// OSD ����8������ʶ���ϵͳֻʵ��6��������ǰ��5���Ǿ�̬���ã�Ҳ���ǻᱣ���������ļ����棬ʶ��������ϵ粻�ᶪʧ�����һ���Ƕ�̬���ӣ����ṩ��
// Ӧ�ó�����ʱ���Ӷ����ַ�����Ƶ��ָ����λ�á���������ڳ���������ᶪʧ��
// ��������:
// 1 - ʱ���ǩ - �̶��ǵ�������Ƶ���Ͻǡ�
// 2 - ʶ�����ǩ����ʶ��������������渳���ʶ������ƣ����� "԰��վE02" �̶���������Ƶ�Ķ�������λ�á�
// 3 - logoͼƬ�����ǵ��ǵ�logo�������滻logo�ļ��ı�logo���ݡ��ļ���ʶ����ڲ� /home/logo.bmp. ͼƬ��С������������ռ̫ͨ��Χ�����ͼƬ����λ������Ƶ���Ͻǡ�
// 4 - ROI ���� ���ӵ�ǰʶ�������õ������Ρ�
// 5 - ����ʶ���ִ� - ���ӵ�ǰʶ������ÿ��ʶ�����仯�ͻ��Զ��滻Ϊ�µ��ִ���ʶ��������ʾλ�á�פ��ʱ���͵���Ч���������á�
// 6 - �û������� ��������ǿ��Ÿ��û���������Լ�������ѶϢ�����ֿ����Ƕ��У��������ѡ������롢�Ҷ��롢���ж��룬�ַ���ɫ��͸���ȶ��������á�
//      Ϊ�˼򻯽ӿڣ����ڲ����õ�������ʾЧ�������ֵ�ɫ����ɫ��͸���ȣ���ʾʱ�䣬�Ƿ񵭳����Ƿ���˸��ʾ����˸��ռ�ձȵȣ���ʱ���ṩ����Щ����
//	    ��Ŀǰ�Ľӿ��������������ģ������޵�ɫ���޵�ɫ�����������������)����ʾʱ��Ϊһֱ��ʾֱ�������������˸����������
// ���º��������OSD�����ṩ�Ŀ��ƽӿڣ�

// ������ƵOSD���ӵ�ʱ���ǩλ�ã�bEnableΪTRUEʹ�ܣ�FALSE���ܵ���; x, yΪ����λ�ã���Ƶ���Ͻ�Ϊԭ�㣩����(0,0)ʹ��Ĭ��ֵ�����Ͻǣ�
DLLAPI BOOL CALLTYPE LPNR_SetOSDTimeStamp(HANDLE h, BOOL bEnable, int x, int y);
// ʹ��/������Ƶ�����������ǩ�� ��ǩ���֡�label���������NULL����ͬʱ����ʶ�����ǩ����ʹbEnable��FALSE��
DLLAPI BOOL CALLTYPE LPNR_SetOSDLabel(HANDLE h, BOOL bEnable, const char *label);
// ʹ��/����LOGOͼƬ�ĵ���
DLLAPI BOOL CALLTYPE LPNR_SetOSDLogo(HANDLE h, BOOL bEnable);
// ʹ��/����ROI���εĵ���
DLLAPI BOOL CALLTYPE LPNR_SetOSDROI(HANDLE h, BOOL bEnable);
// ʹ��/���ܳ���ʶ�����ĵ���
// ���У�'bEnable' �����Ƿ����ʶ��ĳ��ơ�'loc'������ʾ��λ�ã�ȡֵ��ΧΪ[1..3] �ֱ�������½ǣ��м���Ե�����½ǡ�'dwell'�ǵ�����ʾ��פ��ʱ��(��λsec).
//            dwell��ֵ0��ʾһֱ��ʾֱ���µĳ���ʶ���������'bFadeOut' �Ƿ���ϵ���Ч����ֻ��dwell>0ʱ������Ч������Ч������ʱ��̶���2�롣����������ʾ
//            ʱ���� dwell + 2 �롣
//            ������ϸ�µĿ��ƣ��������塢�ֺš�������ɫ��͸���ȵȣ��û���Ҫʹ�õ��ǵ�LPNRTool.exe���ã����ﲻ���Žӿڡ�
DLLAPI BOOL CALLTYPE LPNR_SetOSDPlate(HANDLE h, BOOL bEnable, int loc, int dwell, BOOL bFadeOut);
// �ر��û����ֵ���
DLLAPI BOOL CALLTYPE LPNR_UserOSDOff(HANDLE h);
// ��ʾ�û���������
// 'x', 'y' Ϊ����λ�ã���λ������,���Ͻ�Ϊ(0,0). ���'x'Ϊ -1~-9ʱ���������λ���ھŹ�����ĸ�����-1Ϊ���ϣ�-2Ϊ���ϣ�-3Ϊ����...-9Ϊ���¡�x<0ʱ��yֵ��Ч��
// 'alignΪ���뷽ʽ��1������룬2�Ǿ��ж��룬3���Ҷ���
// 'fontsz' �������С��ȡֵ��Χ24��32��40��48 ��������ֿ����û��ָ�����ֺţ���ȡ���ƣ�
// 'text_color'��������ɫ��RGBֵ
// 'opacity'�ǵ������ֵĲ�͸���Ȱٷֱȣ�ȡֵ��Χ[0~100]��
// 'text' ��Ҫ���ӵ����֣������Ƕ����֣������еĻ��з���'\n'�ᱻ�Ƴ�������������ʾ����һ�С�
DLLAPI BOOL CALLTYPE LPNR_UserOSDOn(HANDLE h, int x, int y, int align, int fontsz, int text_color, int opacity, const char *text);
// ѡ�������ͱ����� - �������ʹ������Ƿ���С�����
// 'encoder' - 0: H264, 1: H265
// 'smallMajor' TRUE ʹ��4/9����ȵ�������, FALSEʹ��ȫ����ȵ�������
// 'smallMinor' TRUE ʹ��1/16����ȵĴ�����, FALSEʹ��1/4����ȵĴ�����
DLLAPI BOOL CALLTYPE LPNR_SetStream(HANDLE h, int encoder, BOOL bSmallMajor, BOOL bSmallMinor);
// ץ��ͼƬ���� 
// ���Ƕ�Ӧ��LPNRTool.exe�������������ץ��ͼƬ������֣�Ҳ����ÿ��ʶ���꣬����ָ��Ҫ�����ЩͼƬ
// 'bDisFull' - TRUE ��ֹ���ȫ�����ץ��ͼ
// 'bEnMidlle' - TRUE ���ץ����ͼ(4/9�����)
// 'bEnSmall' - TRUE ���ץ��Сͼ(1/4�����)
// 'bEnHead' - TRUE �����ͷͼ (720x576 CIF��С)
// 'bEnTiny' - TRUE ���ץ��΢ͼ (1/16�����)
DLLAPI BOOL CALLTYPE LPNR_SetCaptureImage(HANDLE h, BOOL bDisFull, BOOL bEnMidlle, BOOL bEnSmall, BOOL bEnHead, BOOL bEnTiny);
DLLAPI BOOL CALLTYPE LPNR_GetCaptureImage(HANDLE h, BOOL *bDisFull, BOOL *bEnMidlle, BOOL *bEnSmall, BOOL *bEnHead, BOOL *bEnTiny);

// ͨ�û�ȡͼƬ�ӿ�
#define IMGID_PLATEBIN				0		// ���ƶ�ֵͼ
#define IMGID_PLATECOLOR		1		// ���Ʋ�ɫͼ
#define IMGID_CAPFULL				2		// ץ��ȫͼ
#define IMGID_CAPMIDDLE			3		// ץ����ͼ (4/9�����)
#define IMGID_CAPSMALL			4		// ץ��Сͼ (1/4�����)
#define IMGID_CAPTINY				5		// ץ��΢ͼ (1/16����1/25, ��ץ�Ļ�����)
#define IMGID_CAPHEAD				6		// ץ�ĳ�ͷͼ
#define IMGID_BUTT						7
#define IMGID_CAPLARGEST		11		// ���ץ��ͼ��������ץ��ͼ������)
#define IMGID_CAPSMALLEST		12		// ��Сץ��ͼ (������ץ��ͼ����С��)
#define IMGID_CROPIMAGE(n)		(20+(n))	// ��ȡ�ü�ͼƬ֡��n=1~10�����õĲü�����
// ��������������ͨ�û�ȡͼƬ�ķ�ʽ��'imageId' ����ָ����Ҫ�ĸ�ͼƬ, application ����ͼƬ��buffer��ַ��ȷ��buf���ȴ��ڵ���ͼƬ���ȡ�
DLLAPI int CALLTYPE LPNR_GetSelectedImageSize(HANDLE h, int imageId);
DLLAPI int CALLTYPE LPNR_GetSelectedImage(HANDLE h, int imageId, void *buf);
// ���º����ǻ�ȡָ��ͼƬ��ָ�룬'imagesize'�ڷ��غ�ᱣ���ͼƬ�Ĵ�С�����������ֱ�ӷ��ض�̬�����汣���ͼƬ���ݵ�ָ�룬�����Ǹ���ͼƬ
// ����Ч�ʸߣ�����application�����Ըı����ݡ����Ҫ��ʱ��ʹ�����ָ�룬��ҪLPNR_Lock/LPNR_Unlock��ͼƬ���ݼ���������ͼƬ������ʹ�ù�����
// ���޸ģ�������������ɶ�̬���߳̽���ͼƬʱ��suspend��һֱ���û�����LPNR_Unlock()��Żָ���������Ҫ����ʹ�á�
DLLAPI const void * CALLTYPE LPNR_GetSelectedImagePtr(HANDLE h, int imageId, int *imgsize);

// �յ��¼�EVT_FIREDʱ�����ô˽ӿڻ�ȡ����Դ, ����ֵΪ����֮һ
#define	TRG_UPLOAD				0		// ����ͼƬ����
#define	TRG_HOSTTRIGGER		1 		// ��λ������
#define	TRG_LOOPTRIGGER		2 		// ץ����Ȧ����
#define	TRG_AUTOTRIG			3		// ��ʱ�Զ�����
#define	TRG_VLOOPTRG			4 		// ������Ȧ��ⴥ��
#define TRG_OVRSPEED 			5		// ���ٴ���ץ��
DLLAPI int		  CALLTYPE LPNR_GetTriggerSource(HANDLE h);

// ͸������ - ������д����ô��ڵģ�����ʹ�����´���͸���������������ݸ���������������ת�������ڡ���������յ��Ĵ�������
//                  ���Ի��͸������������൱���ڴ����豸������LED��ʾ�����ͼ����֮��һ������Ĵ��ڡ����������ᴫ�������
//                  ֻ�������ݵ���ת��
// ��/�ر�͸������, speed�ǲ�����, Speed=0 Ϊ�رմ���, >0 �򿪴������øò����ʡ�ֻ����8bit data, 1-stop bit, no parity
DLLAPI BOOL CALLTYPE LPNR_COM_init(HANDLE h, int Speed);
// ʹ�ܴ�������Ľ��գ�Ĭ���ǲ�ʹ�ܵģ�ʶ����յ��������벻�ᷢ�͸�û��ʹ�ܵĿͻ��ˡ�������ʹ�ܵĿͻ��˶����յ�ͬ�������ݣ�
// ��̬���յ��������ݺ󣬻ᷢ���¼�EVT_ASYNRX��Windows��Ϣ���ǻص�������
DLLAPI BOOL CALLTYPE LPNR_COM_aync(HANDLE h, BOOL bEnable);
// ���ʹ�������
DLLAPI BOOL CALLTYPE LPNR_COM_send(HANDLE h, const char *data, int len);
// �յ��Ĵ������ݣ���̬�Ᵽ�����ڲ�1K��ring buffer���棬�û�ʹ�����º�����ȡRX����ѶϢ
DLLAPI int CALLTYPE LPNR_COM_iqueue(HANDLE h);		// ��ȡ���յ��Ĵ����ֽ�������δ��ȡ�ߣ�
// ���ƶ�̬�Ᵽ��Ĵ���RX���ݣ���size�ֽڡ���������ݲ����Ƴ���
// ����ֵ: ʵ�ʸ��Ƶ��ֽ��������size > ring buffer�ڿɶ����ֽ���������ֵ��ʵ�ʸ��Ƶ��ֽ���(Ҳ����ring buffer�ڿɶ��ֽ���)��
// ���size <= ring buffer ������ֽ���������ֵ����size.
DLLAPI int CALLTYPE LPNR_COM_peek(HANDLE h, char *RxData, int size);
// �����ϸ����������������ڶ�ȡ����ring buffer���Ƴ���
DLLAPI int CALLTYPE LPNR_COM_read(HANDLE h, char *RxData, int size);
// ��ring buffer���Ƴ�ǰ��'size'�ֽ�
DLLAPI int CALLTYPE LPNR_COM_remove(HANDLE h, int size);
// �����̬���ڱ�������д��ڽ�������
DLLAPI BOOL CALLTYPE LPNR_COM_clear(HANDLE h);
// note:
// LPNR_COM_peek + LPNR_COM_remove = LPNR_COM_read

// log operation
// ��ʱ����/��ֹlog. Log����ֹ��ʱ�򣬵���LPNR_UserLog�������á�Ĭ����enable.
// h �����NULL, ��ʾ����Ĭ��ֵ��֮�󴴽���LPNR�����Ĭ��log�ǹر�(bEnable=FALSE)���ǿ�����(bEnable=TRUE)������Ӱ���Ѿ�������LPNR����
DLLAPI BOOL CALLTYPE LPNR_EnableLog(HANDLE h, BOOL bEnable);
// ������־�ļ������·��, Ĭ���� /rwlog
// h�����NULL����ʾ����Ĭ��·����֮�󴴽���LPNR�������־·������'Dir'Ŀ¼���档��־�ļ�����·����Ϊ; 'Dir'/LPNRDLL-<IP>.log
DLLAPI BOOL CALLTYPE LPNR_SetLogPath(HANDLE h, const char *Dir);
// ������־�ļ�ÿ���Ĵ�С'size' KB, �Լ��������ݵ���ĿΪ'count'����log�ļ������������ʱ�����رղ��ҹ������ݵ���׺-01.log���ļ���ԭ��-01.log����Ϊ-02.log
// һֱ��-<count-1>���������һ���ļ� -<count>.logΪֹ��ԭ����-<count>.log�ļ���ɾ��������������ݻ��ƾ�������������ౣ�漸����־�ļ��Լ�ÿ���ļ��Ĵ�С��
DLLAPI BOOL CALLTYPE LPNR_SetLogSizeAndCount(HANDLE h, int size, int count);
// �û�д�Լ�����־��LPNR����־�ļ����棬������Ҫ��������ʱ�������������ҵ��û�����Ȥ�����ݡ�
DLLAPI BOOL CALLTYPE LPNR_UserLog(HANDLE h, const char *fmt,...);

#if defined _WIN32 || defined _WIN64
// ��PROXY��
// ����������ö�̬����Ϊһ��PROXY����һ���������ڣ�ÿ�����������˫���ڣ�һ�����ڽ�ʶ����Լ������������豸����һ�����ڽӷ�����ʱ����ô�ھ�����
// �ϣ�LPNRTool.exe�Ƿ��ʲ���ÿ��������ʶ����������Ҫ���»��ǿ�����־�����Ǹı����ã���Ҫһ��һ������ȥ�ֳ����Ӳ�����������������������ٹ�·����
// ������»���˷�ʱ�䡣ʹ��PROXY�󣬶�̬����ʶ���һ����һ��ͬ���Ķ˿ڣ����Ա�LPNRTool�����Լ����ӡ����Ӻ���Բ�����������ֱ�������������顣
// ������Ա�����ڿ�������һ̨һ̨���Ӹ��»��Ǹı����ã���ˣ��������̵�ϵͳ���������������ʱ��ǿ�ҽ���ʹ��PROXY���ܡ�
// �����λ�������Ӷ�̨ʶ����Ļ����Ǿ�ֻ��ʹ������һ̨����Ϊ�˿��ǹ̶��ġ�����취�ǳ������������λ���������󶨶��IP��ÿ̨�����PROXY��һ��IP��
// 'Proxy_Init' ʹ��PROXY���ܣ�������PROXY�̡߳�'HostIP' ��ָҪ�󶨶˿ڵ�IP��ַ(��������������ж�����ڣ�ֻ����ϼ����ʵ�����), 
// �����NULL, ��̬���󶨶˿ڵ���������IP��
// 'Proxy_Terminate'����PROXY���ܣ��������ӱ�PROXY�Žӵ����������λ������ᱻ�Ͽ���
// 'Proxy_GetClients' ��ȡ���о���PROXY�Ž�������Ŀͻ���IP�� 'strIP'�Ƕ�ά���飬��������ͻ���IP��ַ, 'ArraySize'��'strIP'��һ��ά�ȵ������С��
// ����ֵ�Ǳ��浽'strIP'��Ŀͻ���IP��ַ�ĸ�����Ҳ�����м����ͻ��ˣ�
DLLAPI BOOL CALLTYPE Proxy_Init(HANDLE h, const char *HostIP);
DLLAPI BOOL CALLTYPE Proxy_Terminate(HANDLE h);
DLLAPI int	 CALLTYPE Proxy_GetClients(HANDLE h, char strIP[][16], int ArraySize);
#endif 

#ifdef __cplusplus
}
#endif

#endif

