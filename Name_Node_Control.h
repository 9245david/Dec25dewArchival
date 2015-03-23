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
#include "DewHa.h"
#include "BlockStruct.h"
#include <stdint.h>
#ifndef RACK_NODE
#define RACK_NODE 6 //机架节点数,同时也是冗余码的K
#define RACK_NUM 3 //机架数,同时也是副本数
#endif
#ifndef IP_LENGTH
#define IP_LENGTH 15
#endif
#define NAME_PORT 6000
#define  BACKLOG 20

typedef struct RegistAndTaskFeedback{
//	 信息1：发送本地剩余带宽，剩余空间大小
//	 信息2：反馈分配的任务完成度
	double wholeBandwidth;//MB/s，总带宽
	double availableBandwidth;//可用带宽
	double wholeStorageSpace;//MB
	double availableStorageSpace;
	int finishedOrNot ;//如果为1表示分配的任务已经完成，为0表示没有完成，只有当第一次注册时默认为1不是这个含义
	int allocatedTask ;//被分配的任务数量，即数据块的个数
	int finishedTask ; //在预想时间内完成的数据块的个数
	int finishedTime ; //如果完成了，提前完成所花费的时间
}nFeedback,*pFeedback;
//extern typedef struct RegistAndTaskFeedback;
//extern typedef nFeedback;

/*
char g_nodeIP[RACK_NODE*RACK_NUM][IP_LENGTH];//节点号与节点ip
int g_nodeConnfd[RACK_NODE*RACK_NUM];
int g_nodeRegist = 0;//已经注册的节点个数，注册过程中加写锁
nFeedback g_nodeFeedback[RACK_NODE*RACK_NUM];//节点号与节点反馈信息
*/
unsigned char g_nodeIP[DATANODE_NUMBER][IP_LENGTH];//节点号与节点ip
int g_nodeConnfd[DATANODE_NUMBER];
int g_nodeRegist = 0;//已经注册的节点个数，注册过程中不需要加写锁，因为是while循环串行注册
nFeedback g_nodeFeedback[DATANODE_NUMBER];//节点号与节点反馈信息
int g_feedbackVersion[DATANODE_NUMBER] = {0};
bool ALL_DATANODE_CONNECTED = false;
struct timeval starttime;
pthread_t  *pthread_node_num = NULL;//与多个节点的通信（任务信息）handle_request
extern char *DatanodeTask[DATANODE_NUMBER];
//extern pTaskHead g_pDatanodeTask;
int NamenodeControlServer();
void *handle_request(void * arg);
int handle_connect(int listen_sock);
void ProcessDatanodeState(char * buff, long length, int connfd);
void SendTaskToDatanode(int connfd);
int TaskSendFinished(int connfd);
void WriteTaskFeedbackLog(int connfd, char *recvbuff, unsigned long length);
void NodeRegist(int nodeConnfd, char *nodeIP, int nodeNum);
int GetNodeIDFromConnfd(int connfd);
//bool VersionUpdated();

#ifndef PLANA
#define PLANA 1//不用流水线，在权重值分配后，直接选择数据块最多的节点作为编码节点
#define PLANB 0//采用流水线，在权重值分配后，采用流水线的方式编码
#endif

typedef struct TaskHead{
	int taskNum;//该节点的任务数量
	pTaskBlock singleStripTask;//指向描述任务的nTaskBlock结构体,会根据任务量申请一个结构体数组malloc
	bool historyNotFinished;//值为1表示上次的任务没有完成，并且预计接下来也完成不了，所以不给其分配任务
							//值为0表示正常的任务的完成情况
}nTaskHead,*pTaskHead;

#define TASK_TIME 20 //20s
#define PAST_TIME 20 //20s
//extern nFeedback g_nodeFeedback[DATANODE_NUMBER];
//extern int g_feedbackVersion[DATANODE_NUMBER];
//extern unsigned char g_nodeIP[DATANODE_NUMBER][IP_LENGTH];//节点号与节点i
int run_plan = 1;//默认执行ＰＬＡＮＡ
int g_weight[DATANODE_NUMBER];
pTaskHead g_pDatanodeTask =NULL;
int g_TaskStartBlockNum ;//全局,每次任务开始的第一个块
int g_versionNum = 1;
void ProvideTaskAlgorithm(int * g_weight,pTaskHead g_pDatanodeTask);
int ProvideTaskFinished();//生成任务结束,等于1结束，等于0未结束
void *ProvideTask(void *arg);
#endif /* NAME_NODE_CONTROL_H_ */
