#!/bin/sh

ps="ptat"
me="`whoami`"

for i in $ps
do
    pid=`ps -aef | grep -e "$me"  | grep -e "$ps" | egrep -vi  "grep|$0" | awk '{print $2}'`
    if [ "$pid" != "" ]; then
        echo "Killing process id... $pid"
        kill -9 $pid
    fi
done

