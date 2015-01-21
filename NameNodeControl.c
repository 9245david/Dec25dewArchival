/*
 * NameNodeControl.c
 *
 *  Created on: Dec 29, 2014
 *      Author: hadoop
 *
 *     typedef struct RegistAndTaskFeedback结构体在DatanodeToNamenode中也有定义，
 *     现在是假设NameNodeControl.c 和DatanodeToNamenode.c是分开两个工程执行的。
 */

/*控制信息包括，接受来自datanode的状态信息，以便生成权值
  接收任务完成度
  分发发送任务
*/

//#include "aa.h"
#include "Name_Node_Control.h"


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
//extern typedef struct RegistAndTaskFeedback;
//extern typedef nFeedback;

/*
char g_nodeIP[RACK_NODE*RACK_NUM][IP_LENGTH];//节点号与节点ip
int g_nodeConnfd[RACK_NODE*RACK_NUM];
int g_nodeRegist = 0;//已经注册的节点个数，注册过程中加写锁
nFeedback g_nodeFeedback[RACK_NODE*RACK_NUM];//节点号与节点反馈信息
*/
char g_nodeIP[DATANODE_NUMBER][IP_LENGTH];//节点号与节点ip
int g_nodeConnfd[DATANODE_NUMBER];
int g_nodeRegist = 0;//已经注册的节点个数，注册过程中不需要加写锁，因为是while循环串行注册
nFeedback g_nodeFeedback[DATANODE_NUMBER];//节点号与节点反馈信息
int g_feedbackVersion[DATANODE_NUMBER] = {0};
bool ALL_DATANODE_CONNECTED = false;
struct timeval starttime;
int NamenodeControlServer()
{
	int listenfd;
	int on,ret;
//	socklen_t len;
	struct sockaddr_in servaddr;
	listenfd = socket(AF_INET,SOCK_STREAM,0);
	on = 1;
	ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(NAME_PORT);
    /*函数bind将服务器的地址和套接字绑定在一起*/
    	if(bind(listenfd,(struct sockaddr *)&servaddr,sizeof(struct sockaddr))==-1)
    	{
    		perror("bind error");
    		exit(-1);
    	}
    /*listen 函数告诉内核，这个套接字可以接受来自客户端的请求,BACKLOG是请求个数*/
    	if(listen(listenfd,BACKLOG)==-1)
    	{
    		perror("listen error\n");
    		exit(-1);
    	}
    	handle_connect(listenfd);
        return 1;
	}

void *handle_request(void * arg)
//接受数据，根据不同类型的数据抛至对应的处理程序，每一个datanode对应一个这样的线程
{
	int connfd ;
	char  recvbuff[DATA_NAME_MAXLENGTH];
	char sendbuff[DATA_NAME_MAXLENGTH];
	long recv,send;
	long task_length = 0;
	connfd = *((int*)arg);
	free(arg);
	if(DataTransportWrite(connfd,"welcome",7)!=7)
	{
		printf(" welcome to datanode error\n");
		close(connfd);
		return NULL;
	}
	//固定时间间隔，接受datanode 状态信息，包括任务完成度，
//	第一次接受状态信息时，因为没有任务所以单次任务完成度为1.
	recv = DataTransportRead(connfd,recvbuff,sizeof(nFeedback));
	if(recv<0)
	{
		printf("namenode recvdata error\n");
		close(connfd);
		return NULL;
	}
	//此处需要设计不同类型的接受数据
	ProcessDatanodeState(recvbuff, recv, connfd);

	while(ALL_DATANODE_CONNECTED == false)NULL;//所有连接建立
	DataTransportWrite(connfd,&starttime,sizeof(struct timeval));// 统一发送归档开始时间


    while(TaskSendFinished(connfd) != 1)//所有的归档任务没有派送完成
    {
    	 // 发送任务给各个节点
    	SendTaskToDatanode(connfd);//此处有一个DataTransportWrite
    	recv = DataTransportRead(connfd,recvbuff,sizeof(nFeedback));//首先接收数据长度参数

    	//接收各个节点的归档进度反馈
    	//此处接受数据的时间控制暂时有datanode控制，假设控制过程为，接收管理节点的starttime  之后有一个监控时间的线程
    	//当约定的时间点到达之后发送反馈信息，这样貌似
    		if(recv<0)
    		{
    			printf("namenode recvdata error\n");
    			close(connfd);
    			return NULL;
    		}
    	WriteTaskFeedbackLog(connfd,recvbuff,sizeof(nFeedback));//将datanode反馈的信息写入日志中
    	ProcessDatanodeState(recvbuff, recv, connfd);//将反馈信息提交给归档管理器

    }
	return NULL;
	}


