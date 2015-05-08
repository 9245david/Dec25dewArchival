/*
 * DatanodeToDatanode.h
 *
 *  Created on: Jan 23, 2015
 *      Author: hadoop
 */

#ifndef DATANODETODATANODE_H_
#define DATANODETODATANODE_H_
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "BlockStruct.h"
#include "Socket_connect_read_write.h"
#include "dew_list.h"
#define DATANODE_PORT 6781
#define  BACKLOG 50
#ifndef DEW_DEBUG
//#define DEW_DEBUG 1
#define DEW_DEBUG 2
#endif
typedef struct buffDiscript{
	char * buff;
	int32_t start;
	int32_t end;
	int32_t length;//已经存的pice数目
	int64_t buffSize;//8MB
	int64_t pice;//每一片buff的大小
	pthread_mutex_t buffLock;
}*pSingleBuff,nSingleBuff;


typedef struct connectClientQueue{
	pSingleBuff pBuffPice;//内存片
	char * IP_ADDR;//地址，服务器端或者客户端
	int connfd;//连接套接字
	list_head listConnect;//连接不同的连接套接字
}*pConnect,nConnect;
/*typedef struct dataBuff{
	pConnect * allBuff;
	int allBuffNum[3];//分割为三段，第一段为接收数据buff 为0即为空,第二段为本地数据buff，第三段为发送数据buff
};*/
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
	int32_t localBlock;//块号
}*pLocalData,nLocalData;

void * ProcessChunkTask(void* argv);
pSingleBuff ApplyBuffFromServerConnection(uint32_t waitedBlockID);
pConnect ApplyForClientConnection(pTaskBlock pChunkTask,int32_t destNum);//依据目的ip地址索要连接
pSingleBuff ApplyBuffFromLocalData(int32_t localBlock);
pSingleBuff AskForMemory();
void SendBackMemory(pSingleBuff pBuffPice);
void* DataToDataTaskServer(void*arg);
void TraslateTaskToTransport(pTaskBlock pChunkTask,int32_t destNum,pTransportBlock pChunkTransport);
void TraslateTaskToServer(pTaskBlock pChunkTask,int32_t destNum,pTransportBlock pChunkTransport);
void *ReadLocalData(void * arg);
void *SendData(void*arg);
void EncodeData(pConnectServer* connfdServer,pSingleBuff* pLocalBuff,pConnect* connfdClient,pTaskBlock pChunkTask);
int32_t handle_connect(int32_t listen_sock);//返回0正常，返回其他值，失败
void *handle_request(void * arg);
off_t FindBlockOffset(int32_t localBlock);
uint64_t ReadDisk(off_t offset, char * buff, uint64_t length);

#endif /* DATANODETODATANODE_H_ */
