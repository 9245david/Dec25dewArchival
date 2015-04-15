/*
 * 头文件
#include <sys/types.h>
#include <sys/ipc.h>
函数原型：
key_t ftok( const char * fname, int id )
fname就是你指定的文件名（已经存在的文件名），一般使用当前目录，如：
key_t key;
key = ftok(".", 1); 这样就是将fname设为当前目录。
id是子序号。虽然是int类型，但是只使用8bits(1-255）。
在一般的UNIX实现中，是将文件的索引节点号取出，前面加上子序号得到key_t的返回值。
如指定文件的索引节点号为65538，换算成16进制为0x010002，而你指定的ID值为38，换算成16进制为0x26，则最后的key_t返回值为0x26010002。
查询文件索引节点号的方法是： ls -i
当删除重建文件后，索引节点号由操作系统根据当时文件系统的使用情况分配，因此与原来不同，所以得到的索引节点号也不同。
如果要确保key_t值不变，要么确保ftok的文件不被删除，要么不用ftok，指定一个固定的key_t值
**************************
（1）创建一个信号量或访问一个已经存在的信号量集。
int semget(key_t key, int nsems, int semflg);
该函数执行成功返回信号量标示符，失败返回-1
参数key是通过调用ftok函数得到的键值，nsems代表创建信号量的个数，如果只是访问而不创建则可以指定该参数为0，我们一旦创建了该信号量，就不能更改其信号量个数，只要你不删除该信号量，你就是重新调用该函数创建该键值的信号量，该函数只是返回以前创建的值，不会重新创建；
semflg 指定该信号量的读写权限，当创建信号量时不许加IPC_CREAT ，若指定IPC_CREAT |IPC_EXCL则创建是存在该信号量，创建失败。
//IPC_CREAT 如果共享内存不存在，则创建一个共享内存，否则打开操作。
//IPC_EXCL  只有在共享内存不存在的时候，新的共享内存才建立，否则就产生错误。
 ********************
（2）设置信号量的值（PV操作）
 * int semop(int  semid, struct  sembuf  *opsptr, size_t  nops);
		(1 semid： 是semget返回的semid
		(2 opsptr： 指向信号量操作结构数组
		(3  nops ： opsptr所指向的数组中的sembuf结构体的个数

	struct sembuf {
    	short sem_num;    // 要操作的信号量在信号量集里的编号，
    	short sem_op;     // 信号量操作
    	short sem_flg;     // 操作表示符
	};
		(4  若sem_op 是正数，其值就加到semval上，即释放信号量控制的资源
    若sem_op 是0，那么调用者希望等到semval变为0，如果semval是0就返回;
若sem_op 是负数，那么调用者希望等待semval变为大于或等于sem_op的绝对值
例如，当前semval为2，而sem_op = -3，那么怎么办？
注意：semval是指semid_ds中的信号量集中的某个信号量的值
		(5  sem_flg
    		SEM_UNDO     由进程自动释放信号量
    		IPC_NOWAIT    不阻塞
   *******************
  (3)int semctl(int  semid, int  semum, int  cmd, ..);

	semid是信号量集合；
	semnum是信号在集合中的序号；
	semum是一个必须由用户自定义的结构体，在这里我们务必弄清楚该结构体的组成：
	union semun
	{
    	int val;                // cmd == SETVAL
    	struct semid_ds *buf     // cmd == IPC_SET或者 cmd == IPC_STAT
    	ushort *array;          // cmd == SETALL，或 cmd = GETALL
	};
	val只有cmd ==SETVAL时才有用，此时指定的semval = arg.val。
	注意：当cmd == GETVAL时，semctl函数返回的值就是我们想要的semval。千万不要以为指定的semval被返回到arg.val中。
    array指向一个数组，当cmd==SETALL时，就根据arg.array来将信号量集的所有值都赋值；当cmd ==GETALL时，就将信号量集的所有值返回到arg.array指定的数组中。
	buf指针只在cmd==IPC_STAT或IPC_SET时有用，作用是semid所指向的信号量集（semid_ds机构体）。一般情况下不常用，这里不做谈论。
	另外，cmd == IPC_RMID还是比较有用的。从内核中删除信号量集合
 */
#ifndef SEMTOOL_H_
#define SEMTOOL_H_
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define ERR_EXIT(m) \
        do \
        { \
                perror(m); \
                exit(EXIT_FAILURE); \
        } while(0)

union semun
{
    int val;                  /* value for SETVAL */
    struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
    unsigned short *array;    /* array for GETALL, SETALL */
    /* Linux specific part: */
    struct seminfo *__buf;    /* buffer for IPC_INFO */
};
int sem_create(key_t key,int nsems);// 依据key值,创建个数为nsems的信号量
int sem_openid(key_t key);//由Ｋｅｙ至得到信号集描述符
int sem_p(int semid, short sem_num);//对信号量集合数组的sem_num号位置进行Ｐ操作（0～信号量数数-1）
int sem_v(int semid, short sem_num);//对信号量集合数组的sem_num号位置进行V操作（0～信号量数数-1）
int sem_d(int semid);//删除信号量集合
int sem_setall(int semid, unsigned short *array);//设置所有的信号量
int sem_setval(int semid, short sem_num, int val);//设置sem_num的信号量的值
int sem_getval(int semid, short sem_num);
int sem_setmode(int semid, char *mode);

