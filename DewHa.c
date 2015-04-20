/*
默认三副本
*/
#include "DewHa.h"

char *Node_bit_map=NULL;
//以节点为单位所有数据块分布位图,一个块对应一个bit,所有节点的位图，节点1开始，偏移0开始
char *Rack_bit_map=NULL;
//以机架为单位所有数据块分布位图，一个块对应两个bit，机架1开始，偏移0开始
Pblk_inverted g_Pblk_invert = NULL;
//对于所有已存在的数据块的位置反向索引，块标识1开始，偏移0开始
list_head  g_PclusterAchival[RACK_NODE*RACK_NUM];
//十八个节点的归档分布头节点
 int blk_id = 0;
int single_node_mapLength=0;
int main1(int argc, char **argv)
{
    int err=0;	
	//printf("0x80 size %d\n",sizeof(0x80));
    if(init_cluster()<0)
	{
		printf("init_cluster error \n");
	}
	printf("init sucesess \n");
	Print_cluster_lay();
	if(DEW_DEBUG==1)print_blk_invert();
	Heter_arch_lay();
	printf("Heter_arch_lay sucesess \n");
	 print_clusterAchival();
	 return 1;
}

int init_cluster()
{
    int i =0 ;
	int j=0;
	int local_blkID = 0;
	int blk_location = 0;//数据块分布在哪个节点1~18
	int rack_location = 0;//数据分块在哪个机架1~3
	int rack_replicate = 0;//在机架上的副本数
	int node_replicate = 0;//在节点上的副本数
	int node_blkOffset = 0;//单个节点上存储了多少个块，也是1开始，0是记录该节点块个数的
	int rack_blknum = 0; //单个机架上存储了多少个块
	char * p_node_bit = NULL;
	char * p_rack_bit = NULL;
//	printf("input the block num (<%d): ",NODE_BLKNUM*RACK_NODE*RACK_NUM/3);
//	scanf("%d",&blk_id);
	blk_id =6;
	if(blk_id%8==0)
	{
	    single_node_mapLength=blk_id/8;
	}
	else 
	{
		single_node_mapLength=blk_id/8+1;  
	}
	Node_bit_map = (char*)malloc(single_node_mapLength*RACK_NODE*RACK_NUM*sizeof(char));
	
	Rack_bit_map = (char*)malloc(single_node_mapLength*2*RACK_NUM*sizeof(char));
	
	g_Pblk_invert = (Pblk_inverted)malloc(blk_id*3*sizeof(Nblk_inverted));//三个副本
	
	if(NULL==Node_bit_map||NULL==Rack_bit_map||NULL==g_Pblk_invert)
	{
	    printf("bitmap malloc error\n");
		return -1;
	}
	
	memset(Node_bit_map,0,single_node_mapLength*RACK_NODE*RACK_NUM*sizeof(char));
	
	memset(Rack_bit_map,0,single_node_mapLength*2*RACK_NUM*sizeof(char));

		for(i=0;i<RACK_NODE*RACK_NUM;i++)//初始化每个节点块数为0
		{
			cluster_lay[0][i]=0;
		}
		for(j=1;j<NODE_BLKNUM+1;j++)
			for(i=0;i<RACK_NODE*RACK_NUM;i++)//初始化每个节点所有块标示为-1
			{
				cluster_lay[j][i]=-1;
			}
    srand((unsigned)time(NULL));
	
	for(local_blkID=1;local_blkID<=blk_id;local_blkID++)
	{
/***************************************************************************此处有遗漏问题
将单节点不存在剩余空间处理为整个集群不存在空间了

	//第一步：判断能存储数据，有一个机架上存在两个空闲节点，另一个机架上存在一个空闲节点，未完成....
	//可简化成每个节点上都存在一个空闲位置
*********************************************/
	    blk_location = rand()%(RACK_NODE*RACK_NUM)+1;//随机生成节点号1~18
	//	rack_replicate=check_rack_map((blk_location-1)/RACK_NODE+1,blk_id);
	    rack_location = (blk_location-1) / RACK_NODE +1; //节点所在机架号
	//	printf("96blk_location %d,rack_location %d\n",blk_location,rack_location);
	    if(cluster_lay[0][blk_location-1]<NODE_BLKNUM)//节点未满
		{
		    node_blkOffset = ++cluster_lay[0][blk_location-1];
		    cluster_lay[node_blkOffset][blk_location-1] = local_blkID;//第一个副本
			
			p_node_bit=(Node_bit_map+single_node_mapLength*(blk_location-1)+(local_blkID-1)/8);
			*p_node_bit = (*p_node_bit)|(0x80>>(local_blkID-1)%8);//更新node位图
	
			p_rack_bit = (Rack_bit_map+single_node_mapLength*(rack_location-1)+(local_blkID-1)/4);
			*p_rack_bit = (*p_rack_bit)|(0x40>>((local_blkID-1)%4*2));
			
			(g_Pblk_invert+3*(local_blkID-1)) -> blk_nodeID = blk_location; //更新 反向索引中的节点号
			(g_Pblk_invert+3*(local_blkID-1)) -> blk_node_offset = node_blkOffset; //更新 块节点偏移
			(g_Pblk_invert+3*(local_blkID-1)) -> blkID = local_blkID;//节点号可以不更新，反向索引本身自带节点标示，为了后续工作
			
		    blk_location = ((blk_location-1) % RACK_NODE +1) % RACK_NODE+1 + (rack_location-1)*RACK_NODE;//同一个机架上的下一个节点
			j =1;
			while(cluster_lay[0][blk_location-1]==NODE_BLKNUM)
			{
				if(j==RACK_NODE-1)
				{
					printf("随机到的节点空间已使用完114\n");
					return -2;//K=6时，同一机架上的另外5个节点均没有空间
				}
			    blk_location = ((blk_location-1) % RACK_NODE +1) % RACK_NODE+1 + (rack_location-1)*RACK_NODE;//同一个机架上的下一个节点
				j++;
				
			}
			node_blkOffset = ++cluster_lay[0][blk_location-1];
			cluster_lay[node_blkOffset][blk_location-1] = local_blkID;//第二个副本
			
			p_node_bit=(Node_bit_map+single_node_mapLength*(blk_location-1)+(local_blkID-1)/8);
			*p_node_bit = (*p_node_bit)|(0x1<<((8-(local_blkID-1)%8)-1));//更新node位图
			
			p_rack_bit = (Rack_bit_map+single_node_mapLength*(rack_location-1)+(local_blkID-1)/4);
			*p_rack_bit = (*p_rack_bit)^(0xE0>>((local_blkID-1)%4*2));
			
			(g_Pblk_invert+3*(local_blkID-1)+1) -> blk_nodeID = blk_location; //更新 反向索引中的节点号
			(g_Pblk_invert+3*(local_blkID-1)+1) -> blk_node_offset = node_blkOffset; //更新 块节点偏移
			(g_Pblk_invert+3*(local_blkID-1)+1) -> blkID = local_blkID;//数据块号可以不更新，反向索引本身自带节点标示，为了后续工作
			
		}
		else 
			{
				printf("随机到的节点空间已使用完 133\n");
				return -3;//随机到的节点空间已使用完
			}
			
		rack_location = ((rack_location-1)%RACK_NUM+1)%RACK_NUM+1;
		
		blk_location = (blk_location-1)% RACK_NODE+1+(rack_location-1)*RACK_NODE;//下一个机架上的同等位置节点
	//	printf("136 rack_location= %d,blk_location = %d,cluster_lay[0][blk_location-1]=%d\n",\
			rack_location,blk_location,cluster_lay[0][blk_location-1]);
		if(cluster_lay[0][blk_location-1]<NODE_BLKNUM)//节点未满
		{
	//		printf("141\n");
		    node_blkOffset = ++cluster_lay[0][blk_location-1];
		//	printf("node_blkOffset=%d\n",node_blkOffset);
		    cluster_lay[node_blkOffset][blk_location-1] = local_blkID;//第三个副本
		//	printf("143\n");
			p_node_bit=(Node_bit_map+single_node_mapLength*(blk_location-1)+(local_blkID-1)/8);
		//	printf("p_node_bit %c\n",*p_node_bit);
			*p_node_bit = (*p_node_bit)|(0x80>>(local_blkID-1)%8);//更新node位图
		//	printf("146\n");
			p_rack_bit = (Rack_bit_map+single_node_mapLength*(rack_location-1)+(local_blkID-1)/4);
			*p_rack_bit = (*p_rack_bit)|(0x40>>((local_blkID-1)%4*2));
			
			(g_Pblk_invert+3*(local_blkID-1)+2) -> blk_nodeID = blk_location; //更新 反向索引中的节点号
			(g_Pblk_invert+3*(local_blkID-1)+2) -> blk_node_offset = node_blkOffset; //更新 块节点偏移
			(g_Pblk_invert+3*(local_blkID-1)+2) -> blkID = local_blkID;//节点号可以不更新，反向索引本身自带节点标示，为了后续工作
	//		printf("149\n");
		}
		else 
			{
				printf("随机到的节点空间已使用完152\n");
				return -3;//随机到的节点空间已使用完
			}
	}
	
	return 1;
    
}

