#ifndef _LPNR_PROTOCOL_INCLUDED_
#define _LPNR_PROTOCOL_INCLUDED_
/*
 * This header file defines data communication for hitool lpnr function with PC tool.
 */
#if !defined linux || !defined __HI_TYPE_H__
typedef unsigned int		HI_U32;
typedef signed int			HI_S32;
typedef unsigned short	HI_U16;
typedef signed short		HI_S16;
typedef unsigned char		HI_U8;
typedef signed char			HI_S8;
typedef enum {
    HI_FALSE = 0,
    HI_TRUE  = 1,
} HI_BOOL;
#endif

#if !defined SPOINT_SRECT_DEFINED
#define SPOINT_SRECT_DEFINED
typedef struct {
	short	x;
	short	y;
} HI_SPOINT, *PHI_SPOINT;

typedef struct {
   unsigned short	x;
   unsigned short	y;
   unsigned short	width;
   unsigned short	height;
} HI_SRECT, *PHI_SRECT;
#endif

#define SZ_LABEL			20		// ��������ֳ���

#ifndef PLATE_TYPE_DEFINED
#define PLATE_TYPE_DEFINED
typedef enum {
	PLT_ERROR = -1,
	PLT_UNKNOWN = 0,
	PLT_REGULAR,		// regular plate for civilian
	PLT_NENRGY,			// new energy (is also one kine of regular plate)
	PLT_POLICE,			// police vehicle
	PLT_ARMPOL,			// armed policy (�侯)
	PLT_MILTRY,			// military vehicle
	PLT_HKMACAU,		// HK, Macao (�۰ĳ���)
	PLT_EMBSY,			// embassy vehicle (ʹ�ݳ�)
	PLT_CNSLT,			// consulate (���¹ݳ�)
	PLT_TRAINEE,		// trainee vehicle
	PLT_TRAILER,		// truck trailer (�ҳ�)
	PLT_HARBOR,			// �ۿڣ������ڲ�����, ����:���H1234, ��XXXX
	PLT_SPECIAL,		// ���⳵��
	PLT_CUSTOMIZED,	// ���ƻ�����
} PLATE_TYPE_E;
#endif

#ifndef PLATE_COLOR_TYPE_DEFINED
#define PLATE_COLOR_TYPE_DEFINED
typedef enum {
	PLC_ERROR = -1,
	PLC_UNKNOWN,
	PLC_BLUE,
	PLC_YELLOW,
	PLC_WHITE,
	PLC_BLACK,
	PLC_GREEN,
	PLC_YELGREEN,
	PLC_BUTT
} PLATE_COLOR_E;
#endif

// ������Դö������
typedef enum { 
	IMG_UPLOAD=0, 			// ����ͼƬ����
	IMG_HOSTTRIGGER, 		// ��λ������
	IMG_LOOPTRIGGER, 		// ץ����Ȧ����
	IMG_AUTOTRIG, 			// ��ʱ�Զ����� (ǿ�Ȳ�����)
	IMG_VLOOPTRG, 			// ������Ȧ��ⴥ��
	IMG_OVRSPEED, 				// ���ٴ���ץ��
	IMG_UNKNOWN=99
} TRIG_SRC_E;

// ʶ��ͼƬ��Сö������
typedef enum { 
	LARGE_IMAGE=0, 
	MIDDLE_IMAGE, 
	SMALL_IMAGE, 
	TINY_IMAGE 
} LPNRIMG_E;
 
// TCP �� UDP �˿� 
#define PORT_LISTEN		6008		// TCP
#define PORT_SEARCH		6009		// UDP

// [Data Type]
#if 0 // for big endian CPU architecture. we want to byte stream sending in sequence of AA BB CC xx
#define DTYP_IMAGE	0xaabbcc00		
#define DTYP_CONF		0xaabbcc11
#define DTYP_EXCONF	0xaabbcc12
#define DTYP_H264CONF  0xaabbcc13
#define DTYP_DATA		0xaabbcc22
#define DTYP_CTRL		0xaabbcc33
#define DTYP_TEXT		0xaabbcc44
#define DTYP_TIME		0xaabbcc55
#define DTYP_UPDSW	0xaabbcc66
#define DTYP_UDP		0xaabbcc77
#define DTYP_HBEAT	0xaabbcc88
#define DTYP_EVENT	0xaabbcc99
#define DTYP_QUERY	0xaabbccaa
#define DTYP_REPLY	0xaabbccf0		// reply DTYP_QUERY
#define DTYP_ACK		0xaabbccdd
#define DTYP_MASK		0xffffff00
#define DTYP_MAGIC	0xaabbcc00
#else		// for little endian CPU architechure.
#define DTYP_IMAGE	0x00ccbbaa		// camera -> host
#define DTYP_CONF		0x11ccbbaa		// both way
#define DTYP_EXCONF	0x12ccbbaa
#define DTYP_H264CONF  0x13ccbbaa
#define DTYP_OSDCONF	0x14ccbbaa
#define DTYP_OSDTEXT	0x15ccbbaa
#define DTYP_DATA		0x22ccbbaa		// camera <-> host 
#define DTYP_CTRL		0x33ccbbaa		// host -> camera
#define DTYP_TEXT		0x44ccbbaa		// camera -> host
#define DTYP_TIME		0x55ccbbaa		// host <-> camera
#define DTYP_UPDSW	0x66ccbbaa		// host -> camera
#define DTYP_UDP		0x77ccbbaa		// host -> camera (UDP command - search and ping)
#define DTYP_HBEAT	0x88ccbbaa		// both way
#define DTYP_EVENT	0x99ccbbaa		// camera -> host
#define DTYP_QUERY	0xe0ccbbaa		// QUERY camera (host -> camera)
#define DTYP_REPLY	0xf0ccbbaa		// reply DTYP_QUERY (cameera -> host)
#define DTYP_ACK		0xddccbbaa		// host -> camera
#define DTYP_MASK		0x00ffffff
#define DTYP_MAGIC	0x00ccbbaa

#endif

// [Data Id]
// image Id - full frame image (when data type is DTYP_IMAGE)
#define IID_LIVE			0
#define IID_CAP				1		// captured image (or uploaded image level 0)
#define IID_HEAD			2		// ��ͷͼƬ��CIF�ߴ磬�Գ���Ϊ���ģ�����ʶ����Ϊ���ģ�
#define IID_QUAD			3		// 1/4����ȵ�ȫͼ
#define IID_T3RD			4		// 4/9�����ͼ
#define IID_HEX				5		// 1/16 or 1/25�����ͼ
#define IID_RAWYUV		6		// Raw YUV422SP or YUV420SP image
#define IID_CAPOSD		7		// �����û�ָ���ַ����ץ��ȫͼ
#define IID_QUADOSD		8		// �����û�ָ���ַ����ץ��1/4ͼ
// image Id - image of plate(s) area
#define IID_PLRGB			11		// plate color rectangular (level 1)
#define IID_PLBIN			12  	// plate binarized (level 1)
#define IID_PLBINCLR	13		// plate binarized after de-clutter (level 1)
#define IID_PLMSKTRM	14		// plate characters group rectangular (level 2)
#define IID_PLMSK			15		// trimed plate binarized image after filter (level 2)
// image of cropped area
#define IID_CROP			19		// user cropped image, ImageAttr.basename will be 'crop1.bmp', 'crop2.bmp'... crop<n>.bmp.. N is cropping area
// image Id - image of plate characters
#define IID_PCCUT			21		// cut
#define IID_PCROT			22		// rotated
#define IID_PCTWSTH		23		// twisted horizontal
#define IID_PCTWSTV		24		// twisted vertical
#define IID_PCNORM		25		// normalized
// other images
// other special cameras
#define IID_VIDOB				51		// Video OB image (1/4����ȣ��е���ROI���Ķ�ֵͼ)
#define IID_QRCODE			52		// QCIF image for QR-Code analysis (gray and binary)
#define IID_QRCIMAGE		53		// qrcode image cropped from captured frame
#define IID_LIVEQCIF		54		// QCIF live frame for motion analysis

// data Id (when data type is DTYP_DATA)
#define DID_BEGIN		0		// begin of LPNR process data/image sending
#define DID_PLATE		1		// plate data
#define DID_TIMING	2		// timing data
#define DID_END			3		// end of LPNR process data/image sending
#define DID_HBEAT		9		// heart beat (reserved)
#define DID_VERSION	10	// firmware version
#define DID_ISP			11		// ISP current setting
#define DID_SHMSTAT	12		// LPNR statistic in shared memory
#define DID_SNKEY		13		// S/N and activiated or not
#define DID_NETCONF	14		// IP, Netmask and default gateway
#define DID_CFGDATA 15		// �������������
													//   DataAttr.val[0] - ��ҹ�л����عֵⷧ
													//   DataAttr.u16Val[2] - ��ʶ����������ʱʱ��(ms)
													//   DataAttr.u16Val[3] - ����ֵ (km/hr)
													//	 DataAttr.u16Val[4] - ����ץ�ĵ�������Ȧ���� (mm)
													//	 DataAtt1r.u16Val[5] - �Ƿ�û�г���Ҳץ�ģ�0-no��1-yes�� 1.3.5.20 �汾����
													//   DataAttr.val[3] - ���⳵��ʹ��
// data Id (for lane controller parts, data type is still DTYP_DATA
#define DID_EXTDIO	101		// external GPIO state (response to CTRL_READEXTDIO)
#define DID_COMDATA	102		// �����첽��������, header.size�����ݳ��ȣ�payload����������
// data Id (when data type is DTYP_UDP)
#define DID_SEARCH		21
#define DID_PING				22
#define DID_ANSWER		23		// answer search
#define DID_TIME				24		// ��ʱ֡ (header��TimerAttr�ṹ��ʱ��)
#define DID_CTRL				25		// UDP����֡������TCP�Ŀ��Ʒ�ʽ�����Խ��ܣ�header.ctrlAttr�ǿ�������

