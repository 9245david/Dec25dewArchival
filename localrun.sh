#!/bin/bash
cd /home/dew/Dec25dewArchival
ifconfig|grep "inet addr:"|grep -v "127.0.0.1"|cut -d: -f2|awk '{print $1}'
./Datanode.out >out.log
ifconfig|grep "inet addr:"|grep -v "127.0.0.1"|cut -d: -f2|awk '{print $1}'

