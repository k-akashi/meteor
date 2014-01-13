#/bin/sh -x

SC_DIR=`dirname $0`;
BIN_DIR="${SC_DIR}/bin";
PATH_BACK=`echo "${PATH}"`
export PATH="${BIN_DIR}:${PATH}"

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
EXEC=$4
IF1="eth0";
IF2="eth1";
INGRESS_IF="ifb0";
TIME=`date  +"%Y%m%d%H%M%S"`

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
echo "sudo time meteor -Q ${scenario_name} -s ${scenario_setting} -c ${scenario_connection} -d bridge -l -e ${EXEC}"
monitor_tc -i ${INGRESS_IF} -q -o /var/tmp/meteor_${TIME} &
sudo time meteor -Q ${scenario_name} -s ${scenario_setting} -c ${scenario_connection} -d bridge -l -I ${IF1} -I ${IF2} -e ${EXEC}

echo "finish!!"
echo ----------------------------
date
killall python;
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
export PATH="${PATH_BACK}"
