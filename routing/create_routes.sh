#/bin/sh

# script for creating tunnels

#IP address base
main_base=172.16.3

for ((from_node=1; from_node<=3; from_node++)); do
    
    echo
    echo Start 'do_routing' for $from_node...

    # prepare ssh command
    ssh_command="ssh -f $main_base.$from_node"

    $ssh_command "cd /usr/home/hoku/QOMET/qomet-1.2/routing; sudo ./do_routing -$from_node -f routing_file.txt; uname -a"
done
