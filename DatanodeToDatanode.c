/*
 * DatanodeToDatanode.c
 *
 *  Created on: Jan 23, 2015
 *      Author: hadoop
 *
 *      本模块实现功能，流水线处理任务，管理处理任务过程中的内存table以及datanode节点之间的连接table
 *      typedef struct taskBlock{
	unsigned int localTaskBlock[EREASURE_N];
		//本地需要读取的数据块，长度信息以数组中的结束标志int值自带
		//数组未满部分以-1填充
	unsigned int waitForBlock;
		//值大于1 表示waitedBlock[EREASURE_N]非空，
		//值为0表示本地数据可以直接发送
	unsigned int encode;
		//值为1 表示waitedBlock[EREASURE_N]为EREASURE_K个校验块，
		//值为0表示waitedBlock[ERASURE_N]为等带的数据块，
		//长度信息以数组中的结束标志int自带
		//此时会发现int waitForBlock参数为非必要参数，因为长度为0即表示无需等待
	unsigned int waitedBlock[EREASURE_N];
	  	//等待的Block只能是数据块或者校验块，不能二者混杂在一起
		//如果是校验块，存放顺序依次为P1‘，P2’，……，Pk'但是无法标记为哪些块生成的校验块，由传输端确定是哪些校验
	unsigned char destIP[IP_LENGTH];
		//目的ＩＰ地址，

}nTaskBlock,*pTaskBlock;

typedef struct transportBlock{
	unsigned int blockType;
	//块类型标识；
	//为0表示数据块。为（1~2k-1）时，表示为校验块，且每个bit位都代表一个参与校验的块。
	//条带有k个数据块。
	unsigned int blockID;
	//块号；
	//数据块号，编号为1~∝，
	//当<块类型标识>显示该块为校验块时，块号信息仅代表所属条带，因为块已被编码
	unsigned int parityID;
	//代表校验块的第几块，因为校验块只能是P1~Pk,所以其值为1~k
	unsigned int offsetInsideBlock;
	//因为网络中传输的块不是一个个完整的块。所以会有其在数据块内偏移，
	//例如64MB的大块被分割为1MB的小块，偏移量标识为0~63
	unsigned long transportSize;
	//传输块大小；
	//实际传输时的块大小，可能为暂定1MB,1024*1024 Byte，为可调整参数

	//此结构体中是否应该有一个char blockData[transportSize];存储数据，但是C不允许定义变长数组
}nTransportBlock,*pTransportBlock;
 */
#include "DatanodeToDatanode.h"
#include "BlockStruct.h"
typedef struct dataBuff{
	char ** allBuff;
	int allBuffNum[3];//分割为三段，第一段为接收数据buff 为0即为空,第二段为本地数据buff，第三段为发送数据buff


};
void * ProcessChunkTask(void* argv)
{
	pTaskBlock pChunkTask = (pTaskBlock)argv;//某一个块的任务

	//本地数据块
	int * localBlock = NULL;
	int localNum = 0;
	int waitBlockNum = 0;
	struct dataBuff chunkBuff;
	localBlock = pChunkTask ->localTaskBlock;
	while(*(localBlock+localNum) != -1)
	{
		localNum ++;
	}
	chunkBuff.allBuffNum[0] = pChunkTask -> waitForBlock;
	chunkBuff.allBuffNum[1] = chunkBuff.allBuffNum[0] + localNum;
	if(pChunkTask->encode != 0)chunkBuff.allBuffNum[2] = chunkBuff.allBuffNum[1]+EREASURE_K;
	chunkBuff.allBuff = (char**)malloc(chunkBuff.allBuffNum[2]*sizeof(char*));
	assert(chunkBuff.allBuff == NULL);
	第一个向连接索要空间
	第二段向本地索要空间
	第三段结合编码信息以及第一段第二段的位置信息得到编码系数K，计算之后发送给第三段
	return NULL;

}
