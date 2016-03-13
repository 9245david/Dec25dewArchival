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
#include <semaphore.h>
#ifndef RACK_NODE
#define RACK_NODE 3 //机架节点数,同时也是冗余码的K
#define RACK_NUM 3 //机架数,同时也是副本数
#endif
#ifndef IP_LENGTH
#define IP_LENGTH 15
#endif
#define NAME_PORT 6780
#define  BACKLOG 20

#pragma pack(push)
#pragma pack(1)
typedef struct RegistAndTaskFeedback{
//	 信息1：发送本地剩余带宽，剩余空间大小
//	 信息2：反馈分配的任务完成度
	double wholeBandwidth;//MB/s，总带宽
	double availableBandwidth;//可用带宽
	double wholeStorageSpace;//MB
	double availableStorageSpace;
	int32_t finishedOrNot ;//如果为1表示分配的任务已经完成，为0表示没有完成，只有当第一次注册时默认为1不是这个含义
	int32_t allocatedTask ;//被分配的任务数量，即数据块的个数
	int32_t finishedTask ; //在预想时间内完成的数据块的个数
	int32_t finishedTime ; //如果完成了，提前完成所花费的时间
        int32_t finishedBlock;//12-17修改，当前窗口已完成数据块
        int32_t unfinishedBlock;//12-17修改，当前窗口未完成数据块unfinishedblock=allocatedblock+上次未完成-finishedblock
        int32_t allocatedBlock;//12-17修改，当前窗口分配的数据块
        int32_t finishedNet;//1-21修改，当前窗口已完成网络数据块
        int32_t unfinishedNet;//1-21修改，当前窗口未完成网络unfinishednet=allocatednet+上次未完成-finishednet
        int32_t allocatedNet;//1-21修改，当前窗口分配的网络数据块
}nFeedback,*pFeedback;
//extern typedef struct RegistAndTaskFeedback;
//extern typedef nFeedback;
#pragma pack(pop)
/*
char g_nodeIP[RACK_NODE*RACK_NUM][IP_LENGTH];//节点号与节点ip
int g_nodeConnfd[RACK_NODE*RACK_NUM];
int g_nodeRegist = 0;//已经注册的节点个数，注册过程中加写锁
nFeedback g_nodeFeedback[RACK_NODE*RACK_NUM];//节点号与节点反馈信息
*/
unsigned char g_nodeIP[DATANODE_NUMBER][IP_LENGTH];//节点号与节点ip
int32_t g_nodeConnfd[DATANODE_NUMBER];
int32_t g_nodeRegist = 0;//已经注册的节点个数，注册过程中不需要加写锁，因为是while循环串行注册
nFeedback g_nodeFeedback[DATANODE_NUMBER] = {0};//节点号与节点反馈信息
int32_t g_feedbackVersion[DATANODE_NUMBER] = {0};
bool ALL_DATANODE_CONNECTED = false;
struct timeval starttime;
pthread_t  *pthread_node_num = NULL;//与多个节点的通信（任务信息）handle_request
extern char *DatanodeTask[DATANODE_NUMBER];
//extern pTaskHead g_pDatanodeTask;
int32_t NamenodeControlServer();
void *handle_request(void * arg);
int32_t handle_connect(int32_t listen_sock);
void ProcessDatanodeState(char * buff, int64_t length, int32_t connfd);
void SendTaskToDatanode(int32_t connfd);
int32_t TaskSendFinished(int32_t connfd);
//void WriteTaskFeedbackLog(int32_t connfd, char *recvbuff, uint64_t length);
void WriteTaskFeedbackLog(int32_t connfd,char *recvbuff,uint64_t length,int32_t node_id,struct timeval recv_feedback_time);
void NodeRegist(int32_t nodeConnfd, char *nodeIP, int32_t nodeNum);
int32_t GetNodeIDFromConnfd(int32_t connfd);
//bool VersionUpdated();

#ifndef PLANA
#define PLANA 1//不用流水线，在权重值分配后，直接选择数据块最多的节点作为编码节点
#define PLANB 0//采用流水线，在权重值分配后，采用流水线的方式编码
#endif

typedef struct TaskHead{
	int32_t taskNum;//该节点的任务数量
	pTaskBlock singleStripTask;//指向描述任务的nTaskBlock结构体,会根据任务量申请一个结构体数组malloc
	bool historyNotFinished;//值为1表示上次的任务没有完成，并且预计接下来也完成不了，所以不给其分配任务
							//值为0表示正常的任务的完成情况
}nTaskHead,*pTaskHead;

#define TASK_TIME 40 //20s
#define PAST_TIME 40 //20s
//extern nFeedback g_nodeFeedback[DATANODE_NUMBER];
//extern int g_feedbackVersion[DATANODE_NUMBER];
//extern unsigned char g_nodeIP[DATANODE_NUMBER][IP_LENGTH];//节点号与节点i
int32_t run_plan = 1;//默认执行ＰＬＡＮＡ
int32_t g_weight[DATANODE_NUMBER];
pTaskHead g_pDatanodeTask =NULL;
int32_t g_TaskStartBlockNum ;//全局,每次任务开始的第一个块
int32_t g_versionNum = 1;
int ProvideTaskAlgorithm(int32_t * g_weight,pTaskHead g_pDatanodeTask);
int32_t ProvideTaskFinished();//生成任务结束,等于1结束，等于0未结束
void *ProvideTask(void *arg);
int  print_task_block(pTaskBlock p_temp_task_block);
void delete_tail_node(list_head * weight_strp_lay);
#endif /* NAME_NODE_CONTROL_H_ */
