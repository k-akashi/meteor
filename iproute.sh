#!/bin/sh -x

dev=lo

sudo /sbin/tc qdisc del dev ${dev} root

#sudo /sbin/tc qdisc    add dev ${dev} root 					handle 200:			prio bands 16
#sudo /sbin/tc qdisc    add dev ${dev} parent 200:1			handle 1200:		htb 					
#sudo /sbin/tc class    add dev ${dev} parent 1200:			classid 1200:1 		htb rate 100Mbit 	
#sudo /sbin/tc class    add dev ${dev} parent 1200: 			classid 1200:11 	htb rate 100Mbit 
#sudo /sbin/tc qdisc    add dev ${dev} parent 1200:11 		handle 2200:		netem delay 10ms
#sudo /sbin/tc filter   add dev ${dev} proto ip parent 200: prio 1 u32 match ip dst 127.0.0.0/8 flowid 1200:11

#sudo /sbin/tc qdisc    add dev ${dev} root 					handle 200:			htb 					
#sudo /sbin/tc class    add dev ${dev} parent 200:			classid 200:1 		htb rate 10Mbit 	
#sudo /sbin/tc class    add dev ${dev} parent 200: 			classid 200:11 		htb rate 10Mbit 
#sudo /sbin/tc qdisc    add dev ${dev} parent 200:11 		handle 1200:		netem delay 10ms
#sudo /sbin/tc filter   add dev ${dev} proto ip parent 200: prio 1 u32 match ip dst 127.0.0.0/8 flowid 200:11

sudo /sbin/tc qdisc    add dev ${dev} root 					handle 1:			htb default 1
sudo /sbin/tc class    add dev ${dev} parent 1:1            classid 1:200       htb rate 10Mbit
sudo /sbin/tc qdisc    add dev ${dev} parent 1:200 	    	handle 200:1 		netem delay 10ms
sudo /sbin/tc filter   add dev ${dev} proto ip parent 1: prio 1 u32 match ip dst 127.0.0.0/8 flowid 1:200

echo ""
sudo /sbin/tc qdisc show dev ${dev}
echo ""
sudo /sbin/tc class show dev ${dev}
echo ""
sudo /sbin/tc filter show dev ${dev}
