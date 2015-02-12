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

typedef struct TaskHead{
	int taskNum;//该节点的任务数量
	pTaskBlock singleStripTask;//指向描述任务的nTaskBlock结构体
	bool historyNotFinished;//值为1表示上次的任务没有完成，并且预计接下来也完成不了，所以不给其分配任务
							//值为0表示正常的任务的完成情况
}nTaskHead,pTaskHead;

#define TASK_TIME 20 //20s
#define PAST_TIME 20 //20s
extern nFeedback g_nodeFeedback[DATANODE_NUMBER];
extern int g_feedbackVersion[DATANODE_NUMBER];
int g_weight[DATANODE_NUMBER];
pTaskHead g_pDatanodeTask =NULL;
int g_TaskStartBlockNum ;//全局,每次任务开始的第一个块
int g_versionNum = 1 ;
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
				g_weight[i] = g_nodeFeedback[i].availableBandwidth * TASK_TIME / BLOCK_SIZE;
			}
		}
		else{//之后的反馈过程
			for(i=0;i<nodeNum;i++)
			{
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
					if(g_weight[i]<0)g_weight[i]=0;
				}
			}//for
			}//else
		ProvideTaskAlgorithm(g_weight,g_TaskStartBlockNum,g_pDatanodeTask);
		//依据权重值，本次分配的起始块号，得到g_pDatanodeTask中存储的每个节点的任务情况
		g_TaskStartBlockNum = g_TaskStartBlockNum + EREASURE_N - EREASURE_K;//增加块号+6
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

void ProvideTaskAlgorithm(int * g_weight,int g_TaskStartBlockNum,pTaskHead g_pDatanodeTask)
{

	return ;
	}
