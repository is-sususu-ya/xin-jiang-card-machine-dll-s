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
 *  �������ʹ�ô˺�����˵����
 *
 *  1. ��ʼ����
 *	   	LPNR_Init ����ΪLPNR�豸��IP��ַ�ִ�������0��ʾ�ɹ���-1 ��ʾʧ�� (IP��ַ���󣬻��Ǹ�IPû����ӦUDPѯ��)
 *		  
 *		 
 *	2. ���ûص�����(linux)��������Ϣ���մ���(Windows)
 *		 LPNR_SetCallBack -- for linux and/or Windows
 *		 LPNR_SetWinMsg   -- for windows
 *			��ʼ���ɹ����������á�
 *
 *	3. ��ȡ���ƺ��룺
 *		 LPNR_GetPlateNumber - ����Ϊ���ú������泵�ƺ����ִ���ָ�룬���ȱ����㹻�����ƺ�����Ϊ
 *       <��ɫ><����><����>�����磺����A123456. ���û�б�ʶ�����ƣ������ִ�"���ƾ�ʶ"��
 *
 *	4. ��ȡ���Ʋ�ɫСͼ
 *		 LPNR_GetPlateColorImage - ����Ϊ����ͼƬ��bufferָ�룬ͼƬ����Ϊbmp��ʽ��ֱ�Ӵ�������buffer��һ��
 *       �ļ�����һ��bmpͼ������������ֵ������buffer�ĳ��ȡ�Ӧ�ó����豣֤buffer�����㹻����Ҫ�ĳ���Ϊ
 *			 ����Сͼ����x���+54����Ϊ����ͼƬ�ߴ�ÿ�δ�������ͬ�������ṩһ�������ܵĳߴ硣
 * 
 *	4. ��ȡ���ƶ�ֵ��Сͼ
 *		 LPNR_GetPlateBinaryImage - ����Ϊ����ͼƬ��bufferָ�룬ͼƬ����Ϊbmp��ʽ��ֱ�Ӵ�������buffer��һ��
 *       �ļ�����һ��bmpͼ������������ֵ������buffer�ĳ��ȡ�Ӧ�ó����豣֤buffer�����㹻����Ҫ�ĳ���Ϊ
 *			 ����Сͼ����x���+54����Ϊ����ͼƬ�ߴ�ÿ�δ�������ͬ�������ṩһ�������ܵĳߴ硣
 *
 *	5. ��ȡץ����ͼ �����г���ʶ������������ͼƬ��
 *		 LPNR_GetCapturedImage - ����Ϊ����ͼƬ��bufferָ�룬ͼƬ����Ϊjpg��ʽ��ֱ�Ӵ�������buffer��һ��
 *       �ļ�����һ��jpegͼ������������ֵ������buffer�ĳ��ȡ�Ӧ�ó����豣֤buffer�����㹻����Ҫ�ĳ��ȸ���Ϊ
 *       ���������� x factor. factor�������õ�JPEGѹ��������ԼΪ0.1~0.5��
 *
 *  6. ��ȡ��ǰʵʱͼ��֡ ����ʼ������ʵʱͼ�����ͣ��ֱ����ǰͼ�������ɲż������£�
 *		 LPNR_GetLiveFrame - ������LPNR_GetCapturedImage��ͬ��Ҳ��һ��jpeg֡�����ݡ�Ĭ����ʹ�ܷ���ʵʱͼ��
 *		 ���Ե���LPNR_EnableLiveFrame����ʵʱͼ���ͣ���С���縺�ɡ�
 *
 *  7. ѯ��״̬�ĺ���
 *		LPNR_IsOnline - �Ƿ����ߣ����ز���ֵ
 *		LPNR_IsIdle - ʶ����Ƿ����ʶ�����У����ز���ֵ
 *     LPNR_GetPlateColorImageSize - ���س��Ʋ�ɫСͼ��С
 *     LPNR_GetPlateBinaryImageSize - ���س��ƶ�ֵ��Сͼ��С
 *     LPNR_GetCapturedImageSize - ����ץ�Ĵ�ͼ��С��# of bytes)
 *     LPNR_GetHeadImageSize - ��ȡ��ͷͼ��С������ʹ�ܷ��ͳ�ͷͼ��
 *     LPNR_GetQuadImageSize - ��ȡ1/4ͼͼƬ��С������ʹ�ܷ���ץ��Сͼ��
 *	   LPNR_GetTiming - ��ȡʶ�����ɵ�ǰʶ�������ĵ�ʱ�� ���ܹ���ʱ����ܴ���ʱ�䣩
 *
 *  8. �ͷŴ��������ݺ�ͼƬ(�������)
 *		 LPNR_ReleaseData
 *
 *  9. ����ץ��ʶ��
 *		 LPNR_SoftTrigger - ץ�� + ʶ��
 *	     LPNR_TakeSnapFrame - ץ��һ��ͼ����������ʶ��
 *
 *	10. ����
 *		 LPNR_Terminate - ���ô˺�������ʹ��LPNR����̬���ڲ������߳̽������ر��������ӡ�
 *		
 *  11. ������������
 *		LPNR_SyncTime - ͬ��ʶ���ʱ��ͼ����ʱ�� ���·���ʱ���
 *		LPNR_EnableLiveFrame - ʹ�ܻ��ǽ���ʶ�������ʵʱͼ��֡
 *      LPNR_ReleaseData - �ͷŵ�ǰʶ������̬���������
 *		LPNR_Lock/LPNR_Unlock - �������������ݣ���ȡʵʱ֡�����κ�ʶ��������֮ǰ�ȼ�������ȡ�������������ȡ�����б������̸߳��ġ�
 *		LPNR_GetPlateAttribute - ��ȡ����/����������Ϣ
 *		LPNR_GetExtDI - ��ȡһ�������չDI״̬
 *		LPNR_GetMachineIP - ��ȡʶ���IP�ִ�
 *		LPNR_GetCameraLabel - ��ȡʶ�����ǩ�����֣�
 *
 *	�¼���ţ�
 *	�ص�����ֻ��һ�����������¼���š�����windows������������Ϣ֪ͨ�� LPARAM���棬WPARAMΪ�����¼��������
 *  ��� (��LPNR_Init����)��
 *    �¼����1��LPNR���ߡ�
 *    �¼����2��LPNR����
 *    �¼����3����ʼ����ʶ����
 *    �¼����4��ʶ����������Ի�ȡ�����
 *	  �¼����5��ʵʱͼ��֡���£����Ի�ȡ�����»��档
 *    �¼����6����������������Ȧ���������λ��Ҫ���յ��¼����4��ſ���ȥ��ȡ������Ϣ��ͼƬ��
 *    �¼����7�������뿪������Ȧ�����
 *    �¼����8����չDI��״̬�б仯��200D, 200P, 500������չIO��
 *
 *  NOTE - ENABLERS
 *  1. ENABLE_LOG	- define �󣬳�����¼��־�ڹ̶����ļ���
 *  2. ENABLE_TESTCODE - define �󣬲��Գ���ʹ�ܣ����Ա������ִ�г�����ԣ�linux�汾���У�
 *
 *  �޸ģ�
 *  A. 2015-10-01��
 *     1. �������� LPNR_GetHeadImageSize, LPNR_GetHeadImage
 *         ���ܣ���ȡ��ͷͼ��ͼ���СΪCIF ��PALΪ720x576�� �Գ���λ��Ϊ���ĵ㣬 û�г�������ʶ����Ϊ���ĵġ�
 *     2. �������� LPNR_GetQuadImageSize, LPNR_GetQuadImage
 *         ���ܣ���ȡ1/4�����ͼ��
 *     3. �������� LPNR_GetHeadImageSize, LPNR_GetHeadImage��ȡ��ͷͼ��ͼƬ��СΪCIF
 *	   ��������ͼ��û���ַ�������Ϣ��Ҫ���ʶ����汾 1.3.1.21 ������֮����С�
 *
 * B 2016-01-15
 *		1. �����¼����6��7��8  �����ʶ�������汾1.3.2.3֮�󣨺����汾��
 *      2. �������� LPNR_GetExtDI�� LPNR_GetMachineIP��LPNR_GetPlateAttribute
 *	    3. ��ʼ��ʶ���ʱ���Ȳ���ָ��IP��ʶ�����û����ӦUDP��ѯ����û�еĻ���ʾ��IPû��ʶ�����û�����߻��ǳ���û�����С�
 *          һ�����ش������������ڳ�ʼ��ʱ������֪��ʶ����ǲ��Ǳ��ס����õȳ�ʱû�����ߡ�
 *		4. ��־����λ���޸� 
 *			- linux: /var/log/lpnr_<ip>.log ��/var/log������ڣ�
 *          - Windows: ./LOG/LPNR-<IP>.log (ע�⣬���򲻻ᴴ��LOG�ļ��У������ֹ�������
 *
 * C 2016-04-13
 *		1. �����ӿ� LPNR_GetCameraLabel
 *
 * D 2016-12-16
 *     1. �����ӿ� - LPNR_QueryPlate: �ڲ��Ƚ���TCP���ӵ�����£��򵥵�һ������ʵ��UDP��������ѯ���ƺ��룬�ر�UDP�����ؽ����
 *          �˺������ڶԳ�λ�������ѯ����λ������Ҫ�����������ӵ�����£�һһ��ѯÿ����λ�ĵ�ǰ���ƺ��롣�����ƺ���������
 *          �����ƾ�ʶ��, ��ʾû�г����������г�û�йҳ��ƣ�����������Ǹ���λ����ѯͣ������λ�����ǰ��λ�ϳ����ĳ��ƺ��롣
 *
 * E 2017-06-07
 *     1. �����ӿ� 
 *			- LPNR_LightCtrl - ����ʶ��������ƣ���200A���ͣ�����ʹ��������ON��OFFѶ�ſ������˻�̧��ˡ�
 *			- LPNR_SetExtDO - ������չDO����߱���չIO�Ļ��Ͳ���Ч��)
 * F 2017-07-11
 *     1. �����ӿ�
 *		  - DLLAPI BOOL CALLTYPE LPNR_SetOSDTimeStamp(HANDLE h, BOOL bEnable, int x, int y);
 *
 * G 2017-09-29
 *		1. ����ʶ�������Ѷ��������ƽӿ�
 *		  - DLLAPI BOOL CALLTYPE LPNR_PulseOut(HANDLE h, int pin, int period, int count);
 *		2. ��������͸������
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
 *    1. LPNR_GetPlateAttribute �����У�����attr������������
 *		  attr[5]��������ɫ���� (PLATE_COLOR_E)
 *        attr[6]:  ��������(PLATE_TYPE_E)
 *    2. ����ɫ���Ƶĳ�����ɫ���ָ�Ϊ"��", ���磺����E12345. �Ա����ɫ���䳵������
 *    ע�� �¼�EVT_PLATENUM����ǰ���͵ĳ��ƺ��룬ʶ����汾1.3.10.3��ʼ������ɫ�Żᷢ�͡���������Ƿ��͡��̡���
 *            �յ�EVT_DONE��ȥ��ȡ�ĳ��ƺ��룬��ֻҪ��֧������Դ����ʶ����汾������ɫ�ƶ��������֡���Ϊ����
 *            ��̬����ݳ�����ɫ���������ɫ���֡�
 *
 *  I 2018-03-26
 *    1. �����Ȧ����˫���ܣ���ͷ����β����ͷ+��β����ʶ�����ǰ�ϱ��ĳ��ƺ�������ǳ�βʶ�𣬻��ڳ��ƺ��������Ϻ�׺
 *        (��β). ��̬���������׺ɾ���������������� attr[1]����Ϊ��β�����յ�EVT_PLATENUM�¼��󣬿��Ի�ȡ���ƺ��루LPNR_GetPlateNumber��
 *       �Լ���ͷ��β��Ϣ��LPNR_GetPlateAttribute�������Ǵ�ʱ��ȡ��attributeֻ��attr[1]��������ġ������ĳ�Ա��Ҫ���յ�EVT_DONE�¼����ٵ���
 *       һ��LPNR_GetPlateAttribute��ȡ��
 *    2. ����һ���¼� EVT_ACK. ʶ����յ����п���֡(DO, PULSE)�󣬻ᷢ��Ӧ��֡��DataType��DTYP_ACK, DataId�����п���֡�Ŀ����� ctrlAttr.code
 *
 * J  2018-03-31
 *    1. LPNR_GetExtDI�ӿڣ���ȡDIֵ����Ϊ LPNR_GetExtDIO (��ȡ��ǰDI��DO��ֵ)
 * 
 * K 2018-05-xx
 *   1. ������Ƶ���ӽӿ�
 *       - LPNR_SetOSDTimeStamp
 *       - LPNR_SetOSDLabel
 *       - LPNR_SetOSDLogo
 *       - LPNR_SetOSDROI
 *       - LPNR_SetOSDPlate
 *       - LPNR_UserOSDOff
 *       - LPNR_UserOSDOn
 *   2. ������Ƶ��ʹ�ܽ��ܽӿ�
 *       - LPNR_SetStream
 *   3. ����ץ��ͼƬ�ϱ�ʹ�ܽ��ܽӿ�
 *       - LPNR_SetCaptureImage
 *
 * L  2018-07-04
 *   1. �޸�覴ã�ԭ�������߳�û�д���select����-1����������ڼ���
 *   2  ���Ӷ���ͼƬ�ͳ���buffer, �յ�DID_BEGIN�󣬰�ǰһ���������ݰᵽ����buffer,��Ȼ�����һ��buffer. ��λ��Ҫ���ݺ�ͼƬʱ���ɶ���bufferȥ��ȡ��
 *       �������Ա��⳵��ʶ�������������Σ���λ��Ҫ��ȡ��һ�εĽ��ʱ����ȡ�������ڶ��δ���ʱ�������������
 *   3  ���ӽӿ� LPNR_GetSelectedImage/LPNR_GetSelectedImageSize���������֮ǰ���������ͬͼƬ�Ľӿڡ���Ϊ�������ݣ�ԭ���Ľӿڻ���
 *       ���Ի�ȡ����ͼƬ����ͼ����ͼ��Сͼ��΢ͼ�����Ʋ�ɫͼ�����ƶ�ֵͼ��
 *   4. LPNR_SetCaptureImage�ӿ��޸ģ�����tiny imageʹ�ܵĲ�������һ����������ץ�Ļ���Ҫ֧��tiny imageץ�ĵ�������İ汾 (1.3.13.12�Լ����İ汾)
 *   5. ����LPNR_GetPlateNumberAdvance�ӿڣ��յ�EVT_PLATENUM�¼���Ҫ��������ӿڲ��ܻ�ȡ����ǰ���͵ĳ��ƺţ�������յ�EVT_DONE֮ǰ����
 *	     LPNR_GetPlateNumber����ȡ��������һ�����Ƶĺ��롣EVT_PLATENUM���EVT_DONE���緢��100~200���롣���ʱ�����Ҫ��ץ��ͼƬ�ַ����ӣ�
 *	     ͼƬѹ����ͼ�������ĵ�ʱ�䡣ʹ�������ͼƬԽ�࣬��ʱԽ����
 *
 * M 2018-07-19
 *   1. ����LOG��������:
 *	     - LPNR_EnableLog - ��ʱʹ��/������־
 *      - LPNR_SetLogPath - ������־�����ļ��У�Ĭ���ļ��п�mtrace.c����ĺ궨�壩
 *      - LPNR_SetLogSizeAndCount - ����ÿ����־�ļ��Ĵ�С���͹������ݵ��ļ�����Ĭ�ϴ�С�͹�������������mtrace.c����ĺ궨�壩
 *	    - LPNR_UserLog - �û�д��־��LPNR����־�ļ����档�����Ա���������Է�װ�Ŀͻ�ָ����̬����򣬿��Խ���־д��
 *                                    LPNR���ڲ���־�ļ����棬ͳһ��¼���ײ����⡣
 *   2. ����proxy��������
 *       proxy���ó��������ܴ�Ϊ������ʶ�����TCP���ӣ��շ�ϵͳһ����������ڳ���һ����������վ���Լ����߼�����λ�����޷�ֱ�ӷ��ʵġ�
 *      ��������Ķ�̬���������������ܴ�proxy���ܣ����ǣ���λ��������������Ҳ�������ӵ����������������������������Ϳ�������ά����Ա
 *      ����Զ�����ӣ����ã����µȡ�
 *      - Proxy_Init  - ʹ�ܲ���ʼ��proxy����
 *      - Proxy_Terminate - ���ܲ�����Proxy���ܣ������Ѿ����ӵ�Զ�̿ͻ��˶��ᱻ�Ͽ��������޷�������
 *      - Proxy_GetClients - ��ȡ��ǰ����proxy�������ӵ�ʶ����Ŀͻ�����Ŀ�Լ����ǵ�IP��ַ
 *   3. ʹ��common������winnet.c �� mtrace.c������ʹ���Դ��ĺ�����
 *
 *  N 2018-09-30
 *    1. ���ץ�Ļ��������ܣ���ȡһ֡���ü�ָ��������͡����ָ��10���ü�����Ŀǰʹ���ֹ����������ļ�������ץ�Ļ�����/home/usercrop.conf)
 *        Ҳ���Է���ץ�Ĳü�����ָ��һ����̬����ü�������Ҫ���ã����ǲ����������������
 *    2. �յ��ü���ץ��ͼƬ�󣬶�̬�ⷢ����ϢEVT_CROPIMG�������Ϣ�ĸ�16λ�ǲü������(1..10)
 *    3. ���һ���ӿ�LPNR_TakeSnapFrameEx��������LPNR_TakeSnapFrame����չ������ָ��ץ���ĸ�����ȵ�ͼ��
 *        ��̬���յ���Ҫ��ץ��֡��һ��������ϢEVT_SNAP�����Ǹ�16λ�ǽ���ȱ�� [1..3]
 *    4. �޸�����BUG��һ��覴á�BUG����IID2Index�������棬IID_HEXת���ǵ�image indexʱ����ĸ���IMGID_CAPSMALL(1/4ͼ), Ӧ�ø�IMGID_CAPTINY(1/16)
 *    5. 覴���ԭ�����Ӻ󣬾�����tickLastHeared��Ա������Ϊ0�������Ļ�������ͻ������Ӻ�һֱû�з��Ͷ�������ʱ�Ͽ��Ͳ��������á����������
 *        �������Ӻ�Ͱ������Աֵ����Ϊ��ǰʱ�䡣�ó�ʱ�Ͽ������Ӻ�������ã�������Ҫ�յ�һ��֡��������á����������覴ü���û��Ӱ�죬��Ϊʶ���
 *        �����ӽ����;ͻᷢ�ͺö൱ǰ���ã��汾�ţ�����ʱ�ӵ�ѶϢ��
 *   NOTE - ʹ�ñ�������һ��Դ�����Ϊ�Լ��Ķ�̬�����Ӧ�ó�����Ҫ����Ŀ�������linux��Makefile�����Լ������Ƿ�Ҫʹ����־
 *                 ���ܡ�����ENABLE_LOG������붨�������ʹ����־���ܡ�Ĭ����û�ж���ġ�Դ���������Ѿ�û�ж��塣
 *              
 * O 2019-06-11
 *     1. Proxy ���ܸ�Ϊͨ�ú�����д��utils_proxy.c. 
 *     2. ����telnet, ftp �Ĵ����ܣ��˿�Ϊ 8023�� 8021
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
#define ENABLE_LOG		// �ŵ���Ŀ�ı��붨������
#define ENABLE_TRACE_FUNCTION  // ҪENABLE_LOG��ǰ�ᣬ�����������á�����Ҫ�Ļ�ע�͵���

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
#define TRACE_FUNCTION(hOBJ)						MTRACE_LOG(hOBJ->hLog,"��%s��\n", __FUNCTION__)
#define TRACE_FUNCTION_FMT(hOBJ,fmt,...)	MTRACE_LOG(hOBJ->hLog, "��%s��- "fmt, __FUNCTION__, ##__VA_ARGS__)
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
#define TRACE_FUNCTION(hOBJ)							MTRACE_LOG(hOBJ->hLog,"��"__FUNCTION__"��\n")
#define TRACE_FUNCTION_FMT(hOBJ,fmt,...)		MTRACE_LOG(hOBJ->hLog, "��"__FUNCTION__"��- "fmt, __VA_ARGS__)
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
	int		acked_ctrl_id;			// �յ������п��������ACK֡DataId, ������ݾ������п���֡�Ŀ��������� header.ctrlAttr.code
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
	// һ��ͼƬ����buffer 
	PVOID		pCapImage[IMGID_BUTT];			// input image in jpeg
	int				szCapImage[IMGID_BUTT];			// input image size
	// һ��������Ϣbuffer�ʹ���ʱ��
	char			strPlate[12];
	PlateInfo	plateInfo;
	int				process_time;		// in msec
	int				elapsed_time;		// in msec
	// ����ͼƬ����buffer
	PVOID		pCapImage2[IMGID_BUTT];			// input image in jpeg
	int				szCapImage2[IMGID_BUTT];			// input image size
	// ����������Ϣbuffer�ʹ���ʱ��
	char			strPlate2[12];
	PlateInfo	plateInfo2;
	int				process_time2;		// in msec
	int				elapsed_time2;		// in msec
	// �ü���ץ��ͼƬ����λ�� - ����ȡ�������ͷ��ڴ棬���Բ����ظ���ȡ���������յ��µ�
	PVOID		pCropImage[10];		
	int				szCropImage[10];
	int				szCropBuf[10];			// crop image buffer  size allocated
	// ʵʱ֡buffer
	PVOID		pLiveImage;
	int				nLiveSize;
	int				nLiveAlloc;
	// ��ǰ���͵ĳ��ƺ��뱣��������, ���buffer���ݲ������ÿ��ʶ���������ǰ���ͺ����
	char			strPlateAdv[12];
	// ����ʶ��ԭ��
	TRIG_SRC_E enTriggerSource;		// TRIG_SRC_E ö�����͵� intֵ 
	//	for model with extended GPIO 
	int				diParam;				// ��16λ��last DI value����16λ��current DI value
	short			dio_val[2];			// [0]��DI��[1]��DO. ����ץ�Ļ��ڿͻ������ӻ���������CTRL_READEXTDIO�ϱ���
											// dio_val[0]Ӧ����Զ��diParam�ĵ�16λ��ͬ��
	
	// �汾ѶϢ
	VerAttr		verAttr;
	// ʱ��ѶϢ����������ʶ�����ʶ����ᷢ������ϵͳʱ��
	CTM			camTime;
	_longtime  camLongTime;		// ʱ����Ϣ��longtime ��ʽ
	int				secDiff;					// PCϵͳʱ�� - ʶ���ϵͳʱ��Ĳ� (��λ��)
	// �������Ϣ
	SIZE		szImage[3];		// 0: full, 1: 1/4, 2: 1/16
	// Logger
	BOOL	bLogEnable;
	HANDLE	hLog;
	// ���������ѶϢ
	ParamConf	paramConf;
	ExtParamConf extParamConf;
	DataAttr		dataCfg;
	H264Conf	h264Conf;
	OSDConf	osdConf;
	OSDPayload osdPayload;
	OSDFixText osdFixText[4];

	// ͸���������ݽ���ring buffer (for model with external serial port)
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
// pos ����ring_buf���λ�ã������ǵڼ������ݣ���head��ʼ����0��
#define PositIndex(h,pos)		( (pos)==(h)->ring_tail ? -1 : ((pos)>=(h)->ring_head ? (pos)-(h)->ring_head : ((MAX_RING_SIZE-(h)->ring_head-1) + (pos))) )
// PositIndex�ķ�������idx����head��ʼ�ĵڼ������ݣ�head��0��������ֵ����ring_buffer���λ�á�
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
	case EVT_ONLINE	:	return "����ʶ�������";
	case EVT_OFFLINE:	return "����ʶ�������";
	case EVT_FIRED:		return "��ʼʶ����";
	case EVT_DONE:		return "ʶ������������ѽ������";
	case EVT_LIVE:		return "ʵʱ֡����";
	case EVT_VLDIN:		return "��������������Ȧ�����";
	case EVT_VLDOUT:	return "�����뿪������Ȧ�����";
	case EVT_EXTDI:		return "��չDI��״̬�仯";
	case EVT_SNAP:		return "���յ�ץ��֡";
	case EVT_ASYNRX:	return "���յ�͸��������������";
	case EVT_PLATENUM: return "���յ���ǰ���͵ĳ��ƺ�";
	case EVT_VERINFO:	return "�յ�ʶ���������汾ѶϢ";
	case EVT_ACK:			return "�յ����п���֡��ACK�ر�";
	case EVT_NEWCLIENT: return "�µ�Զ�̿ͻ������������";
	case EVT_CLIENTGO:	return "Զ�̿ͻ������ӶϿ�";
	}
	return "δ֪�¼����";
}

