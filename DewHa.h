/*
 * DewHa.h
 *
 *  Created on: Jan 14, 2015
 *      Author: hadoop
 */

#ifndef DEWHA_H_
#define DEWHA_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "time.h"
#include <assert.h>
#include "dew_list.h"
#include "stdbool.h"
#define RACK_NODE 6 //机架节点数,同时也是冗余码的K
#define RACK_NUM 3 //机架数,同时也是副本数

#ifndef DATANODE_NUMBER
#define DATANODE_NUMBER 18
#endif

#ifndef EREASURE_N
#define EREASURE_N 9 // 编码参数 RS（n,k）
#define EREASURE_K 3 //码参数 RS（n,k）
#endif

#define BLK_SIZE (64*1024*1024UL) //数据块大小
#define NODE_BLKNUM 10000 //单节点数据块个数

#ifndef TASK_END
#define TASK_END 24
#endif
//#define DEW_DEBUG 1
#define DEW_DEBUG 2
typedef struct block_lay{
	int32_t blk_nodeID;//块所在节点号
	int32_t blk_node_offset;//块所在节点内的偏移（以块为单位）
	int32_t blkID;//数据块的标示
	int32_t blk_next_node;//归档下个节点
	list_head listNode;//连接不同的节点
	list_head listblk;//连接不同的数据块
}Nblk_inverted,*Pblk_inverted;
typedef struct nodeWeight{
        int32_t node_num;
        int32_t node_weight;
        list_head *  p_node_in_strp;//节点在条带分布链表中的位置
        struct nodeWeight *next;
}N_node_weight,*P_node_weight; 
int32_t cluster_lay[NODE_BLKNUM+1][RACK_NODE*RACK_NUM];//整体布局,0行为节点数据块实际存放个数
//
//char *Node_bit_map=NULL;
////以节点为单位所有数据块分布位图,一个块对应一个bit,所有节点的位图，节点1开始，偏移0开始
//char *Rack_bit_map=NULL;
////以机架为单位所有数据块分布位图，一个块对应两个bit，机架1开始，偏移0开始
//Pblk_inverted g_Pblk_invert = NULL;
////对于所有已存在的数据块的位置反向索引，块标识1开始，偏移0开始
//list_head  g_PclusterAchival[RACK_NODE*RACK_NUM];
////十八个节点的归档分布头节点
// int blk_id = 0;
//int single_node_mapLength=0;
int32_t check_node_map(int32_t nodeNum,int32_t blkID);
//依据节点号(K=6时1~18)和块编号判断块是否存在该节点0,1

int32_t check_rack_map(int32_t rackNum,int32_t blkID);
//依据机架号(1~3)和块编号判断块在该机架上的个数，限制为0,1,2

int32_t init_cluster();
//初始化集群数据块布局，同时初始化两个位图。单机架双副本，第三副本在另外的机架，单节点不能存双副本

int32_t Print_cluster_lay();//打印集群节点数据块分布

int32_t Heter_arch_lay();//异构归档分布
int32_t get_blk_offset(int32_t local_blkID,int32_t node_id,Pblk_inverted strp_lay);//将数据块号，节点号，以及得到的节点偏移存在strp_lay指向的结构体中
list_head *check_nodeInList(list_head *p_head_check,list_head *p_blk);
//判断p_blk指针对应的结构体中存在和以p_head_check为头指针的链表中相同的数据块号，否则返回null
int32_t get_length(list_head *p_head);
//得到链表长度

int32_t least_blk(list_head* strp_lay_head);
/*
		依据十字链表得到数据分布,双重指针
		考虑到可能是十字链表首节点删除
		将删除的存储节点添加到g_PclusterAchival中
		删除其他节点中重复数据块（可能是所有数据块）
		*/

int32_t print_clusterAchival();
int32_t print_blk_invert();
int32_t print_double_circular(list_head* strp_lay_head);//打印双向循环链表

//下面为新添加的内容
bool VersionUpdated();
list_head *get_strp_lay(int32_t task_start_block_num);//依据起始块号得到该条带的分布
list_head *get_weight_strp_lay(list_head* strp_lay_head,int32_t * weight);//依据权重值以及条带分布得到任务节点以及数据块的分布
int32_t print_weight_strp_lay(P_node_weight p_node_weight_head);//打印权重值链表
#endif /* DEWHA_H_ */
