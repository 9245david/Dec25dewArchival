/*
 * DatanodeToDatanode.c
 *
 *  Created on: Jan 23, 2015
 *      Author: hadoop
 *      //--------------------并没有真正的读取本次盘数据
 *
 *注意，后期有待验证两个节点之间多个连接，服务端是用一个端口速度更快还是多个端口速度更快
 *     顺序发送的分割了的数据块，会乱续到达吗
 *      本模块实现功能，流水线处理任务，管理处理任务过程中的内存table以及datanode节点之间的连接table
 *typedef struct taskBlock{
			int localTaskBlock[EREASURE_N];
		//本地需要读取的数据块，长度信息以数组中的结束标志int值自带
		//数组未满部分以-1填充
	unsigned int waitForBlock;
		//值大于1 表示waitedBlock[EREASURE_N]非空，
		//值为0表示本地数据可以直接发送
	unsigned int encode;
		//值为1 表示waitedBlock[EREASURE_N]为EREASURE_K个校验块，
		//值为0表示waitedBlock[ERASURE_N]为等带的数据块，
		//长度信息以数组中的结束标志int自带
		//此时会发现int waitForBlock参数为非必要参数，因为长度为0即表示无需等待
	unsigned int waitedBlockType;
	//块类型标识；
	//为0表示数据块。为（1~2k-1）时，表示为校验块，且每个bit位都代表一个参与校验的块。
	//条带有k个数据块。
	unsigned int waitedBlock[EREASURE_N];
	  	//等待的Block只能是数据块或者校验块，不能二者混杂在一起
		//如果是校验块，存放顺序依次为P1‘，P2’，……，Pk‘但是无法标记为哪些块生成的校验块，由传输端确定是哪些校验
	unsigned int destIPNum;//一般情况下均为1,除了最终编码节点的目标ip为多个
	unsigned char destIP[3][IP_LENGTH];
		//目的ＩＰ地址，

}nTaskBlock,*pTaskBlock;
typedef struct transportBlock{
	unsigned int blockType;
	//块类型标识；
	//为0表示数据块。为（1~2k-1）时，表示为校验块，且每个bit位都代表一个参与校验的块。
	//条带有k个数据块。
	unsigned int blockID;
	//块号；
	//数据块号，编号为1~∝，
	//当<块类型标识>显示该块为校验块时，块号信息仅代表所属条带，因为块已被编码
	unsigned int parityID;
	//代表校验块的第几块，因为校验块只能是P1~Pk,所以其值为1~k
	unsigned int offsetInsideBlock;
	//因为网络中传输的块不是一个个完整的块。所以会有其在数据块内偏移，
	//例如64MB的大块被分割为1MB的小块，偏移量标识为0~63
	unsigned long transportSize;
	//传输块大小；
	//实际传输时的块大小，可能为暂定1MB,1024*1024 Byte，为可调整参数

	//此结构体中是否应该有一个char blockData[transportSize];存储数据，但是C不允许定义变长数组
}nTransportBlock,*pTransportBlock;
 */
/*已建立的连接具备多个属性，可以返回连接字，可以任意发送数据，
*可以返回其缓存地址用于发送数据，可以不用其缓存地址发送数据
*/

