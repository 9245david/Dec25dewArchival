#!/bin/bash
IPHEAD=192.168.0.
#for i in 34 35 44 45 46 47 50 52 53 54 55 58 59 63 64 65 67 68 33
for i in 44 45 50 52 54 55 57 60 64 33 
#for i in  $1 $2 $3 $4 $5 $6 $7 $8 $9 33
do
echo $i
ssh root@$IPHEAD$i "date"

done
