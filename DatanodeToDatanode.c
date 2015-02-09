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
#include "BlockStruct.h"
#include "Socket_connect_read_write.h"
#include "dew_list.h"
typedef struct buffDiscript{
	char * buff;
	int start;
	int end;
	int length;//已经存的pice数目
	long buffSize;//8MB
	long pice;//每一片buff的大小
	pthread_mutex_t buffLock;
}*pSingleBuff,nSingleBuff;
typedef struct dataBuff{
	pConnect * allBuff;
	int allBuffNum[3];//分割为三段，第一段为接收数据buff 为0即为空,第二段为本地数据buff，第三段为发送数据buff
};

typedef struct connectClientQueue{
	pSingleBuff pBuffPice;//内存片
	char * IP_ADDR;//地址，服务器端或者客户端
	int connfd;//连接套接字
	list_head listConnect;//连接不同的连接套接字
}*pConnect,nConnect;
typedef struct connectServerQueue{
	pSingleBuff pBuffPice;//内存片
	int connfd;//连接套接字
	pTransportBlock pChunkTransport;
	list_head listConnect;//连接不同的连接套接字
}*pConnectServer,nConnectServer;
typedef struct memoryQueue{
	pSingleBuff pBuffPice;
	list_head listMemory;
}*pMemory,nMemory;
typedef struct localData{
	pSingleBuff pBuffPice;//读取块的内存片
	int localBlock;//块号
}*pLocalData,nLocalData;
pConnect g_pFreeClientBuffList = NULL;
pConnect g_pUsedClientBuffList = NULL;//暂时不用上
pConnectServer g_pServerBuffList = NULL;
pMemory g_pFreeMemoryList = NULL;//所有的内存
pthread_mutex_t g_memoryLock;
pthread_mutex_t g_FreeClientLock;需要初始化
pthread_mutex_t g_ServerLock;需要初始化
void * ProcessChunkTask(void* argv)
{
	pTaskBlock pChunkTask = (pTaskBlock)argv;//某一个块的任务

	//本地数据块
	int * localBlock = NULL;
	int localNum = 0;
	int waitBlockNum = 0;
	int i;
	int equal ;
	pthread_t * thread_client_num =NULL;//客户端发送数据线程
	pthread_t * thread_local_num = NULL;//读取本地数据线程
	struct dataBuff chunkBuff;
	pConnect *connfdClient = NULL;//任务中发送数据块
	pConnectServer * connfdServer = NULL;//任务中的等待数据块
	pLocalData  pLocalGroup = NULL;//任务中的本地数据
	pTransportBlock pChunkTranport = NULL; //任务中的带的数据块描述信息
	pSingleBuff * pLocalBuff = NULL;
	list_head pSearch = NULL;
	connfdClient = (pConnect*)malloc((pChunkTask->destIPNum) * sizeof(pConnect));
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
			(pLocalGroup+i)->localBlock = pChunkTask->waitedBlock[i];
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
			assert(localNum == pChunkTask -> destIPNum);

			for(i=0;i<localNum;i++)
			{
					//	SendBackMemory(connfdClient[i].pBuffPice);
				connfdClient[i].pBuffPice = *(pLocalBuff+i);
				pthread_create(thread_client_num+i,NULL,&SendData,(void*)(*(connfdClient+i)));
			//	thread_client_num++;
			}
	}
	else if(pChunkTask->waitForBlock ==0)//需要编码，但是无需等待数据，只需要本地数据
	{
		for(i=0;i<localNum;i++)
			{
				connfdClient[i].pBuffPice = AskForMemory();
				pthread_create(thread_client_num+i,NULL,&SendData,(void*)(*(connfdClient+i)));
				//thread_client_num++;
			}
	}
	else if((pChunkTask->waitForBlock != 0) &&(pChunkTask->encode !=0))
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
			connfdClient[i].pBuffPice = AskForMemory();
			pthread_create(thread_client_num+i,NULL,&SendData,(void*)(*(connfdClient+i)));
			//thread_client_num++;
		}
		EncodeData(connfdServer,pLocalBuff,connfdClient,pChunkTask);//生产数据
	}
	else
	{
		printf("pChunkTask error\n");
		return NULL;
	}

	return (void*)NULL;

}
pSingleBuff ApplyBuffFromLocalData(int localBlock)
{
	return NULL;
	}