int check_node_map(int nodeNum,int blkID)//依据节点号(K=6时1~18)和块编号判断块是否存在该节点0,1
{
	char * p_node_bit = NULL;
	char result=0;
	//char * p_rack_bit = NULL;
	p_node_bit = (Node_bit_map+single_node_mapLength*(nodeNum-1)+(blkID-1)/8);
	result = (*p_node_bit)>>((8-(blkID-1)%8)-1)&0x1;//更新node位图
	return (int)result;
}

int check_rack_map(int rackNum,int blkID)//依据机架号(1~3)和块编号判断块在该机架上的个数，限制为0,1,2
{
    char * p_rack_bit = NULL;
	char result=0;
    p_rack_bit = (Rack_bit_map+single_node_mapLength*(rackNum-1)+(blkID-1)/4);
	result = ((*p_rack_bit)&(0xE0>>(blkID-1)%4*2))>>((6-blkID-1)%4*2);
	return (int)result;
}

int Heter_arch_lay()//此处RACK_NODE 实际应为K值，正好都为6，代替
{
	int strp_num=0;//striple num
	int strp_id = 0;//条带标识
	int node_id = 0;//节点标识
	int strp_innerID=0;//某个blkID在 其所在条带上对应的位置0~5
	int local_blkID = 0;//本地变量，块标示
	int tempN=0;//  
	int tempB=0;
	int i=0;
	//int *locate = NULL;
	Pblk_inverted strp_lay=NULL;//条带分布十字链表
	Pblk_inverted temp_strp_lay=NULL;
	list_head *strp_lay_head=NULL;//条带分布十字链表头指针,内无数据
	for(i=0;i<RACK_NODE*RACK_NUM;i++)//初始化全局归档链表
	{
		 init_list_head(&(g_PclusterAchival[i]));
	}
	
	strp_num = blk_id /6;
	strp_lay_head  = (list_head *)malloc(sizeof(list_head));
	if(NULL==strp_lay_head){printf("error\n");return -1;}
	//init_list_head(strp_lay_head);
	for(strp_id=1;strp_id<=strp_num;strp_id++)//对于每一个条带
		{
			init_list_head(strp_lay_head);//每次for循环后的节点内存均未释放
			tempN=0;
			for(node_id=1;node_id<=RACK_NODE*RACK_NUM;node_id++)//遍历每个节点，判断每个块在不在
			{
				tempB=0;
				for(strp_innerID=0;strp_innerID<RACK_NODE;strp_innerID++)//条带上的每个块
				{
					local_blkID = (strp_id-1)*RACK_NODE+strp_innerID+1;//此处RACK_NODE 实际应为K值，正好为6
					if(check_node_map(node_id,local_blkID)>0)
					{
						
						
						if(tempN==0&&tempB==0)//十字链表第一个节点，第一个数据块
						{
							strp_lay = (Pblk_inverted)malloc(sizeof(Nblk_inverted));
							//strp_lay_head->next = &(strp_lay->listNode);
							//init_list_head(&(strp_lay->listNode));//第一个节点
							
							init_list_head(&(strp_lay->listblk));//子链表的头
							list_add_tail(&(strp_lay->listNode),strp_lay_head);//第一个节点
							temp_strp_lay = (Pblk_inverted)malloc(sizeof(Nblk_inverted));
							list_add_tail(&(temp_strp_lay->listblk),&(strp_lay->listblk));
							get_blk_offset(local_blkID,node_id,temp_strp_lay);//向temp_strp_lay所指向的节点中填充快号，节点号，偏移
							strp_lay->blk_nodeID = node_id;
							
						}
						    else if(tempN!=0&&tempB==0)//十字链表非首数据节点，第一个数据块
							{
							 	strp_lay = (Pblk_inverted)malloc(sizeof(Nblk_inverted));
								list_add_tail(&(strp_lay->listNode),strp_lay_head);//不是第一个节点，新增节点
								init_list_head(&(strp_lay->listblk));//子链表的头
								
								temp_strp_lay = (Pblk_inverted)malloc(sizeof(Nblk_inverted));
								list_add_tail(&(temp_strp_lay->listblk),&(strp_lay->listblk));
								get_blk_offset(local_blkID,node_id,temp_strp_lay);//向strp_lay所指向的节点中填充快号，节点号，偏移
								strp_lay->blk_nodeID = node_id;
								 
							}
							else //非第一数据块
							{
								 temp_strp_lay = (Pblk_inverted)malloc(sizeof(Nblk_inverted));
								 list_add_tail(&(temp_strp_lay->listblk),&(strp_lay->listblk));//非第一数据块,循环的上一次添加数据一定是同一个数据结点，所以此处的strp_lay可用
								 get_blk_offset(local_blkID,node_id,temp_strp_lay);//向strp_lay所指向的节点中填充快号，节点号，偏移
							}
							
						tempB++;
						
					}

			    }
				if(tempB!=0)tempN++;
			}
			//locate =(int*)malloc(tempN*2*sizeof(int));
			if(DEW_DEBUG==1)print_double_circular(strp_lay_head);
			least_blk(strp_lay_head);
		}
		printf("Heter_arch_lay sucess");
		return 1;
}

