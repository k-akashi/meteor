#/bin/bash -x

if [ $# -lt 3 ]
then
    echo "ERROR: Not enough arguments were provided!"
    echo "Command function: Run QOMET-based WLAN emulation experiment"
    echo "Usage: $(basename $0) <qomet_output_file> <setting_file> <connection_file>"
    exit 1
fi

DIR=$(cd `dirname ${BASH_SOURCE:-$0}`; pwd)
PATH=${PATH}:${DIR}

# Define variables
scenario_name=$1
scenario_setting=$2
scenario_connection=$3

if [ $# -ge 4 ]
then
    EXEC=" -e $4 "
fi
IF1="eth4";
IF2="eth5";
INGRESS_IF="ifb0";
TIME=`date  +"%Y%m%d%H%M%S"`

echo ----------------------------
sudo brctl addbr br0;
sudo brctl stp br0 off;
sudo brctl addif br0 ${IF1};
sudo brctl addif br0 ${IF2};
sudo ip link set up dev br0;

# Start the QOMET emulation
echo "* Starting QOMET emulation..." 
echo "${DIR}/meteor -Q ${scenario_name} -s ${scenario_setting} -c ${scenario_connection} -I ${IF1} -I ${IF2} -d bridge -l -e ${EXEC}"
${DIR}/monitor_tc -i ${INGRESS_IF} -q -o /var/tmp/meteor_${TIME} &
${DIR}/meteor -Q ${scenario_name} -s ${scenario_setting} -c ${scenario_connection} -I ${IF1} -I ${IF2} -d bridge -l ${EXEC}

echo "finish!!"
echo ----------------------------
date
killall python;
sudo ip link set down dev br0;
sudo brctl delif br0 ${IF1};
sudo brctl delif br0 ${IF2};
sudo brctl delbr br0;
echo ----------------------------
echo -- Ending experiment...