// data Id when data type is DTYP_QUERY. Ӧ����DTYP_REPLY, ͬ����Data Id�����ݸ���DataId����
// �����ѯ�������д��󣨲������������ѯ�����ڵĳ�λ), Reply��DataId��16λ�ǲ�ѯ��DataId, ��16λ�Ǵ�����
// DTYP_QUERY������TCP��Ҳ������UDP�����ԣ�DataId��100��ʼ��ţ���DTYP_UDPʹ�õ�DataId��������
// UDP�������ô���ֻҪ����DataId����Ϳ���
#define QID_PKPLATE	100		// ��ǰ���ƺ���,���ڶ೵�ƣ�dataAttr.val[0]Ϊ�ڼ�������. (1~x, 0��ʾȫ������)
												// Ӧ��ʱ������ѶϢ��header.plateInfo�ṹ���档header.plateInfo.plateId�ǳ�λ��(1-based)
#define QID_PICTURE	101	// ��UDPͨ����ȡһ��ץ��֡ dataAttr.val[0]Ϊ�ĸ������ȵ�ͼ (IID_CAP ~ IID_HEX, dataAttr.val[1]Ϊͼ��ʽ��ֻ֧��IFMT_JPEG����IFMT_BMP
// �ظ�ʱ��dataAttr.val[0]Ϊ�ܹ��з�Ϊ����UDP����dataAttr.val[1]Ϊ�ڼ�����, dataAttr.val[2]Ϊͼ���ܳ���, dataAttr.val[3] Ϊ��ǰ����offset
// Reply �Ĵ����루��Reply header��DataId��16λ)

#define QER_PARAM		1		// ��ѯ��������
#define QER_ILLEGAL	2		// ��ǰ״̬�������ѯ
#define QER_INVALID	3		// ��ѯ��QID������

// data Id (when data type is DTYP_TEXT )
#define TID_CAPTION		0			// show on host window caption
#define TID_PROMPT		1			// show a prompt dialog on host
#define TID_LONGTEXT		2			// long text after header. header.size gives text body (include terminaated \0).
#define TID_PLATENUM		3			// 2017-04-18 �����ӵĳ��ƺ�Text
#define TID_QRCODE		4			// QrCode text, if header.size>0, text is in payload, otherwise, it is in header.textAttr.text
#define TID_LOG				5			// ��־�ļ����

// data Id (when data type is DTYP_EVENT)
#define EID_TRIGGER		1			// an analysis is triggered.
#define EID_EXTDI			2			// external DI changed (dataAttr.val[0] di_this, val[1] di_last)
#define EID_VLDIN			3			// ��������������Ȧ���Ѿ���ȡ����ʶ������
#define EID_VLDOUT		4			// �����뿪������Ȧ
#define EID_NOCODE		51		// �����������ɨ��û�гɹ�����
#define EID_DNLDBGN		52		// ��ʼ�����ֶ��������ȫ���
#define EID_DNLDEND		53		// ���������ֶ��������ȫ���
#define EID_ACTIVATED	54		// ɨ���������ɹ�

// data Id (when data type is DTYP_CTRL)

// image format (enum for 'format' member in structure ImageAttr)
#define IFMT_JPEG				1
#define IFMT_BMP				2
#define IFMT_RGB				3
#define IFMT_YUV422SP		4
#define IFMT_YUV420SP		5
#define IFMT_YUV				6		// raw format, without any header
#define IFMT_BUTT				7
// definition of code member in structure CtrlAttr
#define CTRL_TRIGGER		1		// software trigger, no parameter
#define CTRL_IMLEVEL		2		// ����������𣬿ͻ���Ĭ����0��ֻ�пͻ��˷����������������Լ�������>0����ſ����յ�ʶ��ʧ�ܵ�ԭʼͼƬ����
#define CTRL_SETIP			3		// set IP address (param is new IP, also in options)
#define CTRL_LIVEFEED		4		// live feed on/off (param=0 off, 1:full resolution, 2: 1/4 resolution, 3: 1/16 resolution )
#define CTRL_ISP				5		// ISP control. param=0 means trying, param=1 means confirm (write to isp.conf)
														// bit map for param 
#define CTRL_ACTIVATE			6		// option is the activate key
#define CTRL_RESETSHM			7		// reset shm data
#define CTRL_RESEND				8		// resend LPNR key data (vin image, plate rgb image, plate bin image, plate info data, timing data, lpnr attribute data
#define CTRL_SNAPCAP			9		// take a snap input image if lighting is enabled also turn on lighting
														// and send the captured image to host (jpeg)
#define CTRL_TAKEFRAME		10		// get a YUV raw image frame from VI frame buffer from VB pool
#define CTRL_PLATEONLY		11		// only send recognition result (plate# and color). do not send any image result (param=1 plate_only, param=0 with image)
#define CTRL_LIGHT					12		// light control, ctrl.param 1 ���ƣ�0 �ص�. ctrl.option[0~3]�ϲ�Ϊint��Ϊ����ʱ�� (msec) - ֻ�п���ʱ��Ч��0��ʾһֱ���š�
#define CTRL_AUTOROI			13		// auto detect roi for vehicle separator camera
#define CTRL_SPEEDTRG			14		// ���ٴ����� param�ǳ���ֵ��ʹ���ں����������豸������ctrl.paramΪ�ٶ�ֵ (km/h)
#define CTRL_IRCUT					15		// ����IR-cut ctrl.param 1 ʹ��IR�˾���0 ��ʹ���˾�
#define CTRL_EQUALHIST			16		// ����ʵʱ֡���Ϊ�߶Աȣ�ȥ����ߺ����20%���ȵ����أ���ʹ�þ��⻯ֱ��ͼ��߶Աȣ�- ɨ�����������
#define CTRL_LOGALL				17		// QrCode camera����/�ر�LogAll, (ctrl.param ΪEnable/Disable)
#define CTRL_VIDEOOUT			18		// enable/disable analog video output (for testing)
#define CTRL_CROPVIN			19		// �ɼ�ʵʱͼƬ����ȡһ���ķ�������, paramΪ������ (1-based). ���õ�ͼƬ����ͼƬ֡ DTYP_IMAGE/IID_CROP/imageAttr.index==������ ����
														// ��� paramΪ0����ʾ�Ƕ�ָ̬���ü�����ctrlAttr.option�����ַ���ݾ��Ǽ������� (HI_RECT)��memcpy��һ��HI_RECT�ṹ����Ϳ��Ի�ȡ�ü�����
#define CTRL_PROXYLINK		20		// Proxy Server has setups both link and data is ready to be exchanged 
// lane controller parts
#define CTRL_READEXTDIO		101	// read external GPIO (camera �� DTYP_DATA, dataId=DID_EXTDIO��Ӧ, dataAttr.val[0] is di, dataAttr.val[1] is do)
#define CTRL_WRITEEXTDO		102	// write external DO (paramҪ���Ƽ����㣬ctrlAttr.option[i]�ڼ���Do�㣬ctrlAttr.option[i+1]ΪDo��ֵ��һ����ͬʱ����12������ࡣ
#define CTRL_DOPULSE			103	// output DO pulse (50% duty cycle), paramΪ point + count<<8 + (pulse period in msec)<<16. count Ϊ0��ʾһֱ����ֱ���������ø�point��DOֵ(CTRL_WRITEEXTDO)
#define CTRL_COMSET				111	// ���ô��ڲ�����, paramΪ������, ctrlAttr.option[0]�Ƿ����RX����. param=0 Ϊ�رմ���
#define	CTRL_COMTX				112	// ���ʹ������ݣ� header.sizeΪ���ݳ���, payload��Ҫ���͵�����
#define CTRL_COMASYN			113	// �Ƿ����EXTCOM��RX���� param=1���գ�0������(Ĭ��)��
// reboot
#define CTRL_REBOOT				999

// ISP control enabler bit-map													
#define ISP_WRITE_SETTING			0x00000001	// write setting to isp.conf
#define ISP_MASK_GAMA					0x0000000e   // bit1~bit4: Gamma curve (0:1.6, 1:1.8, 2:2.0, 3:2.2, 4:user; default is 1.6)
#define ISP_SHIFT_GAMA				1
#define ISP_MASK_AEW					0x000000f0   // Auto Exposure weighting table weighting within ROI (1~10)
#define ISP_SHIFT_AEW					4
#define	ISP_EN_WDR						0x00000100	// enable WDR (0 disable, 1 enable)
#define ISP_EN_ANTIFLICKER		0x00000200	// enable anti-flicker (0 disable, 1 enable)
#define ISP_DE_AUTOWB					0x00000400  // disable auto-white balance.
#define ISP_EN_DAYNIGHTGAMMA	0x00000800	// enable day/night gamma setting (sync with PARM_EN_DAYNIGHTSW) 
#define ISP_DE_AUTOEXP				0x00001000	// disable AE
#define ISP_MASK_MINAEEXPT		0x0000e000	// ����ع�ʱ����� * 50 (1~7) 
#define ISP_SHIFT_MINAEEXPT		13
#define ISP_MASK_DEFROGVAL		0x000f0000	// 3516A ȥ��ǿ�� (0~15)*16
#define ISP_SHIFT_DEFROGVAL		16					
#define ISP_EN_MIRROR					0x00100000
#define ISP_EN_FLIP						0x00200000
#define ISP_MASK_EXPTIME			0xffc00000	// manual exposure (10 bits) �ֶ������ع�ʱ��ʱ���ֵ����������
#define ISP_SHIFT_EXPTIME			22