#include "DatanodeToDatanode.h"
extern int32_t g_finished_task;
extern pthread_mutex_t g_finished_task_lock;
pConnect g_pFreeClientBuffList = NULL;
pConnect g_pUsedClientBuffList = NULL;//暂时不用上
pConnectServer g_pServerBuffList = NULL;
pMemory g_pFreeMemoryList = NULL;//所有的内存
pthread_mutex_t g_memoryLock;
pthread_mutex_t g_FreeClientLock;//需要初始化
pthread_mutex_t g_ServerLock;//需要初始化
void * ProcessChunkTask(void* argv)
{
	pTaskBlock pChunkTask = (pTaskBlock)argv;//某一个块的任务

	//本地数据块
	int32_t * localBlock = NULL;
	int32_t localNum = 0;
	int32_t waitBlockNum = 0;
	int32_t i;
	int32_t equal ;
	pthread_t * thread_client_num =NULL;//客户端发送数据线程
	pthread_t * thread_local_num = NULL;//读取本地数据线程
	//struct dataBuff chunkBuff;
	pConnect *connfdClient = NULL;//任务中发送数据块
	pConnectServer * connfdServer = NULL;//任务中的等待数据块
	pLocalData  pLocalGroup = NULL;//任务中的本地数据
	pTransportBlock pChunkTranport = NULL; //任务中的带的数据块描述信息
	pSingleBuff * pLocalBuff = NULL;
	list_head * pSearch = NULL;
	assert((pChunkTask->destIPNum>=0)&&(pChunkTask->destIPNum<=18));
	connfdClient = (pConnect*)malloc((pChunkTask->destIPNum) * sizeof(pConnect));
	//需要连接的服务端个数
	assert(connfdClient != NULL);
	localBlock = pChunkTask -> localTaskBlock;
	while(*(localBlock+localNum) != -1)
		{
			localNum ++;
		}
		pLocalBuff = (pSingleBuff *)malloc(localNum*sizeof(pSingleBuff));
		thread_local_num = (pthread_t*)malloc(localNum*sizeof(pthread_t));
		pLocalGroup = (pLocalData)malloc(localNum*sizeof(nLocalData));

		for(i=0;i<localNum;i++)
		{
			*(pLocalBuff+i) = AskForMemory();
			//创建线程读取本地数据
			(pLocalGroup+i)->pBuffPice = *(pLocalBuff+i);
			(pLocalGroup+i)->localBlock = pChunkTask->localTaskBlock[i];
			pthread_create(thread_local_num+i,NULL,&ReadLocalData,(void*)(pLocalGroup+i));

		}

	for(i=0;i<pChunkTask -> destIPNum;i++)
	{

		*(connfdClient+i) = ApplyForClientConnection(pChunkTask,i);
	}

	thread_client_num = (pthread_t*)malloc(localNum*sizeof(pthread_t));
	assert(thread_client_num != NULL);
	if(pChunkTask->encode == 0)//不需编码，即直接发送数据,无需接收数据
	{
	//		assert(localNum == pChunkTask -> destIPNum);//出错不是每个目标节点只发送一个数据

			for(i=0;i<localNum;i++)
			{
					//	SendBackMemory(connfdClient[i].pBuffPice);
				connfdClient[i]->pBuffPice = *(pLocalBuff+i);
				pthread_create(thread_client_num+i,NULL,&SendData,(void*)(*(connfdClient+i)));
			//	thread_client_num++;

			}

			for(i=0;i<localNum;i++)
			{
				if(DEW_DEBUG==1)printf("等待回收内存\n");
				while(connfdClient[i]->pBuffPice->length!=0)NULL;
				SendBackMemory(connfdClient[i]->pBuffPice);
			}

	}
	else if(pChunkTask->waitForBlock ==0)//需要编码，但是无需等待数据，只需要本地数据
	{
		//for(i=0;i<localNum;i++)
		for(i=0;i<EREASURE_K;i++)
			{
				connfdClient[i]->pBuffPice = AskForMemory();
				pthread_create(thread_client_num+i,NULL,&SendData,(void*)(*(connfdClient+i)));
				//thread_client_num++;
			}
		assert(connfdServer ==NULL);
		EncodeData(connfdServer,pLocalBuff,connfdClient,pChunkTask);//生产数据
		//for(i=0;i<localNum;i++)
		for(i=0; i<EREASURE_K; i++)//回收客户端
		{
			if(DEW_DEBUG==1)printf("等待回收内存\n");
			while(connfdClient[i]->pBuffPice->length!=0)NULL;
			SendBackMemory(connfdClient[i]->pBuffPice);
		}
		for(i=0;i<localNum;i++)//回收本地
		{
			if(DEW_DEBUG==1)printf("等待回收内存\n");
			//while(connfdClient[i]->pBuffPice->length!=0)NULL;
			SendBackMemory(connfdClient[i]->pBuffPice);
		}


	}
	else if((pChunkTask->waitForBlock != 0) &&(pChunkTask->encode !=0))
		//获取的匹配server端是带连接信息的已经申请好了内存，不用将该结构还给链表，只用将内存还给内存模块
	{
		pChunkTranport = (pTransportBlock)malloc(sizeof(nTransportBlock));
		connfdServer = (pConnectServer*)malloc((pChunkTask->waitForBlock)*sizeof(pConnectServer));
		assert(connfdServer != NULL);
		for(i=0;i<pChunkTask->waitForBlock;i++)
		{
			TraslateTaskToServer(pChunkTask,i,pChunkTranport);//等待的哪一个数据块
			pSearch = &(g_pServerBuffList->listConnect);
			while(1)//一直等待，直到连接被建立
			{
				if(pSearch ==&(g_pServerBuffList->listConnect))pSearch = pSearch->next;
				connfdServer[i] = container_of(pSearch,nConnectServer,listConnect);
				equal = memcmp(connfdServer[i]->pChunkTransport,pChunkTranport,sizeof(nTransportBlock));
				if(equal != 0)pSearch = pSearch->next;
				else break;
			}
		}
		for(i=0;i<localNum;i++)
		{
			connfdClient[i]->pBuffPice = AskForMemory();
			pthread_create(thread_client_num+i,NULL,&SendData,(void*)(*(connfdClient+i)));
			//thread_client_num++;
		}
		EncodeData(connfdServer,pLocalBuff,connfdClient,pChunkTask);//生产数据

		for(i=0; i<EREASURE_K; i++)//回收客户端
		{
			if(DEW_DEBUG==1)printf("等待回收内存\n");
			while(connfdClient[i]->pBuffPice->length!=0)NULL;
			SendBackMemory(connfdClient[i]->pBuffPice);
		}
		for(i=0;i<localNum;i++)//回收本地
		{
			if(DEW_DEBUG==1)printf("等待回收内存\n");
			//while(pLocalBuff[i]->length!=0)NULL;
			SendBackMemory(pLocalBuff[i]);
		}
		for(i=0;i<pChunkTask->waitForBlock;i++)//回收服务端
		{
			SendBackMemory(connfdServer[i]->pBuffPice);
		}
	}
	else
	{
		printf("pChunkTask error\n");
		return NULL;
	}
	pthread_mutex_lock(&g_finished_task_lock);
	g_finished_task++;
	pthread_mutex_unlock(&g_finished_task_lock);
	return (void*)NULL;

}
pSingleBuff ApplyBuffFromLocalData(int32_t localBlock)
{
	return NULL;
	}
