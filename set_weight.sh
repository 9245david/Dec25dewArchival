#!/bin/bash
weight_array=(`cat conf/weight_nodes.conf`)
node_array=(`cat conf/IP_FOR_SHELL.conf`)
for node_num in {0..8}
do
sed -i 's/\s[0-9]*$/ '${weight_array[$node_num]}'/g' conf/weight${node_array[${node_num}]}.conf
scp -r /home/dew/test/Dec25dewArchival/conf/weight${node_array[${node_num}]}.conf root@192.168.0.${node_array[${node_num}]}:/home/dew/Dec25dewArchival/conf/weight.conf
done
echo weight
echo ${weight_array[@]}
