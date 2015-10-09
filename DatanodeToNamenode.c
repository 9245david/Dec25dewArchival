#include "Socket_connect_read_write.h"
#include "DatanodeToNamenode.h"
#include "BlockStruct.h"
#include <assert.h>
#include "err_backtrace.h"
// oneTasktime 没有设置，FeedbackDToN完全没有填充任何内容
extern PNamenodeID  PnamenodeID;
extern void *ProcessChunkTask(void *argv);
extern void *DataToDataTaskServer(void *arg);
extern pthread_mutex_t g_memoryLock;
//extern char * DtoNbuffer ;
//extern long  DtoNlength ;
struct timeval oneTasktime = {20,0};
char * localIPaddress;
bool sendFeedback = false; //全局变量
pthread_mutex_t lockFeedback;
struct timeval taskStarttime;
int32_t g_recv_end = 0;//0 未结束，1结束
int32_t ALL_TASK_FINISHED = 0;
#pragma pack(push)
#pragma pack(1)
typedef struct RegistAndTaskFeedback{
//	 信息1：发送本地剩余带宽，剩余空间大小
//	 信息2：反馈分配的任务完成度
	double wholeBandwidth;//MB/s，总带宽
	double availableBandwidth;//可用带宽
	double wholeStorageSpace;//MB
	double availableStorageSpace;
	int32_t finishedOrNot ;//如果为1表示分配的任务已经完成，为0表示没有完成，只有当第一次注册时默认为1不是这个含义
	int32_t allocatedTask ;//被分配的任务数量，即数据块的个数
	int32_t finishedTask ; //在预想时间内完成的数据块的个数
	int32_t finishedTime ; //如果完成了，提前完成所花费的时间
}nFeedback,*pFeedback;
#pragma pack(pop)

int32_t g_finished_task = 0;
int32_t g_unfinished_task = 0;
pthread_mutex_t g_finished_task_lock;
int32_t main(int32_t argc,char** argv)
{
	struct sigaction myAction;
system("ifconfig");
myAction.sa_sigaction = print_reason;
sigemptyset(&myAction.sa_mask);
myAction.sa_flags = SA_RESTART | SA_SIGINFO;
sigaction(SIGSEGV, &myAction, NULL);
sigaction(SIGUSR1, &myAction, NULL);
sigaction(SIGFPE, &myAction, NULL);
sigaction(SIGILL, &myAction, NULL);
sigaction(SIGBUS, &myAction, NULL);
sigaction(SIGABRT, &myAction, NULL);
sigaction(SIGSYS, &myAction, NULL);
sigaction(SIGPIPE, &myAction, NULL);//for tcp close ,and write error
	pthread_mutex_init(&g_finished_task_lock,NULL);
	PnamenodeID = (PNamenodeID)malloc(sizeof(NamenodeID));
	assert(PnamenodeID != NULL);
	PnamenodeID->NameToDataPort = DATA_TO_NAME_PORT;
	PnamenodeID->Namenode_ADDR = "192.168.0.33";
	printf("start datanode to namenode\n");
	DatanodeToNamenode(NULL);
	
	if(DEW_DEBUG>0)fprintf(stderr,"Datanode finished\n");
	return 0;
	}
void * DatanodeToNamenode(void * arg)
{
	int32_t sock_DtoN;
	int32_t rt = 0;
	pthread_t pthread_server;
	pthread_mutex_init(&lockFeedback,NULL);
	pthread_mutex_init(&g_memoryLock,NULL);
	//pthread_mutex_init(&g_ServerLock);
	//pthread_mutex_init(&g_FreeClientLock);
	sock_DtoN = DatanodeRegistOnNamenode();//注册namenode
	assert(sock_DtoN > 0);
	rt = pthread_create(&pthread_server,NULL,&DataToDataTaskServer,(void*)NULL);//响应连接
		assert(0 == rt);
	DatanodeControlwithNamenode(sock_DtoN);//任务信息的交流，以及时间控制
	if(DEW_DEBUG>0)fprintf(stderr,"等待DataToDataTaskServer\n");
	pthread_join(pthread_server,NULL);
	return NULL;

	}