pSingleBuff ApplyBuffFromServerConnection(uint32_t waitedBlockID)
//依据等待的块号向已连接sock索要内存,如果连接没有建立，等待连接，后面的程序也会阻塞一段时间
{

	return NULL;
	}
pConnect ApplyForClientConnection(pTaskBlock pChunkTask,int32_t destNum)
//依据目的ip地址索要连接，如果没有连接则建立连接
//连接的缓存没有申请，
{
	int32_t connfd=0;
	char * destIP = pChunkTask->destIP[destNum];
	pConnect tmpConnect =NULL;
	list_head * pSearch = NULL;
	pTransportBlock pChunkTransport =NULL;
	pChunkTransport = (pTransportBlock)(malloc)(sizeof(nTransportBlock));
	assert(pChunkTransport!=NULL);

	if(g_pFreeClientBuffList ==NULL)
	{
		g_pFreeClientBuffList = (pConnect)malloc(sizeof(nConnect));//创建连接
		assert(g_pFreeClientBuffList !=NULL);
		init_list_head(&(g_pFreeClientBuffList->listConnect));
	}
	pSearch = g_pFreeClientBuffList->listConnect.next;
	while(pSearch != &(g_pFreeClientBuffList->listConnect))//遍历链表是否存在destIP的空连接
	{
		tmpConnect = container_of(pSearch,nConnect,listConnect);
		if(strcmp(tmpConnect->IP_ADDR,destIP)==0)
		{
			list_del(pSearch);
			break;
		}
		else pSearch = pSearch->next;
	}
	if(pSearch == &(g_pFreeClientBuffList->listConnect))
	{
		tmpConnect = (pConnect)malloc(sizeof(nConnect));
		assert(tmpConnect !=NULL);
		pServerNodeID pDestNodeID =NULL;
		pDestNodeID = (pServerNodeID)malloc(sizeof(ServerNodeID));
		pDestNodeID -> NameToDataPort = DATANODE_PORT;
		pDestNodeID -> Namenode_ADDR = destIP;
		connfd = ClientConnectToServer(pDestNodeID);
		tmpConnect->IP_ADDR = destIP;
		tmpConnect->connfd = connfd;

	/*
	 *此处需要添加全局锁，因为申请内存的过程必须是互斥的
	 *pthread_mutex_lock(&g_memoryLock);
	 *tmpConnect->pBuffPice = AskForMemory();
	 *pthread_mutex_unlock(&g_memoryLock);
	*/
	}
	TraslateTaskToTransport(pChunkTask,destNum,pChunkTransport);
	DataTransportWrite(connfd,(char*)pChunkTransport,sizeof(nTransportBlock));

//	pServerNodeID pDestNodeID =NULL;
//	pDestNodeID = (pServerNodeID)malloc(sizeof(ServerNodeID));
//	pDestNodeID -> NameToDataPort = DATANODE_PORT;
//    pDestNodeID -> Namenode_ADDR = destIP;
//
//	ClientConnectToServer(pDestNodeID);
	return tmpConnect;
}