static const char *GetTriggerSourceText(TRIG_SRC_E enTrig)
{
	switch(enTrig)
	{
	case IMG_UPLOAD:
		return "��λ������ͼƬʶ��";
	case IMG_HOSTTRIGGER:
		return "��λ������";
	case IMG_LOOPTRIGGER:
		return "��ʱ�Զ�����";
	case IMG_AUTOTRIG:
		return "��ʱ�Զ�����";
	case IMG_VLOOPTRG:
		return "������Ȧ����";
	case IMG_OVRSPEED:
		return "���ٴ���";
	}
	return "Unknown Trigger Source";
}

static const char *GetImageName(int index)
{
	const char *img_name[] = { "���ƶ�ֵͼ", "���Ʋ�ɫͼ", "ץ��ȫͼ", "ץ����ͼ", "ץ��Сͼ", "ץ��΢ͼ", "ץ�ĳ�ͷͼ" };
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

		TRACE_LOG(hObj, "�����¼���� %d  (%s)  - param=%d ��Ӧ�ó���.\n", evnt, GetEventText(evnt), param );
	}
	if ( hObj->cbfxc )  hObj->cbfxc((HANDLE)hObj, lParam);
#ifdef WIN32
	if ( hObj->hWnd )
		PostMessage( hObj->hWnd, hObj->nMsgNo, (DWORD)hObj, lParam );
