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
#include "semtool.h"
FILE* logFile = NULL;//打开一个已经存在的文件
pthread_mutex_t logFileLock;
key_t key_task ;
int main(int argc,char**argv)
{
	int i;
	pthread_t pthread_provide_task;
	int rt ;
	unsigned short sem_init_array[DATANODE_NUMBER] ={0};

	int semid;
	 key_task = ftok(".", 1);
	semid = sem_create(key_task,DATANODE_NUMBER);
	sem_setall(semid,sem_init_array);
	init_cluster();
	Print_cluster_lay();
	rt = pthread_create(&pthread_provide_task,NULL,&ProvideTask,(void*)NULL);
	if(DEW_DEBUG ==1)printf("创建了生产任务进程\n");
	NamenodeControlServer();

	assert(rt==0);
	for(i=0;i<DATANODE_NUMBER;i++)
			{
				pthread_join(*(pthread_node_num+i),NULL);
			}
	pthread_join(pthread_provide_task,NULL);
	fclose(logFile);
	sem_d(semid);
}
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
	struct sockaddr_in cliaddr;
	socklen_t len;
	int node_num;
	int semid;
	connfd = *((int*)arg);
	free(arg);
	node_num = GetNodeIDFromConnfd(connfd);
	semid = sem_openid(key_task);
	if(DataTransportWrite(connfd,"welcome",7)!=7)
	{
		printf(" welcome to datanode error\n");
		close(connfd);
		return NULL;
	}
	if(DEW_DEBUG==1)printf("send welcome to datanode\n");
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

//	if(DEW_DEBUG==1)printf("ALL_DATANODE_CONNECTED\n");
	DataTransportWrite(connfd,(char*)&starttime,sizeof(struct timeval));// 统一发送归档开始时间
	//
	len = sizeof(struct sockaddr_in);
	getpeername(connfd,(struct sockaddr*)&cliaddr,&len);
	if(DEW_DEBUG ==1)printf("send starttime to %s ,%lds \n",inet_ntoa(cliaddr.sin_addr),starttime.tv_sec);
//	int semid;
    while(TaskSendFinished(connfd) != 1)//所有的归档任务没有派送完
    {
    	 // 发送任务给各个节点
    	sem_p(semid,node_num);
    	SendTaskToDatanode(connfd);//此处有一个DataTransportWrite
    	recv = DataTransportRead(connfd,recvbuff,sizeof(nFeedback));//首先接收数据长度参数

    	//接收各个节点的归档进度反馈
    	//此处接受数据的时间控制暂时有datanode控制，假设控制过程为，接收管理节点的starttime  之后有一个监控时间的线程
    	//当约定的时间点到达之后发送反馈信息，这样貌似
    		if(recv<0)
    		{
    			printf("namenode nFeedback error\n");
    			close(connfd);
    			return NULL;
    		}
	if(DEW_DEBUG ==1)printf("recv nFeedback from %d",GetNodeIDFromConnfd(connfd));
    	WriteTaskFeedbackLog(connfd,recvbuff,sizeof(nFeedback));//将datanode反馈的信息写入日志中
    	ProcessDatanodeState(recvbuff, recv, connfd);//将反馈信息提交给归档管理器

    }

	if(DEW_DEBUG ==1)printf("get out of handle_request\n");
	return NULL;
	}


