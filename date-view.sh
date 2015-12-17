#!/bin/bash
IPHEAD=192.168.0.
#for i in 34 35 44 45 46 47 50 52 53 54 55 58 59 63 64 65 67 68 33
for i in 44 45 50 53 54 55 57 60 64 33 
#for i in  $1 $2 $3 $4 $5 $6 $7 $8 $9 33
do
echo $i
ssh root@$IPHEAD$i "date"
string=`date -d today +"%Y%m%d%H%M"`
declare -i flag
declare -i loop=0
flag=`ps aux |grep Namenode.out|wc -l`
while (($flag>1));
do
  flag=`ps aux |grep Namenode.out|wc -l`
  if (($loop<6));then
     sleep 1
     let ++loop
     echo loop is $loop
  else
     break
  fi
done

echo $string

done
