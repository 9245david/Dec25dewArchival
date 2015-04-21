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
#define DEW_DEBUG 1
typedef struct NodeStatus{
double ability_netwidth;//MB/s
double available_space;//disk space MB
double work_percent;//allocated job has finished ** percent
}NodeStatus,*PNodeStatus;

typedef struct NamenodeID{
char *Namenode_ADDR;
int NameToDataPort;
}NamenodeID,*PNamenodeID,ServerNodeID,*pServerNodeID;
PNamenodeID  PnamenodeID;
int ClientConnectToServer(PNamenodeID arg);
long DataTransportRead(int sock_fd,char * buffer,long length);
long DataTransportWrite(int sock_fd,char * buffer,long length);

#endif /* SOCKET_CONNECT_READ_WRITE_H_ */
