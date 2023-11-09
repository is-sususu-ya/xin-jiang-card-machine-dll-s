#ifndef UTILS_MSQ_INCLUDED
#define UTILS_MSQ_INCLUDED

       
/*
	  message queue 是Unix系统家族常用的IPC机制，可以跨进程交换讯息。在多进程/线程的系统里，IPC机制是
	  不可少的。它适合发送简短的消息，让程序流程可以根据消息来推动。类似于Windows的消息机制，但是它比
	  Windows的消息机制强的是它可以带内容（Windows消息只能带两个参数 WPARAM,LPARAM）
	  每个消息的最大长度默认是8192字节(系统参数/proc/sys/kernel/msgmax)，
	  每个消息队列的最大可用空间是16384字节(系统参数/proc/sys/kernel/msgmnb)，
	  所有消息队列总计可用空间是64K字节(无系统参数文件)
	  整个系统可以创建的最大message queue数目为16 (系统参数/proc/sys/kernel/msgmni).
	  (以上参数不一定每个系统相同)
	  
	  很长的消息不适合用msq直接交换，而是应该写成ramfs里的文件，再把文件名用消息发送给接收方，由接收方
	  自己去读文件。
	  每个消息包可以有不同长度，它的前4个字节是消息类型(mtype)，可以当作是个消息编号，后面跟着消息内容(mtext)。
	  消息内容长度可以是0~(msgmax-4)。
	  mtype的规划很重要，需要根据应用系统的需要规划。
	  
	  case 1: 只有一个消息接收者，1到多个消息发送者. 消息传送路径都是单向的。
	  	这是最简单的机制，可以把消息编号根据重要性编号，越重要的消息编号越小。例如 1~9是紧急事件，
	  	10~99是一般事件，100~999是可以忽略的消息。这样当接受方调用消息接收函数msq_recv时，只要mtype值
	  	给-1000, 那样就会获取到当前消息队列里面mtype值最小的一个消息。
	  	
	  case 2: 1~N个进程（线程、程序功能模块等都一样，以下统称进程），每个都可以发送也可以接收给任何其他进程。
	  	此种情况，需要每个进程赋予一个事先规划好的编号。例如进程A是1号、进程B是2号.... 
	  	当然所有程序都知道每个进程被赋予的编号，要发给谁，mtype就给目的进程的编号。每个进程只能接收
	  	属于自己编号的消息。此种情况，mtype实际是被用来指定消息的接收方编号，真正的消息编号（一个接收方
	  	大多会接收不同类型的消息）要放在消息内容里面，应用程序自己要规划消息内容(mtext)的数据结构来体现
	  	消息编号和对应于该编号的数据内容（如果有的话）。例如：
	  	 struct common_msgbuf {
	  	  long sender_id;			// 谁发送的，也就是发送方的固定mtype
	  	 	int msg_id;					// 消息类型编号
	  	 	// 根据不同类型的消息可能还有附加数据的，全部定义在一个 union里面
	  	 	union {
	  	 	  struct msg_1_dependant {...};
	  	 	  struct msg_2_dependant {...};
					...
	  	 	  struct msg_x_dependant {...};
	  	 	 };
	  	  };
	  	 这样接收到消息的人，完成处理后如果需要应答，也是发送一个common_msgbuf的结构，发送的mtype
	  	 就是接收到的common_msgbuf里面的sender_id. 例如
	  	 [receiver 1]
	  	 strcut common_msgbuf recv_msg;
	  	 long my_type = 10;
	  	 for(;;)
	  	 {
	  	 		if ( msq_recv(msqid,&my_type,&recv_msg, sizeof(recv_msg),0) > 0 )
	  	 		{
	  	 			struct common_msgbuf send_msg;
	  	 			send_msg.sender_id = my_type;
	  	 			// 根据接收到的recv_msg.event, 处理事务，填充send_msg的响应内容
	  	 			...
	  	 			// 发送应答
	  	 			msg_send(msqid, recv_msg.sender_id, &send_msg, sizeof(send_msg),0);
	  	 		}
	  	 }
	  	 
	  	 [sender 1]
	  	 #define MY_TYPE_ID		101
	  	 #define MOD_TRANS_ID	1		// 管理transaction的进程mtype编号
	  	 struct common_msgbuf send_msg;
	  	 send_msg.sender_id = MY_TYPE_ID;
	  	 send_msg.msg_id = MSG_GET_TRANS;
	  	 msq_send(msqid, MOD_TRANS_ID, &send_msg, sizeof(send_msg), 0);
	  	 // 接收应答应该在自己的消息接收循环里面处理，这里是直接接收
	  	 long my_type = MY_TYPE_ID;
	  	 struct common_msgbuf recv_msg;
	  	 for( not time-out yet )
	  	 {
	  	 		if (msq_recv(msqid, &my_type, &recv_msg, sizeof(recv_msg),1)==-1 )
	  	 			usleep(1000);
	  	 }
	  	 
	  case 3: 1个主进程，接收大部分的消息，N个辅助进程，大多数的消息由他们发送，但是也有其他进程会接收
	     由主进程（也可能是其他进程）发送的其他消息。
	     此种情况，将消息编号1~N赋予主进程，其他会接收消息的进程赋予N+1 ~ N+X的编号。每个进程只接收属于自己
	     编号的消息。对于主进程，消息编号还是消息编号的意义，对于其他接收方进程，消息编号就是用来指定接收方
	     是哪个进程而已，真正的消息编号和内容要放在自己定义的数据结构里面，把这个结构当消息内容发送。
	     
	     通常会用到的情况是发送者发送消息给接收者，要求获取某些接收者才知道的讯息，接收者收到后把需要的讯息
	     发送给发送者。为了让接收者知道是谁发送的，发送者应该把自己用来接收的消息编号放到发送给接收者的mtext
	     里面。例如：
	     [MASTER PROCESS]
	     for(;;)
	     {
	     		long mtype = -100;		// 1~100为主进程接收的消息
	     		struct common_msgbuf  recv_msg;
	     		if ( msq_recv(msqid, &mtype, &recv_msg, sizeof(recv_msg), 0)>0 )
	     		{
		     		// 根据收到的消息类型mtype,处理消息，填充应答
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
		     		// 如果需要应答，填充应答帧
		     		strcut common_msgbuf send_msg;
		     		send_msg.sender_id = 0;  // 0 是主进程
		     		// 发送应答帧
		     		msq_send(msqid, recv_msg.sender_id, &send_msg, sizeof(send_msg), 0);
		     }
	     }
			
			[其它进程]
			// 如果发送给非MASTER PROCESS, msq_send里的mtype是目的进程指定的mtype，如果发送给
			// MASTER PROCESS, mtype就是消息类型本身(1~100)
			// 消息内容结构里面的sender_id成员一样填自己的mtype编号（谁发送的），
			// msg_id成员是填应用系统定义的消息编号(要做什么事情,或是提供什么讯息)。
			
	  一个应用系统（多进程或是单进程）应该使用一个message queue就足够，当然也可以让每个进程使用一个message queue
	  这样就是多个case 1的情况。但是不建议这样做，因为message queue的数量是很少的资源。而且每个进程都需要去
	  attach多个message queue（如果需要发送消息给多个进程），用起来会比较麻烦。
 */       
 
 
