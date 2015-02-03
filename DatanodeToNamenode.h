/*
 * DatanodeToNamenode.h
 *
 *  Created on: Jan 23, 2015
 *      Author: hadoop
 */

#ifndef DATANODETONAMENODE_H_
#define DATANODETONAMENODE_H_
void * DatanodeToNamenode(void * arg);
int DatanodeRegistOnNamenode(void);
int DatanodeControlwithNamenode(int sock_DtoN);
int TaskRecvFinished(char * localIPaddress);
void * ProcessTime(void * taskTime);
void ProcessTask(char *recvTaskBuff,long recv);//将任务抛给任务处理模块


#endif /* DATANODETONAMENODE_H_ */