list_head *get_strp_lay(int task_start_block_num)//依据起始块号得到该条带的分布.listNode,十字链表首节点无数据，只有节点号
//task_start_block_num默认初始块号为0开始，但是local_blkID是1开始，此处写的有点纠结
{
	int tempN=0;//
	int tempB=0;
	//int strp_id = 0;//条带标识
	int node_id = 0;//节点标识
	int strp_innerID=0;//某个blkID在 其所在条带上对应的位置0~5
	int local_blkID = 0;//本地变量，块标示
	Pblk_inverted strp_lay=NULL;//条带分布十字链表
	Pblk_inverted temp_strp_lay=NULL;
	list_head *strp_lay_head=NULL;//条带分布十字链表头指针,内无数据
	strp_lay_head  = (list_head *)malloc(sizeof(list_head));
	assert(strp_lay_head != NULL);
	init_list_head(strp_lay_head);//初始化条带分布链表头
	tempN=0;
	for(node_id=1;node_id<=RACK_NODE*RACK_NUM;node_id++)//遍历每个节点，判断每个块在不在
		{
			tempB=0;
			for(strp_innerID=0;strp_innerID<RACK_NODE;strp_innerID++)//条带上的每个块
				{
					local_blkID = task_start_block_num+strp_innerID+1;//此处RACK_NODE 实际应为K值，正好为6
					if(check_node_map(node_id,local_blkID)>0)
					{
						if(tempN==0&&tempB==0)//十字链表第一个节点，第一个数据块
						{
							strp_lay = (Pblk_inverted)malloc(sizeof(Nblk_inverted));
							//strp_lay_head->next = &(strp_lay->listNode);
							//init_list_head(&(strp_lay->listNode));//第一个节点
							init_list_head(&(strp_lay->listblk));//子链表的头
							list_add_tail(&(strp_lay->listNode),strp_lay_head);//第一个节点
							temp_strp_lay = (Pblk_inverted)malloc(sizeof(Nblk_inverted));
							list_add_tail(&(temp_strp_lay->listblk),&(strp_lay->listblk));
							get_blk_offset(local_blkID,node_id,temp_strp_lay);//向temp_strp_lay所指向的节点中填充快号，节点号，偏移
							strp_lay->blk_nodeID = node_id;

						}
						    else if(tempN!=0&&tempB==0)//十字链表非首数据节点，第一个数据块
							{
							 	strp_lay = (Pblk_inverted)malloc(sizeof(Nblk_inverted));
								list_add_tail(&(strp_lay->listNode),strp_lay_head);//不是第一个节点，新增节点
								init_list_head(&(strp_lay->listblk));//子链表的头

								temp_strp_lay = (Pblk_inverted)malloc(sizeof(Nblk_inverted));
								list_add_tail(&(temp_strp_lay->listblk),&(strp_lay->listblk));
								get_blk_offset(local_blkID,node_id,temp_strp_lay);//向strp_lay所指向的节点中填充快号，节点号，偏移
								strp_lay->blk_nodeID = node_id;

							}
							else //非第一数据块
							{
								 temp_strp_lay = (Pblk_inverted)malloc(sizeof(Nblk_inverted));
								 list_add_tail(&(temp_strp_lay->listblk),&(strp_lay->listblk));//非第一数据块,循环的上一次添加数据一定是同一个数据结点，所以此处的strp_lay可用
								 get_blk_offset(local_blkID,node_id,temp_strp_lay);//向strp_lay所指向的节点中填充快号，节点号，偏移
							}

						tempB++;

					}

			    }
				if(tempB!=0)tempN++;
			}
	if(DEW_DEBUG==1)print_double_circular(strp_lay_head);
	return strp_lay_head;
}
list_head *get_weight_strp_lay(list_head* strp_lay_head,int * weight)
//依据三副本的条带数据块分布，和节点权重值表，得到对应的归档条带任务节点数据块分布。
//假设nodeID 是由1开始的
{
//	list_head * weight_strp_lay = NULL;
	list_head * p_temp_node = NULL;
	int i = 0;
	int j = 0;
	int flag = 0;// 冒泡过程有交换数据则为0,没有交换数据则为1,没有交换即退出
	int nodeID = 0; //节点号
	int node_num = 0;//包含了数据块的节点数目
	int node_weight_tmp = 0;
	int block_id = 0;//系统block_id 由1开始
	int block_num = 0;//等于6时说明条带的任务列表生成完毕
	int block_exist[EREASURE_N-EREASURE_K] = {0};//块已经存在对应的条带内偏移为1,否则为0
	P_node_weight p_node_weight_head = NULL;//权值链表首节点
	P_node_weight p_node_weight_tmp = NULL;//中间临时节点
	P_node_weight p_node_weight_tail = NULL;//尾节点
	P_node_weight p1,p2;//节点权重值冒泡排序

	list_head * task_strp_lay_head = NULL;//任务链表条带分布
	Pblk_inverted node_strp_block=NULL;//一个节点上的块链表
	Pblk_inverted tmp_node_strp_block=NULL;
	list_head *p_task_sort1,*p_task_sort2,*p_task_sort3;//用于生成的任务节点的排序
	int length2,length3;//生成的任务节点的块个数，也用于排序
	p_node_weight_head = (P_node_weight)malloc(sizeof(N_node_weight));
	assert(p_node_weight_head !=NULL);
	p_node_weight_head->next = NULL;
	p_node_weight_tail = p_node_weight_head;
	//weight_strp_lay = (list_head*)malloc(sizeof(list_head));
//	assert(weight_strp_lay !=NULL);
//	init_list_head(weight_strp_lay);
	p_temp_node = strp_lay_head->next;
	while(p_temp_node != strp_lay_head)//将有数据的节点权重值选出来，存在
	{
		nodeID = container_of(p_temp_node,Nblk_inverted,listNode)->blk_nodeID;
		p_node_weight_tmp = (P_node_weight)malloc(sizeof(N_node_weight));
		p_node_weight_tmp->node_num = nodeID;
		p_node_weight_tmp->node_weight = weight[nodeID-1];
		p_node_weight_tmp->p_node_in_strp = &(container_of(p_temp_node,Nblk_inverted,listNode)->listblk);
		p_node_weight_tmp->next = NULL;

		p_node_weight_tail->next = p_node_weight_tmp;
		p_node_weight_tail = p_node_weight_tail->next;

		node_num++;
		p_temp_node = p_temp_node->next;

	}
	//按权重值由大到小排序,冒泡排序
	for(i=node_num;i>1;i--)
	{
		flag = 1;//默认无交换，循环退出
		p1 = p_node_weight_head->next;
		p2 = p1->next;
		if(node_num == 1)break;

		for(j=0;j<i-1;j++)
		{
			if(p1->node_weight < p2->node_weight)
			{
				flag = 0;
				nodeID = p1->node_num;
				p1->node_num = p2->node_num;
				p2->node_num = nodeID;

				node_weight_tmp = p1->node_weight;
				p1->node_weight = p2->node_weight;
				p2->node_weight = node_weight_tmp;

				p_temp_node = p1->p_node_in_strp;
				p1->p_node_in_strp = p2->p_node_in_strp;
				p2->p_node_in_strp = p_temp_node;

			}
			p1 = p1->next;
			p2 = p2->next;
		}
		if(flag == 1)break;
	}
	if(DEW_DEBUG ==1)print_weight_strp_lay(p_node_weight_head);
	//权重值和分布得到任务
	task_strp_lay_head = (list_head *)malloc(sizeof(list_head));
	init_list_head(task_strp_lay_head);//初始化任务链表头
	p1 = p_node_weight_head->next;
	while(block_num != (EREASURE_N-EREASURE_K))
	{
		//p1 = p_node_weight_head->next;
		//assert(p1 != NULL);//权值节点遍历一遍之后数据还没有找全
		if(p1 == NULL)
		{
			if(DEW_DEBUG == 1)printf("分配完毕，无权重值满足要求\n");
			return NULL;//权值节点遍历一遍之后数据还没有找全,即任务的分配结束
		}
		node_weight_tmp = p1->node_weight;
		nodeID = p1->node_num;
		//p_node_weight_head->next = p1->next;
		p_temp_node = p1->p_node_in_strp->next;
	//free(p1);

	node_strp_block = (Pblk_inverted)malloc(sizeof(Nblk_inverted));
	assert(node_strp_block !=NULL);
	list_add_tail(&(node_strp_block->listNode),task_strp_lay_head);
	node_strp_block->blk_nodeID = nodeID;
	init_list_head(&(node_strp_block->listblk));
	while((p_temp_node != p1->p_node_in_strp)&&(node_weight_tmp > 0))//权值还有，节点上的数据也还有,添加某个节点上的数据块
	{
		block_id = container_of(p_temp_node,Nblk_inverted,listblk)->blkID;
		if(block_exist[(block_id-1)%(EREASURE_N-EREASURE_K)] != 1)
		{
			tmp_node_strp_block = (Pblk_inverted)malloc(sizeof(Nblk_inverted));
			list_add_tail(&(tmp_node_strp_block->listblk),&(node_strp_block->listblk));
			tmp_node_strp_block->blk_node_offset = container_of(p_temp_node,Nblk_inverted,listblk)->blk_node_offset;
			tmp_node_strp_block->blkID = container_of(p_temp_node,Nblk_inverted,listblk)->blkID;
			tmp_node_strp_block->blk_nodeID = nodeID;
			node_weight_tmp--;
			block_exist[(block_id-1)%(EREASURE_N-EREASURE_K)] = 1;
			block_num ++;
		}

		p_temp_node = p_temp_node->next;
	}
	p1->node_weight = node_weight_tmp;
	g_weight[nodeID-1] = node_weight_tmp;//权重值会更新，每次分配一个条带之后
	p1 = p1->next;
	}
	//给节点权重值进行排序，降序排列
	node_num  =0;
	p_temp_node = task_strp_lay_head->next;
	while(p_temp_node != task_strp_lay_head)
	{
		node_num ++;
		p_temp_node = p_temp_node->next;
	}
	p_task_sort1 = p_task_sort2=p_task_sort3=NULL;
	for(i = node_num; i>1; i--)//冒泡排序将该条带的任务数据块分布由数据块由多至少排列
	{
		p_task_sort1 = task_strp_lay_head;
		p_task_sort2 = p_task_sort1->next;
		p_task_sort3 = p_task_sort2->next;
		flag =1;//默认无交换，循环退出
		for(j = 0; j < i-1; j++)
		{
			length2 = get_length(&(container_of(p_task_sort2,Nblk_inverted,listNode)->listblk));
			length3 = get_length(&(container_of(p_task_sort3,Nblk_inverted,listNode)->listblk));
			if(length2 < length3)
			{
				p_task_sort1->next = p_task_sort3;
				p_task_sort2->next = p_task_sort3->next;
				p_task_sort3->next = p_task_sort2;
				p_task_sort3->prev = p_task_sort1;
				p_task_sort2->prev = p_task_sort3;
				p_task_sort2->next->prev = p_task_sort2;
				flag = 0;
			}
			p_task_sort1 = p_task_sort1->next;
			p_task_sort2 = p_task_sort1->next;
			p_task_sort3 = p_task_sort2->next;
		}
	//	assert(p_task_sort3 == task_strp_lay_head);
		if(flag == 1)break;//没有交换产生
	}
	if(DEW_DEBUG ==1)
		{
			printf("task lay\n");
			print_double_circular(task_strp_lay_head);
		}

	return task_strp_lay_head;

	}