#define ISP_GetGama(p)				( ((p) & ISP_MASK_GAMA) >> ISP_SHIFT_GAMA )
#define ISP_SetGama(p,v)			(p) = ( ((p) & ~ISP_MASK_GAMA) | ( (v) << ISP_SHIFT_GAMA) )
#define ISP_GetAEW(p)					( ((p) & ISP_MASK_AEW) >> ISP_SHIFT_AEW )
#define ISP_SetAEW(p,v)				(p) = ( ((p) & ~ISP_MASK_AEW) | ( (v) << ISP_SHIFT_AEW) )
#define ISP_ENABLE_WDR(p)			( ((p) & ISP_EN_WDR) != 0 )
#define ISP_SET_ENABLE_WDR(p)	((p) |= ISP_EN_WDR)
#define ISP_CLR_ENABLE_WDR(p)	((p) &= ~ISP_EN_WDR)
#define ISP_ENABLE_ANTIFLICKER(p)  ( ((p) & ISP_EN_ANTIFLICKER) != 0 )
#define ISP_SET_ENABLE_ANTIFLICKER(p)  ((p) |= ISP_EN_ANTIFLICKER)
#define ISP_CLR_ENABLE_ANTIFLICKER(p)  ((p) &= ~ISP_EN_ANTIFLICKER)
#define ISP_ENABLE_MIRROR(p)		( ((p) & ISP_EN_MIRROR) != 0 ) 
#define ISP_SET_ENABLE_MIRROR(p)  ((p) |= ISP_EN_MIRROR)
#define ISP_CLR_ENABLE_MIRROR(p)  ((p) &= ~ISP_EN_MIRROR)
#define ISP_ENABLE_FLIP(p)		( ((p) & ISP_EN_FLIP) != 0 ) 
#define ISP_SET_ENABLE_FLIP(p)  ((p) |= ISP_EN_FLIP)
#define ISP_CLR_ENABLE_FLIP(p)  ((p) &= ~ISP_EN_FLIP)
#define ISP_ENABLE_WRITE(p)		( ((p) & ISP_WRITE_SETTING) != 0 )
#define ISP_DISABLE_AWB(p)		( ((p) & ISP_DE_AUTOWB) != 0)
#define ISP_SET_DISABLE_AWB(p)	((p) |= ISP_DE_AUTOWB)
#define ISP_CLR_DISABLE_AWB(p)	((p) &= ~ISP_DE_AUTOWB)
#define ISP_DISABLE_AE(p)				( ((p) & ISP_DE_AUTOEXP) != 0)
#define ISP_SET_DISABLE_AE(p)		((p) |= ISP_DE_AUTOEXP)
#define ISP_CLR_DISABLE_AE(p)		((p) &= ~ISP_DE_AUTOEXP)
#define ISP_GetAEMinExpTime(p)	( ((p) & ISP_MASK_MINAEEXPT) >> ISP_SHIFT_MINAEEXPT )
#define ISP_SetAEMinExpTime(p,v)	(p) = ( ((p) & ~ISP_MASK_MINAEEXPT) | ( (v) << ISP_SHIFT_MINAEEXPT) )
#define ISP_GetExptime(p)					( ((p) & ISP_MASK_EXPTIME) >> ISP_SHIFT_EXPTIME )
#define ISP_SetExptime(p,v)				(p) = ( ((p) & ~ISP_MASK_EXPTIME) | ( (v) << ISP_SHIFT_EXPTIME) )
#define ISP_GetDefrogVal(p)			( ((p) & ISP_MASK_DEFROGVAL) >> ISP_SHIFT_DEFROGVAL )
#define ISP_SetDefrogVal(p,v)		(p) = ( ((p) & ~ISP_MASK_DEFROGVAL) | ( (v) << ISP_SHIFT_DEFROGVAL) )
#define ISP_GetNormGamma(u8v)	( ((double)(u8v)) / 10 )
#define ISP_IsEnableDayNightGamma(p)	 ( ((p) & ISP_EN_DAYNIGHTGAMMA) != 0)
#define ISP_EnableDayNightGamma(p,en) \
	do { \
		if ( en ) \
			p |= ISP_EN_DAYNIGHTGAMMA; \
		else \
			p &= ~ISP_EN_DAYNIGHTGAMMA; \
	} while (0)
														/* option array for CTRL_ISP:
														 * 0: Brightness compensation: 0 ~ 255, 128 is default
														 * 1: Saturation: 0 ~ 255, 128 is default
														 * 2: Sharpness: 0 ~ 255. 128 is default.
														 * 3: Shuttle Speed (the lowest) - 10~1000 (means 1/10~1/1000 sec.) (�����ֵ��speed/5, ��Ϊֻ��һ��byte) 
														 * 4: De-noize target threshold (0~255, 0 for none).
														 * 5: DRC strength (0~255, def:128)
														 * 6: CSC luminance (0~100, def 50)
														 * 7: CSC contrast value (0~100, def 50)
														 * 8: CSC hue value (0~100, def. 50)
														 * 9: CSC saturation (0~100, def. 50)
														 * 10: AG low
														 * 11: AG high
														 * 12: DG low
														 * 13: DG high
														 * 14: JPEG quality (1~99)
														 * 15: AE Exp Step (0~255) - note: set too high will cause brightness fructuation ���ֶ��ع�ʱ�����ﴢ�����AG��
														 * 16: AE exp tolerance (0~255) ���ֶ��ع�ʱ�����ﴢ�����DG��
														 * 17: Snap mode Analog Gain   ��3516A���ﴢ����� ISP Gain Max (min��1)��
														 * 18: Snap mode Digital Gain
														 * 19: Snap mode Exposure time (unit is 100 usec)
														 * 20: Snap trigger offset time in 100 usec. negative means time before CCD scan. 
														 * 21: Manual WB color temperature (x1000) is temoerature K)  (for 3516 only)
														 * 22: day time gamma value (for user defined) (3 for 0.3, 28 for 2.8 etc) for 3516A, this is u16HighColorTemp/50 member of ISP_AWB_ATTR_S
														 * 23: night time gamma value (for user define)(3 for 0.3, 28 for 2.8 etc) for 3516A, this is u16LowColorTemp/50 member of ISP_AWB_ATTR_S 
														 */

						
// parameter options (bit difinition for 'enabler' member in structure ParamConf)
#define PARM_EN_LOOPTRI						0x00000001		// enable loop trigger
#define PARM_EN_HEXLIVE						0x00000002		// enable send 1/16 image as live feed frame
#define PARM_EN_QUADLIVE					0x00000004		// enable send quad frame as live feed 
#define PARM_EN_LIGHTING					0x00000008		// enable lighting control
//#define PARM_EN_LOOPPULSE					0x00000010		// not used. loop signal is pulse mode.
#define PARM_EN_HEXIMG						0x00000010		// enable hex image output
#define PARM_EN_RETRYNEXT					0x00000020		// enable retry again by capture a frame do LPNR again
#define PARM_EN_DGDOWNONNIGHT			0x00000040		// 3516A reduce DG and ISP gain on night time when capture (available only when day-night detection & lighting enabled)
#define PARM_EN_DGDOWNONDAYTIME		0x00000040		// 3516 200A reduce AG, DG and ET on day time when capture (available only when day-night detection & lighting enabled)
#define PARM_EN_FLASHLIGHT				0x00000080		// yes for flash light, no for LED light
#define PARM_EN_SENDQTRIMG				0x00000100		// send quarter image (jpeg) as captured image (�㽭������Ҫ)
#define PARM_EN_DAYNIGHTSW				0x00000200		// enable day-night detection and setting for AG-DG-ET capture setting
#define PARM_EN_2NDLEDLIGHT				0x00000400		// �����2�����(�����ⲹ�⣬������LED���������ֻ��ҹ���⣬�Ǿ����ϲŵ���)
#define PARM_EN_VLOOP							0x00000800		// enable virtual loop
//#define PARM_EN_FREQLIGHT					0x00001000		// Ƶ���� (not used)
#define PARM_EN_IRLIGHT						0x00001000		// ץ�Ĳ���ʹ�ú���� - ���Ϲر�IR-cut
//#define PARM_DE_VDRPTSAMEPLATE		0x00002000		// ��ֹ��Ƶ���������ͬһ�������ظ��ϱ�
#define PARM_EN_VDRPTNOPLATE			0x00002000		// ������ƵҲ�ϱ��޳��ƣ����ǲ��ظ��ϱ���
#define PARM_EN_LIGHTING_ONNIGHT	0x00004000		// �����ֻ����������
#define PARM_EN_SAVERAW						0x00008000		// û��ʶ��Ļ�����ԭʼͼƬ����ҪSD����- NOT USED NOW
#define PARM_ENHANCEHZ_MASK				0x003f0000		// ǿ���� 0~63 ��Ŀǰֻ�õ�0~31��
#define PARM_ENHANCEHZ_SHIF				16
#define PARM_EN_DNLDYUV						0x00400000		// ��������ԭʼͼ�񵽿ͻ���
#define PARM_EN_YUV4ALL						0x00800000		// ÿ��ʶ����������YUVͼ��(û��ʹ������Ļ���ֻ��δʶ������أ�
// ��������
#define PARM_EN_OVLPLIMG					0x01000000		// ���ӳ���ͼ��1/4�����ͼ���½�
#define PARM_DE_CHARO							0x02000000		// ���ƽ�ֹ����O���������滻ΪD��
#define PARM_EN_HALFGAINONREDO		0x04000000		// ��ʶ������ʱ��������
#define PARM_DE_FULLIMG						0x08000000		// ��ֹ���ȫͼ
#define PARM_EN_HEADIMG						0x10000000		// ���������ͷͼ
#define PARM_EN_QUADIMG						0x20000000		// �������1/4�����ͼ
#define PARM_EN_SPEEDING					0x40000000		// ʹ�ܳ���ץ�ģ�ֻ�����ж���������Ļ��ͣ�������Ȧ2�����ڵڶ�����Ȧץ�ĳ���)
#define PARM_EN_T3RDIMG						0x80000000		// �������4/9�����ͼ

