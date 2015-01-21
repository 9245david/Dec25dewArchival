/*
 * dewArchival.c
 *
 *  Created on: Dec 25, 2014
 *      Author: hadoop
 */
//#include "test_incline.h"
#include "aa.h"
int mainhh()
/*
 datanode与namenode通信模块，端口6000
 一/ datanode to namenode
 信息1：发送本地剩余带宽，剩余空间大小
 信息2：反馈分配的任务完成度
 二/ namenode to datanode
 信息1：分配的任务细节

 datanode client与datanode server 通信模块

 信息1：约定连接以及数据传输端口（通过发送控制信息），监听端口7000
       一定对应一个用于传输控制信息的server线程监听控制信息

 信息2：（端口约定规则，一个任务对应一个端口还是不同任务对应一个端口），传输数据，
        数据传输端口7000+Nodenum1+Nodenum2
        暂定牺牲一点性能，一次连接之对应一个条带的任务这样就不会存在两个节点因为不同条带的任务而同时存在多个连接

 client 与datanode server 通信模块,端口8000
 播放trace

 */
{
	//test();
//	test();
	printf("length is %d",strlen("hello"));
	return 0;
	}