pSingleBuff ApplyBuffFromServerConnection(unsigned int waitedBlockID)
//依据等待的块号向已连接sock索要内存,如果连接没有建立，等待连接，后面的程序也会阻塞一段时间
{

	return NULL;
	}
pConnect ApplyForClientConnection(pTaskBlock pChunkTask,int destNum)
//依据目的ip地址索要连接，如果没有连接则建立连接
//连接的
{
	int connfd=0;
	char * destIP = pChunkTask.destIP[destNum];
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
{
	int serverNum  = pChunkTask->waitForBlock;
	int i =0;
	int localNum = 0;
	int clientNum = pChunkTask->destIPNum;
	long piceNum = (BLOCK_SIZE/BUFF_PICE_SIZE);
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
			(*(connfdServer+i))->pBuffPice->start = (*(connfdServer+i))->pBuffPice->start +1;
			pthread_mutex_lock(&((*(connfdServer+i))->pBuffPice->buffLock));
			(*(connfdServer+i))->pBuffPice->length = (*(connfdServer+i))->pBuffPice->length -1;
			pthread_mutex_unlock(&((*(connfdServer+i))->pBuffPice->buffLock));

		}
		for(i=0;i<clientNum;i++)
		{
			connfdClient->pBuffPice->end = connfdClient->pBuffPice->end + 1;
			pthread_mutex_lock(&(connfdClient->pBuffPice->buffLock));
			connfdClient->pBuffPice->length = connfdClient->pBuffPice->length + 1;
			pthread_mutex_unlock(&(connfdClient->pBuffPice->buffLock));
		}
	}

	}