/* msq_create 创建一个message queue, 'path'是一个存在的路径， progid是1~255的一个数据。
 * 这是利用ftok来产生唯一的ipc key。返回值是一个msqid。
 * 'share'为0时，如果指定的queue已经存在，会返回错误。
 * 'share'为1时,如果message queue已经存在，就相当于msq_attach
 * 这表示另外一个copy的程序已经在运行。
 * 返回值:
 * >=0 msqid (message queue id), -1 失败
 * 后续对这个message queue的操作需要这个创建时返回的msqid
 * msq_destroy 用来销毁一个创建好的message queue.
 * message queue应该由主进程（线程）创建和销毁，其他进程（线程）使用attach获取msqid
 */
int msq_create(const char *path, int progid, int share);
int msq_destroy(int msqid);

/*
 * msq_attach - attach已经创建了的message queue. 返回值是msqid
 */
int msq_attach(const char *path, int progid);

/*
 * msq_send - 发送一个消息到message queue.
 * [IN]
 * 'msqid' - 消息队列ID
 * 'mtype' - 发送的消息号 (>0)
 * 'mtext' - 消息的mtext (NULL表示没有mtext, size忽略)
 * 'size' - data的长度 (>=0)
 * 'nowait' - 0 for block mode, message queue 没有足够剩余空间时，函数block，等待有空间后发送再返回。
 *           != 0 for non-block mode, 空间不足时，立刻返回失败。
 * return value: 0 success, -1 failure (errno is the error code)
 */
int msq_send(int msqid, long mtype, void *mtext, int size, int nowait);

/*
 *  msg_recv: 由message queue里面获取一个消息包
 * [IN]
 *  'msqid'为message queue id 
 * [IN/OUT]
 *  'mtype'输入值是要接收的消息的mtype, 可以有以下值
 *  0 - 接收第一个消息，不管消息包的mtype值
 *  >0 - 接收消息队列里mtype符合这个值的第一个消息包
 *  <0 - 接收消息队列里面 mtype小于等于 abs('mtype')并且mtype值最小的一个包。例如，队列里面有5个包，mtype分别为5,4,3,2,1
 *       如果参数'mtype'为-3，那mtype为1的包会被优先获取。
 *  'mtype'在程序返回时，赋值为接收到的消息包的mtype值。'mtype'输入值<=0时，函数成功则输出值和输入值不一样。
 *    
 * [OUT]
 * 'mtext' 接收函数将消息包的内容复制到这个地址里面
 * [IN]
 * 'size' 'data'所指buffer的大小。如果获取的消息包mtext长度大于size,超出的部分会被砍掉。
 * 'nowait' - 是否等待meqid里面有符合条件的消息包。
 *   0 - 不等待，没有message queue没有所需要的消息包就立刻返回-1
 *   1 - 等待
 *
 * 返回值
 *  -1: 失败，errno为错误码
 *  >=0: 收到的消息包内容mtext的字节数(复制到data里的字节数).
 */
int msq_recv(int msqid, long *mtype, void *mtext, int size, int nowait);

/*
 * msg_info - 获取message queue的使用情况讯息
 * [IN]
 *  'msqid' - 要获取讯息的message queue的id号
 * [OUT]
 *  'qnum' - 消息队列里的消息包数量 (可以给NULL)
 *  'qbytes' - 消息队列的总分配空间 (可以给NULL)
 *  'qfree' - 消息队列里剩余的可用空间（字节数）(可以给NULL)
 * 返回值
 *  0 success, -1 failure
 */
int msq_info(int msqid, int *qnum, int *qbytes, int *qfree);

/*
 * msq_extend - 改变message queue的分配空间 (需要是message queue的拥有者，也就是创建这个queue的uid), 
 * mnb是新的message queue大小。如果mnb值比当前值小就是缩小，一般调用这个函数是用来扩展的，所以取这个名字。
 * 'mnb'值如果设置的比系统参数MSGMNB (同/proc/sys/kernel/msgmnb内容)，需要有权限的程序才可以。
 */
int msq_extend(int msqid, int mnb);

#endif
