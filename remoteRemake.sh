#!/bin/bash
IPHEAD=192.168.0.
#for i in 34 35 44 45 46 47 50 52 53 54 55 58 59 63 64 65 67 68
for i in 45 46 50 52 44 55 59 63 64
#for i in 34
do
#        ssh root@$IPHEAD$i mkdir /home/dew
#       ssh root@$IPHEAD$i mkdir /home/dew/Dec25dewArchival
#        scp /home/dew/Dec25dewArchival/* root@$IPHEAD$i:/home/dew/Dec25dewArchival
#	ssh root@$IPHEAD$i chmod +x /home/dew/Dec25dewArchival/Datanode.out
echo $i
#	ssh root@192.168.0.$i "/home/dew/Dec25dewArchival/Datanode.out >>/home/dew/Dec25dewArchival/out.log" &
#	ssh root@192.168.0.$i "chmod +x /home/dew/Dec25dewArchival/Datanode.out"
	ssh root@$IPHEAD$i /home/dew/Dec25dewArchival/remake.sh 
done