pSingleBuff ApplyBuffFromClientConnection(unsigned char * destIP)
{
	return NULL;
	}
void EncodeData(pConnectServer* connfdServer,pSingleBuff* pLocalBuff,pConnect* connfdClient,pTaskBlock pChunkTask)
//connfdServer可以为空买，即不用等待数据
{
	int32_t serverNum  = pChunkTask->waitForBlock;
	int32_t i =0;
	int32_t localNum = 0;
	int32_t clientNum = pChunkTask->destIPNum;
	int64_t piceNum = (BLOCK_SIZE/BUFF_PICE_SIZE);
	while(pChunkTask->localTaskBlock[i] !=-1)
	{
		i++;
		assert(i<(EREASURE_N-EREASURE_K));
		localNum ++;
	}
	while((piceNum--)>=0)
	{
		for(i=0;i<localNum;i++)
		{
			while((*(pLocalBuff+i))->length ==0){};
		}
		for(i=0;i<serverNum;i++)
		{
			assert(connfdServer!=NULL);
			while((*(connfdServer+i))->pBuffPice->length==0){};
		}
		//编码数据块操作，暂时省略，就是将k带入计算
		for(i=0;i<localNum;i++)
		{
			(*(pLocalBuff+i))->start = (*(pLocalBuff+i))->start +1;
			pthread_mutex_lock(&((*(pLocalBuff+i))->buffLock));
			(*(pLocalBuff+i))->length = (*(pLocalBuff+i))->length -1;
			pthread_mutex_unlock(&((*(pLocalBuff+i))->buffLock));
		}
		for(i=0;i<serverNum;i++)
		{
			assert(connfdServer!=NULL);
			(*(connfdServer+i))->pBuffPice->start = (*(connfdServer+i))->pBuffPice->start +1;
			pthread_mutex_lock(&((*(connfdServer+i))->pBuffPice->buffLock));
			(*(connfdServer+i))->pBuffPice->length = (*(connfdServer+i))->pBuffPice->length -1;
			pthread_mutex_unlock(&((*(connfdServer+i))->pBuffPice->buffLock));

		}
		for(i=0;i<clientNum;i++)
		{
			connfdClient[i]->pBuffPice->end = connfdClient[i]->pBuffPice->end + 1;
			pthread_mutex_lock(&(connfdClient[i]->pBuffPice->buffLock));
			connfdClient[i]->pBuffPice->length = connfdClient[i]->pBuffPice->length + 1;
			pthread_mutex_unlock(&(connfdClient[i]->pBuffPice->buffLock));
		}
	}

	}