int print_weight_strp_lay(P_node_weight p_node_weight_head)//打印权重值链表，为单项链表
{
	P_node_weight p_node_weight_tmp = NULL;
	int node_num = 0;
	assert(p_node_weight_head != NULL);
	p_node_weight_tmp = p_node_weight_head->next;
	while(p_node_weight_tmp != NULL)
	{
		assert(node_num <= DATANODE_NUMBER);
		printf("nodeID is %d,node_weight is %d \n ",p_node_weight_tmp->node_num,p_node_weight_tmp->node_weight);
		p_node_weight_tmp = p_node_weight_tmp->next;
		node_num ++;
	}	
	return 1;
}
int get_blk_offset(int local_blkID,int node_id,Pblk_inverted strp_lay)//将数据块号，节点号，以及得到的节点偏移存在strp_lay指向的结构体中
{
	strp_lay->blk_nodeID = node_id;//块所在节点
	strp_lay->blkID = local_blkID;//数据块标识	
	if((g_Pblk_invert+3*(local_blkID-1))->blk_nodeID==node_id)
		{							
			strp_lay->blk_node_offset=(g_Pblk_invert+3*(local_blkID-1))->blk_node_offset;
		}
		else if((g_Pblk_invert+3*(local_blkID-1)+1)->blk_nodeID==node_id)
		{							
			strp_lay->blk_node_offset=(g_Pblk_invert+3*(local_blkID-1)+1)->blk_node_offset;
		}
		else if((g_Pblk_invert+3*(local_blkID-1)+2)->blk_nodeID==node_id)
		{
			strp_lay->blk_node_offset=(g_Pblk_invert+3*(local_blkID-1)+2)->blk_node_offset;
		}
		else 
		{
			printf("g_Pblk_invert error\n");
			return -1;
		}
	return 1;
}

