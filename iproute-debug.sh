#!/bin/sh -x

dev=lo

sudo /sbin/tc qdisc del dev ${dev} root

#sudo /home/k-akashi/work/iproute2-2.6.38/tc qdisc    add dev ${dev} root 					handle 200:			prio bands 16
#sudo /home/k-akashi/work/iproute2-2.6.38/tc qdisc    add dev ${dev} parent 200:1			handle 1200:		htb 					
#sudo /home/k-akashi/work/iproute2-2.6.38/tc class    add dev ${dev} parent 1200:			classid 1200:1 		htb rate 100Mbit 	
#sudo /home/k-akashi/work/iproute2-2.6.38/tc class    add dev ${dev} parent 1200: 			classid 1200:11 	htb rate 100Mbit 
#sudo /home/k-akashi/work/iproute2-2.6.38/tc qdisc    add dev ${dev parent 1200:11 		handle 2200:		netem delay 10ms
#sudo /home/k-akashi/work/iproute2-2.6.38/tc filter   add dev ${dev} proto ip parent 200: prio 1 u32 match ip dst 127.0.0.0/8 flowid 1200:11

sudo /home/k-akashi/work/iproute2-2.6.38/tc/tc qdisc    add dev ${dev} root 					handle 200:			htb 					
sudo /home/k-akashi/work/iproute2-2.6.38/tc/tc class    add dev ${dev} parent 200:			classid 200:1 		htb rate 10Mbit 	
sudo /home/k-akashi/work/iproute2-2.6.38/tc/tc class    add dev ${dev} parent 200: 			classid 200:11 		htb rate 10Mbit 
sudo /home/k-akashi/work/iproute2-2.6.38/tc/tc qdisc    add dev ${dev} parent 200:11 		handle 1200:		netem delay 10ms
sudo /home/k-akashi/work/iproute2-2.6.38/tc/tc filter   add dev ${dev} proto ip parent 200: prio 1 u32 match ip dst 127.0.0.0/8 flowid 200:11

sudo /home/k-akashi/work/iproute2-2.6.38/tc/tc class    change dev ${dev} parent 200: 			classid 200:11 		htb rate 50Mbit
echo ""
sudo /sbin/tc qdisc show dev ${dev}
echo ""
sudo /sbin/tc class show dev ${dev}
echo ""
sudo /sbin/tc filter show dev ${dev}