// bit-map of ParamConf.ex_enabler[1]
#define PARM_ENX_REDUCEGAINPERIOD			0x00000001		// ����ָ��ʱ�ν�������
#define PARM_ENX_JPGPLATECOLOR					0x00000002		// ʹ�ܳ��Ʋ�ɫͼ���ΪJPEG��Ĭ����BMP)
#define PARM_ENX_JPGPLATEBIN						0x00000004		// ʹ�ܳ��ƶ�ֵͼ���ΪJPEG��Ĭ����BMP)

#define IS_EN_LOOPTRIG(param)				((param).enabler & PARM_EN_LOOPTRI)
#define SET_EN_LOOPTRIG(param)			((param).enabler |= PARM_EN_LOOPTRI)
#define CLR_EN_LOOPTRIG(param)			((param).enabler &= ~PARM_EN_LOOPTRI)
//#define IS_EN_DBLPLATE(param)				((param).enabler & PARM_EN_DBLPLATE)
#define IS_EN_HEXLIVE(param)				((param).enabler & PARM_EN_HEXLIVE)
#define SET_EN_HEXLIVE(param)			((param).enabler |= PARM_EN_HEXLIVE)
#define CLR_EN_HEXLIVE(param)			((param).enabler &= ~PARM_EN_HEXLIVE)
#define IS_EN_QUADLIVE(param)				((param).enabler & PARM_EN_QUADLIVE)
#define SET_EN_QUADLIVE(param)			((param).enabler |= PARM_EN_QUADLIVE)
#define CLR_EN_QUADLIVE(param)			((param).enabler &= ~PARM_EN_QUADLIVE)
//#define IS_EN_MULTIPLATE(param) 		((param).enabler & PARM_EN_MULTIPLATE)
//#define SET_EN_MULTIPLATE(param) 		((param).enabler |= PARM_EN_MULTIPLATE)
//#define CLR_EN_MULTIPLATE(param) 		((param).enabler &= ~PARM_EN_MULTIPLATE)
#define IS_EN_LIGHTING(param)				((param).enabler & PARM_EN_LIGHTING)
#define SET_EN_LIGHTING(param)			((param).enabler |= PARM_EN_LIGHTING)
#define CLR_EN_LIGHTING(param)			((param).enabler &= ~PARM_EN_LIGHTING)
//#define IS_EN_LOOPSIGPULSE(param)		((param).enabler & PARM_EN_LOOPPULSE)  	obsoleted
//#define SET_EN_LOOPSIGPULSE(param)	((param).enabler |= PARM_EN_LOOPPULSE)
//#define CLR_EN_LOOPSIGPULSE(param)	((param).enabler &= ~PARM_EN_LOOPPULSE)
#define IS_EN_RETRYNEXT(param)			((param).enabler & PARM_EN_RETRYNEXT)
#define SET_EN_RETRYNEXT(param)			((param).enabler |= PARM_EN_RETRYNEXT)
#define CLR_EN_RETRYNEXT(param)			((param).enabler &= ~PARM_EN_RETRYNEXT)
#define IS_EN_SENDQTRIMG(param)			((param).enabler & PARM_EN_SENDQTRIMG)	// ������ץ��Сͼ (�������ȵ�JPEG����ѹ��1/4ͼ)
#define IS_EN_DAYNIGHTMODE(param)		((param).enabler & PARM_EN_DAYNIGHTSW)
#define SET_EN_DAYNIGHTMODE(param)	((param).enabler |= PARM_EN_DAYNIGHTSW)
#define CLR_EN_DAYNIGHTMODE(param)	((param).enabler &= ~PARM_EN_DAYNIGHTSW)
#define IS_EN_2NDLEDLIGHT(param)		((param).enabler & PARM_EN_2NDLEDLIGHT)
#define SET_EN_2NDLEDLIGHT(param)		((param).enabler |= PARM_EN_2NDLEDLIGHT)
#define CLR_EN_2NDLEDLIGHT(param)		((param).enabler &= ~PARM_EN_2NDLEDLIGHT)
#define IS_EN_VLOOP(param)					((param).enabler & PARM_EN_VLOOP)
#define SET_EN_VLOOP(param)					((param).enabler |= PARM_EN_VLOOP)
#define CLR_EN_VLOOP(param)					((param).enabler &= ~PARM_EN_VLOOP)
//#define IS_DE_VDRPTSAMEPLATE(param)	((param).enabler & PARM_DE_VDRPTSAMEPLATE)
//#define SET_DE_VDRPTSAMEPLATE(param)((param).enabler |= PARM_DE_VDRPTSAMEPLATE)
//#define CLR_DE_VDRPTSAMEPLATE(paarm)((param).enabler &= ~PARM_DE_VDRPTSAMEPLATE)
#define IS_EN_VDRPTNOPLATE(param)		((param).enabler & PARM_EN_VDRPTNOPLATE)
#define IS_EN_FLASHLIGHT(param)			((param).enabler & PARM_EN_FLASHLIGHT)
#define SET_EN_FLASHLIGHT(param)		((param).enabler |= PARM_EN_FLASHLIGHT)
#define CLR_EN_FLASHLIGHT(param)		((param).enabler &= ~PARM_EN_FLASHLIGHT)
#define IS_EN_IRLIGHT(param)			((param).enabler & PARM_EN_IRLIGHT)
#define SET_EN_IRLIGHT(param)			((param).enabler |= PARM_EN_IRLIGHT)
#define CLR_EN_IRLIGHT(param)			((param).enabler &= ~PARM_EN_IRLIGHT)
#define IS_EN_LIGHTING_ONNIGHT(param)  ((param).enabler & PARM_EN_LIGHTING_ONNIGHT)
#define SET_EN_LIGHTING_ONNIGHT(param) ((param).enabler |= PARM_EN_LIGHTING_ONNIGHT)
#define CLR_EN_LIGHTING_ONNIGHT(param) ((param).enabler &= ~PARM_EN_LIGHTING_ONNIGHT)
#define IS_EN_REDUCEGAINONNIGHT(param) ((param).enabler & PARM_EN_DGDOWNONNIGHT)
#define SET_EN_REDUCEGAINONNIGHT(param)((param).enabler |= PARM_EN_DGDOWNONNIGHT)
#define CLR_EN_REDUCEGAINONNIGHT(param)((param).enabler &= ~PARM_EN_DGDOWNONNIGHT)
#define IS_EN_REDUCEGAINONDAYTIME(param) ((param).enabler & PARM_EN_DGDOWNONDAYTIME)
#define IS_EN_SAVERAW(param)				((param).enabler & PARM_EN_SAVERAW)
#define SET_EN_SAVERAW(param)				((param).enabler |= PARM_EN_SAVERAW)
#define CLR_EN_SAVERAW(param)				((param).enabler &= ~PARM_EN_SAVERAW)
#define GET_LOOPTRIG_DELAY(param) 	((param).others[0] )			// loop triggle delay
#define SET_LOOPTRIG_DELAY(param,v) ((param).others[0]=v )		// loop triggle delay
#define GET_SAMEPLATE_MEMSEC(param)	((param).others[1] )			// ������Ȧͬһ���Ƽ���ʱ������λ�룩
#define SET_SAMEPLATE_MEMSEC(param,v)	((param).others[1]=v )			// ������Ȧͬһ���Ƽ���ʱ������λ�룩
#define GET_LOOPSIG_DEADBAND(param)	((param).others[2] )
//#define GET_SYNC_OFFSET(param)			((param).others[1] )			// not used. Ƶ����ͬ���ź�ƫ����(��λ106usec)
//#define SET_SYNC_OFFSET(param,v)		((param).others[1]=v )		// not used
//#define GET_LOOPPULSE_WIDTH(param)	((param).others[2] )		// not used
//#define SET_LOOPPULSE_WIDTH(param,v)((param).others[2]=v )	// not used
//#define GET_CREDIT_THRESHOLD(param)		((param).others[3] )		// not used
//#define SET_CREDIT_THRESHOLD(param,v) ((param).others[3]=v )	// not used
#define GET_REDUCEGAIN_HOUR_BEGIN(param) ((param).others[3] & 0xff)
#define GET_REDUCEGAIN_HOUR_END(param) 	 (((param).others[3]>>8) & 0xff)
//#define GET_PLATE_WIDTH(param)			((param).others[4])
//#define SET_PLATE_WIDTH(param,v)			((param).others[4]=v)
//#define GET_FREQLIGHT_DUTYRATIO(param) ((param).others[5] & 0xff )
//#define SET_FREQLIGHT_DUTYRATIO(param,v) ((param).others[5] |= ((v) & 0xff) )
#define GET_REDUCE_RATIO(param)			((param).others[5] >> 8)
#define SET_REDUCE_RATIO(param,v)		((param).others[5] |= (v) << 8)
#define GET_ENHANCEHZ(param)				(((param).enabler & PARM_ENHANCEHZ_MASK) >> PARM_ENHANCEHZ_SHIF )
#define SET_ENHANCEHZ(param,v)			(param).enabler = ((param).enabler & ~PARM_ENHANCEHZ_MASK) | ((v) << PARM_ENHANCEHZ_SHIF)
#define IS_DE_FULLIMG(param)				((param).enabler & PARM_DE_FULLIMG)
#define SET_DE_FULLIMG(param)			((param).enabler |= PARM_DE_FULLIMG)
#define CLR_DE_FULLIMG(param)			((param).enabler &= ~PARM_DE_FULLIMG)
#define IS_EN_HEADIMG(param)			((param).enabler & PARM_EN_HEADIMG)
#define SET_EN_HEADIMG(param)			((param).enabler |= PARM_EN_HEADIMG)
#define CLR_EN_HEADIMG(param)			((param).enabler &= ~PARM_EN_HEADIMG)
#define IS_EN_QUADIMG(param)			((param).enabler & PARM_EN_QUADIMG)
#define SET_EN_QUADIMG(param)			((param).enabler |= PARM_EN_QUADIMG)
#define CLR_EN_QUADIMG(param)			((param).enabler &= ~PARM_EN_QUADIMG)
#define IS_EN_HEXIMG(param)			((param).enabler & PARM_EN_HEXIMG)
#define SET_EN_HEXIMG(param)			((param).enabler |= PARM_EN_HEXIMG)
#define CLR_EN_HEXIMG(param)			((param).enabler &= ~PARM_EN_HEXIMG)
#define IS_EN_SPEEDING(param)			((param).enabler & PARM_EN_SPEEDING)
#define SET_EN_SPEEDING(param)		((param).enabler |= PARM_EN_SPEEDING)
#define CLR_EN_SPEEDING(param)		((param).enabler &= ~PARM_EN_SPEEDING)
#define IS_EN_T3RDIMG(param)			((param).enabler & PARM_EN_T3RDIMG)
#define SET_EN_T3RDIMG(param)			((param).enabler |= PARM_EN_T3RDIMG)
#define CLR_EN_T3RDIMG(param)			((param).enabler &= ~PARM_EN_T3RDIMG)
#define IS_EN_DNLDYUV(param)			((param).enabler & PARM_EN_DNLDYUV)
#define SET_EN_DNLDYUV(param)			((param).enabler |= PARM_EN_DNLDYUV)
#define CLR_EN_DNLDYUV(param)			((param).enabler &= ~PARM_EN_DNLDYUV)
#define IS_EN_YUV4ALL(param)			((param).enabler & PARM_EN_YUV4ALL)
#define SET_EN_YUV4ALL(param)			((param).enabler |= PARM_EN_YUV4ALL)
#define CLR_EN_YUV4ALL(param)			((param).enabler &= ~PARM_EN_YUV4ALL)
#define IS_EN_OVLPLIMG(param)			((param).enabler & PARM_EN_OVLPLIMG)
#define SET_EN_OVLPLIMG(param)		(param).enabler |= PARM_EN_OVLPLIMG)
#define CLR_EN_OVLPLIMG(param)		(param).enabler &= ~PARM_EN_OVLPLIMG)
#define IS_DE_CHARO(param)				((param).enabler & PARM_DE_CHARO)
#define IS_EN_HALFGAINONREDO(param) ((param).enabler & PARM_EN_HALFGAINONREDO)
// for paramConf.ex_enabler[1]
#define IS_EN_REDUCEGAIN_PERIOD(param)				((param).ex_enabler[1] & PARM_ENX_REDUCEGAINPERIOD)
#define IS_EN_OUTPUT_PLATECOLORJPG(param)  		((param).ex_enabler[1] & PARM_ENX_JPGPLATECOLOR)
#define IS_EN_OUTPUT_PLATEBINJPG(param)  			((param).ex_enabler[1] & PARM_ENX_JPGPLATEBIN)

