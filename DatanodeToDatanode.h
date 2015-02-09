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
#define DATANODE_PORT 6001
#define  BACKLOG 20
void * ProcessChunkTask(void* argv);
pSingleBuff ApplyBuffFromServerConnection(unsigned int waitedBlockID);
int ApplyForClientConnection(unsigned char * destIP);//依据目的ip地址索要连接
pSingleBuff ApplyBuffFromLocalData(int localBlock);
pSingleBuff AskForMemory();
void* DataToDataTaskServer(void*arg);
void TraslateTaskToTransport(pTaskBlock pChunkTask,int destNum,pTransportBlock pChunkTransport);
void TraslateTaskToServer(pTaskBlock pChunkTask,int destNum,pTransportBlock pChunkTransport);

#endif /* DATANODETODATANODE_H_ */