int handle_connect(int listen_sock)//返回0正常，返回其他值，失败
{
		int * Pconnfd;
		socklen_t len;
		int i;
		struct sockaddr_in cliaddr;
		//pthread_t  *pthread_node_num = NULL;
		int rt;
		len = sizeof(cliaddr);
		int nodenum = DATANODE_NUMBER;
		logFile = fopen("TaskFeedbackLog.log","w");
		assert(logFile!=NULL);
		pthread_mutex_init(&logFileLock,NULL);
		pthread_node_num = (pthread_t*)malloc(nodenum*sizeof(pthread_t));
		assert(pthread_node_num !=NULL);
		if(DEW_DEBUG ==1)printf("等待节点注册\n");
		while((nodenum)>0)//等待所有datanode连接
		{
			Pconnfd = (int*)malloc(sizeof(int));
			*Pconnfd = accept(listen_sock,(struct sockaddr*)&cliaddr,&len);
			if (-1 != *Pconnfd)
				printf("server: got connection from client %s \n",inet_ntoa(cliaddr.sin_addr));
			//注册,连接套接字*Pconnfd与节点号以及节点iｐ的对应关系，在这之前必须保证已经有iｐ和节点号的对应关系(不用，NodeRegist完成该功能)
			g_nodeRegist ++;
			NodeRegist(*Pconnfd,inet_ntoa(cliaddr.sin_addr),g_nodeRegist);
			rt = pthread_create(pthread_node_num+(DATANODE_NUMBER-nodenum),NULL,&handle_request,(void*)Pconnfd);

			if(0 != rt)
					{
					printf("pthread_create error\n");
					return rt;
				         }
			nodenum--;
		}
		ALL_DATANODE_CONNECTED = true;//所有连接已经建立
//		close(listen_sock);
		gettimeofday(&starttime,NULL);

	if(DEW_DEBUG==1)printf("all the node have regist, namenode set the starttime\n");
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
	if(DEW_DEBUG == 1)printf("节点号%d,IP %s 注册\n",nodeNum-1,nodeIP);

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
	//VersionUpdated();

	return ;
	}
void SendTaskToDatanode(int connfd)
//所有节点的任务信息生成之后，每个节点的任务（没有任务的接受空任务）,每个节点连接对应一个节点
/*
 *
 * 需要ArchivalManager.c将任务已经生成才可以 char *DatanodeTask[DATANODE_NUMBER];
 * 	此处留有疑问，可不可以设计为只发送任务给有任务的节点，反馈信息也是只接收对应节点的反馈信息，
 *	这样对于feedback的同步g_feedbackVersion的判断机制也需要重新设计
 */
{
	int taskLength = 0;
	int nodeID = 0;
	char  * buff_datanode_task = NULL;
	long send;
	nodeID = GetNodeIDFromConnfd(connfd);
	assert(nodeID >= 0 && nodeID < DATANODE_NUMBER);
	if(DEW_DEBUG==1)printf("inside SendTaskToDatanode %d\n",nodeID);
	//taskLength = *(long *)(DatanodeTask[nodeID]);
	taskLength =(g_pDatanodeTask[nodeID].taskNum)*sizeof(nTaskBlock);
	//taskLength += sizeof(long);
	buff_datanode_task = (char*)malloc(taskLength*sizeof(char));
	assert(buff_datanode_task != NULL);
	memcpy(buff_datanode_task,&(g_pDatanodeTask[nodeID].taskNum),g_pDatanodeTask[nodeID].taskNum);
	memcpy(buff_datanode_task+sizeof(int),g_pDatanodeTask[nodeID].singleStripTask,taskLength);
	send = DataTransportWrite(connfd,buff_datanode_task, sizeof(int)+taskLength);
	if(send<0)
        {
                printf("send task to datanode error\n");
                close(connfd);
                return ;
        }
	if(DEW_DEBUG==1)printf("outside SendTaskToDatanode\n");
	return ;
	}

int TaskSendFinished(int connfd)
//检查connfd对应的datanode的所有归档任务是否完成，如果完成，就返回1;否则返回0
{
	if(g_TaskStartBlockNum >= 6)return 1;
//	if(g_TaskStartBlockNum >= 996)return 1;
		else return 0;
	//return 0;

	}
