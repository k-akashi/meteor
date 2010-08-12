#/bin/sh

# script for destroying tunnels

main_base=172.16.3
tunnel_base=10.3
alias_base=172.16.103

modulo_difference()
{
    if [ $1 -gt $2 ]; then
	return $(($1-$2))
    else
	return $(($1-$2+$3))
    fi
}

for ((from_node=1; from_node<=3; from_node++)); do
    
    echo
    echo Settings for node $from_node...

    # prepare ssh command
    ssh_command="ssh $main_base.$from_node"

    # unset alias
    $ssh_command sudo ifconfig fxp0 delete $alias_base.$from_node

    # disable ip forwarding
    #$ssh_command sudo sysctl -w net.inet.ip.forwarding=0

    for ((to_node=1; to_node<=3; to_node++)); do
	if [ $from_node -ne $to_node ]; then

	    echo Destroying tunnel from $from_node to $to_node...

	    # compute interface number
	    modulo_difference $to_node $from_node 3
	    if_number_from=$(($? - 1))

            # destroy gif interfaces and tunnels
	    $ssh_command sudo ifconfig gif${if_number_from} destroy

	fi
    done
done


# create gif interfaces and tunnels
#ifconfig gif0 create tunnel 172.16.3.1 172.16.3.2
#ifconfig gif0 10.3.1.0 10.3.2.0 netmask 255.0.0.0
#ifconfig gif1 create tunnel 172.16.3.1 172.16.3.3
#ifconfig gif1 10.3.1.1 10.3.3.1 netmask 255.0.0.0

# set alias
#ifconfig fxp0 alias 172.16.103.1 netmask 255.255.255.0

# ip forwarding
#sysctl -w net.inet.ip.forwarding=1
#sysctl net.inet.ip.forwarding



