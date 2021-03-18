#!/bin/bash

SPEED=1
DELAY=1
LOSS=0
CORRUPT=0

{
    killall -9 link
    killall -9 recv
    killall -9 send
} &> /dev/null

./link_emulator/link speed=$SPEED delay=$DELAY loss=$LOSS corrupt=$CORRUPT &> /dev/null &
sleep 1
./recv &
sleep 1

./send fisier.in 

#sleep 2
#echo "Finished transfer, checking files"
#diff tema.txt recv_tema.txt