// parameter options (bit difinition for 'enabler' member in structure ExtParamConf)
#define EXTPARM_EN_OVLPLATE		0x00000001
#define EXTPARM_EN_OVLTIME		0x00000002
#define EXTPARM_EN_OVLLABEL		0x00000004
#define EXTPARM_EN_OVLRCTIME	0x00000008		// ʹ�ܵ���ʶ��ʱ��
#define EXTPARM_EN_OVLMASK		0x0000000f
// other config enablers
#define EXTPARM_EN_TIMERTRIG	0x00000010		// ʹ�ܶ�ʱ���� ��ʱ��������CfgData.u16Val[8]��
#define EXTPARM_EN_FIXNUMLST	0x00000020
#define EXTPARM_EN_ETCLANE		0x00000040		// ʹ��ETC�����ã�ʹ�ܵĻ���ץ���ع��ٶ�������1/400���ϣ�������1/150����
#define EXTPARM_DE_HOSTSTIME 	0x00000080		// ��ֹ��λ����ʱ���ʹ�õ�����NTPD��ʱ

#define IS_EN_TIMERTRIG(exparm)	(((exparm).enabler & EXTPARM_EN_TIMERTRIG)!=0)
#define IS_EN_FIXNUMLST(exparm)	(((exparm).enabler & EXTPARM_EN_FIXNUMLST)!=0)
#define IS_EN_ETCLANE(exparm)		(((exparm).enabler & EXTPARM_EN_ETCLANE)!=0)
#define IS_DE_HOSTSTIME(exparm)	(((exparm).enabler & EXTPARM_DE_HOSTSTIME)!=0)

#define IS_EN_OVERLAY(exparm)		(((exparm).enabler & EXTPARM_EN_OVLMASK)!=0)
#define SET_EN_OVERLAY(exparm)	(((exparm).enabler |= EXTPARM_EN_OVLMASK))
#define CLR_EN_OVERLAY(exparm)	(((exparm).enabler &= ~EXTPARM_EN_OVLMASK))
#define IS_EN_OVLPLATE(exparm)	(((exparm).enabler & EXTPARM_EN_OVLPLATE) != 0)
#define SET_EN_OVLPLATE(exparm)	(((exparm).enabler |= EXTPARM_EN_OVLPLATE))
#define CLR_EN_OVLPLATE(exparm)	(((exparm).enabler &= ~EXTPARM_EN_OVLPLATE))
#define IS_EN_OVLTIME(exparm)		(((exparm).enabler & EXTPARM_EN_OVLTIME) != 0)
#define SET_EN_OVLTIME(exparm)	(((exparm).enabler |= EXTPARM_EN_OVLTIME))
#define CLR_EN_OVLTIME(exparm)	(((exparm).enabler &= ~EXTPARM_EN_OVLTIME))
#define IS_EN_OVLLABEL(exparm)	(((exparm).enabler & EXTPARM_EN_OVLLABEL) != 0)
#define SET_EN_OVLLABEL(exparm)	(((exparm).enabler |= EXTPARM_EN_OVLLABEL))
#define CLR_EN_OVLLABEL(exparm)	(((exparm).enabler &= ~EXTPARM_EN_OVLLABEL))
#define IS_EN_OVLRCTIME(exparm)	(((exparm).enabler & EXTPARM_EN_OVLRCTIME) != 0)
#define SET_EN_OVLRCTIME(exparm)	(((exparm).enabler |= EXTPARM_EN_OVLRCTIME))
#define CLR_EN_OVLRCTIME(exparm)	(((exparm).enabler &= ~EXTPARM_EN_OVLRCTIME))
#define EXTPARM_LOOPMODE_MASK		0x00000300
#define EXTPARM_LOOPMODE_SHIFT	8
#define EXTPARM_SAMEPLATE_MASK	0x00000C00		// ��Ƶ����ʱ����������ʶ����ͬ���ϱ� 0~3 
#define EXTPARM_SAMEPLATE_SHIFT 10
#define EXTPARM_FORMAT_MASK		0x0000F000		
#define EXTPARM_FORMAT_SHIFT	12
#define EXTPARM_FONT_IDMASK			0x000F0000		// 0: ���壬1:ϸ���壬2:���壬3:�����Ʋʣ��п���)
#define EXTPARM_FONT_IDSHIFT		16
#define EXTPARM_SIZE_IDMASK			0x00F00000		// 0:24, 1:32, 2:40, 3:48. 4:56. 5:64
#define EXTPARM_SIZE_IDSHIFT		20
#define EXTPARM_FGCOLOR_IDMASK	0x0F000000	// ��ɫ;��ɫ;���ɫ;��ɫ;���ɫ;��ɫ;��ɫ;��ɫ;����;����;����;ɭ����;��ɫ
#define EXTPARM_FGCOLOR_IDSHIFT	24
#define EXTPARM_BGCOLOR_IDMASK	0xF0000000	// ��ɫ;��ɫ;��ɫ;���ɫ;��ɫ;���ɫ;��ɫ;��ɫ;��ɫ;����;����;����;ɭ����;��ɫ
#define EXTPARM_BGCOLOR_IDSHIFT	28
#define EXTPARM_GET_LOOPMODE(p)		( (((p).enabler) & EXTPARM_LOOPMODE_MASK) >> EXTPARM_LOOPMODE_SHIFT)
#define EXTPARM_GET_SAMEPLATECNT(p)			( (((p).enabler) & EXTPARM_SAMEPLATE_MASK) >> EXTPARM_SAMEPLATE_SHIFT)
#define EXTPARM_GET_FORMAT(p)			( (((p).enabler) & EXTPARM_FORMAT_MASK) >> EXTPARM_FORMAT_SHIFT)
#define EXTPARM_GET_FONTID(p)			( (((p).enabler) & EXTPARM_FONT_IDMASK) >> EXTPARM_FONT_IDSHIFT)
#define EXTPARM_GET_SIZEID(p)			( (((p).enabler) & EXTPARM_SIZE_IDMASK) >> EXTPARM_SIZE_IDSHIFT)
#define EXTPARM_GET_FGCOLORID(p)	( (((p).enabler) & EXTPARM_FGCOLOR_IDMASK) >> EXTPARM_FGCOLOR_IDSHIFT)
#define EXTPARM_GET_BGCOLORID(p)	( (((p).enabler) & EXTPARM_BGCOLOR_IDMASK) >> EXTPARM_BGCOLOR_IDSHIFT)
	
#define EXTPARM_SETID(p,id,mask,shift) \
	do { \
		(p).enabler &= ~mask; \
		(p).enabler |= (id) << shift; \
	} while(0)