list_head *check_nodeInList(list_head *p_head_check,list_head *p_blk)
//判断p_blk指针对应的结构体中存在和以p_head_check为头指针的链表中相同的数据块号，否则返回null
{
	list_head *p_temp_head=NULL;
	Pblk_inverted pTemp_check=NULL;
	Pblk_inverted pTemp_blk=NULL;
	p_temp_head=p_head_check->next;
	while(p_temp_head!=p_head_check)
	{
		/*if(container_of(p_temp_head,Nblk_inverted,listblk)->blkID==container_of(p_blk,Nblk_inverted,listblk)->blkID)
		{
			printf("id is %d ,node is %d\n",container_of(p_temp_head,Nblk_inverted,listblk)->blkID,container_of(p_temp_head,Nblk_inverted,listblk)->blk_nodeID);
			return p_temp_head;
		}*/
		pTemp_check = container_of(p_temp_head,Nblk_inverted,listblk);
		pTemp_blk = container_of(p_blk,Nblk_inverted,listblk);
		if(pTemp_check->blkID==pTemp_blk->blkID)
		{
			if(DEW_DEBUG==1)printf("id is %d ,node is %d\n",pTemp_check->blkID,pTemp_check->blk_nodeID);
			return p_temp_head;
		}
		p_temp_head=p_temp_head->next;
	}
	return NULL;
}