int32_t DatanodeRegistOnNamenode(void)
//连接namenode节点，在namenode上注册本datanode,主要是将ip与连接套接字一一对应
//如果想写的更全面，应该还有本地所有数据块的id注册在namenode上，本次暂不实现该功能，工作量略大，先实现框架
{
	int32_t sock_DtoN;
	char namenodeAck[8]={};
	struct sockaddr_in cliaddr;
	socklen_t len;
	sock_DtoN = ClientConnectToServer(PnamenodeID);
	len = sizeof(struct sockaddr_in);
	getsockname(sock_DtoN,(struct sockaddr*)&cliaddr,&len);
	localIPaddress = inet_ntoa(cliaddr.sin_addr);
	printf("local ip is %s\n",localIPaddress);
	assert(sock_DtoN > 0);
	if(DataTransportRead(sock_DtoN,namenodeAck,7)!=7)
	{
		if(DEW_DEBUG ==2)
		{
			fprintf(stderr,"welcome from namenode error\n");
		}
		printf(" welcome from namenode error\n");
		close(sock_DtoN);
		return -1;
		}
	if(DEW_DEBUG >=1)fprintf(stderr,"node regist sucucess socket is %d\n",sock_DtoN);
	return sock_DtoN;//返回连接之后的套接字，方便之后与namenode通信
   // close(sock_DtoN);//所有任务完成了才能关闭
	}

