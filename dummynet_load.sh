
# Script to load DUMMYNET as a kernel module
# Note: This also works if used via ssh, since we immediately set a rule 
# to allow all packets, so that the following commands can be run as well

kldload /boot/kernel/dummynet.ko && ipfw add 50000 allow ip from any to any