pSingleBuff AskForMemory()//向内存模块申请内存，需要加锁因为不同的访问必须是互斥的
//g_pFreeMemoryList 为空闲内存模块链表，如果是第一次就申请20个内存片，以双向循环链表的方式存在g_pFreeMemoryList 为首节点的
//双向链表中，如果空闲链表为空，则申请内存是增加一个内存片在链表尾部。分配内存就是将链表的尾节点对应的内存分出去，然后从链表尾部删除
//分配了之后再初始化一些参数，这样在归还缓存时便不需要初始化了，因为每次使用之前就会对其进行初始化
{
	int32_t baseNum = 20;
	int32_t i =0;
	pMemory tmpMemory = NULL;
	char * tmpBuff = NULL;
	pSingleBuff tmpSingleBuff =NULL;
	pthread_mutex_lock(&g_memoryLock);
	if(g_pFreeMemoryList == NULL)//如果为空,直接申请二十片内存
	{
		g_pFreeMemoryList = (pMemory)malloc(sizeof(nMemory));
		assert(g_pFreeMemoryList !=NULL);
		init_list_head(&(g_pFreeMemoryList->listMemory));//添加空的头节点
		for(i=0;i<baseNum;i++)//申请20个尾节点
		{
			tmpMemory = (pMemory)malloc(sizeof(nMemory));
			assert(tmpMemory !=NULL);
			list_add_tail(&(tmpMemory->listMemory),&(g_pFreeMemoryList->listMemory));
			tmpBuff = (char*)malloc(BUFF_SIZE*sizeof(char));
			assert(tmpBuff !=NULL);
			tmpSingleBuff = (pSingleBuff)malloc(sizeof(nSingleBuff));
			assert(tmpSingleBuff != NULL);
			tmpSingleBuff -> buff = tmpBuff;
//			tmpSingleBuff -> buffSize = BUFF_SIZE;
//			tmpSingleBuff -> pice = BUFF_PICE_SIZE;
//			tmpSingleBuff -> start = 0;
//			tmpSingleBuff -> end = 0;//end < buffSize/pice;
//			tmpSingleBuff -> length = 0;
			pthread_mutex_init(&(tmpSingleBuff->buffLock),NULL);
			tmpMemory->pBuffPice = tmpSingleBuff;

		}


	}
	else if(g_pFreeMemoryList->listMemory.next == &(g_pFreeMemoryList->listMemory))
	//不为空，但是没有了内存片，增加一个内存片
	{
		tmpMemory = (pMemory)malloc(sizeof(nMemory));
		assert(tmpMemory !=NULL);
		list_add_tail(&(tmpMemory->listMemory),&(g_pFreeMemoryList->listMemory));
		tmpBuff = (char*)malloc(BUFF_SIZE*sizeof(char));
		assert(tmpBuff ==NULL);
		tmpSingleBuff = (pSingleBuff)malloc(sizeof(nSingleBuff));
		assert(tmpSingleBuff != NULL);
//		tmpSingleBuff -> buff = tmpBuff;
//		tmpSingleBuff -> buffSize = BUFF_SIZE;
//		tmpSingleBuff -> pice = BUFF_PICE_SIZE;
//		tmpSingleBuff -> start = 0;
//		tmpSingleBuff -> end = 0;//end < buffSize/pice;
//		tmpSingleBuff -> length = 0;
		pthread_mutex_init(&(tmpSingleBuff->buffLock),NULL);

		tmpMemory->pBuffPice = tmpSingleBuff;
	}
	tmpMemory = container_of(g_pFreeMemoryList->listMemory.prev,nMemory,listMemory);
	tmpSingleBuff -> buffSize = BUFF_SIZE;
	tmpSingleBuff -> pice = BUFF_PICE_SIZE;
	tmpSingleBuff -> start = 0;
	tmpSingleBuff -> end = 0;//end < buffSize/pice;
	tmpSingleBuff -> length = 0;
	tmpSingleBuff = tmpMemory ->pBuffPice;
	list_del(g_pFreeMemoryList->listMemory.prev);
	pthread_mutex_unlock(&g_memoryLock);
	return tmpSingleBuff;
	}

