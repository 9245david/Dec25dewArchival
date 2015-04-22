/*
 * Socket_connect_read_write.h
 *
 *  Created on: Dec 29, 2014
 *      Author: hadoop
 */

#ifndef SOCKET_CONNECT_READ_WRITE_H_
#include "aa.h"
#define SOCKET_CONNECT_READ_WRITE_H_
#define DATA_TO_NAME_PORT 6000
//#define NAME_TO_DATA_PORT 6000 //no need
//#define DEW_DEBUG 1
#define DEW_DEBUG 2
typedef struct NodeStatus{
double ability_netwidth;//MB/s
double available_space;//disk space MB
double work_percent;//allocated job has finished ** percent
}NodeStatus,*PNodeStatus;

typedef struct NamenodeID{
char *Namenode_ADDR;
int32_t NameToDataPort;
}NamenodeID,*PNamenodeID,ServerNodeID,*pServerNodeID;
PNamenodeID  PnamenodeID;
int32_t ClientConnectToServer(PNamenodeID arg);
int64_t DataTransportRead(int32_t sock_fd,char * buffer,int64_t length);
int64_t DataTransportWrite(int32_t sock_fd,char * buffer,int64_t length);

#endif /* SOCKET_CONNECT_READ_WRITE_H_ */
