#make产生两个可执行文件，一个数据节点，一个管理节点
#gcc -g Socket_connect_read_write.c DewHa.c DatanodeToDatanode.c DatanodeToNamenode.c -o Datanode.out -lpthread

#gcc -g Socket_connect_read_write.c DewHa.c NameNodeControl.c -o NameNode.out -lpthread

Datanode:Socket.o DewHa.o DatanodeToDatanode.o DatanodeToNamenode.o
	gcc -g3 Socket.o DewHa.o DatanodeToDatanode.o DatanodeToNamenode.o -o Datanode.out -lpthread 
Socket.o:Socket_connect_read_write.c
	gcc -g3 -c Socket_connect_read_write.c -o Socket.o
DewHa.o :DewHa.c
	gcc -g3 -c DewHa.c -o DewHa.o
NameNodeControl.o:NameNodeControl.c
	gcc -g3 -c NameNodeControl.c -o NameNodeControl.o
DatanodeToDatanode.o:DatanodeToDatanode.c
	gcc -g3 -c DatanodeToDatanode.c -o DatanodeToDatanode.o
DatanodeToNamenode.o:DatanodeToNamenode.c
	gcc -g3 -c DatanodeToNamenode.c -o DatanodeToNamenode.o
Namenode:Socket.o DewHa.o NameNodeControl.o
	gcc -g3 Socket.o DewHa.o NameNodeControl.o -o NameNode.out -lpthread
clean:
	rm -rf *.o *.out

