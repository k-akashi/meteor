#!/bin/sh

DEV=lo

while [ 1 ]
do
	/sbin/tc qdisc show dev ${DEV}
	/sbin/tc class show dev ${DEV}
	/sbin/tc filter show dev ${DEV}
	echo ""
	sleep 1
done
