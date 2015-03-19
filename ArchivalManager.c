/*
 * ArchivalManager.c
 *
 *  Created on: Jan 14, 2015
 *      Author: hadoop
 */
#include "DewHa.h"
#include "BlockStruct.h"
#include <stdint.h>
#ifndef FEEDBACK
#define FEEDBACK
typedef struct RegistAndTaskFeedback{
//	 信息1：发送本地剩余带宽，剩余空间大小
//	 信息2：反馈分配的任务完成度
	uint64_t wholeBandwidth;//B/s，总带宽
	uint64_t availableBandwidth;//可用带宽
	uint64_t wholeStorageSpace;//B
	uint64_t availableStorageSpace;
	unsigned int finishedOrNot ;//如果为1表示分配的任务已经完成，为0表示没有完成，只有当第一次注册时默认为1不是这个含义
	unsigned int allocatedTask ;//被分配的任务数量，即数据块的个数
	unsigned int finishedTask ; //在预想时间内完成的数据块的个数
	unsigned int finishedTime ; //如果完成了，提前完成所花费的时间
}nFeedback,*pFeedback;
#endif

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
extern nFeedback g_nodeFeedback[DATANODE_NUMBER];
extern int g_feedbackVersion[DATANODE_NUMBER];
extern unsigned char g_nodeIP[DATANODE_NUMBER][IP_LENGTH];//节点号与节点i
int run_plan = 1;//默认执行ＰＬＡＮＡ
int g_weight[DATANODE_NUMBER];
pTaskHead g_pDatanodeTask =NULL;
int g_TaskStartBlockNum ;//全局,每次任务开始的第一个块
int g_versionNum = 1;
void ProvideTaskAlgorithm(int * g_weight,pTaskHead g_pDatanodeTask);
int ProvideTaskFinished();//生成任务结束,等于1结束，等于0未结束
void *ProvideTask(void *arg)
//利用反馈得到的g_nodeFeedback[DATANODE_NUMBER]信息生成每个节点的任务列表
//每一个datanode有一个人任务列表，列表是一长串字符串，字符串的第一个long表示该任务列表的长度，是sizeof(nTaskBlock)的整数倍
//
{
	int nodeNum = DATANODE_NUMBER;
	int i=0;
	g_pDatanodeTask = (pTaskHead)malloc(sizeof(nTaskHead)*DATANODE_NUMBER);
	assert(g_pDatanodeTask != NULL);
	g_TaskStartBlockNum = 0;//初始化块号
	while(ProvideTaskFinished()!= 1)
	{
		while(VersionUpdated() == false);
		if(VersionUpdated()==1)//第一次接收到反馈，即所有节点初始化过程
		{
			for(i=0;i<nodeNum;i++)
			{
				g_pDatanodeTask[i].taskNum = 0;
				g_weight[i] = g_nodeFeedback[i].availableBandwidth * TASK_TIME / BLOCK_SIZE;
				g_pDatanodeTask[i].historyNotFinished = false;//默认正常任务完成情况
				g_pDatanodeTask[i].singleStripTask = NULL;
			}
		}
		else{//之后的反馈过程
			for(i=0;i<nodeNum;i++)
			{
				g_pDatanodeTask[i].taskNum = 0;
				g_pDatanodeTask[i].singleStripTask = NULL;
				g_pDatanodeTask[i].historyNotFinished = false;//默认正常任务完成情况
				if(g_nodeFeedback[i].allocatedTask == 0)//上一个回合并没有分配任务
				{
					g_weight[i] = g_nodeFeedback[i].availableBandwidth * TASK_TIME / BLOCK_SIZE;

				}
				else if(g_nodeFeedback[i].finishedOrNot == 1)//任务已经完成，依据完成时间的比例得到下次发送的任务个数上限
				{
					g_weight[i] =	g_nodeFeedback[i].allocatedTask * TASK_TIME / g_nodeFeedback[i].finishedTime;
				}
				else//上一回合分配了任务，但是任务没有完成，只能以上次已经完成的块数作为本次的能力，能力减去为完成的工作
				{
					g_weight[i] = g_nodeFeedback[i].finishedTask*2 - g_nodeFeedback[i].allocatedTask;
					if(g_weight[i]<0)
						{
							g_weight[i]=0;
							g_pDatanodeTask[i].historyNotFinished = true;
						}
				}
			}//for
			}//else
	//	ProvideTaskAlgorithm(g_weight,g_TaskStartBlockNum,g_pDatanodeTask);
		//依据权重值，本次分配的起始块号，得到g_pDatanodeTask中存储的每个节点的任务情况
	//	g_TaskStartBlockNum = g_TaskStartBlockNum + EREASURE_N - EREASURE_K;//增加块号+6
		ProvideTaskAlgorithm(g_weight,g_pDatanodeTask);
		//依据权重值，本次分配的起始块号，得到g_pDatanodeTask中存储的每个节点的任务情况
	}//while
	return NULL;
}

bool VersionUpdated()
{
	int i = 0;
	int j = 0;
	j = g_versionNum;
	for(i = 1; i < DATANODE_NUMBER; i++)
	{
		if(j != g_feedbackVersion[i])return false;
	}
	g_versionNum++;
	return true;

	}
