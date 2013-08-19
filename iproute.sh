#!/bin/sh -x

DEV="lo"
TC="/sbin/tc"

sudo /sbin/tc qdisc del dev ${dev} root

#sudo ${TC} qdisc   add dev ${DEV} root                 handle 200:         prio bands 16
#sudo ${TC} qdisc   add dev ${DEV} parent 200:1         handle 1200:        htb                     
#sudo ${TC} class   add dev ${DEV} parent 1200:         classid 1200:1      htb rate 100Mbit    
#sudo ${TC} class   add dev ${DEV} parent 1200:         classid 1200:11     htb rate 100Mbit 
#sudo ${TC} qdisc   add dev ${DEV} parent 1200:11       handle 2200:        netem delay 10ms
#sudo ${TC} filter  add dev ${DEV} proto ip parent 200: prio 1 u32 match ip dst 127.0.0.0/8 flowid 1200:11

#sudo ${TC} qdisc   add dev ${DEV} root                 handle 200:         htb                     
#sudo ${TC} class   add dev ${DEV} parent 200:          classid 200:1       htb rate 10Mbit     
#sudo ${TC} class   add dev ${DEV} parent 200:          classid 200:11      htb rate 10Mbit 
#sudo ${TC} qdisc   add dev ${DEV} parent 200:11        handle 1200:        netem delay 10ms
#sudo ${TC} filter  add dev ${DEV} proto ip parent 200: prio 1 u32 match ip dst 127.0.0.0/8 flowid 200:11

#sudo ${TC} qdisc   add dev ${DEV} root                 handle 1:           htb default 1
#sudo ${TC} class   add dev ${DEV} parent 1:1           classid 1:c8        htb rate 10Mbit
#sudo ${TC} qdisc   add dev ${DEV} parent 1:c8          handle c8:1         netem delay 10ms
#sudo ${TC} filter  add dev ${DEV} proto ip parent 1: prio 1 u32 match ip dst 127.0.0.0/32 flowid 1:c8

sudo ${TC} qdisc   del dev ifb0 root
sudo ${TC} qdisc   del dev ${DEV} root
sudo ${TC} qdisc   del parent ffff: dev ${DEV} 
sudo ${TC} qdisc   add dev ${DEV} ingress
sudo ${TC} filter  add dev ${DEV} parent ffff: protocol all prio 10 u32 match u32 0 0 flowid 1:1  action mirred egress redirect dev ifb0
sudo ${TC} qdisc   add dev ifb0 root                  handle 1:         htb
sudo ${TC} class   add dev ifb0 parent 1:1            classid 1:c8      htb rate 10Mbit
sudo ${TC} qdisc   add dev ifb0 parent 1:c8           handle c8:1       netem delay 10ms
#sudo ${TC} filter  add dev ifb0 proto ip parent 1: prio 1 u32 match ip src 127.0.0.1/32 match ip dst 127.0.0.1/32 flowid 1:c8
sudo ${TC} filter  add dev ifb0 proto all parent 1: prio 1 u32 match u16 0x5600 0xffff at -4 match u32 0x000cdbf3 0xffffffff at -8 flowid 1:c8

echo ""
sudo /sbin/tc qdisc show dev ${dev}
echo ""
sudo /sbin/tc class show dev ${dev}
echo ""
sudo /sbin/tc filter show dev ${dev}