void WriteTaskFeedbackLog(int connfd,char *recvbuff,unsigned long length)
{
	pFeedback FeedbackDToN = (pFeedback)recvbuff;
	//int logFd = open("TaskFeedbackLog.log",O_WRONLY|O_CREAT,0666);//使用O_CREAT时必须用参数mode = 0666
	//本来想用·pwrite这样就不用加互斥锁，但是写入结构化的数据，在文件读取时只能用函数写太麻烦了，就用了文件读写函数没有用系统调用
	
	if(DEW_DEBUG==1)printf("inside WriteTaskFeedbackLog \n");
	pthread_mutex_lock(&logFileLock);
	fprintf(logFile,"%lf,%lf,%lf,%lf",FeedbackDToN->wholeBandwidth,FeedbackDToN->availableBandwidth,FeedbackDToN->wholeStorageSpace,FeedbackDToN->availableStorageSpace);
	fprintf(logFile,"%u,%u,%u,%u\n",FeedbackDToN->finishedOrNot,FeedbackDToN->allocatedTask,FeedbackDToN->finishedTask,FeedbackDToN->finishedTime);
	pthread_mutex_unlock(&logFileLock);
	/*
	uint64_t wholeBandwidth;//B/s，总带宽
	uint64_t availableBandwidth;//可用带宽
	uint64_t wholeStorageSpace;//B
	uint64_t availableStorageSpace;
	unsigned int finishedOrNot ;//如果为1表示分配的任务已经完成，为0表示没有完成，只有当第一次注册时默认为1不是这个含义
	unsigned int allocatedTask ;//被分配的任务数量，即数据块的个数
	unsigned int finishedTask ; //在预想时间内完成的数据块的个数
	unsigned int finishedTime ; //如果完成了，提前完成所花费的时间
	*/
	return ;
	}


int GetNodeIDFromConnfd(int connfd)
//根据连接套接字得到节点号0~17
{
	int i = 0;

//	if(DEW_DEBUG==1)printf("inside GetNodeIDFromConnfd \n");
	for(i=0; i< DATANODE_NUMBER; i++)
	{
		if (g_nodeConnfd[i] == connfd)return i;
	}
	assert(i == DATANODE_NUMBER);
	return -1;

	}

void *ProvideTask(void *arg)
//利用反馈得到的g_nodeFeedback[DATANODE_NUMBER]信息生成每个节点的任务列表
//每一个datanode有一个人任务列表，列表是一长串字符串，字符串的第一个long表示该任务列表的长度，是sizeof(nTaskBlock)的整数倍
//
{
	int nodeNum = DATANODE_NUMBER;
	int i=0;
	int semid = sem_openid(key_task);
	g_pDatanodeTask = (pTaskHead)malloc(sizeof(nTaskHead)*DATANODE_NUMBER);
	assert(g_pDatanodeTask != NULL);
	g_TaskStartBlockNum = 0;//初始化块号
	if(DEW_DEBUG ==1)printf("start ProvideTask\n");
	while(ProvideTaskFinished()!= 1)
	{
		while(VersionUpdated() == false);
		if(DEW_DEBUG ==1)printf("Version updated in ProvideTask\n");
		if((g_versionNum-1)==1)//第一次接收到反馈，即所有节点初始化过程
		{
			for(i=0;i<nodeNum;i++)
			{
				g_pDatanodeTask[i].taskNum = 0;
				g_weight[i] = g_nodeFeedback[i].availableBandwidth * TASK_TIME / (BLOCK_SIZE/1024/1024);
				g_pDatanodeTask[i].historyNotFinished = false;//默认正常任务完成情况
				g_pDatanodeTask[i].singleStripTask = NULL;
			}
		}
		else{//之后的反馈过程
			for(i=0;i<nodeNum;i++)
			{
				g_pDatanodeTask[i].taskNum = 0;
				g_pDatanodeTask[i].singleStripTask = NULL;
				g_pDatanodeTask[i].historyNotFinished = false;//默认正常任务完成情况
				if(g_nodeFeedback[i].allocatedTask == 0)//上一个回合并没有分配任务
				{
					g_weight[i] = g_nodeFeedback[i].availableBandwidth * TASK_TIME / (BLOCK_SIZE/1024/1024);

				}
				else if(g_nodeFeedback[i].finishedOrNot == 1)//任务已经完成，依据完成时间的比例得到下次发送的任务个数上限
				{
					assert(g_nodeFeedback[i].finishedTime >0);
					g_weight[i] =	g_nodeFeedback[i].allocatedTask * TASK_TIME / g_nodeFeedback[i].finishedTime;
				}
				else//上一回合分配了任务，但是任务没有完成，只能以上次已经完成的块数作为本次的能力，能力减去为完成的工作
				{
					g_weight[i] = g_nodeFeedback[i].finishedTask*2 - g_nodeFeedback[i].allocatedTask;
					if(g_weight[i]<0)
						{
							g_weight[i]=0;
							g_pDatanodeTask[i].historyNotFinished = true;
						}
				}
			}//for
			}//else
	//	ProvideTaskAlgorithm(g_weight,g_TaskStartBlockNum,g_pDatanodeTask);
		//依据权重值，本次分配的起始块号，得到g_pDatanodeTask中存储的每个节点的任务情况
	//	g_TaskStartBlockNum = g_TaskStartBlockNum + EREASURE_N - EREASURE_K;//增加块号+6
		if(DEW_DEBUG==1)
		{
			for(i=0;i<nodeNum;i++)
			printf("g_weight[%d] = %d\n",i,g_weight[i]);
		}
		ProvideTaskAlgorithm(g_weight,g_pDatanodeTask);
		//依据权重值，本次分配的起始块号，得到g_pDatanodeTask中存储的每个节点的任务情况
		for(i = 0; i<DATANODE_NUMBER; i++)
		{
			sem_v(semid,i);
		}

	}//while
	if(DEW_DEBUG==1)printf("go of out ProvideTask\n");

	return NULL;
}

