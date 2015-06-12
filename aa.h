//#include<stdio.h>
//#include<stdlib.h>
//static inline int test()
//{
//	static int num =1;
//	num ++;
//	printf("num = %d\n",num);
//	return num;
//	}
#ifndef AA_H_
#define AA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>
#include <stdbool.h>
#define DATANODE_NUMBER 9
#define DATA_NAME_MAXLENGTH 10000ul// namenode 与datanode相互通信时，控制信息最大长度
//因为还没有具体的传输控制信息结构体，所以用该参数的地方都需要修改，然后最好设计成每次传输的任务长度是一样的
//
#endif
