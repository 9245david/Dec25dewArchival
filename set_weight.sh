#!/bin/bash
weight_array=(`cat conf/weight_nodes.conf`)
node_array=(`cat conf/IP_FOR_SHELL.conf`)
net_array=(`cat conf/network_nodes.conf`)
for node_num in $(seq 0 8)
do
NODE=${node_array[${node_num}]}
IP=192.168.0.$NODE
WEIGHT=${weight_array[$node_num]}
NET=${net_array[$node_num]}
ethX=`ssh root@$IP "ifconfig |grep eth"|awk '{print $1}'`
echo $node_num $WEIGHT
sed -i 's/\s[0-9]*$/ '$WEIGHT'/g' conf/weight$NODE.conf
scp -r /home/dew/test/Dec25dewArchival/conf/weight$NODE.conf root@$IP:/home/dew/Dec25dewArchival/conf/weight.conf
#ssh root@$IPHEAD$i "tc -s qdisc ls dev $ethX"
ssh root@$IP "tc qdisc del dev $ethX root"
ssh root@$IP "tc qdisc add dev $ethX root tbf rate $NET latency 50ms burst 800000"


done
echo weight
echo ${weight_array[@]}