void SendBackMemory(pSingleBuff pBuffPice)
{
	pMemory tmpMemory = NULL;
	tmpMemory = (pMemory)malloc(sizeof(nMemory));
	assert(tmpMemory !=NULL);
	pthread_mutex_lock(&g_memoryLock);
	list_add_tail(&(tmpMemory->listMemory),&(g_pFreeMemoryList->listMemory));
	pthread_mutex_unlock(&g_memoryLock);
	tmpMemory->pBuffPice = pBuffPice;
	}
void TraslateTaskToServer(pTaskBlock pChunkTask,int32_t destNum,pTransportBlock pChunkTransport)
{
	int32_t * localBlock = NULL;
	int32_t localNum = 0;
	int32_t waitBlockNum = 0;
	int32_t i =0;
	int32_t offsetInsideChunk =-1;//0~6
	pChunkTransport->chunkID = pChunkTask->chunkID;
	pChunkTransport->blockType = pChunkTask->waitedBlockType;
	if(pChunkTransport->blockType ==0)
	{
		pChunkTransport->blockID = pChunkTask->waitedBlock[destNum];

		pChunkTransport->parityID = -1;

	}
	else
	{
		pChunkTransport->blockID = -1;
		pChunkTransport->parityID = pChunkTask->waitedBlock[destNum];
	}

	pChunkTransport->offsetInsideBlock =-1;
	pChunkTransport->transportSize = BUFF_PICE_SIZE;
}

void TraslateTaskToTransport(pTaskBlock pChunkTask,int32_t destNum,pTransportBlock pChunkTransport)
{
	int32_t * localBlock = NULL;
	int32_t localNum = 0;
	int32_t waitBlockNum = 0;
	int32_t i =0;
	int32_t offsetInsideChunk =-1;//0~6
	localBlock = pChunkTask -> localTaskBlock;
	while(*(localBlock+localNum) != -1)
		{
			localNum ++;
		}
	if((pChunkTask->encode == 0)&&(pChunkTask->waitForBlock == 0))
	{
		pChunkTransport->blockType = 0;
		pChunkTransport->blockID = pChunkTask->localTaskBlock[destNum];
		pChunkTransport->chunkID = (pChunkTransport->blockID)/(EREASURE_N - EREASURE_K);
		pChunkTransport->parityID = -1;
	}
	else
	{
		pChunkTransport->blockType = pChunkTask-> waitedBlockType;
		for(i=0;i<localNum;i++)
		{
			offsetInsideChunk=(pChunkTask->localTaskBlock[i])%(EREASURE_N - EREASURE_K);
			pChunkTransport->blockType = (pChunkTransport->blockType)|(0x1<<offsetInsideChunk);
		}
		pChunkTransport->blockID = -1;
		pChunkTransport->chunkID =  pChunkTask->chunkID;
		pChunkTransport->parityID = destNum;
	}
	pChunkTransport->offsetInsideBlock =-1;
	pChunkTransport->transportSize = BUFF_PICE_SIZE;
}

void* DataToDataTaskServer(void*arg)
{
	//作为处理任务的服务端，等待建立连接，将缓存挂在table中，接收数据

		int32_t listenfd;
		int32_t on,ret;
	//	socklen_t len;
		struct sockaddr_in servaddr;
		listenfd = socket(AF_INET,SOCK_STREAM,0);
		on = 1;
	    if(DEW_DEBUG ==1)printf("inside DataToDataTaskServer\n");
		ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );
	    bzero(&servaddr,sizeof(servaddr));
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	    servaddr.sin_port = htons(DATANODE_PORT);
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

	return NULL;
}

