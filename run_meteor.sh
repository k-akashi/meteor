#/bin/sh -x

SC_DIR=`dirname $0`;
BIN_DIR="${SC_DIR}/bin";
PATH_BACK=`echo "${PATH}"`
export PATH="${BIN_DIR}:${PATH}"

if [ $# -lt 2 ]
then
    echo "ERROR: Not enough arguments were provided!"
    echo "Command function: Run QOMET-based WLAN emulation experiment"
    echo "Usage: $(basename $0) <qomet_output_file> <setting_file> <connection_file>"
    exit 1
fi

# Define variables
scenario_name=$1
scenario_setting=$2
TIME=`date  +"%Y%m%d%H%M%S"`

####################
# Run the experiment
####################

# Setup bridge interface
echo ----------------------------
# Start the QOMET emulation
echo "* Starting QOMET emulation..." 
echo "sudo time ./bin/meteor -q ${scenario_name} -s ${scenario_setting} -m in -l -i 0"
sudo time ./bin/meteor -q ${scenario_name} -s ${scenario_setting} -m in -l -i 0
#monitor_tc -i ${INGRESS_IF} -q -o /var/tmp/meteor_${TIME} &

date

###############
# Final actions
###############
echo
echo -- Ending experiment...
export PATH="${PATH_BACK}"
