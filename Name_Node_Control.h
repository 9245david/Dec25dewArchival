/*
 * Name_Node_Control.h
 *
 *  Created on: Jan 7, 2015
 *      Author: hadoop
 *      RACK_NODE最开始的定义在 DewHa.h里面，该定义与aa.h中DATANODE_NUMBER定义重复
 *      IP_LENGTH在BlockStruct.h里面有定义
 */

#ifndef NAME_NODE_CONTROL_H_
#define NAME_NODE_CONTROL_H_
#include "Socket_connect_read_write.h"

#ifndef RACK_NODE
#define RACK_NODE 6 //机架节点数,同时也是冗余码的K
#define RACK_NUM 3 //机架数,同时也是副本数
#endif
#ifndef IP_LENGTH
#define IP_LENGTH 13
#endif
#define NAME_PORT 6000
#define  BACKLOG 20

int NamenodeControlServer();
void *handle_request(void * arg);
int handle_connect(int listen_sock);
void ProcessDatanodeState(char * buff, long length, int connfd);
void SendTaskToDatanode(int connfd);
int TaskSendFinished(int connfd);
void WriteTaskFeedbackLog(int connfd, char *recvbuff, unsigned long length);
void NodeRegist(int nodeConnfd, char *nodeIP, int nodeNum);
int GetNodeIDFromConnfd(int connfd);
bool VersionUpdated();
#endif /* NAME_NODE_CONTROL_H_ */