int32_t handle_connect(int32_t listen_sock)//返回0正常，返回其他值，失败
{
		int32_t * Pconnfd;
		socklen_t len;
		struct sockaddr_in cliaddr;
		pthread_t  pthread_do;
		int32_t rt;
		len = sizeof(cliaddr);
		pthread_mutex_init(&g_ServerLock,NULL);

	    if(DEW_DEBUG ==1)printf("inside handle_connect\n");
		while(1)//等待所有datanode连接
		{
			Pconnfd = (int32_t*)malloc(sizeof(int32_t));
			*Pconnfd = accept(listen_sock,(struct sockaddr*)&cliaddr,&len);
			if (-1 != *Pconnfd)
				printf("datanode: got connection from client %s \n",inet_ntoa(cliaddr.sin_addr));
			//注册,连接套接字*Pconnfd与节点号以及节点iｐ的对应关系，在这之前必须保证已经有iｐ和节点号的对应关系
			rt = pthread_create(&pthread_do,NULL,&handle_request,(void*)Pconnfd);
			if(0 != rt)
					{
					printf("pthread_create error\n");
					return rt;
				         }
		}
		return 0;
	}
void *handle_request(void * arg)
//每一个块建立一个这样的连接，一旦连接建立便可以接收数据，并且将连接信息挂在在g_pServerBuffList的表中
//不用替换内存，使用完后也不用在table中删除。因为Server端被连接，一旦client端确立了连接之后就会发送数据块信息
//更新table中对应套接字所附带的待接收数据块信息，原有的数据块信息一定是过时的未在使用的。
{
	int32_t connfd ;
	pSingleBuff  pBuffPice =NULL;
	char sendbuff[DATA_NAME_MAXLENGTH];
	int64_t recv,send;
	int64_t task_length = 0;
	int64_t piceNum = 0;
	pConnectServer tmpConnectServer = NULL;
	pTransportBlock pChunkTransport =NULL;
	pChunkTransport = (pTransportBlock)(malloc)(sizeof(nTransportBlock));
	assert(pChunkTransport!=NULL);
	connfd = *((int32_t*)arg);
	free(arg);
	if(DEW_DEBUG ==1)printf("inside hanlde_request %d\n",connfd);
	pthread_mutex_lock(&g_ServerLock);
			if(g_pServerBuffList == NULL)
			{
				g_pServerBuffList = (pConnectServer)malloc(sizeof(nConnectServer));
				assert(g_pServerBuffList != NULL);
				init_list_head(&(g_pServerBuffList->listConnect));

			}
			tmpConnectServer = (pConnectServer)malloc(sizeof(nConnectServer));
			assert(tmpConnectServer!= NULL);
			tmpConnectServer->connfd = connfd;
			tmpConnectServer->pBuffPice = AskForMemory();
			tmpConnectServer->pChunkTransport = pChunkTransport;
			list_add_tail(&(g_pServerBuffList->listConnect),&(tmpConnectServer->listConnect));
			pthread_mutex_unlock(&g_ServerLock);
	while(1)//反复利用不同的数据块
	{

	    if(DEW_DEBUG ==1)printf("inside hanlde_request while\n");
		recv = DataTransportRead(connfd,(char*)pChunkTransport,sizeof(nTransportBlock));
		if(recv<0)
		{
			if(DEW_DEBUG ==2)
					{
						fprintf(stderr,"datanode recvdata error\n");
					}
			printf("datanode recvdata error\n");
			close(connfd);
			return NULL;
		}

		piceNum = (BLOCK_SIZE/BUFF_PICE_SIZE);
		while((piceNum--)>=0)
		{
			pBuffPice = tmpConnectServer->pBuffPice;
			while(pBuffPice->length ==(BUFF_SIZE/BUFF_PICE_SIZE));

			recv = DataTransportRead(connfd,pBuffPice->buff+(pBuffPice->end)*BUFF_PICE_SIZE,BUFF_PICE_SIZE);
			if(recv<0)
					{
						if(DEW_DEBUG ==2)
								{
									fprintf(stderr,"datanode recvdata error\n");
								}
						printf("datanode recvdata 2 error\n");
						close(connfd);
						return NULL;
					}
			pBuffPice->end = (pBuffPice->end +1)%(BUFF_SIZE/BUFF_PICE_SIZE);
			pthread_mutex_lock(&(pBuffPice->buffLock));
			pBuffPice->length = pBuffPice->length +1;
			pthread_mutex_unlock(&(pBuffPice->buffLock));

		}

	}
	return NULL;
	}
