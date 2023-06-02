#ifndef TFI_HEADER_INCLUDED
#define TFI_HEADER_INCLUDED

typedef void * HANDLE;
typedef int BOOL;
typedef unsigned char BYTE;
 
#ifdef linux
#define _API_EXPORT
#define CALLING_CONV
typedef unsigned long COLORREF;
#else
#define _API_EXPORT __declspec(dllexport)
#define CALLING_CONV __stdcall
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef short SHORT;


_API_EXPORT HANDLE CALLING_CONV TFI_Create();
_API_EXPORT BOOL CALLING_CONV TFI_Destroy(HANDLE h);
#ifdef linux
_API_EXPORT BOOL CALLING_CONV TFI_OpenCom(HANDLE h, const char *tty_device);
#else
_API_EXPORT BOOL CALLING_CONV TFI_OpenCom(HANDLE h, int iComID);
#endif
_API_EXPORT BOOL CALLING_CONV TFI_OpenNet(HANDLE h, const char *chIP);

_API_EXPORT BOOL CALLING_CONV TFI_Close(HANDLE h) ;
_API_EXPORT BOOL CALLING_CONV TFI_ClearAll(HANDLE h);
_API_EXPORT BOOL CALLING_CONV TFI_GetDeviceInfo(HANDLE h, char *pInfo );
_API_EXPORT BOOL CALLING_CONV TFI_SendLine(HANDLE h,int nLineNo,COLORREF nColor,int nRollType,int nRolltime,const char *szText);
//_API_EXPORT BOOL CALLING_CONV TFI_SendMultiLine(HANDLE h, COLORREF nColor, const char *text);
_API_EXPORT BOOL CALLING_CONV TFI_SetMode(HANDLE h,BYTE mode,char *pRowType);

_API_EXPORT BOOL CALLING_CONV TFI_AlarmSet(HANDLE h,BYTE bAlarm,BYTE nTime);
// vul 音量，可调范围0-7
_API_EXPORT BOOL CALLING_CONV  TFI_SendVoice(HANDLE h, int vul,  const char* stVoice);
_API_EXPORT BOOL CALLING_CONV TFI_SetPassLight(HANDLE h,int x,int y,BYTE bPass);
_API_EXPORT BOOL CALLING_CONV TFI_CLEAR_PIP_MODE(HANDLE h,BYTE nLineNo);
_API_EXPORT BOOL CALLING_CONV TFI_DISPLAY_PIP_MODE(HANDLE h,BYTE nLineNo,SHORT x,SHORT y,BYTE lineH,COLORREF nColor,const char *szText);

// Logger
_API_EXPORT BOOL CALLING_CONV TFI_EnableLog(HANDLE h, BOOL bEnable);
_API_EXPORT BOOL CALLING_CONV TFI_SetLogPath(HANDLE h, const char* Path);

// 0 - 回调，QrCode 数据为二维码文本讯息，
typedef void (*TFI_CallBack)( const char *QR_code_buf );
_API_EXPORT BOOL CALLING_CONV TFI_SetCallBack( TFI_CallBack cb);

#define	CMD_GET_INFO		0x02
#define	CMD_ALRAM_CTL		0x03
#define	CMD_CLEAR			0x04
#define	CMD_SET_MODE		0x05
#define	CMD_SHOW_LINE		0x20
#define CMD_SET_PICUTRE		0x21
#define	CMD_SET_PASSLIGHT	0x33
#define	CMD_PIP_DISPLAY		0x24
#define	CMD_PIP_CLEAR		0x25

#define	MODE_ROW	1
#define	MODE_PIC	2
#define	MODE_MIX	3
#define	MODE_PAGE	4

#ifdef __cplusplus
}
#endif

#endif