int get_length(list_head *p_head)
//得到链表长度,不算空表头
{
	int i;
	list_head *p_temp_head=NULL;
	
	p_temp_head=p_head->next;
	for(i=1;p_temp_head!=p_head;i++)
	{
		p_temp_head=p_temp_head->next;
	}
	return i-1;
}
/*******************/
/*		
		依据十字链表得到数据分布,双重指针
		考虑到可能是十字链表首节点删除
		将删除的存储节点添加到g_PclusterAchival中
		删除其他节点中重复数据块（可能是所有数据块）
		*/
int least_blk(list_head* strp_lay_head)
{
	int nodeCount =0;//含有该条带数据块的节点个数
	int nodeBlkCount = 0;//单节点块个数
	int maxBlkCount = 0;//最大块数 
	int maxBlkNode = 0;//最大块数的节点号
	int totalBlkCount = 0;//收集的块个数上限K=6
	list_head *p_Head_node=NULL;//每个子链的listNode 头节点
	list_head *p_max_node = NULL;//最长的子链listNode头结点
	list_head *p_temp_node = NULL;//删除节点时的临时指针
	list_head *p_temp_blk=NULL;
	list_head *p_max_blk=NULL;//最长的子链第一个有数据listblk
	list_head *p_del_blk=NULL;//用于遍历最长子链，依次在剩余十字链表中删除
	
	list_head *p_repDel = NULL;//需要删除的重复数据块标识listblk
	int i=0;
	p_Head_node = strp_lay_head->next;
	
	nodeCount = get_length(strp_lay_head);
	p_temp_blk = &(container_of(p_Head_node,Nblk_inverted,listNode)->listblk);
	while(totalBlkCount<RACK_NODE)
	{
		maxBlkCount=0;
		p_Head_node = strp_lay_head->next;
		
		nodeCount = get_length(strp_lay_head);
		p_temp_blk = &(container_of(p_Head_node,Nblk_inverted,listNode)->listblk);
		for(i=1;i<=nodeCount;i++)//寻找最大
		{
			nodeBlkCount=get_length(p_temp_blk);
			if(maxBlkCount<nodeBlkCount)
			{
				maxBlkCount=nodeBlkCount;
			//	maxBlkNode = i;//此处错误，是机器节点号，不是链表中的第几个节点
				maxBlkNode = container_of(p_Head_node,Nblk_inverted,listNode)->blk_nodeID;
				p_max_node = p_Head_node;
				p_max_blk = p_temp_blk;
			}
			p_Head_node = p_Head_node->next;//下一个节点
			p_temp_blk = &(container_of(p_Head_node,Nblk_inverted,listNode)->listblk);
		}
		
		//if(maxBlkNode==1)strp_lay_head->next = p_max_node->next;//将十字链表头节点下移
		list_del(p_max_node);//将数据块最多的节点在十字链表中删除
		/*if(g_PclusterAchival[maxBlkNode-1]->next==NULL)init_list_head(p_max_node);//将最大的插入到归档链中，如果是头节点
			else list_add_tail(p_max_node,g_PclusterAchival[maxBlkNode-1]->next);//如果不是头节点
			*/
		list_add_tail(p_max_node, (list_head*)&(g_PclusterAchival[maxBlkNode-1]));//将最大的插入到归档链中
		
		for(p_del_blk=p_max_blk->next;p_del_blk!=p_max_blk;p_del_blk=p_del_blk->next)//删除重复
		{
			p_Head_node=strp_lay_head->next;
			while(p_Head_node!=strp_lay_head)//每个listNode中均删除
			{
				p_temp_blk = &(container_of(p_Head_node,Nblk_inverted,listNode)->listblk);//子链的头节点指针
				p_repDel=check_nodeInList(p_temp_blk,p_del_blk);
				if(p_repDel!=NULL)
				{
					if(p_repDel->next==p_repDel->prev)//子链唯一的节点
					{

						list_del(p_repDel);//删除子链结点
						free(container_of(p_repDel,Nblk_inverted,listblk));
					//	list_del(p_temp_blk);//删除头结点
					//	free(container_of(p_temp_blk,Nblk_inverted,listblk));
					p_temp_node = p_Head_node;
					p_Head_node=p_Head_node->next;
					
						//list_del(p_temp_node);//删除十字链表节点链中的空链,此处删除后for循环无法进行
						//	free(container_of(p_temp_node,Nblk_inverted,listNode));
							
					}
					else 
					{
						list_del(p_repDel);
						p_Head_node=p_Head_node->next;
					}
			//		printf("ss");
					//print_double_circular(strp_lay_head);
				}
				
				else p_Head_node=p_Head_node->next;
			//	printf("delte\n");
			}
		}
		totalBlkCount+=maxBlkCount;
	}
	if(DEW_DEBUG==1)print_double_circular(strp_lay_head);
	printf("least_blk ok\n");
	return 1;
}

