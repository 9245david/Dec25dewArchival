/*
 * DatanodeToDatanode.c
 *
 *  Created on: Jan 23, 2015
 *      Author: hadoop
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
	long buffSize;//8MB
	long pice;//每一片buff的大小
}*pSingleBuff,nSingleBuff;
typedef struct dataBuff{
	pSingleBuff * allBuff;
	int allBuffNum[3];//分割为三段，第一段为接收数据buff 为0即为空,第二段为本地数据buff，第三段为发送数据buff
};

typedef struct connectQueue{
	pSingleBuff pBuffPice;//内存片
	char * IP_ADDR;//地址，服务器端或者客户端
	int connfd;//连接套接字
	list_head listConnect;//连接不同的连接套接字
}*pConnect,nConnect;
typedef struct memoryQueue{
	pSingleBuff pBuffPice;
	list_head listMemory;
}*pMemory,nMemory;
pConnect g_pFreeClientBuffList = NULL;
pConnect g_pUsedClientBuffList = NULL;//暂时不用上
pConnect g_pFreeServerBuffList = NULL;
pConnect g_pUsedServerBuffList = NULL;//暂时用不上
pMemory g_pFreeMemoryList = NULL;//所有的内存
pthread_mutex_t g_memoryLock;
pthread_mutex_t g_FreeClientLock;
void * ProcessChunkTask(void* argv)
{
	pTaskBlock pChunkTask = (pTaskBlock)argv;//某一个块的任务

	//本地数据块
	int * localBlock = NULL;
	int localNum = 0;
	int waitBlockNum = 0;
	int i;
	struct dataBuff chunkBuff;
	pConnect connfdClient = NULL;

	connfdClient = (pConnect)malloc((pChunkTask->destIPNum) * sizeof(nConnect));
	assert(connfdClient == NULL);
	localBlock = pChunkTask -> localTaskBlock;
	for(i=0;i<pChunkTask -> destIPNum;i++)
	{
		connfdClient[i] = ApplyForClientConnection(pChunkTask,i);
	}
	while(*(localBlock+localNum) != -1)
	{
		localNum ++;
	}
	chunkBuff.allBuffNum[0] = pChunkTask -> waitForBlock;
	chunkBuff.allBuffNum[1] = chunkBuff.allBuffNum[0] + localNum;
	if(pChunkTask->encode != 0)chunkBuff.allBuffNum[2] = chunkBuff.allBuffNum[1]+EREASURE_K;
	chunkBuff.allBuff = (pSingleBuff*)malloc(chunkBuff.allBuffNum[2]*sizeof(pSingleBuff*));
	assert(chunkBuff.allBuff == NULL);
	//多次索要buff的过程不需要多线程，因为必须每段内存都分配完成后流水线才可以进行，
	//并且如果并行的索要了内存，最后还是需要加锁判断内存是否分配完成
	//第一个向连接索要空间
	for(i=0; i<chunkBuff.allBuffNum[0]; i++)
	{
	chunkBuff.allBuff[i] = ApplyBuffFromServerConnection(pChunkTask ->waitedBlock[i]);

	}
	assert(i == chunkBuff.allBuffNum[0]);
	//第二段向本地索要空间
	for( ;i< chunkBuff.allBuffNum[1]; i++)
	{
		chunkBuff.allBuff[i] = ApplyBuffFromLocalData(pChunkTask->localTaskBlock[i]);
	}
	//第三段结合编码信息以及第一段第二段的位置信息得到编码系数K，计算之后发送给第三段
	if(i == chunkBuff.allBuffNum[2])
	{
		//处理直接发送数据,
		//将发送端的连接缓存指针替换为chunkBuff
		//ExchangeBuff((struct dataBuff*)&chunkBuff,pChunkTask,connfdClient);
		for(i=0;i< pChunkTask -> destIPNum;i++)//替换客户端缓存为前面两个部分的缓存，归还占有的缓存
		{
			SendBackMemory(connfdClient[i].pBuffPice);
			connfdClient[i].pBuffPice = chunkBuff.allBuff[i];
		}

	}
	else
	{
		//处理将数据编码后发送
		for(; i< chunkBuff.allBuffNum[2]; i++)
		{
			chunkBuff.allBuff[i] = ApplyBuffFromClientConnection(pChunkTask->destIP[0]);

		}
		EncodeData((struct dataBuff*)&chunkBuff,pChunkTask);
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
pConnect ApplyForClientConnection(pTaskBlock pChunkTask,int destNum)//依据目的ip地址索要连接，如果没有连接则建立连接
//连接的
{
	int connfd=0;
	char * destIP = pChunkTask.destIP[destNum];
	pConnect tmpConnect =NULL;
	list_head * pSearch = NULL;
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
		//此处需要添加全局锁，因为申请内存的过程必须是互斥的
		pthread_mutex_lock(&g_memoryLock);
		g_pFreeClientBuffList->pBuffPice = AskForMemory();
		pthread_mutex_unlock(&g_memoryLock);
	}

	DataTransportWrite(connfd,);

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
void EncodeData((struct dataBuff*)pChunkBuff,pTaskBlock pChunkTask)
{

	}
pSingleBuff AskForMemory()//向内存模块申请内存，需要加锁因为不同的访问必须是互斥的
{
	int baseNum = 20;
	int i =0;
	pMemory tmpMemory = NULL;
	char * tmpBuff = NULL;
	pSingleBuff tmpSingleBuff =NULL;
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
		tmpMemory->pBuffPice = tmpSingleBuff;
	}
	tmpMemory = container_of(g_pFreeMemoryList->listMemory.prev,nMemory,listMemory);
	tmpSingleBuff = tmpMemory ->pBuffPice;
	list_del(g_pFreeMemoryList->listMemory.prev);
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

	}