int32_t DatanodeControlwithNamenode(int32_t sock_DtoN)
//依据连接套接字与namenode通信i
//返回2表示任务已接收完毕
//接收namenode分配过来的任务，并且定时反馈任务的完成度
{

	pFeedback FeedbackDToN = NULL ;
	int64_t length = sizeof(nFeedback);
	int64_t recv = 0;
	int64_t send = 0;
	int64_t task_length = 0;
	int32_t task_num = 0;
	int32_t recv_time = 0;
	struct timeval once_task_finish_time;
        int64_t once_finish = 0;
	char recvTaskBuff[DATA_NAME_MAXLENGTH];
	int rt = 0;
	pthread_t datanodeTime;
////
struct sockaddr_in cliaddr;
        socklen_t len;
        len = sizeof(struct sockaddr_in);
        getsockname(sock_DtoN,(struct sockaddr*)&cliaddr,&len);
        localIPaddress = inet_ntoa(cliaddr.sin_addr);
        printf("local ip is %s\n",localIPaddress);
////
	if(DEW_DEBUG==1)printf("inside DatanodeControlwithNamenode\n");
	assert(sock_DtoN > 0);
	FeedbackDToN = (pFeedback)malloc(sizeof(nFeedback));
	FeedbackDToN->allocatedTask = 0;
	FeedbackDToN->availableBandwidth = AVAILABLE;
	FeedbackDToN->wholeBandwidth = WHOLE;
	FeedbackDToN->finishedOrNot =1;
	FeedbackDToN->finishedTime =20;

	if(DataTransportWrite(sock_DtoN, (char*)FeedbackDToN, length) != length)
		{
			fprintf(stderr," write data to namenode error\n");
			close(sock_DtoN);
			return -1;
		}
	
	assert(sock_DtoN > 0);
	if(DEW_DEBUG==1)printf("receive time\n");
//	recv = DataTransportRead(sock_DtoN,recvTaskBuff,sizeof(struct timeval));
	recv = DataTransportRead(sock_DtoN,recvTaskBuff,sizeof(int32_t));
//	if(recv != sizeof(struct timeval))
	if(recv != sizeof(int32_t))
	{
		if(DEW_DEBUG ==2)
				{
					fprintf(stderr,"read time error\n");
				}
		printf("read time error\n");
		return -1;
	}
	if(DEW_DEBUG==1)printf("receive time success\n");
	assert(recv > 0);
	memcpy(&recv_time, recvTaskBuff, sizeof(int32_t));
	taskStarttime.tv_sec = (long int)recv_time;
	taskStarttime.tv_usec = 0;	
//	memcpy(&taskStarttime,recvTaskBuff,sizeof(struct timeval));
	//处理时间的函数ProcessTime(&taskStarttime);
	//接收到归档开始时间之后创建一个记录时间的线程，
	//定时给DatanodeToNamenode线程触发反馈归档进度的请求
	if(DEW_DEBUG ==2)fprintf(stderr,"recv starttime ,%lds,local ip %s\n",taskStarttime.tv_sec,localIPaddress);
	rt = pthread_create(&datanodeTime, NULL, &ProcessTime, (void*)(&taskStarttime));
	assert(0 == rt);
//	while (TaskRecvFinished(localIPaddress) != 1)
	while(1)
	{
		memset(recvTaskBuff,-1,sizeof(char)*DATA_NAME_MAXLENGTH);
		recv = DataTransportRead(sock_DtoN,recvTaskBuff,sizeof(int32_t));//首先接收数据长度参数
		if(DEW_DEBUG >0)fprintf(stderr,"recvTaskBuff read task num is %d,local ip %s\n",*(int32_t *)recvTaskBuff,localIPaddress);
		if(recv != sizeof(int32_t))
		{
			if(DEW_DEBUG ==2)
					{
						fprintf(stderr,"read task length error %ld\n",recv);
					}
			printf("read task length error\n");
			return -1;
		}
		memcpy(&task_num,recvTaskBuff,sizeof(int32_t));
		  //  	task_length = *(int *)recvTaskBuff;
		task_length = task_num;
		if(DEW_DEBUG == 1)fprintf(stderr,"read task num is %ld\n",task_length);
		    	//DataTransportRead(sock_DtoN,recvTaskBuff,task_length);
	//	if(task_num == 0){g_recv_end =1;break;}
		if(task_num == -1){g_recv_end =1;break;}

	assert(sock_DtoN > 0);
	fprintf(stderr,"sock %d,task_length %lld,sizeof(nTaskBlock)is %lld,local ip %s\n",sock_DtoN,task_length,task_length*sizeof(nTaskBlock),localIPaddress);
		recv = DataTransportRead(sock_DtoN,recvTaskBuff,task_length * sizeof(nTaskBlock));
	if(sock_DtoN == 0)fprintf(stderr,"sock %d,task_length %ld,local ip %s\n",sock_DtoN,task_length,localIPaddress);
//	assert(sock_DtoN > 0);
		    	//接手namenode发送过来的任务
		    		if(recv != task_length *sizeof(nTaskBlock))
		    		{
		    			if(DEW_DEBUG ==2)
		    					{
		    						fprintf(stderr,"datanode recvtask error\n");
		    					}
		    			printf("datanode recvtask error\n");
		    			close(sock_DtoN);
		    			return -1;
		    		}
		pthread_mutex_lock(&g_finished_task_lock);
		if(task_length>0)g_unfinished_task = g_unfinished_task+task_length;
		pthread_mutex_unlock(&g_finished_task_lock);
		ProcessTask(recvTaskBuff,recv);//将任务抛给任务处理模块
		//如果是最后一次任务，即空任务，此时ProcessTask函数会将TaskRecvFinished设置为1
		fprintf(stderr,"等待时间到,IP = %s\n",localIPaddress);
		while(sendFeedback == false);//时间到了发送任务反馈给namenode
		fprintf(stderr,"时间到，准备发送反馈,IP = %s\n",localIPaddress);
		FeedbackDToN->availableBandwidth = AVAILABLE;
		FeedbackDToN->wholeBandwidth = WHOLE;
		
		pthread_mutex_lock(&g_finished_task_lock);
	//	FeedbackDToN-> allocatedTask = task_length;
		FeedbackDToN-> allocatedTask = g_unfinished_task;
		FeedbackDToN->finishedTask = g_finished_task;
		g_finished_task = 0;
		pthread_mutex_unlock(&g_finished_task_lock);
		fprintf(stderr,"获得锁，发送反馈,IP = %s\n",localIPaddress);
		FeedbackDToN->finishedTime =20;
	//	if(g_finished_task >= task_length)
		assert(g_unfinished_task>=0);
		if(g_unfinished_task == 0)
		{
			FeedbackDToN->finishedOrNot =1;
		}
		else
		{
			FeedbackDToN->finishedOrNot =0;
		}
                if(DEW_DEBUG >0)fprintf(stderr,"发送feedback,local ip %s\n",localIPaddress);
		assert(sock_DtoN>0);
		send = DataTransportWrite(sock_DtoN, (char*)FeedbackDToN, length);
		if(send != length)
		{
			fprintf(stderr,"send feedback error \n");
			return -1;
		}
		pthread_mutex_lock(&lockFeedback);
		assert(sendFeedback == true);
		sendFeedback = false;
		pthread_mutex_unlock(&lockFeedback);

	}
	/*	
		pthread_mutex_lock(&g_finished_task_lock);
                g_finished_task = 0;
                pthread_mutex_unlock(&g_finished_task_lock);
	*/
		recv = 0;//最后一次一定是0,在前面的while（1）里面跳出的
                ProcessTask(recvTaskBuff,recv);//将任务抛给任务处理模块
                //如果是最后一次任务，即空任务，此时ProcessTask函数会将TaskRecvFinished设置为1
		fprintf(stderr,"节点等待所有任务完成，最后一个feedback\n");
		fprintf(stderr,"等待时间到,IP = %s\n",localIPaddress);
                while(sendFeedback == false);//时间到了发送任务反馈给namenode
                FeedbackDToN->availableBandwidth = AVAILABLE;
                FeedbackDToN->wholeBandwidth = WHOLE;
		while(g_unfinished_task!=0);
		fprintf(stderr,"节点所有任务完成\n");
		pthread_mutex_lock(&g_finished_task_lock);
                FeedbackDToN-> allocatedTask = task_length;
                FeedbackDToN->finishedTask = g_finished_task;
		pthread_mutex_unlock(&g_finished_task_lock);
		FeedbackDToN->finishedTime =20;
                //if(g_finished_task >= task_length)
		if(g_unfinished_task == 0)
                {
                        FeedbackDToN->finishedOrNot =1;
                }
                else
                {
                        FeedbackDToN->finishedOrNot =0;
                }
                send = DataTransportWrite(sock_DtoN, (char*)FeedbackDToN, length);
                if(send != length)
                {
                        printf("send feedback error \n");
                        return -1;
                }
                pthread_mutex_lock(&lockFeedback);
                assert(sendFeedback == true);
                sendFeedback = false;
                pthread_mutex_unlock(&lockFeedback);

       if(DEW_DEBUG >0)fprintf(stderr,"to namenode over\n");
	ALL_TASK_FINISHED = 1;
	return 2;
	}

