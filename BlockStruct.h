/*
 * BlockStruct.h
 *
 *  Created on: Jan 12, 2015
 *      Author: hadoop
 *（1） 在结构体 typedef struct taskBlock{……}中，假设了三个条件
 *     条件一：接受的数据不会是数据块和校验块的混合体
 *     条件三：ＩＰ地址字符个数固定
 *     条件二：目的地址只有一个
 *（2） 在结构体 transportBlock 中，
 *		是否应该有一个char blockData[transportSize];存储数据，
 *		但是C不允许定义变长数组
 *
 *（3） 偏移 off_t：32位机器下默认是32位长，这时无法对大于4G的文件偏移操作，这时off_t = __off_t；如果想进行大于4G的文件偏移操作，可以在程序中加入头文件之前定义
#define _FILE_OFFSET_BITS 64
这时off_t = __off64_t，具体定义在unistd.h中；对于64位机，默认就是64位长。
 */

#ifndef BLOCKSTRUCT_H_
#define BLOCKSTRUCT_H_
#define BLOCK_SIZE (64*1024*1024UL)
#define BUFF_SIZE (8*1024*1024UL)//内存片大小8M
#define BUFF_PICE_SIZE (1024*1024UL)//每次传输的数据大小1M
#define EREASURE_N 9 // 编码参数 RS（n,k）
#define EREASURE_K 3 //码参数 RS（n,k）
#define IP_LENGTH 15//192.168.0.111,长度会变化192.168.11.111
typedef struct taskBlock{
	int chunkID;
	//条带号
	int localTaskBlock[EREASURE_N];
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
	unsigned int waitedBlockType;
		//块类型标识；
		//为0表示数据块。为（1~2k-1）时，表示为校验块，且每个bit位都代表一个参与校验的块。
		//条带有k个数据块。
	unsigned int waitedBlock[EREASURE_N];
	  	//等待的Block只能是数据块或者校验块，不能二者混杂在一起
		//如果是校验块，存放顺序依次为P1‘，P2’，……，Pk‘但是无法标记为哪些块生成的校验块，由传输端确定是哪些校验
	unsigned int destIPNum;//一般情况下均为1,除了最终编码节点的目标ip为多个
	unsigned char destIP[3][IP_LENGTH];
		//目的ＩＰ地址，字符串的形式存储带字符串结束符

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
	unsigned int chunkID;
	//代表条带号
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


#endif /* BLOCKSTRUCT_H_ */
