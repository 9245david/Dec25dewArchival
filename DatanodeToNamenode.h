/*
 * DatanodeToNamenode.h
 *
 *  Created on: Jan 23, 2015
 *      Author: hadoop
 */

#ifndef DATANODETONAMENODE_H_
#define DATANODETONAMENODE_H_
void * DatanodeToNamenode(void * arg);
int32_t DatanodeRegistOnNamenode(void);
int32_t DatanodeControlwithNamenode(int32_t sock_DtoN);
int32_t TaskRecvFinished(char * localIPaddress);
void * ProcessTime(void * taskTime);
void ProcessTask(char *recvTaskBuff,int64_t recv);//将任务抛给任务处理模块
int32_t run_plan = 1;
#ifndef DEW_DEBUG
#define DEW_DEBUG 1
#endif
#ifndef PLANA
#define PLANA 1
#define PLANB 0
#endif

#endif /* DATANODETONAMENODE_H_ */
