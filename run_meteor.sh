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

####################
# Run the experiment
####################

# Start server application
echo ----------------------------

# Start the QOMET emulation
echo "* Starting QOMET emulation..." 
echo "sudo time ./bin/meteor -Q ${scenario_name} -s ${scenario_setting} -c ${scenario_connection} -d bridge"
sudo time ./bin/meteor -Q ${scenario_name} -s ${scenario_setting} -c ${scenario_connection} -d bridge

echo "finish!!"
echo ----------------------------
date
echo ----------------------------

###############
# Final actions
###############
echo
echo -- Ending experiment...
