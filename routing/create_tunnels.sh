#/bin/sh

# script for creating tunnels

#IP address base
main_base=172.16.3
tunnel_base=10.3
alias_base=172.16.103

# compute modulo difference between two numbers
# arguments are as follows: number1 number2 modulo_base
modulo_difference()
{
    if [ $1 -gt $2 ]; then
	return $(($1-$2))
    else
	return $(($1-$2+$3))
    fi
}

#for ((from_node=1; from_node<=1; from_node++)); do
for ((from_node=1; from_node<=3; from_node++)); do
    
    echo
    echo Settings for node $from_node...

    # prepare ssh command
    ssh_command="ssh $main_base.$from_node"

    # set alias
    $ssh_command sudo ifconfig fxp0 alias $alias_base.$from_node netmask 255.255.255.0

    # enable ip forwarding
    $ssh_command sudo sysctl -w net.inet.ip.forwarding=1

    for ((to_node=1; to_node<=3; to_node++)); do
	if [ $from_node -ne $to_node ]; then

	    echo Creating tunnel from $from_node to $to_node...

	    # compute interface number
	    modulo_difference $to_node $from_node 3
	    if_number_from=$(($? - 1))
	    modulo_difference $from_node $to_node 3
	    if_number_to=$(($? - 1))

            # create gif interfaces and tunnels
	    $ssh_command sudo ifconfig gif${if_number_from} create tunnel $main_base.$from_node $main_base.$to_node
	    $ssh_command sudo ifconfig gif${if_number_from} $tunnel_base.$from_node.$if_number_from $tunnel_base.$to_node.$if_number_to netmask 255.0.0.0
	    echo ---configuration done
	fi
    done
done

# below are the old commands...

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