void *SendData(void*arg)//只是发送缓存，发送数据，发送完成之后需要解放占有的缓存空间
{
	pConnect connfdClient = (pConnect)arg;
	int32_t connfd = connfdClient->connfd;
	pSingleBuff pBuffPice = connfdClient->pBuffPice;
	int64_t piceNum = (BLOCK_SIZE/BUFF_PICE_SIZE);
	 if(DEW_DEBUG ==1)printf("inside SendData\n");
	pthread_detach(pthread_self());
	while((piceNum--)>=0)
			{
				while(pBuffPice->length ==0);

				DataTransportWrite(connfd,pBuffPice->buff+(pBuffPice->start)*BUFF_PICE_SIZE,BUFF_PICE_SIZE);
				pBuffPice->start = (pBuffPice->start +1)%(BUFF_SIZE/BUFF_PICE_SIZE);
				pthread_mutex_lock(&(pBuffPice->buffLock));
				pBuffPice->length = pBuffPice->length -1;
				pthread_mutex_unlock(&(pBuffPice->buffLock));

			}
	pthread_exit(0);
	//return NULL;
	}
void *ReadLocalData(void * arg)
//并没有真正的读取本次盘数据
{
	int32_t localBlock;
	pLocalData p_tmpLocalData =NULL;
	off_t offset = 0;
	pSingleBuff pBuffPice = NULL;
	pMemory tmpMemory = NULL;
	int64_t piceNum = (BLOCK_SIZE/BUFF_PICE_SIZE);
	if(DEW_DEBUG ==1)printf("inside ReadLocalData\n");
	pthread_detach(pthread_self());
	p_tmpLocalData = (pLocalData)arg;
	localBlock = p_tmpLocalData->localBlock;
	 pBuffPice = p_tmpLocalData->pBuffPice;
	//根据块号找到块偏移，读取数据块
	offset = FindBlockOffset(localBlock);
	while((piceNum--)>=0)
	{
		while(pBuffPice->length == (BUFF_SIZE/BUFF_PICE_SIZE)){};//缓存已满
		ReadDisk(offset,pBuffPice->buff+pBuffPice->length*BUFF_PICE_SIZE,BUFF_PICE_SIZE);

		pthread_mutex_lock(&(pBuffPice->buffLock));
		pBuffPice->length = pBuffPice->length +1;
		pthread_mutex_unlock(&(pBuffPice->buffLock));
		//参数end，piceNum,offset读取本地磁盘数据pBuffPice->buff
		pBuffPice->end = (pBuffPice->end + 1)%(BUFF_SIZE/BUFF_PICE_SIZE);
	}
	//可选择在这里循环判断长度是否为0,为0就回收本地内存给链表,SendBackMemory调用函数即可，重复了
//	while(pBuffPice->length !=0)NULL;
//	tmpMemory = (pMemory)malloc(sizeof(nMemory));
//	tmpMemory->pBuffPice = pBuffPice;
//	pthread_mutex_lock(&g_memoryLock);
//	list_add_tail(&(tmpMemory->listMemory),&(g_pFreeMemoryList->listMemory));
//	pthread_mutex_unlock(&g_memoryLock);
pthread_exit(0);
	}
off_t FindBlockOffset(int32_t localBlock)
//输入参数为数据块号，根据数据块号找到对应数据的偏移
{
	return 0;

	}
uint64_t ReadDisk( off_t offset, char * buff, uint64_t length)
{
	char * path;//每个节点都会有对应的/dev/sda

	return BUFF_PICE_SIZE;
	}