#endif
}

// nWhich 1: һ��buffer, 2: ����buffer, 4: crop image
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
	return TRUE;		// ������Զ��ɹ�
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
	// 1. ��������
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
	// 2. ����Mutex�� and ���������߳�

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


// ����Ӧ�ó�����¼��ص�����
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
// ����Ӧ�ó��������Ϣ�Ĵ��ھ������Ϣ���
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
	TRACE_FUNCTION_FMT(pHvObj, "Ӧ�ó����ȡ���ƺ��� (%s)\n", pHvObj->strPlate);
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
	TRACE_FUNCTION_FMT(pHvObj, "Ӧ�ó����ȡ��ǰ���͵ĳ��ƺ��� (%s)\n", pHvObj->strPlateAdv);
	strcpy(buf, pHvObj->strPlateAdv );
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_GetPlateAttribute(HANDLE h, BYTE *attr)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_FUNCTION_FMT(pHvObj, "Ӧ�ó����ȡ���ƺ��� (%s)\n", pHvObj->strPlate);
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
	TRACE_FUNCTION_FMT(pHvObj," Ӧ�ó����ȡ���Ʋ�ɫͼƬ size=%d\n", pHvObj->szCapImage2[IMGID_PLATECOLOR] );
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
	TRACE_FUNCTION_FMT(pHvObj,"Ӧ�ó����ȡ���ƶ�ֵͼƬ size=%d\n", pHvObj->szCapImage2[IMGID_PLATEBIN] );
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
	// Release ����buffer
	ReleaseData(pHvObj, 2);
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_SoftTrigger(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_LOG(pHvObj,"��LPNR_SoftTrigger��\n");
	if ( pHvObj->enLink != NORMAL )
	{
		TRACE_LOG(pHvObj,"\t - ץ�Ļ�����!\n");
		return FALSE;
	}
	if ( pHvObj->enOper != OP_IDLE )
	{
		TRACE_LOG(pHvObj,"\t - ץ�Ļ�æ����ǰ������δ���!\n");
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
		//TRACE_LOG(pHvObj,"\t - ץ�Ļ�����!\n");
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
		TRACE_LOG(pHvObj," - ץ�Ļ�����!\n");
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
		TRACE_LOG(pHvObj,"\t - ץ�Ļ�����!\n");
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
		TRACE_LOG(pHvObj,"\t-  ʧ�ܣ�ʶ�������!\n");
		return FALSE;
	}
	// �������
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
	TRACE_FUNCTION_FMT(pHvObj,"Enable Lighting=%s��nSizeCode=%d \n", bFlashLight ? "Yes" : "No", nSizeCode );
	if ( pHvObj->enLink != NORMAL )
	{
		TRACE_LOG(pHvObj,"\t-  ʧ�ܣ�ʶ�������!\n");
		return FALSE;
	}
	if ( nSizeCode < 1 || 3 < nSizeCode )
	{
		TRACE_LOG(pHvObj,"\t- �������� nSizeCode ȡֵ��Χ����Ϊ[1..3]\n");
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
		TRACE_LOG(pHvObj,"\t-  ʧ�ܣ�ʶ�������!\n");
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
		TRACE_LOG(pHvObj,"\t-  ʧ�ܣ�ʶ�������!\n");
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
		TRACE_LOG(pHvObj,"\t-  ʧ�ܣ�ʶ�������!\n");
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
		else if ( stricmp(extName, ".yuv")==0 )		// LPNRTool�Ὣԭʼyuv����ΪBMP�����������ʽ��ʵ��������ʵ�����ڵ�
			format = IFMT_YUV;						// ����ʶ�������Ҫ����֡�����ж�ͼ�Ŀ�Ⱥ͸߶ȣ�����������κκϷ������������д���
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
		TRACE_LOG(pHvObj,"\t- ʧ�ܣ�ʶ�������!\n");
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
			; // TRACE_LOG(NULL, "��λ���Ӧ��֡����(0x%x)����ID(%d)����\n", rplyHeader.DataType, rplyHeader.DataId);
		}
	}
	else
	{
		; // TRACE_LOG(NULL, "--> ��λ���Ӧ��ʱ!\n");
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
	if ( tout < 1000 )  tout = 1000;		// ���ٸ�1��

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
				// ����image��buffer������������ݴ��󣬻��Ƿ��͵�UDP������64K
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
		if ( loc != 1 && loc != 2 && loc != 3 ) loc = 2;		// Ĭ��������
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
	header.osdText.nFontId = 0;		// ����
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
	TRACE_FUNCTION_FMT(pHvObj,"Ӧ�ó����ȡץ��ͼƬ size=%d\n", pHvObj->szCapImage2[IMGID_CAPFULL] );
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
	TRACE_FUNCTION_FMT(pHvObj,"Ӧ�ó����ȡ��ͷͼƬ size=%d\n", pHvObj->szCapImage2[IMGID_CAPHEAD] );
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
	TRACE_FUNCTION_FMT(pHvObj,"Ӧ�ó����ȡ4/9ͼƬ size=%d\n", pHvObj->szCapImage2[IMGID_CAPMIDDLE] );
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
	TRACE_FUNCTION_FMT(pHvObj,"Ӧ�ó����ȡ1/4ͼƬ size=%d\n", pHvObj->szCapImage2[IMGID_CAPSMALL]  );
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
					TRACE_LOG(pHvObj, "��ȡ����֡ͷ��õĳ�����%d, ��Ҫ%d�ֽڡ�����ͬ������һ��֡ͷ��\n",
							rc, sizeof(dataHeader) );
					nskip = sock_drain_until( m_sock, soh, 3 );
					TRACE_LOG(pHvObj,"--> ���� %d �ֽ�!\n", nskip);
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
						TRACE_LOG(pHvObj,"==> ���յ��ü�ץ��ͼ%d - %s (%d bytes)\n", index, dataHeader.imgAttr.basename, dataHeader.size);
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
						TRACE_LOG(pHvObj,"==> ���յ�%s - %s (%d bytes) Image DataId=%d, Index=%d - ImageId=%d\n", 
									GetImageName(index), dataHeader.imgAttr.basename, dataHeader.size, dataHeader.DataId,dataHeader.imgAttr.index, index);
						if ( strcmp(dataHeader.imgAttr.basename,"vsnap.jpg") == 0 )
						{
							//  ����snap capture, �û��յ���Ϣ��ͻ�����ȡ������Ӧ��ֱ�ӹ�����szCapImage2��pCapImage2
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
						TRACE_LOG(pHvObj,"==> ����ʶ������͵�ͼ%s (%d bytes)\n", dataHeader.imgAttr.basename, dataHeader.size);
					}
				}  // else if not live image
			}
			else if ( dataHeader.DataType == DTYP_DATA )
			{
				switch (dataHeader.DataId )
				{
				case DID_BEGIN:
					memset( &pHvObj->plateInfo, 0, sizeof(PlateInfo) );
					strcpy( pHvObj->strPlate, "  ���ƾ�ʶ" );
					pHvObj->enOper = OP_RREPORT;
					break;
				case DID_END:
					pHvObj->enOper = OP_IDLE;
					tickStartProc = 0;
					// �����������ƽ������ֵ
					// 0 Ϊ ���Ķ� 0~100
					// 1 Ϊ ��ͷ��β��Ϣ 1 ��ͷ��0xff ��β��0 δ֪
					// 2 Ϊ������ɫ���� ������ɿ���
					pHvObj->plateInfo.MatchRate[3] = (BYTE)dataHeader.dataAttr.val[2];	// ����
					pHvObj->plateInfo.MatchRate[4] = (BYTE)dataHeader.dataAttr.val[1];	// ����Դ
					// ���������ݺ�ͼƬ��һ��bufferת������buffer, ��λ����ȡͼƬ�ͳ�����Ϣ���ɶ���buffer��ȡ
					RolloverData(pHvObj);
					TRACE_LOG(pHvObj, "==>ʶ����������ݽ������!\n");
					NoticeHostEvent(pHvObj, EVT_DONE );
					break;
				case DID_PLATE:
					Mutex_Lock(pHvObj);
					memcpy( &pHvObj->plateInfo, &dataHeader.plateInfo, sizeof(PlateInfo) );
					switch( pHvObj->plateInfo.plateCode & 0x00ff )
					{
					case PLC_BLUE:
						strcpy( pHvObj->strPlate, "��" );
						break;
					case PLC_YELLOW:
						strcpy( pHvObj->strPlate, "��" );
						break;
					case PLC_WHITE:
						strcpy( pHvObj->strPlate, "��" );
						break;
					case PLC_BLACK:
						strcpy( pHvObj->strPlate, "��" );
						break;
					case PLC_GREEN:
						strcpy( pHvObj->strPlate, "��" );
						break;
					case PLC_YELGREEN:
						strcpy( pHvObj->strPlate, "��" );
						break;
					default:
						strcpy( pHvObj->strPlate, "��" );
					}
					strcat( pHvObj->strPlate, pHvObj->plateInfo.chNum );
					TRACE_LOG(pHvObj, "==> ���յ�ʶ��ĳ��� ��%s��\n", pHvObj->strPlate );
					pHvObj->plateInfo.MatchRate[5] = (BYTE)pHvObj->plateInfo.plateCode & 0x00ff;	// ������ɫ����
					pHvObj->plateInfo.MatchRate[6] = (BYTE)((pHvObj->plateInfo.plateCode>>8) & 0xff);	// �������ʹ���
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
					TRACE_LOG(pHvObj, "==> ʶ�������ʶ�� (%s)����ʼ����!\n", GetTriggerSourceText(pHvObj->enTriggerSource) );
					NoticeHostEvent(pHvObj, EVT_FIRED );
					break;
				case EID_VLDIN:
					TRACE_LOG(pHvObj, "==> ��������������Ȧʶ����!\n");
					NoticeHostEvent(pHvObj, EVT_VLDIN );
					break;
				case EID_VLDOUT:
					TRACE_LOG(pHvObj, "==> �����뿪������Ȧʶ����!\n");
					NoticeHostEvent(pHvObj, EVT_VLDOUT );
					break;
				case EID_EXTDI:
					pHvObj->diParam = dataHeader.evtAttr.param;
					pHvObj->dio_val[0] = pHvObj->diParam & 0xffff;
					TRACE_LOG(pHvObj, "==> EXT DI ״̬�仯 0x%x -> 0x%x\n", 
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
				TRACE_LOG(pHvObj, "�յ���չ���ò��� - label=%s\n", pHvObj->label);
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
				// �յ���ǰ���͵ĳ��ƺ���
				char *ptr;
				// ������ƺ����к�׺ (��β)�����磺"����E12345(��β)"����Ҫ�Ѻ�׺����
				if ( (ptr=strchr(dataHeader.textAttr.text,'('))!=NULL)
				{
					*ptr = '\0';
					pHvObj->plateInfo.MatchRate[1] = 0xff;
				}
				strcpy(pHvObj->strPlateAdv ,dataHeader.textAttr.text);
				NoticeHostEvent(pHvObj, EVT_PLATENUM );
				TRACE_LOG(pHvObj, "�յ���ǰ���͵ĳ��ƺ���: %s\n", pHvObj->strPlateAdv);
			}
			else if ( dataHeader.DataType==DTYP_TIME )
			{
				ParseCameraTimeHeader(pHvObj,&dataHeader.timeAttr);
			}
			else if ( dataHeader.DataType==DTYP_ACK && dataHeader.DataId!=0 )
			{
				// �յ�����Ӧ��֡
				pHvObj->acked_ctrl_id = dataHeader.DataId;
				NoticeHostEvent(pHvObj, EVT_ACK );
				TRACE_LOG(pHvObj, "�յ��������� %d��Ӧ��֡!\n", pHvObj->acked_ctrl_id);
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
			TRACE_LOG(pHvObj,"LPNR ����ͼƬʱ�䳬��2�룬��λ״̬������!\n");
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
 
 // ��Windowsģʽ��֧��Proxy���ܣ���Ϊ��ЩAPI����Windows��ʵ�ֲ��Ҳ��Թ�
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

	
	TRACE_LOG(pHvObj, "=====��PROXY MASTER THREAD BEGIN��=====\n");
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
				header.dataAttr.val[2] = 1;		// ����PROXY ���ӵ�
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
	TRACE_LOG(pHvObj, "=====��PROXY MASTER THREAD EXIT��=====\n");
	closesocket(pHvObj->proxy_listen);
	closesocket(pHvObj->proxy_udp);
	pHvObj->proxy_listen = INVALID_SOCKET;
	pHvObj->proxy_udp = INVALID_SOCKET;
	// �Ѿ������Ĵ������ӻ����Լ�����ֱ���Լ��Ͽ�����Proxy_Terminate������
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
