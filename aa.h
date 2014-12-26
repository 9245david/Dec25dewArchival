#include<stdio.h>
#include<stdlib.h>
static inline int test()
{
	static int num =1;
	num ++;
	printf("num = %d\n",num);
	return num;
	}