#define EXTPARM_SET_LOOPMODE(p,m) EXTPARM_SETID(p,m,EXTPARM_LOOPMODE_MASK,EXTPARM_LOOPMODE_SHIFT)
#define EXTPARM_SET_SAMEPLATECNT(p,cnt) EXTPARM_SETID(p,cnt,EXTPARM_SAMEPLATE_MASK,EXTPARM_SAMEPLATE_SHIFT)
#define EXTPARM_SET_FORMAT(p,fmt)  EXTPARM_SETID(p,fmt,EXTPARM_FORMAT_MASK,EXTPARM_FORMAT_SHIFT)
#define EXTPARM_SET_FONTID(p,id)  EXTPARM_SETID(p,id,EXTPARM_FONT_IDMASK,EXTPARM_FONT_IDSHIFT)
#define EXTPARM_SET_SIZEID(p,id)  EXTPARM_SETID(p,id,EXTPARM_SIZE_IDMASK,EXTPARM_SIZE_IDSHIFT)
#define EXTPARM_SET_FGCOLORID(p,id)  EXTPARM_SETID(p,id,EXTPARM_FGCOLOR_IDMASK,EXTPARM_FGCOLOR_IDSHIFT)
#define EXTPARM_SET_BGCOLORID(p,id)  EXTPARM_SETID(p,id,EXTPARM_BGCOLOR_IDMASK,EXTPARM_BGCOLOR_IDSHIFT)
#define EXTPARM_GET_LABEL(p)			(p.label)
#define EXTPARM_SET_LABEL(p,lb)	\
	do { \
		memcpy(p.label,lb,SZ_LABEL); \
		p.label[SZ_LABEL-1] = '\0'; \
	while (0) 
	
// extern DataAttr	g_CfgData;
/* g_CfgData members
	val[8]:
	 [0] - g_nDayNightThreshold
	 [3] - ʹ��ʶ��ĳ�������
	 
	s16Val[16]
	 [0~1] used by val[0]
	 [2] - redo delay (ms)
	 [3] - speed limmit (km/h)
	 [4] - loop space (cm)
	 [5] - cfg_enabler
	 [6,7] - used by val[3]
	 [8] - timer trigger (msec)
	 [9] - used by u8Val[18,19]
	 [10] - speed limit upper bond (km/h), over this limit consider as bad data
	u16Val[16]
	s8Val[32]
	u8Val[32]
	 [18][19] - ��ʶ������ָ��ʱ�� ��ʼ - ����
*/
#define CFG_EN_SPEEDCAPALL		0x0001		// ����ץ�Ļ����г�����ץ��
#define CFG_EN_REDOINTRNG			0x0002		// ��ʶ������ֻ��ָ��ʱ����
#define CFG_NOTICE_PLATEONLY	0x0004		// ֻ����ʶ���ƣ��������κ�������Ϣ
#define CFG_EN_SDSAVEPLATECLR	0x0008		// ʹ��SD����ʶ��ɹ��󱣴泵�Ʋ�ɫͼ (Ĭ��ֻ����ץ��ͼ)
#define CFG_EN_SDSAVEPLATEBIN	0x0010		// ʹ��SD����ʶ��ɹ��󱣴泵�ƶ�ֵͼ (Ĭ��ֻ����ץ��ͼ)
//-----------------------------------------------------------------	
typedef struct tagTimeAttr
{
	HI_S16	year;			// 2013
	HI_S16	month;		// 1~12
	HI_S16	day;			// 1~31
	HI_S16 	hour;			// 0 ~ 23
	HI_S16	minute;		// 0 ~ 59
	HI_S16	second;		// 0 ~ 59
	HI_S16	msec;			// 0 ~ 999
	HI_S16	resv[9];	// since 1.3.15.2, here we put video resolution for full, quad, hex (6*short)
} TimeAttr;

typedef struct tagImageAttr
{
	HI_S16	width;			// not required for JPG and BMP format. Only required for raw format (RGB, 422SP, 420SP)
	HI_S16	height;			// not required for JPG and BMP format. Only required for raw format (RGB, 422SP, 420SP)
	HI_S16	format;		
	HI_S16	index;			// 1 based image index (for IID_PLxx, this is plate index, for IID_PC, this plate * 10 + char Id)
											// for example, 11 is plate 1, char 1. 12 is plate 1 char 2 etc.)
	char		basename[24];		// image name, null terminated
} ImageAttr;

typedef struct tagCtrlAttr
{
	HI_S32	code;
	HI_S32	param;
	HI_U8		option[24];
} CtrlAttr;

typedef struct tagPlateInfo
{
	HI_S16	plateId;		// usually is one. Unless multiple plates detection is enabled.
	HI_S16	plateCode;	// low byte is color code PLATE_COLOR_E, high byte is type code PLATE_TYPE_E
	char  	chNum[12];	// plate string
	HI_U8		MatchRate[8];
	HI_SRECT plate_rect;	// plate rect relative to whole image
} PlateInfo;

#define PLATEINFO_INITIALIZER \
	{ 0, 0,{0,0,0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0} }
	
typedef struct tagTimeInfo
{
	HI_S32	totalElapsed;		// ��16λ��������������ʱ��
	HI_S32	totalProcess;		// B0 ��λ��ʽ, B1 Yroi, B2 Yavg, B3 MDʹ��ʱ��
	union {
		struct {
		HI_S32	timeDelay;		// ����(��Ȧ�ź�)����ʼִ��(�ɼ�ͼ��)����ʱ
		HI_S32	timeCap;			// �ɼ�ͼ�񻨵�ʱ��
		HI_S32	timeCap2;			// ����һ�εĲɼ�ʱ��
		HI_S32 	timeProc2;		// ����һ�εĴ���ʱ��
		};
		struct {
		HI_S32	timeAdpbin;			// ��16λ��adaptive binarization time����16λ�ǽ�����ά��ʱ��
		HI_S32	timeLocQrcode;	// ��16λ�Ƕ�λ��ά��λ��ʱ�䣬���8λ�Ƕ�λ��������, �θ�8λ�ǻҽ�����ʱ��
		HI_S32	timeDecode;			// ��16λ�ǽ���ʱ��, ��16λ��COG�������ʱ��
		HI_S32 	timeOverlay;		// ��16λ�ǵ����ַ�ʱ��
		};
	};
	HI_S32	timeFile;					// д�ļ�ʱ��
	HI_S32	timeJPGEn;				// JPEG����ʱ��
} TimeInfo;

typedef struct tagParamConf {
	HI_S16	ROI[4];				// top, right, bottom, left
	union {
		HI_S8	s8_param[8];		// 0: ���Ŷ�, 1..3 reserved
		HI_S16	s16_param[4];	// [0] used by s8_param[0..1], [1] reserved, [2..3] used by ex_enabler[1]
		HI_U32	ex_enabler[2];	// NOTE - ֻ��[1]�����ã�0��param[0..3]ռ��
	};
	HI_S32	enabler;			// bit-wise enabler
	HI_S16	others[6];		// 0 ��Ȧ������ʱ (msec)
												// 1 ������Ȧ���䳵��ʱ�����룩��Ƶ���������޳���ʱ�䳬�����ֵ�����ǣ�ͬ�����ƿ����ٶ��ϱ�
												// 2 ��ȦѶ��deadband
												// 3 ץ�Ľ����ع�ʱ��: begin hour + end hour<<8
												// 4 ���ƿ�� (for ShaiHe engine)
												// 5 ����ʶ�����
												//   ��8λ��Ƶ����duty ratio (50��ʾ50%)
												//   ��8λ��ץ�Ľ����ع����(��8λ��2��ʾ1/2, 4��ʾ1/4, 0 ΪĬ��)
											  // 5 ��ά��ɨ�����
											  //	 ��8λ: ɨ���ִ������ʽ
} ParamConf;

typedef struct tagExtParamConf {
	HI_S16	ROI[4];					// secondary ROI for soft-trigger
	HI_S32	enabler;				// bit-wise (0: overlay plate, 1: overlay time, 2: overlay label
	char		label[SZ_LABEL];
} ExtParamConf;

// [H264 Config]
typedef enum  {
	BASELINE_PROFILE =0,
	MAIN_PROFILE,
	HIGH_PROFILE
} Enum_H264Profile;

typedef enum {
	MODE_CBR = 1,
	MODE_VBR,
	MODE_ABR,
	MODE_FIXQP
} Enum_H264RcMode;

typedef struct tagH264Conf {
	Enum_H264Profile  enProfile;
	Enum_H264RcMode enRcMode;
	HI_U32	u32BitRate;			//  max. bitrate for VBR,  or target bitrate for CBR. not used for FixQP Rc Mode
	HI_S16  s16QP[2];					// [0] is min, [1] is max for VBR, [0] is I-Qp, [1] is P-Qp for FixQP mode
	union {
		HI_U8	 	u8Param[16];		// [0]: GOP, 1: [1] enable mini stream (1 minor, 2 major, 3 both), 2: encoder (0:H264,1:H265)
														// [3]: fps divider(1,2,3,5,6,10) (major divider<<4|minor divider)
														// [4]..[7] reserved
		HI_U16	u16Param[8];		// [4]/[5]: major stream resolution
														// [6]/[7]: minor stream resolution
		HI_U32	u32Param[4];
	};
} H264Conf;
#define IS_EN_MINIMINOR(h264conf)	(((h264conf).u8Param[1] & 0x01)!=0)
#define IS_EN_MINIMAJOR(h264conf)	(((h264conf).u8Param[1] & 0x02)!=0)

// [OSD Config]
#define OSD_EN_TIMESTAMP	0x00000001
#define OSD_EN_LOGO				0x00000002
#define OSD_EN_LABEL			0x00000004
#define OSD_EN_PLATE			0x00000008
#define OSD_EN_STEXT1 		0x00000010	// ����Ƶ�ϵ��Ӿ�̬�ִ�1�����ݣ�λ����OSDCONF��payload���棩
#define OSD_EN_STEXT2 		0x00000020	// ����Ƶ�ϵ��Ӿ�̬�ִ�2�����ݣ�λ����OSDCONF��payload���棩
#define OSD_EN_STEXT3 		0x00000040	// ����Ƶ�ϵ��Ӿ�̬�ִ�3�����ݣ�λ����OSDCONF��payload���棩��Ƶ�������������ƣ�ֻ��1��2��������
#define OSD_EN_STEXT4 		0x00000080	// ����Ƶ�ϵ��Ӿ�̬�ִ�4�����ݣ�λ����OSDCONF��payload���棩��Ƶ�������������ƣ�ֻ��1��2��������
#define OSD_EN_STEXTMASK	0x000000f0
#define OSD_EN_ROI				0x00001000	
#define OSD_EN_TFMT_SL2HY	0x00002000		// ��ʱ����ӵĸ�ʽ����'/'��Ϊ'-'
#define OSD_TSPOS_MASK		0x000f0000	// time stamp position mask (0~5���ֱ��������-����-����-����-����-����
#define OSD_TSPOS_SHIFT   16
#define OSD_TSFMT_MASK		0x00f00000	// time stamp format mask (0~7)
#define OSD_TSFMT_SHIFT		20
#define OSD_NPFMT_MASK		0x0f000000	// no plate overlay style (���ƾ�ʶʱ��������)
#define OSD_NPFMT_SHIFT		24


#define OSDCONF_SETVAL(oc,val,mask,shift) \
	do { \
		(oc)->enabler &= ~mask; \
		(oc)->enabler |= (val) << shift; \
	} while(0)

#define OSD_IS_EN_LOGO(oc)				( ((oc)->enabler & OSD_EN_LOGO)==OSD_EN_LOGO)
#define OSD_IS_EN_LABEL(oc)				( ((oc)->enabler & OSD_EN_LABEL)==OSD_EN_LABEL )
#define OSD_IS_EN_TIMESTAMP(oc)		( ((oc)->enabler & OSD_EN_TIMESTAMP)==OSD_EN_TIMESTAMP )
#define OSD_IS_EN_TSFMT_SL2HY(oc)	( ((oc)->enabler & OSD_EN_TFMT_SL2HY)==OSD_EN_TFMT_SL2HY)
#define OSD_IS_EN_PLATE(oc)				( ((oc)->enabler & OSD_EN_PLATE)==OSD_EN_PLATE )
#define OSD_IS_EN_STEXT(oc,n)			( ((oc)->enabler & (OSD_EN_STEXT1 << (n)) ) != 0)			// n is 0..3
#define OSD_IS_EN_ANYSTEXT(oc)		( ((oc)->enabler & OSD_EN_STEXTMASK) != 0)			// n is 0..3
#define OSD_IS_EN_ROI(oc)					( ((oc)->enabler & OSD_EN_ROI) == OSD_EN_ROI )
#define OSD_SET_EN_LOGO(oc)				((oc)->enabler |= OSD_EN_LOGO)
#define OSD_SET_EN_LABEL(oc)			((oc)->enabler |= OSD_EN_LABEL)
#define OSD_SET_EN_TIMESTAMP(oc)	((oc)->enabler |= OSD_EN_TIMESTAMP)
#define OSD_SET_EN_PLATE(oc)			((oc)->enabler |= OSD_EN_PLATE)
#define OSD_SET_EN_STEXT(oc,n)		((oc)->enabler |= (OSD_EN_STEXT1 << (n)) )		// n is 0..3
#define OSD_SET_EN_ROI(oc)				((oc)->enabler |= OSD_EN_ROI)
#define OSD_CLR_EN_LOGO(oc)				((oc)->enabler &= ~OSD_EN_LOGO)
#define OSD_CLR_EN_LABEL(oc)			((oc)->enabler &= ~OSD_EN_LABEL)
#define OSD_CLR_EN_TIMESTAMP(oc)	((oc)->enabler &= ~OSD_EN_TIMESTAMP)
#define OSD_CLR_EN_PLATE(oc)			((oc)->enabler &= ~OSD_EN_PLATE)
#define OSD_CLR_EN_STEXT(oc,n)		((oc)->enabler &= ~(OSD_EN_STEXT1 << (n)) )			// n is 0..3
#define OSD_CLR_EN_ROI(oc)				((oc)->enabler &= ~OSD_EN_ROI)
#define OSD_GET_TSPOS(oc)					( ((oc)->enabler & OSD_TSPOS_MASK) >> OSD_TSPOS_SHIFT)
#define OSD_SET_TSPOS(oc,pos)			OSDCONF_SETVAL(oc,pos,OSD_TSPOS_MASK,OSD_TSPOS_SHIFT)
#define OSD_GET_TSFMT(oc)					( ((oc)->enabler & OSD_TSFMT_MASK) >> OSD_TSFMT_SHIFT)
#define OSD_SET_TSFMT(oc,fmt)			OSDCONF_SETVAL(oc,fmt,OSD_TSFMT_MASK,OSD_TSFMT_SHIFT)
#define OSD_GET_NPFMT(oc)					( ((oc)->enabler & OSD_NPFMT_MASK) >> OSD_NPFMT_SHIFT)
#define OSD_SET_NPFMT(oc,fmt)			OSDCONF_SETVAL(oc,fmt,OSD_NPFMT_MASK,OSD_NPFMT_SHIFT)
// OSDConf.param[] array index
#define OSD_PARM_DWELL			0
#define OSD_PARM_FADEOUT		1
#define OSD_PARM_FLASH			2
#define OSD_PARM_ONTIME			3
#define OSD_PARM_OFFTIME		4
#define OSD_PARM_ALIGN			5		// user text alignment
#define OSD_PARM_SCALE			6		// font scale x2, x3, x4 - for user text
#define OSD_PARM_TARGET			7		// 0 overlay on video, 1 overlay on captured image

#pragma pack (push,2)
// this structure is also used as OSDText
typedef struct tagOSDConf {
	HI_U32	enabler;					// for OSDText, this is addr of payload (text body), for OSDConf, this is addr of plate number
	short		x;								// this is X-coord of upper-left corner of text
														// if < 0; -1 ~ -9 means ul, uc, ur, ml, mc, mr, ll, lc, lr respectively
	short 	y;								// this is Y-coord of upper-left corner of text (if x<0, y is not used)
	short		nFontId;					// 0 ����, 1 ����
	short	  nFontSize;				// 32, 40, 48, 56, 64
	HI_U32	RGBForgrund;			// for text type, this is text color
	HI_U32  RGBBkground;			// for BMP overlay, this is color which to apply background alpha
	HI_U16  alpha[2];					// forground/nackground alpha			
	HI_U8		param[8];					// 0: dwell time, 1: fade-out, 2: flashing, 3: on_time (100s of msec), 4: off_time (100s of msec)�� 
														// 5: align (user text), 6: scale (user text) 7: overlay target (0 for video, 1 for capture image)
} OSDConf, *POSDConf;


typedef OSDConf OSDText;		// OSDText�������Ӷ�̬�ַ����������������ļ�����

// payload of OSD config.
// ���½ṹ��OSDConfһ���ͣ�����֡��payload�����Էֱ���4���������������
// ���û�з��ͣ�����Ĭ��ֵ��ȫ����0
// ����ṹ�Ƕ��������Ŀ���������Ŀ����ǰֻʵ��ʱ���ǩ��λ�ã�x,y���ɶ��塣�������Ч
#define OSDID_TIMESTAMP	0		// ʱ���ǩ
#define OSDID_LOGO			1		// runwell logo
#define OSDID_LABEL			2		// ���������
#define OSDID_PLATE			3		// ���ƺ���
#define NUM_OSDID				4
typedef struct tagOSDPayload {
	HI_U32	enabler;					// ��enable��bit����ʹ����������ã�������Ĭ������
	short		x[NUM_OSDID];							// this is X-coord of upper-left corner of text
	short 	y[NUM_OSDID];							// this is Y-coord of upper-left corner of text
	short		nFontId[NUM_OSDID];				// 0 ����, 1 ����
	short	  nFontSize[NUM_OSDID];			// 32, 40, 48, 56, 64
	HI_U32	RGBForgrund[NUM_OSDID];		// for text type, this is text color
	HI_U32  RGBBkground[NUM_OSDID];			// for BMP overlay, this is color which to apply background alpha
	HI_U16  alpha[NUM_OSDID][2];						// forground/background alpha			
} OSDPayload, *POSDPayload;

// 2018-12-04 ��������ṹ��������OSDConf���� header.osdConf + OSDPayload + OSDFixText[4]
// OSDFixText���ӳ��ľ�̬���֣��Ĵ���ĳЩ�ط���Ҫʹ�ã�������Ҫ�������á���Щ������OSD�����ļ�����
// OSDConf.enabler�����OSD_EN_STEXT1~OSD_EN_STEXT4�������Ƿ�ʹ��OSDFixText[0]~OSDFixText[3]������
// Ŀǰ��Ϊ�����������������ƣ�����ֻ֧��OSD_EN_STEXT1,OSD_EN_STEXT2. 3��4�����������Ҫ�Ļ�������
// ��ROI��Label��������
typedef struct tagOSDFixText		
{
	short		x;							// this is X-coord of upper-left corner of text
	short 	y;							// this is Y-coord of upper-left corner of text
	short		nFontId;				// 0 ����, 1 ����
	short	  nFontSize;			// 32, 40, 48, 56, 64
	HI_U32	RGBForgrund;		// for text type, this is text color
	HI_U32  RGBBkground;		// for BMP overlay, this is color which to apply background alpha
	HI_U16  alpha[2];				// forground/background alpha			
	char text[80];					// fixed long text to overlay on video
} OSDFixText, *POSDFixText;

#pragma pack (pop)


typedef union tagDataAttr {
	HI_S32	val[8];
	HI_S16	s16Val[16];
	HI_U16	u16Val[16];
	HI_S8		s8Val[32];
	HI_U8		u8Val[32];
} DataAttr;

typedef struct tagTextAttr {
	char		text[32];
} TextAttr;

typedef struct tagVerAttr {
	char 	strVersion[16];	// ������汾��
	int		algver;					// �㷨�汾��
	int 	param[3];				// 0: ������汾�Ŷ�����ֵ��1:������ͺ�+Sensor�ͺţ�2:��������ʹ�ܵ�capability
} VerAttr;

typedef struct tagEvtAttr {
	int 	param;
	char  text[28];
} EvtAttr;

typedef struct tagDataHeader
{
	HI_S32	DataType;		// data type (what kind of data)
	HI_S32	DataId;			// which data of this kind. So far, only DataType==DTYP_DATA may use this Id
	union {
	TimeAttr	timeAttr;					// DataType is DTYP_TIME
	ImageAttr	imgAttr;					// DataType is DTYP_IMAGE
	CtrlAttr	ctrlAttr;					// DataType is DTYP_CTRL
	PlateInfo	plateInfo;				// DataType is DTYP_DATA, DataId is DID_PLATE
	TimeInfo	timeInfo;					// DataType is DTYP_DATA, DataId is DID_TIMING
	ParamConf paramConf;				// DataType is DTYP_CONF
	ExtParamConf extParamConf;	// DaraType is DTYP_EXCONF
	DataAttr	dataAttr;					// DataType is DTYP_DATA, DataId is one of DID_END, DID_ISP, DID_SNKEY 
	TextAttr	textAttr;					// DataType is DTYP_TEXT
	VerAttr		verAttr;					// DataType is DTYP_DATA, DataId is DID_VERSION
	H264Conf	h264Conf;					// DataType is DTYP_H264CONF
	OSDConf 	osdConf;					// DataType is DTYP_OSDCONF
	OSDText		osdText;					// DataType is DTYP_OSDTEXT
	EvtAttr		evtAttr;					// DataType is DTYP_EVENT, DataId is event Id
	};
	HI_S32	size;				// data size followed (payload of header)
} DataHeader;

#define CTRL_HEADER_INIT( hdr, c, p )	\
	do { \
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_CTRL; \
		hdr.ctrlAttr.code = c; \
		hdr.ctrlAttr.param = p; \
	} while (0)


#define CTRL_HEADER_INITEX( hdr, c, p, opt, sz )	\
	do { \
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_CTRL; \
		hdr.ctrlAttr.code = c; \
		hdr.ctrlAttr.param = p; \
		memcpy( hdr.ctrlAttr.option, opt, 24 ); \
		hdr.size = sz; \
	} while (0)

#define IMAGE_HEADER_INIT( hdr, id, w, h, fmt, s )	\
	do { \
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_IMAGE; \
		hdr.DataId = id; \
		hdr.imgAttr.width = w; \
		hdr.imgAttr.height = h; \
		hdr.imgAttr.format = fmt; \
		hdr.size = s; \
	} while(0)

#define IMAGE_HEADER_INITEX( hdr, id, w, h, fmt, idx, nm, s )	\
	do { \
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_IMAGE; \
		hdr.DataId = id; \
		hdr.imgAttr.width = w; \
		hdr.imgAttr.height = h; \
		hdr.imgAttr.format = fmt; \
		hdr.imgAttr.index = idx; \
		strncpy(hdr.imgAttr.basename, nm, 24); \
		hdr.imgAttr.basename[23] = '\0'; \
		hdr.size = s; \
	} while(0)
		
#define DATA_HEADER_INIT(hdr, id, pattr) \
	do { \
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_DATA; \
		hdr.DataId = id; \
		if ( id == DID_PLATE ) \
			memcpy( &hdr.plateInfo, pattr, sizeof(PlateInfo) ); \
		else if ( id == DID_TIMING ) \
			memcpy( &hdr.timeInfo, pattr, sizeof(TimeInfo) ); \
		else if ( id == DID_END ) \
			memcpy( hdr.dataAttr.val, pattr, sizeof(DataAttr) ); \
		else if (id==DID_ISP || id==DID_SNKEY || id==DID_NETCONF) \
			memcpy( hdr.dataAttr.u8Val, pattr, sizeof(DataAttr) ); \
	} while(0)
			
#define DATA_HEADER_INITEX(hdr, id, sz) \
	do { \
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_DATA; \
		hdr.DataId = id; \
		hdr.size = sz; \
	} while(0)
			
#define VERSION_HEADER_INIT( hdr, algv, s, p1, p2, p3 ) \
	do { \
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_DATA; \
		hdr.DataId = DID_VERSION; \
		strncpy( hdr.verAttr.strVersion, s, 16 ); \
		hdr.verAttr.strVersion[15] = '\0'; \
		hdr.verAttr.algver = algv; \
		hdr.verAttr.param[0] = p1; \
		hdr.verAttr.param[1] = p2; \
		hdr.verAttr.param[2] = p3; \
	} while (0)
			
#define TEXT_HEADER_INIT( hdr, id, s ) \
	do { \
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_TEXT; \
		hdr.DataId = id; \
		strncpy( hdr.textAttr.text, s, 32 ); \
		hdr.textAttr.text[31] = '\0'; \
	} while (0)

#define TEXT_HEADER_INITEX( hdr, cap, sz ) \
	do { \
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_TEXT; \
		hdr.DataId = TID_LONGTEXT; \
		if ( cap != NULL ) \
		{ \
			strncpy( hdr.textAttr.text, cap, 32 ); \
			hdr.textAttr.text[31] = '\0'; \
		} \
		hdr.size = sz; \
	} while (0)

#define UPDATE_HEADER_INIT( hdr, s, n ) \
	do { \
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_UPDSW; \
		strncpy( hdr.textAttr.text, s, 31 ); \
		hdr.textAttr.text[31] = '\0'; \
		hdr.size = n; \
	} while (0)

#define TIME_HEADER_INIT( hdr, y, m, d, hh, mm, ss, ms ) \
	do { \
		memset( &hdr, 0, sizeof(hdr) ); \
		hdr.DataType = DTYP_TIME; \
		hdr.timeAttr.year  = y; \
		hdr.timeAttr.month = m; \
		hdr.timeAttr.day = d; \
		hdr.timeAttr.hour = hh; \
		hdr.timeAttr.minute = mm; \
		hdr.timeAttr.second = ss; \
		hdr.timeAttr.msec = ms; \
	} while (0)
			
#define ACK_HEADER_INIT( hdr ) \
do { \
	memset( &hdr, 0, sizeof(hdr) ); \
	hdr.DataType = DTYP_ACK; \
} while (0)

#define ACK_HEADER_INITEX( hdr, id ) \
do { \
	memset( &hdr, 0, sizeof(hdr) ); \
	hdr.DataType = DTYP_ACK; \
	hdr.DataId = id; \
} while (0)

#define HBEAT_HEADER_INIT( hdr ) \
do { \
	memset( &hdr, 0, sizeof(hdr) ); \
	hdr.DataType = DTYP_HBEAT; \
} while (0)


#define SEARCH_HEADER_INIT(hdr) \
do { \
	memset( &hdr, 0, sizeof(hdr) ); \
	hdr.DataType = DTYP_UDP; \
	hdr.DataId = DID_SEARCH; \
} while (0)

#define EVENT_HEADER_INIT(hdr, id, p, t) \
do { \
	memset( &hdr, 0, sizeof(hdr) ); \
	hdr.DataType = DTYP_EVENT; \
	hdr.DataId = id; \
	hdr.evtAttr.param = p; \
	strncpy(hdr.evtAttr.text,t,24); \
	hdr.evtAttr.text[23] = '\0'; \
} while (0)

#define H264_HEADER_INIT(hdr, conf) \
do { \
	memset( &hdr, 0, sizeof(hdr) ); \
	hdr.DataType = DTYP_H264CONF; \
	memcpy(&hdr.h264Conf, &conf, sizeof(H264Conf)); \
} while(0)

#define OSD_HEADER_INIT( hdr, conf ) \
do { \
	memset( &hdr, 0, sizeof(hdr) ); \
	hdr.DataType = DTYP_OSDCONF; \
	memcpy( &hdr.osdConf, conf, sizeof(OSDConf)); \
} while (0)

#define OSDTEXT_HEADER_INIT( hdr, conf ) \
do { \
	memset( &hdr, 0, sizeof(hdr) ); \
	hdr.DataType = DTYP_OSDTEXT; \
	if ( conf != NULL ) \
		memcpy( &hdr.osdText, conf, sizeof(OSDText)); \
} while (0)

#define PLATENUM_HEADER_INIT(hdr, plnum ) \
do { \
	memset( &hdr, 0, sizeof(hdr) ); \
	hdr.DataType = DTYP_TEXT; \
	hdr.DataId = TID_PLATENUM; \
	strcpy(hdr.textAttr.text, plnum); \
} while (0)

#define QUERY_HEADER_INIT(hdr,qid) \
do { \
	memset( &hdr, 0, sizeof(hdr) ); \
	hdr.DataType = DTYP_QUERY; \
	hdr.DataId = qid; \
} while (0)

#define QUERY_REPLAY_HREADER_INIT(hdr, qid,qer,pln) \
do { \
	memset( &hdr, 0, sizeof(hdr) ); \
	hdr.DataType = DTYP_REPLY; \
	hdr.DataId = qid | (qer<<16); \
	if (pln) \
	   strcpy(hdr.plateInfo.chNum, pln); \
} while (0)

//#define sendHostHeaderText(cl,fmt,...)	sendHostText(cl,TID_CAPTION,fmt)
//#define sendHostPromptText(cl,fmt,...)	sendHostText(cl,TID_PROMPT,fmt)

#define IsValidHeader( hdr)  ( ((hdr).DataType & DTYP_MASK) == DTYP_MAGIC )

#endif