bool VersionUpdated()
{
	int i = 0;
	int j = 0;
//	j = g_versionNum;
	 j = g_versionNum + 1;//由于g_feedbackVersion 在连接时+1，在第一次接受feedback时+1 ，所以此处需要+1
//	if(DEW_DEBUG==1)printf("inside VersionUpdated \n");
	for(i = 1; i < DATANODE_NUMBER; i++)
	{
		if(j != g_feedbackVersion[i])return false;
	}
	g_versionNum++;
	return true;
	}
int ProvideTaskFinished()//生成任务结束,等于1结束，等于0未结束
{
	if(g_TaskStartBlockNum >= 6)return 1;
//	if(g_TaskStartBlockNum >= 996)return 1;
	else return 0;

	}

void ProvideTaskAlgorithm(int * g_weight,pTaskHead g_pDatanodeTask)
//依据起始块号和权重值信息，得到任务信息，终止条件为权值不能满足下一个条带的任务分配
{
	list_head * strp_lay_head = NULL ;

	int task[DATANODE_NUMBER][EREASURE_N - EREASURE_K+2];//与每个节点一一对应,0号位置存储节点号，1号位置存储节点数据块个数
	int node_num = 0;//参与任务的节点个数
	int block_num = 0;//单个节点上数据块的个数
	int node_ID = 0;//节点号
	int local_node_ID = 0;//本地节点号
	int i,j,k=0;
	list_head * weight_strp_lay = NULL;
	list_head * p_temp_node = NULL;
	list_head * p_temp_blk_head = NULL;//子链表头节点
	list_head * p_temp_blk = NULL;//子链表临时节点
	pTaskBlock p_temp_task_block = NULL;
	strp_lay_head = get_strp_lay(g_TaskStartBlockNum);

	if(DEW_DEBUG==1)printf("inside ProvidTaskAlgorithm \n");
	weight_strp_lay = get_weight_strp_lay(strp_lay_head,g_weight);
	if(DEW_DEBUG ==1)
		{
			printf("task lay\n");
			print_double_circular(weight_strp_lay);
		}
	while(weight_strp_lay !=NULL)//一次循环对应一个条带的任务分配
	{
		p_temp_node = weight_strp_lay->next;
		node_num = 0;//初始化节点个数
		while(p_temp_node != weight_strp_lay)//遍历所有任务节点
		{
			p_temp_blk_head = &(container_of(p_temp_node,Nblk_inverted,listNode)->listblk);
			node_ID = container_of(p_temp_node,Nblk_inverted,listNode)->blk_nodeID;
			p_temp_blk = p_temp_blk_head->next;
			block_num = 0;//初始化节点上数据块个数,0号位置用来存储节点号
			task[node_num][0] = node_ID;
			while(p_temp_blk != p_temp_blk_head)//将该节点上的所有数据块添加在任务int task[18][6]里面
			{
				task[node_num][block_num+2] = container_of(p_temp_blk,Nblk_inverted,listblk)->blkID;
				p_temp_blk = p_temp_blk->next;
				block_num ++;
			}
			task[node_num][1] = block_num;
			node_num ++;
			p_temp_node = p_temp_node->next;
		}//while 遍历所有任务节点

		if(run_plan == PLANA)
			{
			node_ID = task[0][0];
			(g_pDatanodeTask +node_ID-1)->taskNum ++;
			block_num = task[0][1];
			(g_pDatanodeTask +node_ID-1)->singleStripTask = \
					realloc((g_pDatanodeTask +node_ID-1)->singleStripTask,sizeof(nTaskBlock));
			p_temp_task_block = (g_pDatanodeTask +node_ID-1)->singleStripTask;
			p_temp_task_block->chunkID = g_TaskStartBlockNum/(EREASURE_N-EREASURE_K);
			for(i=0;i<EREASURE_N;i++)p_temp_task_block->localTaskBlock[i] = -1;//初始化数组为-1，void * ProcessChunkTask(void* argv)里面有用到
			for(i=0;i<block_num;i++)p_temp_task_block->localTaskBlock[i] = task[0][i+2];
			p_temp_task_block->waitForBlock = EREASURE_N-EREASURE_K - block_num;
			p_temp_task_block->encode = 1;
			p_temp_task_block->waitedBlockType = 0;
			if(p_temp_task_block->waitForBlock > 0)
			{
				j=0;
				for(i=1;i<node_num;i++)
				{
					for(k=0;k<task[1][1];k++)
					{
						p_temp_task_block->waitedBlock[j++] = task[i][k+2];
					}
				}
				assert(j == p_temp_task_block->waitForBlock);
			}
			p_temp_task_block->destIPNum =0;
			//p_temp_task_block->destIP[];


				for(i=1; i<node_num; i++)
				{

					local_node_ID = task[i][0];
					(g_pDatanodeTask +local_node_ID-1)->taskNum ++;
					block_num = task[i][1];
					(g_pDatanodeTask +local_node_ID-1)->singleStripTask = \
							realloc((g_pDatanodeTask +local_node_ID-1)->singleStripTask,sizeof(nTaskBlock));
					p_temp_task_block = (g_pDatanodeTask +local_node_ID-1)->singleStripTask;
					p_temp_task_block->chunkID = g_TaskStartBlockNum/(EREASURE_N-EREASURE_K);
					for(i=0;i<EREASURE_N;i++)p_temp_task_block->localTaskBlock[i] = -1;
					for(i=0;i<block_num;i++)p_temp_task_block->localTaskBlock[i] = task[0][i+2];
					p_temp_task_block->waitForBlock = 0;
					p_temp_task_block->encode = 0;
					p_temp_task_block->waitedBlockType = 0;
					//p_temp_task_block->waitedBlock[j++] = 0;//不等待数据，无需接收数据
					p_temp_task_block->destIPNum =1;
					memcpy(p_temp_task_block->destIP[0],g_nodeIP[node_ID],IP_LENGTH);

				}
			}
			else//执行PLANB
			{

			}

		g_TaskStartBlockNum = g_TaskStartBlockNum + EREASURE_N - EREASURE_K;
		if(g_TaskStartBlockNum >= 6)return;
		strp_lay_head = get_strp_lay(g_TaskStartBlockNum);
		weight_strp_lay = get_weight_strp_lay(strp_lay_head,g_weight);

	}//while 一次循环对应一个条带的任务分配


	if(DEW_DEBUG==1)printf("inside ProvidTaskAlgorithm \n");
	return ;
	}
