#!/bin/bash
for task in 360   
do
sed -i 's/\s[0-9]*$/ '$task'/g' ./conf/task.conf
./set_weight.sh
date
#cat ./conf/weight.conf
#cat ./conf/task.conf
echo > TaskFeedbackLog.log.new$task
echo > weightFeedback.log
killall NameNode.out
./remove_all_semid.sh
./runclean.sh `cat conf/IP_FOR_SHELL.conf`
#./copy.sh `cat conf/IP_FOR_SHELL.conf`
sleep 5
./NameNode.out&
./run.sh `cat conf/IP_FOR_SHELL.conf`&

rm -rf loop_out.log
date >> loop_out.log
#./auto_run.sh&
declare -i flag
declare -i loop=0
#flag=`ps aux |grep NameNode.out|wc -l`
flag=`ipcs -s |grep root|grep 9|awk '{print $2}'|wc -l`
while (($flag>0));
do
  if (($loop<10));then
     #flag=`ps aux |grep NameNode.out|wc -l`
flag=`ipcs -s |grep root|grep 9|awk '{print $2}'|wc -l`
     sleep 40
     let ++loop
     echo loop is $loop flag is $flag >>loop_out.log
     date >>loop_out.log
  else
     break
  fi
done
#killall NameNode.out
string=`date -d today +"%Y%m%d%H%M"`
weight=`cat conf/weight_nodes.conf| sed 's/ //g'`
cat TaskFeedbackLog.log.new$task > out_${string}_${task}_${weight}
cat weightFeedback.log > weightFeedback.log_${string}_${task}_${weight}
#rm -rf TaskFeedbackLog.log.new$task
cat TaskFeedbackLog.log.new$task >> TaskFeedbackLog.log.12_8.new$task
rm -rf TaskFeedbackLog.log.new$task
./deal_result.sh out_${string}_${task}_${weight}

done