int sem_create(key_t key,int nsems)
// 依据key值,创建个数为nsems的信号量
{
    int semid = semget(key, nsems, 0666 | IPC_CREAT | IPC_EXCL);
    if (semid == -1)
        ERR_EXIT("semget");

    return semid;
}

int sem_openid(key_t key)//由Ｋｅｙ至得到信号集描述符
{
    int semid = semget(key, 0, 0);
    if (semid == -1)
        ERR_EXIT("semget");

    return semid;
}

int sem_p(int semid, short sem_num)//对信号量集合数组的sem_num号位置进行Ｐ操作（0～信号量数数-1）
{
    struct sembuf sb = {sem_num, -1, /*IPC_NOWAIT*/SEM_UNDO};
    int ret = semop(semid, &sb, 1);
    if (ret == -1)
        ERR_EXIT("semop dew");

    return ret;
}

int sem_v(int semid, short sem_num)//对信号量集合数组的sem_num号位置进行V操作（0～信号量数数-1）
{
    struct sembuf sb = {sem_num, 1, /*0*/SEM_UNDO};
    int ret = semop(semid, &sb, 1);
    if (ret == -1)
        ERR_EXIT("semop dew");

    return ret;
}

int sem_d(int semid)//删除信号量集合
{
    int ret = semctl(semid, 0, IPC_RMID, 0);
    if (ret == -1)
        ERR_EXIT("semctl");
    return ret;
}
int sem_setall(int semid, unsigned short *array)//设置所有的信号量
{
	 union semun arg;
	 arg.array = array;
	 int ret = semctl(semid, 0, SETALL, arg);
	 if (ret == -1)
	         ERR_EXIT("semctl  SETALL");
	     return ret;
	}
int sem_setval(int semid, short sem_num, int val)//设置sem_num的信号量的值
{
    union semun arg;
    arg.val = val;
    int ret = semctl(semid, sem_num, SETVAL, arg);
    if (ret == -1)
        ERR_EXIT("semctl");

    printf("value updated...\n");
    return ret;
}

int sem_getval(int semid, short sem_num)
{
    int ret = semctl(semid, sem_num, GETVAL, 0);
    if (ret == -1)
        ERR_EXIT("semctl");

    printf("current val is %d\n", ret);
    return ret;
}

int sem_getmode(int semid)
{
    union semun su;
    struct semid_ds sem;
    su.buf = &sem;
    int ret = semctl(semid, 0, IPC_STAT, su);
    if (ret == -1)
        ERR_EXIT("semctl");

    printf("current permissions is %o\n", su.buf->sem_perm.mode);
    return ret;
}

int sem_setmode(int semid, char *mode)
{
    union semun su;
    struct semid_ds sem;
    su.buf = &sem;

    int ret = semctl(semid, 0, IPC_STAT, su);
    if (ret == -1)
        ERR_EXIT("semctl");

    printf("current permissions is %o\n", su.buf->sem_perm.mode);
    sscanf(mode, "%o", (unsigned int *)&su.buf->sem_perm.mode);
    ret = semctl(semid, 0, IPC_SET, su);
    if (ret == -1)
        ERR_EXIT("semctl");

    printf("permissions updated...\n");

    return ret;
}

void usage(void)
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "semtool -c\n");
    fprintf(stderr, "semtool -d\n");
    fprintf(stderr, "semtool -p\n");
    fprintf(stderr, "semtool -v\n");
    fprintf(stderr, "semtool -s <val>\n");
    fprintf(stderr, "semtool -g\n");
    fprintf(stderr, "semtool -f\n");
    fprintf(stderr, "semtool -m <mode>\n");
}

int mainSEM(int argc, char *argv[])
{
    int opt;

    opt = getopt(argc, argv, "cdpvs:gfm:");
    if (opt == '?')
        exit(EXIT_FAILURE);
    if (opt == -1)
    {
        usage();
        exit(EXIT_FAILURE);
    }

    key_t key = ftok(".", 's');
    int semid;
    switch (opt)
    {
    case 'c':
        sem_create(key,1);
        break;
    case 'p':
        semid = sem_openid(key);
        sem_p(semid, 0);
        sem_getval(semid, 0);
        break;
    case 'v':
        semid = sem_openid(key);
        sem_v(semid, 0);
        sem_getval(semid, 0);
        break;
    case 'd':
        semid = sem_openid(key);
        sem_d(semid);
        break;
    case 's':
        semid = sem_openid(key);
        sem_setval(semid, 0, atoi(optarg));
        break;
    case 'g':
        semid = sem_openid(key);
        sem_getval(semid, 0);
        break;
    case 'f':
        semid = sem_openid(key);
        sem_getmode(semid);
        break;
    case 'm':
        semid = sem_openid(key);
        sem_setmode(semid, argv[2]);
        break;
    }

    return 0;
}

#endif
