#make产生两个可执行文件，一个数据节点，一个管理节点
#gcc -g Socket_connect_read_write.c DewHa.c DatanodeToDatanode.c DatanodeToNamenode.c -o Datanode.out -lpthread

#gcc -g Socket_connect_read_write.c DewHa.c NameNodeControl.c -o NameNode.out -lpthread

Datanode:Socket.o DewHa.o DatanodeToDatanode.o DatanodeToNamenode.o
	gcc Socket.o DewHa.o DatanodeToDatanode.o DatanodeToNamenode.o -o Datanode.out -lpthread 
Socket.o:Socket_connect_read_write.c
	gcc -g -c Socket_connect_read_write.c -o Socket.o
DewHa.o :DewHa.c
	gcc -g -c DewHa.c -o DewHa.o
NameNodeControl.o:NameNodeControl.c
	gcc -g -c NameNodeControl.c -o NameNodeControl.o
DatanodeToDatanode.o:DatanodeToDatanode.c
	gcc -g -c DatanodeToDatanode.c -o DatanodeToDatanode.o
DatanodeToNamenode.o:DatanodeToNamenode.c
	gcc -g -c DatanodeToNamenode.c -o DatanodeToNamenode.o
Namenode:Socket.o DewHa.o NameNodeControl.o
	gcc -g Socket.o DewHa.o NameNodeControl.o -o NameNode.out -lpthread
clean:
	rm -rf *.o *.out