int ProvideTaskFinished()//生成任务结束,等于1结束，等于0未结束
{
	if(g_TaskStartBlockNum >= 996)return 1;
	else return 0;

	}

void ProvideTaskAlgorithm(int * g_weight,pTaskHead g_pDatanodeTask)
//依据起始块号和权重值信息，得到任务信息，终止条件为权值不能满足下一个条带的任务分配
{
	list_head * strp_lay_head = NULL ;

	int task[DATANODE_NUMBER][EREASURE_N - EREASURE_K+2];//与每个节点一一对应,0号位置存储节点号，1号位置存储节点数据块个数
	int node_num = 0;//参与任务的节点个数
	int block_num = 0;//单个节点上数据块的个数
	int node_ID = 0;//节点号
	int local_node_ID = 0;//本地节点号
	int i,j,k=0;
	list_head * weight_strp_lay = NULL;
	list_head * p_temp_node = NULL;
	list_head * p_temp_blk_head = NULL;//子链表头节点
	list_head * p_temp_blk = NULL;//子链表临时节点
	pTaskBlock p_temp_task_block = NULL;
	strp_lay_head = get_strp_lay(g_TaskStartBlockNum);

	weight_strp_lay = get_weight_strp_lay(strp_lay_head,g_weight);
	while(weight_strp_lay !=NULL)//一次循环对应一个条带的任务分配
	{
		p_temp_node = weight_strp_lay->next;
		node_num = 0;//初始化节点个数
		while(p_temp_node != weight_strp_lay)//遍历所有任务节点
		{
			p_temp_blk_head = &(container_of(p_temp_node,Nblk_inverted,listNode)->listblk);
			node_ID = container_of(p_temp_node,Nblk_inverted,listNode)->blk_nodeID;
			p_temp_blk = p_temp_blk_head->next;
			block_num = 0;//初始化节点上数据块个数,0号位置用来存储节点号
			task[node_num][0] = node_ID;
			while(p_temp_blk != p_temp_blk_head)//将该节点上的所有数据块添加在任务int task[18][6]里面
			{
				task[node_num][block_num+2] = container_of(p_temp_blk,Nblk_inverted,listblk)->blkID;
				p_temp_blk = p_temp_blk->next;
				block_num ++;
			}
			task[node_num][1] = block_num;
			node_num ++;
			p_temp_node = p_temp_node->next;
		}//while 遍历所有任务节点

		if(run_plan == PLANA)
			{
			node_ID = task[0][0];
			(g_pDatanodeTask +node_ID-1)->taskNum ++;
			block_num = task[0][1];
			(g_pDatanodeTask +node_ID-1)->singleStripTask = \
					realloc((g_pDatanodeTask +node_ID-1)->singleStripTask,sizeof(nTaskBlock));
			p_temp_task_block = (g_pDatanodeTask +node_ID-1)->singleStripTask;
			p_temp_task_block->chunkID = g_TaskStartBlockNum/(EREASURE_N-EREASURE_K);
			for(i=0;i<block_num;i++)p_temp_task_block->localTaskBlock[i] = task[0][i+2];
			p_temp_task_block->waitForBlock = EREASURE_N-EREASURE_K - block_num;
			p_temp_task_block->encode = 1;
			p_temp_task_block->waitedBlockType = 0;
			if(p_temp_task_block->waitForBlock > 0)
			{
				j=0;
				for(i=1;i<node_num;i++)
				{
					for(k=0;k<task[1][1];k++)
					{
						p_temp_task_block->waitedBlock[j++] = task[i][k+2];
					}
				}
				assert(j == p_temp_task_block->waitForBlock);
			}
			p_temp_task_block->destIPNum =0;
			//p_temp_task_block->destIP[];


				for(i=1; i<node_num; i++)
				{

					local_node_ID = task[i][0];
					(g_pDatanodeTask +local_node_ID-1)->taskNum ++;
					block_num = task[i][1];
					(g_pDatanodeTask +local_node_ID-1)->singleStripTask = \
							realloc((g_pDatanodeTask +local_node_ID-1)->singleStripTask,sizeof(nTaskBlock));
					p_temp_task_block = (g_pDatanodeTask +local_node_ID-1)->singleStripTask;
					p_temp_task_block->chunkID = g_TaskStartBlockNum/(EREASURE_N-EREASURE_K);
					for(i=0;i<block_num;i++)p_temp_task_block->localTaskBlock[i] = task[0][i+2];
					p_temp_task_block->waitForBlock = 0;
					p_temp_task_block->encode = 0;
					p_temp_task_block->waitedBlockType = 0;
					//p_temp_task_block->waitedBlock[j++] = 0;//不等待数据，无需接收数据
					p_temp_task_block->destIPNum =1;
					memcpy(p_temp_task_block->destIP[0],g_nodeIP[node_ID],IP_LENGTH);

				}
			}
			else//执行PLANB
			{

			}

		g_TaskStartBlockNum = g_TaskStartBlockNum + EREASURE_N - EREASURE_K;
		if(g_TaskStartBlockNum >= 996)return;
		strp_lay_head = get_strp_lay(g_TaskStartBlockNum);
		weight_strp_lay = get_weight_strp_lay(strp_lay_head,g_weight);

	}//while 一次循环对应一个条带的任务分配


	return ;
	}
