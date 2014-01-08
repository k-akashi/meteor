#/bin/sh -x

if [ $# -lt 3 ]
then
    echo "ERROR: Not enough arguments were provided!"
    echo "Command function: Run QOMET-based WLAN emulation experiment"
    echo "Usage: $(basename $0) <qomet_output_file> <setting_file> <connection_file>"
    exit 1
fi

# Define variables
scenario_name=$1
scenario_setting=$2
scenario_connection=$3
IF1="eth0";
IF2="eth1";

####################
# Run the experiment
####################

# Setup bridge interface
echo ----------------------------
sudo brctl addbr br0;
sudo brctl stp br0 off;
sudo brctl addif br0 ${IF1};
sudo brctl addif br0 ${IF2};
sudo ip link set up dev br0;

# Start the QOMET emulation
echo "* Starting QOMET emulation..." 
echo "sudo time ./bin/meteor -Q ${scenario_name} -s ${scenario_setting} -c ${scenario_connection} -d bridge -l"
sudo time ./bin/meteor -Q ${scenario_name} -s ${scenario_setting} -c ${scenario_connection} -d bridge -l -I ${IF1} -I ${IF2}

echo "finish!!"
echo ----------------------------
date
sudo ip link set down dev br0;
sudo brctl delif br0 ${IF1};
sudo brctl delif br0 ${IF2};
sudo brctl delbr br0;

echo ----------------------------

###############
# Final actions
###############
echo
echo -- Ending experiment...