pSingleBuff AskForMemory()//向内存模块申请内存，需要加锁因为不同的访问必须是互斥的
{
	int baseNum = 20;
	int i =0;
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
			assert(tmpBuff ==NULL);
			tmpSingleBuff = (pSingleBuff)malloc(sizeof(nSingleBuff));
			assert(tmpSingleBuff != NULL);
			tmpSingleBuff -> buff = tmpBuff;
			tmpSingleBuff -> buffSize = BUFF_SIZE;
			tmpSingleBuff -> pice = BUFF_PICE_SIZE;
			tmpSingleBuff -> start = 0;
			tmpSingleBuff -> end = 0;//end < buffSize/pice;
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
		tmpSingleBuff -> buff = tmpBuff;
		tmpSingleBuff -> buffSize = BUFF_SIZE;
		tmpSingleBuff -> pice = BUFF_PICE_SIZE;
		tmpSingleBuff -> start = 0;
		tmpSingleBuff -> end = 0;//end < buffSize/pice;
		tmpSingleBuff -> length = 0;
		tmpMemory->pBuffPice = tmpSingleBuff;
	}
	tmpMemory = container_of(g_pFreeMemoryList->listMemory.prev,nMemory,listMemory);
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
void TraslateTaskToServer(pTaskBlock pChunkTask,int destNum,pTransportBlock pChunkTransport)
{
	int * localBlock = NULL;
	int localNum = 0;
	int waitBlockNum = 0;
	int i =0;
	int offsetInsideChunk =-1;//0~6
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

void TraslateTaskToTransport(pTaskBlock pChunkTask,int destNum,pTransportBlock pChunkTransport)
{
	int * localBlock = NULL;
	int localNum = 0;
	int waitBlockNum = 0;
	int i =0;
	int offsetInsideChunk =-1;//0~6
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

int handle_connect(int listen_sock)//返回0正常，返回其他值，失败
{
		int * Pconnfd;
		socklen_t len;
		struct sockaddr_in cliaddr;
		pthread_t  pthread_do;
		int rt;
		len = sizeof(cliaddr);

		while(1)//等待所有datanode连接
		{
			Pconnfd = (int*)malloc(sizeof(int));
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
	int connfd ;
	pSingleBuff  pBuffPice =NULL;
	char sendbuff[DATA_NAME_MAXLENGTH];
	long recv,send;
	long task_length = 0;
	long piceNum = 0;
	pConnectServer tmpConnectServer = NULL;
	pTransportBlock pChunkTransport =NULL;
	pChunkTransport = (pTransportBlock)(malloc)(sizeof(nTransportBlock));
	assert(pChunkTransport!=NULL);
	connfd = *((int*)arg);
	free(arg);
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
		recv = DataTransportRead(connfd,(char*)pChunkTransport,sizeof(nTransportBlock));
		if(recv<0)
		{
			printf("datanode recvdata error\n");
			close(connfd);
			return NULL;
		}

		piceNum = (BLOCK_SIZE/BUFF_PICE_SIZE);
		while((piceNum--)>=0)
		{
			pBuffPice = tmpConnectServer->pBuffPice;
			while(pBuffPice->length ==(BUFF_SIZE/BUFF_PICE_SIZE));

			DataTransportRead(connfd,pBuffPice->buff+(pBuffPice->end)*BUFF_PICE_SIZE,BUFF_PICE_SIZE);
			pBuffPice->end = (pBuffPice->end +1)%(BUFF_SIZE/BUFF_PICE_SIZE);
			pthread_mutex_lock(&(pBuffPice->buffLock));
			pBuffPice->length = pBuffPice->length +1;
			pthread_mutex_unlock(&(pBuffPice->buffLock));

		}

	}
	return NULL;
	}
void *SendData(void*arg)
{
	pConnect connfdClient = (pConnect)arg;
	int connfd = connfdClient->connfd;
	pSingleBuff pBuffPice = connfdClient->pBuffPice;
	long piceNum = (BLOCK_SIZE/BUFF_PICE_SIZE);
	while((piceNum--)>=0)
			{
				while(pBuffPice->length ==0);

				DataTransportWrite(connfd,pBuffPice->buff+(pBuffPice->start)*BUFF_PICE_SIZE,BUFF_PICE_SIZE);
				pBuffPice->start = (pBuffPice->start +1)%(BUFF_SIZE/BUFF_PICE_SIZE);
				pthread_mutex_lock(&(pBuffPice->buffLock));
				pBuffPice->length = pBuffPice->length -1;
				pthread_mutex_unlock(&(pBuffPice->buffLock));

			}
	return NULL;
	}
void ReadLocalData(void * arg)
//并没有真正的读取本次盘数据
{
	int localBlock;
	pLocalData p_tmpLocalData =NULL;
	off_t offset = 0;
	pSingleBuff pBuffPice = NULL;
	long piceNum = (BLOCK_SIZE/BUFF_PICE_SIZE);
	p_tmpLocalData = (pLocalData)arg;
	localBlock = p_tmpLocalData->localBlock;
	 pBuffPice = p_tmpLocalData->pBuffPice;
	//根据块号找到块偏移，读取数据块
	offset = FindBlockOffset(localBlock);
	while((piceNum--)>=0)
	{
		while(pBuffPice->length == (BUFF_SIZE/BUFF_PICE_SIZE)){};//缓存已满
		pthread_mutex_lock(&(pBuffPice->buffLock));
		pBuffPice->length = pBuffPice->length +1;
		pthread_mutex_unlock(&(pBuffPice->buffLock));
		//参数end，piceNum,offset读取本地磁盘数据pBuffPice->buff
		pBuffPice->end = (pBuffPice->end + 1)%(BUFF_SIZE/BUFF_PICE_SIZE);
	}

	}
off_t FindBlockOffset(int localBlock)
//输入参数为数据块号，根据数据块号找到对应数据的偏移
{
	return 0;

	}
