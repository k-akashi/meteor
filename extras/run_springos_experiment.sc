
assure num_nodes=50
export num_nodes

nodeclass client_class
{
	method "thru"
	partition 2 
	ostype "FreeBSD"
	
	scenario
	{
		#initialize constants 
		test_name="routing_test"
		test_duration="150"
		packet_size="1024"
		offered_load="200k"

		#receive my_id
		recv my_id

		#signal setup is finished
		send "setup_done"

		#wait for start message
		recv start_msg

		#run experiment
		callw "/bin/sh" "run_experiment_node.sh" test_name my_id offered_load test_duration packet_size > "/tmp/scenario.log"
	}
}

#define the clients
nodeset clients class client_class num num_nodes 

#set the IP addresses of the clients
for(i=0;i<num_nodes;i++)
{
	clients[i].agent.ipaddr = "172.16.3."+tostring(10+i)
	clients[i].agent.port = "2345"
} 

#scenario itself
scenario
{

	#send id to nodes
	for(i=0;i<num_nodes;i++)
	{
		send clients[i] tostring(i)
	}

	#wait for all clients to finish setup
	sync
	{
		msgmatch clients[0] "setup_done"
	}

	#send start_msg to all clients
	multisend clients "start"

	# wait for clients to end execution
	sleep 150
}
