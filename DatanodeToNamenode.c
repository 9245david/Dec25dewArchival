#include "Socket_connect_read_write.h"
#include "DatanodeToNamenode.h"
#include "BlockStruct.h"
#include <assert.h>
// oneTasktime 没有设置，FeedbackDToN完全没有填充任何内容
extern PNamenodeID  PnamenodeID;
extern void *ProcessChunkTask(void *argv);
extern void *DataToDataTaskServer(void *arg);
extern pthread_mutex_t g_memoryLock;
//extern char * DtoNbuffer ;
//extern long  DtoNlength ;
struct timeval oneTasktime;
char * localIPaddress;
bool sendFeedback = false; //全局变量
pthread_mutex_t lockFeedback;
struct timeval taskStarttime;
typedef struct RegistAndTaskFeedback{
//	 信息1：发送本地剩余带宽，剩余空间大小
//	 信息2：反馈分配的任务完成度
	double wholeBandwidth;//MB/s，总带宽
	double availableBandwidth;//可用带宽
	double wholeStorageSpace;//MB
	double availableStorageSpace;
	int finishedOrNot ;//如果为1表示分配的任务已经完成，为0表示没有完成，只有当第一次注册时默认为1不是这个含义
	int allocatedTask ;//被分配的任务数量，即数据块的个数
	int finishedTask ; //在预想时间内完成的数据块的个数
	int finishedTime ; //如果完成了，提前完成所花费的时间
}nFeedback,*pFeedback;

int g_finished_task = 0;
pthread_mutex_t g_finished_task_lock;
int main(int argc,char** argv)
{
	pthread_mutex_init(&g_finished_task_lock,NULL);
	PnamenodeID = (PNamenodeID)malloc(sizeof(NamenodeID));
	assert(PnamenodeID != NULL);
	PnamenodeID->NameToDataPort = DATA_TO_NAME_PORT;
	PnamenodeID->Namenode_ADDR = "192.168.0.33";
	printf("start datanode to namenode\n");
	DatanodeToNamenode(NULL);
	return 0;
	}
void * DatanodeToNamenode(void * arg)
{
	int sock_DtoN;
	int rt = 0;
	pthread_t pthread_server;
	pthread_mutex_init(&lockFeedback,NULL);
	pthread_mutex_init(&g_memoryLock,NULL);
	//pthread_mutex_init(&g_ServerLock);
	//pthread_mutex_init(&g_FreeClientLock);
	sock_DtoN = DatanodeRegistOnNamenode();//注册namenode
	assert(sock_DtoN > -1);
	rt = pthread_create(&pthread_server,NULL,&DataToDataTaskServer,(void*)NULL);//响应连接
		assert(0 == rt);
	DatanodeControlwithNamenode(sock_DtoN);//任务信息的交流，以及时间控制
	pthread_join(pthread_server,NULL);
	return NULL;

	}

int DatanodeRegistOnNamenode(void)
//连接namenode节点，在namenode上注册本datanode,主要是将ip与连接套接字一一对应
//如果想写的更全面，应该还有本地所有数据块的id注册在namenode上，本次暂不实现该功能，工作量略大，先实现框架
{
	int sock_DtoN;
	char namenodeAck[8]={};
	struct sockaddr_in cliaddr;
	socklen_t len;
	sock_DtoN = ClientConnectToServer(PnamenodeID);
	len = sizeof(struct sockaddr_in);
	getsockname(sock_DtoN,(struct sockaddr*)&cliaddr,&len);
	localIPaddress = inet_ntoa(cliaddr.sin_addr);
	printf("local ip is %s\n",localIPaddress);
	assert(sock_DtoN > -1);
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
	if(DEW_DEBUG ==1)printf("node regist sucucess\n");
	return sock_DtoN;//返回连接之后的套接字，方便之后与namenode通信
   // close(sock_DtoN);//所有任务完成了才能关闭
	}

