/*
 * ArchivalManager.c
 *
 *  Created on: Jan 14, 2015
 *      Author: hadoop
 */
#include "DewHa.h"
#ifndef FEEDBACK
#define FEEDBACK
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
#endif
extern nFeedback g_nodeFeedback[DATANODE_NUMBER];
extern int g_feedbackVersion[DATANODE_NUMBER];

char * DatanodeTask[DATANODE_NUMBER];
void ProvideTask()
//利用反馈得到的g_nodeFeedback[DATANODE_NUMBER]信息生成每个节点的任务列表
//每一个datanode有一个人任务列表，列表是一长串字符串，字符串的第一个int表示该任务列表的长度，是sizeof(nTaskBlock)的整数倍
//
{
	while(VersionUpdated() == false);

	}

bool VersionUpdated()
{
	int i = 0;
	int j = 0;
	j = g_feedbackVersion[0];
	for(i = 1; i < DATANODE_NUMBER; i++)
	{
		if(j != g_feedbackVersion[i])return false;
	}
	return true;

	}