int32_t TaskRecvFinished(char * localIPaddress)
{
	//任务接收完成，与否，每个数据节点的参数是本地的iｐ地址，因为iｐ可以标识不同的节点
	//是 返回1 ，否返回0, 可以有namenode 结束任务发送时发送标志过来
	//考虑到namenode最后一次任务发送给datanode之后，任务还需要处理，将结束标志设为，datanode完成最后一次任务之后，
	//namenode仍然发生一次空任务，标志该节点的归档完全结束，故localIPaddress 参数可取消，
	//不加这最后一次ProcessTime里面的while循环的结束条件就会有问题
	assert(localIPaddress != NULL);
	if(g_recv_end ==1) return 1;
	return 0;
	}

void * ProcessTime(void * taskTime)
//从归档任务开始，每隔oneTasktime.tv_sec时间，触发本地节点发送feedback给namenode
{
	struct timeval tmpTaskStarttime;//namenode发送过来的归档开始时间
	struct timeval timeNow;//当前的时间
	struct timeval archiveAlreadyTime;//归档任务第一次产生到datanode接收到任务开始处理的时间
	struct timeval sleepTime;
	memcpy(&tmpTaskStarttime, (struct timeval*)taskTime, sizeof(struct timeval));
	gettimeofday(&timeNow,NULL);
	timersub(&timeNow, &tmpTaskStarttime, &archiveAlreadyTime);
//此处若是接收到taskTime时时间已过去很久，就会出问题，sleepTime会为负。接收的taskTime出错也会出问题
	timersub(&oneTasktime, &archiveAlreadyTime, &sleepTime);
	if(DEW_DEBUG ==2)fprintf(stderr,"recvTime %ld,timeNow %ld\n",tmpTaskStarttime.tv_sec,timeNow.tv_sec);
	assert((sleepTime.tv_sec >= 0));
	assert((archiveAlreadyTime.tv_sec <= 10) );
	sleep(sleepTime.tv_sec);
	pthread_mutex_lock(&lockFeedback);
	sendFeedback = true;
	pthread_mutex_unlock(&lockFeedback);
	//已经回复了一个feedback
	while(TaskRecvFinished(localIPaddress) != 1)
	{
		sleep(oneTasktime.tv_sec);
		while(true == sendFeedback);//当时为什么会用while
	
	//	assert(sendFeedback == false);//时间到了不一定就可以发送了，状态位sendFeedback 可能还是true,因为发送过程太慢了？
		if(true != sendFeedback)
		{
			pthread_mutex_lock(&lockFeedback);
			sendFeedback = true;
			pthread_mutex_unlock(&lockFeedback);
		}
	}
	if(DEW_DEBUG > 1)fprintf(stderr,"time finished\n");
	return NULL;
	}