int DatanodeControlwithNamenode(int sock_DtoN)
//依据连接套接字与namenode通信
//接收namenode分配过来的任务，并且定时反馈任务的完成度
{

	pFeedback FeedbackDToN = NULL ;
	long length = sizeof(nFeedback);
	long recv = 0;
	long send = 0;
	long task_length = 0;
	char recvTaskBuff[DATA_NAME_MAXLENGTH];
	int rt = 0;
	pthread_t datanodeTime;
	if(DEW_DEBUG==1)printf("inside DatanodeControlwithNamenode\n");
	assert(sock_DtoN > 0);
	FeedbackDToN = (pFeedback)malloc(sizeof(nFeedback));
	FeedbackDToN->allocatedTask = 0;
	FeedbackDToN->availableBandwidth = 64;
	FeedbackDToN->wholeBandwidth = 100;
	FeedbackDToN->finishedOrNot =1;

	if(DataTransportWrite(sock_DtoN, (char*)FeedbackDToN, length) != length)
		{
			printf(" write data to namenode error\n");
			close(sock_DtoN);
			return -1;
		}
	
	if(DEW_DEBUG==1)printf("receive time\n");
	recv = DataTransportRead(sock_DtoN,recvTaskBuff,sizeof(struct timeval));
	if(recv != sizeof(struct timeval))
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
	memcpy(&taskStarttime,recvTaskBuff,sizeof(struct timeval));
	//处理时间的函数ProcessTime(&taskStarttime);
	//接收到归档开始时间之后创建一个记录时间的线程，
	//定时给DatanodeToNamenode线程触发反馈归档进度的请求
	rt = pthread_create(&datanodeTime, NULL, &ProcessTime, (void*)(&taskStarttime));
	assert(0 == rt);
	while (TaskRecvFinished(localIPaddress) != 1)
	{
		recv = DataTransportRead(sock_DtoN,recvTaskBuff,sizeof(int));//首先接收数据长度参数
		if(recv != sizeof(int))
		{
			if(DEW_DEBUG ==2)
					{
						fprintf(stderr,"read task length error\n");
					}
			printf("read task length error\n");
			return -1;
		}
		    	task_length = *(int *)recvTaskBuff;
		    	//DataTransportRead(sock_DtoN,recvTaskBuff,task_length);
		recv = DataTransportRead(sock_DtoN,recvTaskBuff,task_length * sizeof(nTaskBlock));
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
		g_finished_task = 0;
		pthread_mutex_unlock(&g_finished_task_lock);
		ProcessTask(recvTaskBuff,recv);//将任务抛给任务处理模块
		//如果是最后一次任务，即空任务，此时ProcessTask函数会将TaskRecvFinished设置为1
		while(sendFeedback == false);//时间到了发送任务反馈给namenode
		FeedbackDToN-> allocatedTask = task_length;
		FeedbackDToN->availableBandwidth = 64;
		FeedbackDToN->wholeBandwidth = 100;
		FeedbackDToN->finishedTask = g_finished_task;
		if(g_finished_task >= task_length)
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

	}

	return 1;
	}

int TaskRecvFinished(char * localIPaddress)
{
	//任务接收完成，与否，每个数据节点的参数是本地的iｐ地址，因为iｐ可以标识不同的节点
	//是 返回1 ，否返回0, 可以有namenode 结束任务发送时发送标志过来
	//考虑到namenode最后一次任务发送给datanode之后，任务还需要处理，将结束标志设为，datanode完成最后一次任务之后，
	//namenode仍然发生一次空任务，标志该节点的归档完全结束，故localIPaddress 参数可取消，
	//不加这最后一次ProcessTime里面的while循环的结束条件就会有问题
	assert(localIPaddress != NULL);
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
	timersub(&oneTasktime, &archiveAlreadyTime, &sleepTime);
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

	return NULL;
	}

void ProcessTask(char *recvTaskBuff,long recv)
//pthread_task_num线程返回表示该条带的任务完成（其实是编码完成，但是也几乎等于完成了，因为每编码一个块就会立即发送）
{
	int TaskNum = 0;
	int i = 0;
	pthread_t * pthread_task_num = NULL;

	int rt = 0;
	pTaskBlock pChunkTask = NULL;
	assert(recv % sizeof(nTaskBlock) == 0);
	TaskNum = recv / sizeof(nTaskBlock);
	if(TaskNum == 0)
	{
	//	处理没有任务的情况
	return;
	}
	pthread_task_num = (pthread_t *)malloc(TaskNum * sizeof(pthread_t));
	assert(pthread_task_num != NULL);

	for(i = 0; i< TaskNum; i++)
	{//此处应该是并行创建处理单个条带任务的线程

		pChunkTask = (pTaskBlock)(recvTaskBuff + i*sizeof(nTaskBlock));
		rt = pthread_create(pthread_task_num, NULL, &ProcessChunkTask, (void*)pChunkTask);
		assert(0 == rt);
		pthread_task_num ++;
	}

}

