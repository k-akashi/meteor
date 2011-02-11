
# Run QOMET-based WLAN emulation experiment

# Define variables
scenario_file=$1

bandwidth_values_file=${scenario_file}.bandwidth
loss_values_file=${scenario_file}.loss
delay_values_file=${scenario_file}.delay

test_duration=60
nap_duration=460
#number_iterations=$(($test_duration*2))


########################################
# Prepare the system by loading dummynet
########################################

echo
echo -- Loading dummynet and doing basic configuration...
#sudo ./dummynet_load.sh
#sudo ipfw add 100 pipe 100 ip from 127.0.0.1 to 127.0.0.1 out

####################
# Run the experiment
####################

echo
echo -- Running experiment...
# Load loss and delay values from files
echo -n Loading $bandwidth_values_file, $loss_values_file, and $delay_values_file ...
declare -a bandwidth_array
bandwidth_array=( `cat "$bandwidth_values_file"`) 
declare -a loss_array
loss_array=( `cat "$loss_values_file"`) 
declare -a delay_array
delay_array=( `cat "$delay_values_file"`) 
element_count=${#bandwidth_array[*]}
echo " done (element count = $element_count)"

# Start server application
echo
echo "* Starting server..."
/usr/local/netperf/netserver

# Wait for server to start
sleep 3

index=0
bandwidth=${bandwidth_array[0]}
loss=${loss_array[0]}
delay=${delay_array[0]}

# Prepare the emulator for the first time period
echo
echo "* Dummynet configuration #$index: bandwidth=${bandwidth}bit/s loss=$loss delay=${delay}ms"
#sudo ipfw pipe 100 config bw ${bandwidth}bit/s delay ${delay}ms plr $loss
sudo tc qdisc add dev eth1 root handle 10: netem delay ${delay}ms limit 10000
sudo tc qdisc add dev eth1 parent 10:1 handle 100: pfifo limit 100000 
#sudo tc qdisc add dev eth1 parent 10:1 handle 100: tbf rate ${bandwidth}bit limit 15Kb buffer 10Kb/8

# Start client application
echo "* Starting client..."
/usr/local/netperf/netperf -l $test_duration &

echo ----------------------------
date
echo ----------------------------
for((index=1; index < $element_count; index++)); do
#for((index=1; index < $number_iterations; index++)); do
    # Sleep for the previous configuration
    perl nap.pl $nap_duration

    # Prepare the emulator for the next time period
    bandwidth=${bandwidth_array[$index]}
    loss=${loss_array[$index]}
    delay=${delay_array[$index]}    
    echo "* Dummynet configuration #$index: bandwidth=${bandwidth}bit/s loss=$loss delay=${delay}ms"
    #sudo ipfw pipe 100 config bw ${bandwidth}bit/s delay ${delay}ms plr $loss
	sudo tc qdisc change dev eth1 root handle 10: netem delay ${delay}ms
	#sudo tc qdisc change dev eth1 parent 10:1 handle 100: tbf rate ${bandwidth}bit limit 15Kb buffer 10Kb/8
done

# Do a last sleep for the last configuration
perl nap.pl $nap_duration

echo ----------------------------
date
echo ----------------------------

###############
# Final actions
###############
echo
echo -- Ending experiment...

# Kill processes
#echo Killing client...
#killall netperf
sleep 3
echo Killing server...
killall netserver

# Process output files
#echo Processing output files...

# Clean the system
echo Unloading dummynet...
#sudo ipfw delete 100 
#sudo ./dummynet_unload.sh
sudo tc qdisc del dev eth1 root