void ProcessTask(char *recvTaskBuff,int64_t recv)
//pthread_task_num线程返回表示该条带的任务完成（其实是编码完成，但是也几乎等于完成了，因为每编码一个块就会立即发送）
//之前直接使用recvTaskBuff,因为recvTaskBuff是Datanode接收任务的，所以下一次接收任务时就会被覆盖
{
	int32_t TaskNum = 0;
	int32_t i = 0;
	pthread_t * pthread_task_num = NULL;
	char *local_recvTaskBuff = NULL;
	int32_t rt = 0;
	pTaskBlock pChunkTask = NULL;
	assert(recv % sizeof(nTaskBlock) == 0);
	TaskNum = recv / sizeof(nTaskBlock);
	if(TaskNum == 0)
	{
	//	处理没有任务的情况
       if(DEW_DEBUG >0)fprintf(stderr,"本次接受的任务长度为0\n");
	return;
	}
	assert(TaskNum>0);
	local_recvTaskBuff = (char *)malloc(TaskNum*sizeof(nTaskBlock));
	assert(local_recvTaskBuff != NULL);
	memcpy(local_recvTaskBuff,recvTaskBuff,TaskNum*sizeof(nTaskBlock));
	if(run_plan == PLANA)//PLANA的意思是采用并行的方式发送任务中的每一个数据块，并发太高性能会比较差
	{
		pthread_task_num = (pthread_t *)malloc(TaskNum * sizeof(pthread_t));
		assert(pthread_task_num != NULL);

		for(i = 0; i< TaskNum; i++)
		{//此处应该是并行创建处理单个条带任务的线程

			pChunkTask = (pTaskBlock)(local_recvTaskBuff + i*sizeof(nTaskBlock));
			rt = pthread_create(pthread_task_num, NULL, &ProcessChunkTask, (void*)pChunkTask);
			assert(0 == rt);
			pthread_task_num ++;
		}
	}
	else if(run_plan == PLANB)
	{
		
		pthread_task_num = (pthread_t *)malloc(TaskNum * sizeof(pthread_t));
                assert(pthread_task_num != NULL);
                           
                for(i = 0; i< TaskNum; i++)
                {//此处应该是并行创建处理单个条带任务的线程
                           
                        pChunkTask = (pTaskBlock)(local_recvTaskBuff + i*sizeof(nTaskBlock));
                        rt = pthread_create(pthread_task_num, NULL, &ProcessChunkTask, (void*)pChunkTask);
                        assert(0 == rt);
                        pthread_task_num ++;
                }    
	}
	else 
	{
		fprintf(stderr,"has no plan for the chunk\n");
	}
       if(DEW_DEBUG >0)fprintf(stderr,"完成接受自NameNode的一次任务\n");

}

