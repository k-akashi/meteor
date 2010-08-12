
# Post-process QOMET output files and extract deltaQ parameters
# in 3 separate files

if [ $# -ne 3 ]
then
    echo "ERROR: Not enough arguments were provided!"
    echo "Usage: $(basename $0) <filename.xml.out> <from_node_id> <to_node_id>"
    exit 1
fi

scenario_file=${1%out}
from_node=$2
to_node=$3

bandwidth_column=11
loss_column=12
delay_column=13

# save bandwidth values to a separate file
grep ^[0-9] ${scenario_file}out | cut -f 2- -d " " | grep ^$from_node | cut -f 5- -d " " | grep ^$to_node | cut -f $bandwidth_column -d " " > ${scenario_file}from_${from_node}.to_${to_node}.bandwidth

# save loss values to a separate file
grep ^[0-9] ${scenario_file}out | cut -f 2- -d " " | grep ^$from_node | cut -f 5- -d " " | grep ^$to_node | cut -f $loss_column -d " " > ${scenario_file}from_${from_node}.to_${to_node}.loss

# save delay values to a separate file
grep ^[0-9] ${scenario_file}out | cut -f 2- -d " " | grep ^$from_node | cut -f 5- -d " " | grep ^$to_node | cut -f $delay_column -d " " > ${scenario_file}from_${from_node}.to_${to_node}.delay

