#!/bin/bash
node_array=(`cat conf/IP_FOR_SHELL.conf`)
NET_ON=0
for node_num in {0..8}
do
NODE=${node_array[${node_num}]}
IP=192.168.0.$NODE
    if (($NET_ON==0));then
       # PING=`ping $IP -c 1|grep ttl|awk '{print $2}'`
       # if (("$PING"X=="bytes"X));then
       #    NET_ON=1
       #    break
       # fi

        SSH=`ssh root@$IP ls 2>&1|grep "No route to host"|wc -l`
        if (($SSH==0));then
           NET_ON=1
           break
        fi
    fi
done

if (($NET_ON==0));then
    echo net is off
    reboot
else
    echo net is on
fi 


