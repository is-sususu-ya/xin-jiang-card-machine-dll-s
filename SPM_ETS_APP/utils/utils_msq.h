#ifndef UTILS_MSQ_INCLUDED
#define UTILS_MSQ_INCLUDED

       
/*
	  message queue ��Unixϵͳ���峣�õ�IPC���ƣ����Կ���̽���ѶϢ���ڶ����/�̵߳�ϵͳ�IPC������
	  �����ٵġ����ʺϷ��ͼ�̵���Ϣ���ó������̿��Ը�����Ϣ���ƶ���������Windows����Ϣ���ƣ���������
	  Windows����Ϣ����ǿ���������Դ����ݣ�Windows��Ϣֻ�ܴ��������� WPARAM,LPARAM��
	  ÿ����Ϣ����󳤶�Ĭ����8192�ֽ�(ϵͳ����/proc/sys/kernel/msgmax)��
	  ÿ����Ϣ���е������ÿռ���16384�ֽ�(ϵͳ����/proc/sys/kernel/msgmnb)��
	  ������Ϣ�����ܼƿ��ÿռ���64K�ֽ�(��ϵͳ�����ļ�)
	  ����ϵͳ���Դ��������message queue��ĿΪ16 (ϵͳ����/proc/sys/kernel/msgmni).
	  (���ϲ�����һ��ÿ��ϵͳ��ͬ)
	  
	  �ܳ�����Ϣ���ʺ���msqֱ�ӽ���������Ӧ��д��ramfs����ļ����ٰ��ļ�������Ϣ���͸����շ����ɽ��շ�
	  �Լ�ȥ���ļ���
	  ÿ����Ϣ�������в�ͬ���ȣ�����ǰ4���ֽ�����Ϣ����(mtype)�����Ե����Ǹ���Ϣ��ţ����������Ϣ����(mtext)��
	  ��Ϣ���ݳ��ȿ�����0~(msgmax-4)��
	  mtype�Ĺ滮����Ҫ����Ҫ����Ӧ��ϵͳ����Ҫ�滮��
	  
	  case 1: ֻ��һ����Ϣ�����ߣ�1�������Ϣ������. ��Ϣ����·�����ǵ���ġ�
	  	������򵥵Ļ��ƣ����԰���Ϣ��Ÿ�����Ҫ�Ա�ţ�Խ��Ҫ����Ϣ���ԽС������ 1~9�ǽ����¼���
	  	10~99��һ���¼���100~999�ǿ��Ժ��Ե���Ϣ�����������ܷ�������Ϣ���պ���msq_recvʱ��ֻҪmtypeֵ
	  	��-1000, �����ͻ��ȡ����ǰ��Ϣ��������mtypeֵ��С��һ����Ϣ��
	  	
	  case 2: 1~N�����̣��̡߳�������ģ��ȶ�һ��������ͳ�ƽ��̣���ÿ�������Է���Ҳ���Խ��ո��κ��������̡�
	  	�����������Ҫÿ�����̸���һ�����ȹ滮�õı�š��������A��1�š�����B��2��.... 
	  	��Ȼ���г���֪��ÿ�����̱�����ı�ţ�Ҫ����˭��mtype�͸�Ŀ�Ľ��̵ı�š�ÿ������ֻ�ܽ���
	  	�����Լ���ŵ���Ϣ�����������mtypeʵ���Ǳ�����ָ����Ϣ�Ľ��շ���ţ���������Ϣ��ţ�һ�����շ�
	  	������ղ�ͬ���͵���Ϣ��Ҫ������Ϣ�������棬Ӧ�ó����Լ�Ҫ�滮��Ϣ����(mtext)�����ݽṹ������
	  	��Ϣ��źͶ�Ӧ�ڸñ�ŵ��������ݣ�����еĻ��������磺
	  	 struct common_msgbuf {
	  	  long sender_id;			// ˭���͵ģ�Ҳ���Ƿ��ͷ��Ĺ̶�mtype
	  	 	int msg_id;					// ��Ϣ���ͱ��
	  	 	// ���ݲ�ͬ���͵���Ϣ���ܻ��и������ݵģ�ȫ��������һ�� union����
	  	 	union {
	  	 	  struct msg_1_dependant {...};
	  	 	  struct msg_2_dependant {...};
					...
	  	 	  struct msg_x_dependant {...};
	  	 	 };
	  	  };
	  	 �������յ���Ϣ���ˣ���ɴ���������ҪӦ��Ҳ�Ƿ���һ��common_msgbuf�Ľṹ�����͵�mtype
	  	 ���ǽ��յ���common_msgbuf�����sender_id. ����
	  	 [receiver 1]
	  	 strcut common_msgbuf recv_msg;
	  	 long my_type = 10;
	  	 for(;;)
	  	 {
	  	 		if ( msq_recv(msqid,&my_type,&recv_msg, sizeof(recv_msg),0) > 0 )
	  	 		{
	  	 			struct common_msgbuf send_msg;
	  	 			send_msg.sender_id = my_type;
	  	 			// ���ݽ��յ���recv_msg.event, �����������send_msg����Ӧ����
	  	 			...
	  	 			// ����Ӧ��
	  	 			msg_send(msqid, recv_msg.sender_id, &send_msg, sizeof(send_msg),0);
	  	 		}
	  	 }
	  	 
	  	 [sender 1]
	  	 #define MY_TYPE_ID		101
	  	 #define MOD_TRANS_ID	1		// ����transaction�Ľ���mtype���
	  	 struct common_msgbuf send_msg;
	  	 send_msg.sender_id = MY_TYPE_ID;
	  	 send_msg.msg_id = MSG_GET_TRANS;
	  	 msq_send(msqid, MOD_TRANS_ID, &send_msg, sizeof(send_msg), 0);
	  	 // ����Ӧ��Ӧ�����Լ�����Ϣ����ѭ�����洦��������ֱ�ӽ���
	  	 long my_type = MY_TYPE_ID;
	  	 struct common_msgbuf recv_msg;
	  	 for( not time-out yet )
	  	 {
	  	 		if (msq_recv(msqid, &my_type, &recv_msg, sizeof(recv_msg),1)==-1 )
	  	 			usleep(1000);
	  	 }
	  	 
	  case 3: 1�������̣����մ󲿷ֵ���Ϣ��N���������̣����������Ϣ�����Ƿ��ͣ�����Ҳ���������̻����
	     �������̣�Ҳ�������������̣����͵�������Ϣ��
	     �������������Ϣ���1~N���������̣������������Ϣ�Ľ��̸���N+1 ~ N+X�ı�š�ÿ������ֻ���������Լ�
	     ��ŵ���Ϣ�����������̣���Ϣ��Ż�����Ϣ��ŵ����壬�����������շ����̣���Ϣ��ž�������ָ�����շ�
	     ���ĸ����̶��ѣ���������Ϣ��ź�����Ҫ�����Լ���������ݽṹ���棬������ṹ����Ϣ���ݷ��͡�
	     
	     ͨ�����õ�������Ƿ����߷�����Ϣ�������ߣ�Ҫ���ȡĳЩ�����߲�֪����ѶϢ���������յ������Ҫ��ѶϢ
	     ���͸������ߡ�Ϊ���ý�����֪����˭���͵ģ�������Ӧ�ð��Լ��������յ���Ϣ��ŷŵ����͸������ߵ�mtext
	     ���档���磺
	     [MASTER PROCESS]
	     for(;;)
	     {
	     		long mtype = -100;		// 1~100Ϊ�����̽��յ���Ϣ
	     		struct common_msgbuf  recv_msg;
	     		if ( msq_recv(msqid, &mtype, &recv_msg, sizeof(recv_msg), 0)>0 )
	     		{
		     		// �����յ�����Ϣ����mtype,������Ϣ�����Ӧ��
		     		switch(mtype)
		     		{
		     		case 1:			// for example, means get current transaction data
			     		send_msg.msg_id = MTYP_TRANSDATA;
		     			memcpy(&send_msg.transData, transaction, sizeof(*transaction));
		     			break;
		     		case 2:
		     			...
						default:		// invalid message type
							// log error message		     		
		     		}
		     		// �����ҪӦ�����Ӧ��֡
		     		strcut common_msgbuf send_msg;
		     		send_msg.sender_id = 0;  // 0 ��������
		     		// ����Ӧ��֡
		     		msq_send(msqid, recv_msg.sender_id, &send_msg, sizeof(send_msg), 0);
		     }
	     }
			
			[��������]
			// ������͸���MASTER PROCESS, msq_send���mtype��Ŀ�Ľ���ָ����mtype��������͸�
			// MASTER PROCESS, mtype������Ϣ���ͱ���(1~100)
			// ��Ϣ���ݽṹ�����sender_id��Աһ�����Լ���mtype��ţ�˭���͵ģ���
			// msg_id��Ա����Ӧ��ϵͳ�������Ϣ���(Ҫ��ʲô����,�����ṩʲôѶϢ)��
			
	  һ��Ӧ��ϵͳ������̻��ǵ����̣�Ӧ��ʹ��һ��message queue���㹻����ȻҲ������ÿ������ʹ��һ��message queue
	  �������Ƕ��case 1����������ǲ���������������Ϊmessage queue�������Ǻ��ٵ���Դ������ÿ�����̶���Ҫȥ
	  attach���message queue�������Ҫ������Ϣ��������̣�����������Ƚ��鷳��
 */       
 
 
