#!/bin/bash
IPHEAD=192.168.0.
#for i in 34 35 44 45 46 47 50 52 53 54 55 58 59 63 64 65 67 68 33
#for i in 45 46 50 52 44 55 59 63 64 
for i in  $1 $2 $3 $4 $5 $6 $7 $8 $9
do
echo $i
#	ssh root@$IPHEAD$i "date -R"
#	ssh root@$IPHEAD$i "cat /etc/timezone"
# ssh root@$IPHEAD$i "date"
        ssh root@$IPHEAD$i "date -s \"2015-7-03 20:10:00\""
#	ssh root@$IPHEAD$i "ulimit -c unlimited" // 这条远程指令无效
#	ssh root@$IPHEAD$i "echo 'core.%e.%p.%t' > /proc/sys/kernel/core_pattern"
done
date -s "2015-7-03 20:10:00"