int Print_cluster_lay()//打印集群节点数据块分布
{
	int i =0;
	int j =0;
	int cluster_line =0;
	cluster_line = cluster_lay[0][0];
	printf("第一行为每个节点中数据的块数\n");
	for(i=1;i<18;i++)
	{
		if(cluster_line<cluster_lay[0][i])cluster_line=cluster_lay[0][i];
	}
	for(j=1;j<=18;j++)
	{
		printf("N%-3d ",j);
	}
	printf("\n");
	for(j=0;j<=cluster_line;j++)
	{
		for(i=0;i<18;i++)
		{
			printf("%-4d ",cluster_lay[j][i]);
		}
		if(j==0)printf("\n");
		printf("\n");
	}
	return 1;
}

int print_clusterAchival()
{
	int i = 0;
	list_head *p_Head_node=NULL;//每个子链的listNode 头节点
	list_head *p_Head_blk=NULL;
	list_head *p_temp_blk=NULL;
	Pblk_inverted pTemp=NULL;
	for(i=0;i<RACK_NODE*RACK_NUM;i++)
	{
		printf("node %4d: ",i);
		for(p_Head_node=g_PclusterAchival[i].next;p_Head_node!=(list_head *)&(g_PclusterAchival[i]);p_Head_node=p_Head_node->next)
		{
			pTemp = container_of(p_Head_node,Nblk_inverted,listNode);
			p_Head_blk = &(pTemp->listblk);
			for(p_temp_blk=p_Head_blk->next;p_temp_blk!=p_Head_blk;p_temp_blk=p_temp_blk->next)
			{
				printf ("%4d ",container_of(p_temp_blk,Nblk_inverted,listblk)->blkID);
			}
		}
		printf("\n");
	}
	return 1;
}

