#!/bin/bash

n=0
while [ $n -lt 100000 ];
do
    n=$[$n+1];
#    ./stress_client -s 192.168.56.101:55555 -f 5000 --payload=44000 &
    ./stress_client -s $1 -f 500 --payload=20 &


done


