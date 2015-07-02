#include "Socket_connect_read_write.h"

//NamenodeID namenodeID={
//"127.0.0.1";
//6000;
//};

//extern PnamenodeID;
//PNamenodeID PnamenodeID = (PNamenodeID)(&namenodeID);

//返回sockfd,输入参数arg,可以将其设置为namenodeID的指针，
//这样就不需要用全局变量了namenodeID
int32_t ClientConnectToServer(PNamenodeID arg)
{
int32_t sock_fd;
int32_t i = 0;
int32_t t[5] = {1,2,4,8,16};
struct sockaddr_in srvaddr;
//struct sockaddr_in cliaddr;
if((sock_fd=socket(AF_INET,SOCK_STREAM,0))==-1)

	{

		perror("create socket error\n");

		exit(-1);

	}
/*给服务器地址赋值*/
srvaddr.sin_family=AF_INET;
srvaddr.sin_port=htons(arg->NameToDataPort);
if(!inet_aton(arg->Namenode_ADDR,&srvaddr.sin_addr))
{printf("bad address\n");exit(-1);}

/*用connect于这个远程服务器建立一个internet连接*/
/*
if(connect(sock_fd,(struct sockaddr*)&srvaddr,sizeof(struct sockaddr))==-1)
{
	fprintf(stderr,"error dest addr %s\n",arg->Namenode_ADDR);
	perror("connect error");
	exit(-1);
}
*/
while(connect(sock_fd,(struct sockaddr*)&srvaddr,sizeof(struct sockaddr))==-1)
{
	if((errno == EINTR)&&(i<=5))
	{	
		sleep(t[i]);
		i++;
		continue;
	}
	else
	{	
		fprintf(stderr,"connect error dest addr %s\n",arg->Namenode_ADDR);
		perror("connect error");
		sleep(30);
		exit(-1);
	}
}
return sock_fd;
}

/*传输数据,输入参数为建立了连接的sock_fd,存储数据的buffer,数据传输的长度length*/
int64_t DataTransportRead(int32_t sock_fd,char * buffer,int64_t length)
{
	int64_t totalsize = 0;
	int64_t recvsize = 0;
	 struct sockaddr_in cliaddr;
	 struct sockaddr_in servaddr;
        socklen_t len;

	len = sizeof(struct sockaddr_in);
        getpeername(sock_fd,(struct sockaddr*)&cliaddr,&len);
        getsockname(sock_fd,(struct sockaddr*)&servaddr,&len);
	if(buffer == NULL)fprintf(stderr,"buffer==NULL,sockfd =%d,buffer = %p,length =%ld,local%s,dest%s\n",\
						sock_fd,buffer,length,inet_ntoa(servaddr.sin_addr),inet_ntoa(cliaddr.sin_addr));	
	assert(length >=0);
	assert(buffer!=NULL);
	if(length == 0)return 0;
	while(totalsize<length)
	{
		recvsize=read(sock_fd,buffer+totalsize,length-totalsize);
		if(recvsize<0)
		{
			if(errno==EINTR)recvsize = 0;//请求被中断
				else {
					perror("read error dew\n");
					fprintf(stderr,"sockfd =%d,buffer = %p,length =%ld,local%s,dest%s\n",\
						sock_fd,buffer,length,inet_ntoa(servaddr.sin_addr),inet_ntoa(cliaddr.sin_addr));	
					fprintf(stderr,"dest %s\n",inet_ntoa(cliaddr.sin_addr));	
					return -1;
				}
		}else if (recvsize == 0)continue;
			totalsize+=recvsize;
	}
	assert(totalsize == length);
	return totalsize;
}
int64_t DataTransportWrite(int32_t sock_fd,char * buffer,int64_t length)
{
        int64_t totalsize = 0;
        int64_t sendsize = 0;
	struct sockaddr_in cliaddr;
        struct sockaddr_in servaddr;
        socklen_t len;
	if(length == 0)return 0;
	len = sizeof(struct sockaddr_in);
        getpeername(sock_fd,(struct sockaddr*)&cliaddr,&len);
        getsockname(sock_fd,(struct sockaddr*)&servaddr,&len);

	if(buffer == NULL)
	{
		fprintf(stderr,"buffer==NULL,sockfd =%d,buffer = %p,length =%ld,local%s\n",\
						sock_fd,buffer,length,inet_ntoa(servaddr.sin_addr));
		fprintf(stderr,"dest %s\n",inet_ntoa(cliaddr.sin_addr));	
	}
	assert(length >=0);
	assert(buffer!=NULL);
        while(totalsize<length)
        {
            sendsize=write(sock_fd,buffer+totalsize,length-totalsize);
            if(sendsize<0)
             {
                if(errno==EINTR)sendsize = 0;
                else{
			//fprintf(stderr,"sockfd =%d,buffer = %p,length =%ld",sock_fd,buffer,length);	

			fprintf(stderr,"sockfd =%d,buffer = %p,length =%ld,local%s,dest%s\n",\
						sock_fd,buffer,length,inet_ntoa(servaddr.sin_addr),inet_ntoa(cliaddr.sin_addr));	
			fprintf(stderr,"dest %s\n",inet_ntoa(cliaddr.sin_addr));	
                	perror("write error dew\n");
                	return -1;
                }
             }else if(sendsize == 0)continue;
                totalsize+=sendsize;
        }
        assert(totalsize == length);
        return totalsize;
} 
/*
char * ExchangeStructToString(void * p_struct,size_t struct_length)
{
	char * p_string = (char*)malloc(struct_length*sizeof(char));
	assert(p_string == NULL);
    assert(p_struct == NULL);
    memset(p_string,0,struct_length);
	}

*/
