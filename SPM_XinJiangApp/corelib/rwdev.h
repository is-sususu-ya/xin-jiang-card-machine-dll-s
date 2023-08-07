#ifndef RW_DEVICE_HEADER
#define RW_DEVICE_HEADER

// ������ʾ
#define IOC_LED_DISPLAY 1
//����������
#define IOC_VOICE_PLAY  2

// �¼���-
// ��ά��׼����
#define EVT_QR_READY    1
// ҳ���л�ָ�param ��ʾ�л����ڼ�ҳ��arg������л�����ʾ��ά����棬��arg��ʾ��ά������
// ����param��
// 0: ��ҳ��Ĭ�Ͻ���
// 1: ������������
// 2: ��ʾ�ȴ�֧������,��ʾ֧����ά��
// 3: ��ʾɨ��ɹ����ȴ�֧����ɽ���
// 4����ʾ֧���ɹ�����
// 5��֧��ʧ�ܽ���
#define EVT_CHANGE_PAGE	        2 

typedef void (*RWDevCallBack)(int code, int param, void *arg);
extern RWDevCallBack DevCallBack;

int RWDevStart();
int RWDevRegisterCallBack( RWDevCallBack cb );
int RWDevStop();
int RWDevIoctl( int code, int param, void *arg );


#endif
