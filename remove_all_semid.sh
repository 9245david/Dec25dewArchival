#!/bin/bash
SEMNUM=18
SEMID=$(ipcs -s |grep root|grep $SEMNUM|awk '{print $2}')
echo $SEMID
ipcrm -s  $SEMID