/* msq_create ����һ��message queue, 'path'��һ�����ڵ�·���� progid��1~255��һ�����ݡ�
 * ��������ftok������Ψһ��ipc key������ֵ��һ��msqid��
 * 'share'Ϊ0ʱ�����ָ����queue�Ѿ����ڣ��᷵�ش���
 * 'share'Ϊ1ʱ,���message queue�Ѿ����ڣ����൱��msq_attach
 * ���ʾ����һ��copy�ĳ����Ѿ������С�
 * ����ֵ:
 * >=0 msqid (message queue id), -1 ʧ��
 * ���������message queue�Ĳ�����Ҫ�������ʱ���ص�msqid
 * msq_destroy ��������һ�������õ�message queue.
 * message queueӦ���������̣��̣߳����������٣��������̣��̣߳�ʹ��attach��ȡmsqid
 */
int msq_create(const char *path, int progid, int share);
int msq_destroy(int msqid);

/*
 * msq_attach - attach�Ѿ������˵�message queue. ����ֵ��msqid
 */
int msq_attach(const char *path, int progid);

/*
 * msq_send - ����һ����Ϣ��message queue.
 * [IN]
 * 'msqid' - ��Ϣ����ID
 * 'mtype' - ���͵���Ϣ�� (>0)
 * 'mtext' - ��Ϣ��mtext (NULL��ʾû��mtext, size����)
 * 'size' - data�ĳ��� (>=0)
 * 'nowait' - 0 for block mode, message queue û���㹻ʣ��ռ�ʱ������block���ȴ��пռ�����ٷ��ء�
 *           != 0 for non-block mode, �ռ䲻��ʱ�����̷���ʧ�ܡ�
 * return value: 0 success, -1 failure (errno is the error code)
 */
