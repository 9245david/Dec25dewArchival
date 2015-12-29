#!/bin/bash
#./date-set.sh `cat conf/IP_FOR_SHELL.conf`
./copy.sh `cat conf/IP_FOR_SHELL.conf`
./remoteRemake.sh `cat conf/IP_FOR_SHELL.conf`
make Namenode
./remove_all_semid.sh
./runclean.sh `cat conf/IP_FOR_SHELL.conf`
./run.sh `cat conf/IP_FOR_SHELL.conf`&
./NameNode.out
./runclean.sh `cat conf/IP_FOR_SHELL.conf`

