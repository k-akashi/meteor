#!/bin/sh -x

dev=lo

root_id=1
netem_id=`expr ${root_id} + 1000`
class_id=`expr ${root_id} + $1`

#sudo /sbin/tc qdisc del dev ${dev} root

#sudo /home/k-akashi/work/iproute2-2.6.38/tc qdisc    add dev ${dev} root 					handle ${root_id}:			prio bands 16
#sudo /home/k-akashi/work/iproute2-2.6.38/tc qdisc    add dev ${dev} parent ${root_id}:1			handle 1200:		htb 					
#sudo /home/k-akashi/work/iproute2-2.6.38/tc class    add dev ${dev} parent 1200:			classid 1200:1 		htb rate 100Mbit 	
#sudo /home/k-akashi/work/iproute2-2.6.38/tc class    add dev ${dev} parent 1200: 			classid 1200:11 	htb rate 100Mbit 
#sudo /home/k-akashi/work/iproute2-2.6.38/tc qdisc    add dev ${dev parent 1200:11 		handle 2200:		netem delay 10ms
#sudo /home/k-akashi/work/iproute2-2.6.38/tc filter   add dev ${dev} proto ip parent ${root_id}: prio 1 u32 match ip dst 127.0.0.0/8 flowid 1200:11

class_id=1
sudo /home/k-akashi/work/iproute2-2.6.38/tc/tc qdisc    add dev ${dev} root 				                handle  ${root_id}:			        htb 
sudo /home/k-akashi/work/iproute2-2.6.38/tc/tc class    add dev ${dev} parent ${root_id}:			        classid ${root_id}:1 		        htb rate 10Mbit 	
while [ $class_id -le 10000 ]
do
    netem_id=`expr ${class_id} + 10000`
    sudo /home/k-akashi/work/iproute2-2.6.38/tc/tc class    add dev ${dev} parent ${root_id}: 		        	classid ${root_id}:${class_id} 		htb rate 10Mbit 
    sudo /home/k-akashi/work/iproute2-2.6.38/tc/tc qdisc    add dev ${dev} parent ${root_id}:${class_id} 		handle ${netem_id}:		netem delay 10ms
    sudo /home/k-akashi/work/iproute2-2.6.38/tc/tc filter   add dev ${dev} proto ip parent ${root_id}: prio 1 u32 match ip dst 127.0.0.0/8 flowid ${root_id}:${class_id}
    class_id=`expr $class_id + 1`
done

sudo /home/k-akashi/work/iproute2-2.6.38/tc/tc class    change dev ${dev} parent ${root_id}: 			classid ${root_id}:11 		htb rate 50Mbit
echo ""
sudo /sbin/tc qdisc show dev ${dev}
echo ""
sudo /sbin/tc class show dev ${dev}
echo ""
sudo /sbin/tc filter show dev ${dev}