int msq_send(int msqid, long mtype, void *mtext, int size, int nowait);

/*
 *  msg_recv: ��message queue�����ȡһ����Ϣ��
 * [IN]
 *  'msqid'Ϊmessage queue id 
 * [IN/OUT]
 *  'mtype'����ֵ��Ҫ���յ���Ϣ��mtype, ����������ֵ
 *  0 - ���յ�һ����Ϣ��������Ϣ����mtypeֵ
 *  >0 - ������Ϣ������mtype�������ֵ�ĵ�һ����Ϣ��
 *  <0 - ������Ϣ�������� mtypeС�ڵ��� abs('mtype')����mtypeֵ��С��һ���������磬����������5������mtype�ֱ�Ϊ5,4,3,2,1
 *       �������'mtype'Ϊ-3����mtypeΪ1�İ��ᱻ���Ȼ�ȡ��
 *  'mtype'�ڳ��򷵻�ʱ����ֵΪ���յ�����Ϣ����mtypeֵ��'mtype'����ֵ<=0ʱ�������ɹ������ֵ������ֵ��һ����
 *    
 * [OUT]
 * 'mtext' ���պ�������Ϣ�������ݸ��Ƶ������ַ����
 * [IN]
 * 'size' 'data'��ָbuffer�Ĵ�С�������ȡ����Ϣ��mtext���ȴ���size,�����Ĳ��ֻᱻ������
 * 'nowait' - �Ƿ�ȴ�meqid�����з�����������Ϣ����
 *   0 - ���ȴ���û��message queueû������Ҫ����Ϣ�������̷���-1
 *   1 - �ȴ�
 *
 * ����ֵ
 *  -1: ʧ�ܣ�errnoΪ������
 *  >=0: �յ�����Ϣ������mtext���ֽ���(���Ƶ�data����ֽ���).
 */
int msq_recv(int msqid, long *mtype, void *mtext, int size, int nowait);

/*
 * msg_info - ��ȡmessage queue��ʹ�����ѶϢ
 * [IN]
 *  'msqid' - Ҫ��ȡѶϢ��message queue��id��
 * [OUT]
 *  'qnum' - ��Ϣ���������Ϣ������ (���Ը�NULL)
 *  'qbytes' - ��Ϣ���е��ܷ���ռ� (���Ը�NULL)
 *  'qfree' - ��Ϣ������ʣ��Ŀ��ÿռ䣨�ֽ�����(���Ը�NULL)
 * ����ֵ
 *  0 success, -1 failure
 */
int msq_info(int msqid, int *qnum, int *qbytes, int *qfree);

/*
 * msq_extend - �ı�message queue�ķ���ռ� (��Ҫ��message queue��ӵ���ߣ�Ҳ���Ǵ������queue��uid), 
 * mnb���µ�message queue��С�����mnbֵ�ȵ�ǰֵС������С��һ��������������������չ�ģ�����ȡ������֡�
 * 'mnb'ֵ������õı�ϵͳ����MSGMNB (ͬ/proc/sys/kernel/msgmnb����)����Ҫ��Ȩ�޵ĳ���ſ��ԡ�
 */
int msq_extend(int msqid, int mnb);

#endif
