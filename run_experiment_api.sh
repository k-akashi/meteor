#/bin/sh -x
# Run QOMET-based WLAN emulation experiment

# NOTE: This script uses an API-based program to configure dummynet 
#       and to have precise timing control for step execution
#       These functions are provided by the "wireconf" library
#       and the example program "do_wireconf"

if [ $# -lt 1 ]
then
    echo "ERROR: Not enough arguments were provided!"
    echo "Command function: Run QOMET-based WLAN emulation experiment"
    echo "Usage: $(basename $0) <qomet_output_file>"
    exit 1
fi

# Define variables
test_name=$1
output_name=${test_name}

from_node=1
to_node=2
client_address=10.100.10.1
server_address=10.100.10.2
#server_address=192.168.200.2
rule_number=100
pipe_number=200
packet_size=1200
test_duration=60
offered_load=10M

########################################
# Start tcpdump
########################################

#echo
#echo -- Starting tcpdump...
#sudo tcpdump -n -i lo0 -w ${output_name}_tcpdump.out &


########################################
# Configure alias interfaces and routes
########################################

#sudo ifconfig lo0 inet 127.0.0.2 netmask 255.0.0.0 alias
#sudo route add -host 127.0.0.2 127.0.0.1 0
#sudo ifconfig lo0 inet 127.0.0.2 -alias


########################################
# Prepare the system by loading dummynet
########################################

#echo
#echo -- Loading dummynet...
# the command below is not needed if dummynet is compiled into the kernel
#sudo ./dummynet_load.sh

####################
# Run the experiment
####################

echo
echo "-- Running experiment..."

# Start server application
echo
echo "* Starting server..."
#/usr/local/bin/iperf -s -u -i 0.5 -f k > ${output_name}_iperf_s.out -l $packet_size&

# Wait for server to start
#sleep 1

# Start client application
echo "* Starting client..."
#/usr/local/bin/iperf -c $server_address -B $client_address -u -i 0.5 -l $packet_size -b $offered_load -t $test_duration -f k > ${output_name}_iperf_c.out &
#/usr/local/bin/iperf -c $server_address -u -i 0.5 -l $packet_size -b $offered_load -t $test_duration -f k > ${output_name}_iperf_c.out &

# Start ping
echo "* Starting ping..."
#sudo ping -i 0.25 -s $packet_size $server_address > ${output_name}_ping.out &

echo
echo ----------------------------
date
echo ----------------------------

# Start the QOMET emulation
echo "* Starting QOMET emulation..." 
echo "sudo time ./wireconf/do_wireconf -q $test_name -f $from_node -F $client_address -t $to_node -T $server_address -r $rule_number -p $pipe_number -d out"
sudo time ./wireconf/do_wireconf -q $test_name -f $from_node -F $client_address -t $to_node -T $server_address -r $rule_number -p $pipe_number -d out

echo "finish!!"
echo ----------------------------
date
echo ----------------------------

###############
# Final actions
###############
echo
echo -- Ending experiment...

# Kill processes

echo Terminating ping...
#sudo killall -INT ping

#sleep 2
echo Killing iperf server...
#killall -INT iperf

#echo Killing tcpdump...
#sudo killall tcpdump

# Process output files
#echo Processing output files...

# Clean the system
#echo Unloading dummynet...
#sudo ./dummynet_unload.sh