int handle_connect(int listen_sock)//返回0正常，返回其他值，失败
{
		int * Pconnfd;
		socklen_t len;
		struct sockaddr_in cliaddr;
		pthread_t  pthread_do;
		int rt;
		len = sizeof(cliaddr);
		int nodenum = DATANODE_NUMBER;
		while(nodenum--)//等待所有datanode连接
		{
			Pconnfd = (int*)malloc(sizeof(int));
			*Pconnfd = accept(listen_sock,(struct sockaddr*)&cliaddr,&len);
			if (-1 != *Pconnfd)
				printf("server: got connection from client %s \n",inet_ntoa(cliaddr.sin_addr));
			//注册,连接套接字*Pconnfd与节点号以及节点iｐ的对应关系，在这之前必须保证已经有iｐ和节点号的对应关系
			g_nodeRegist ++;
			NodeRegist(*Pconnfd,inet_ntoa(cliaddr.sin_addr),g_nodeRegist);
			rt = pthread_create(&pthread_do,NULL,&handle_request,(void*)Pconnfd);
			if(0 != rt)
					{
					printf("pthread_create error\n");
					return rt;
				         }
		}
		ALL_DATANODE_CONNECTED = true;//所有连接已经建立
//		close(listen_sock);
		gettimeofday(&starttime,NULL);
		return 0;
	}
void NodeRegist(int nodeConnfd,char *nodeIP,int nodeNum)
/*
 * 节点号与套接字对应关系，节点号与节点IP对应关系，节点号与feedback版本对应关系
 * 参数nodeConnfd 为datanode节点连接套接字
 * 参数nodeIP 为datanode的ip，由inet_ntoa(cliaddr.sin_addr)产生的字符串
 * 参数nodeNum 为节点注册时的顺序，排在第几位连接的，先来计数，从1开始计数，由全局变量g_nodeRegist传入
 */
{
	g_nodeConnfd[nodeNum - 1] = nodeConnfd;
	memcpy(g_nodeIP[nodeNum - 1], nodeIP, strlen(nodeIP));
	g_feedbackVersion[nodeNum - 1] = 1;

	}
void ProcessDatanodeState(char * buff, long length, int connfd)
/*
 * 将状态信息以及任务完成度提交给归档管理模块，用与生成所有节点的任务
 * buff为Namenode接收的Datanode的反馈信息
 * long 为反馈信息buff的长度
 * connfd 为datanode节点连接套接字
 */
{
	int i = 0;
	for(i = 0; i < DATANODE_NUMBER; i++)
	{
		if(g_nodeConnfd[i] == connfd)break;
	}
	assert(i < DATANODE_NUMBER);
	memcpy(&(g_nodeFeedback[i]), buff, length);
	g_feedbackVersion[i]++ ;//所有feedback 的版本必须一致
	VersionUpdated();

	return ;
	}
void SendTaskToDatanode(int connfd)
//所有节点的任务信息生成之后，每个节点的任务（没有任务的接受空任务）,每个节点连接对应一个节点
/*
 * 	此处留有疑问，可不可以设计为只发送任务给有任务的节点，反馈信息也是只接收对应节点的反馈信息，
 *	这样对于feedback的同步g_feedbackVersion的判断机制也需要重新设计
 */
{

	return ;
	}

int TaskSendFinished(int connfd)
//检查connfd对应的datanode的所有归档任务是否完成，如果完成，就返回1;否则返回0
{
	return 0;

	}
void WriteTaskFeedbackLog(int connfd,char *recvbuff,unsigned long length)
{

	return ;
	}
bool VersionUpdated()
{
	int i = 0;
	int j = 0;
	j = g_nodeRegist[0];
	for(i = 1; i < DATANODE_NUMBER; i++)
	{
		if(j != g_nodeRegist[i])return false;
	}
	return true;

	}