int print_blk_invert()
{
	int i = 0;
	Pblk_inverted PTemp=NULL;
	for(i=0;i<3*blk_id;i++)
	{
		PTemp=g_Pblk_invert+i;
		printf("blkID %d,node %d,offset %d \n",PTemp->blkID,PTemp->blk_nodeID,PTemp->blk_node_offset);
		
	}
	printf("print_blk_invert ok\n");
	return 1;
}

int print_double_circular(list_head* strp_lay_head)
{
	list_head *p_Head_node=NULL;//每个子链的listNode 头节点
	list_head *p_Head_blk=NULL;//每个子链的listblk 头节点
	list_head *p_temp_blk=NULL;//每个子链的listblk 临时指针
	Pblk_inverted pTemp=NULL;
	p_Head_node = strp_lay_head->next;
	while(p_Head_node!=strp_lay_head)
	{
		pTemp = container_of(p_Head_node,Nblk_inverted,listNode);
		p_Head_blk = &(pTemp->listblk);
		p_temp_blk = p_Head_blk->next;
		while(p_temp_blk!=p_Head_blk)
		{
			pTemp = container_of(p_temp_blk,Nblk_inverted,listblk);
			printf(" node=%d blkID=%d ",pTemp->blk_nodeID,pTemp->blkID);
			p_temp_blk = p_temp_blk->next;
		}
		printf("\n");
		
		p_Head_node=p_Head_node->next;
	}
	return 1;
}

