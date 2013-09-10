
/*
 * Copyright (c) 2006-2013 The StarBED Project  All rights reserved.
 *
 * See the file 'LICENSE' for licensing information.
 *
 */

/************************************************************************
 *
 * QOMET Emulator Implementation
 *
 * File name: connection.c
 * Function: Source file related to the connection scenario element
 *
 * Author: Razvan Beuran
 *
 * $Id: connection.c 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/


#include <string.h>
#include <stdint.h>

#include "message.h"
#include "deltaQ.h"

#include "connection.h"

#include "wlan.h"
#include "ethernet.h"
#include "active_tag.h"
#include "zigbee.h"
#include "wimax.h"


//////////////////////////////////
// Global variable initialization
//////////////////////////////////

char *connection_standards[] =
  { "802.11b", "802.11g", "802.11a", "Eth-10Mbps",
  "Eth-10/100 Mbps", "Eth-10/100/1000Mbps",
  "ActiveTag", "ZigBee"
};


/////////////////////////////////////////
// Connection structure functions
/////////////////////////////////////////

// init a connection;
// return SUCCESS on succes, ERROR on error
int
connection_init (struct connection_class *connection, char *from_node,
		 char *to_node, char *through_environment,
		 int packet_size, int standard, int channel,
		 int RTS_CTS_threshold, int consider_interference)
{
  strncpy (connection->from_node, from_node, MAX_STRING - 1);
  connection->from_node_index = INVALID_INDEX;
  strncpy (connection->from_interface, DEFAULT_STRING, MAX_STRING - 1);
  connection->from_interface_index = INVALID_INDEX;
  connection->from_id = INVALID_INDEX;

  strncpy (connection->to_node, to_node, MAX_STRING - 1);
  connection->to_node_index = INVALID_INDEX;
  strncpy (connection->to_interface, DEFAULT_STRING, MAX_STRING - 1);
  connection->to_interface_index = INVALID_INDEX;
  connection->to_id = INVALID_INDEX;

  strncpy (connection->through_environment, through_environment,
	   MAX_STRING - 1);
  connection->through_environment_index = INVALID_INDEX;

  connection->packet_size = packet_size;
  connection->channel = channel;
  connection->RTS_CTS_threshold = RTS_CTS_threshold;
  connection->consider_interference = consider_interference;

  // initialize dynamic parameters
  connection->Pr = 0;
  connection->SNR = 0;
  connection->distance = -1;
  connection->concurrent_stations = 0;
  connection->interference_noise = MINIMUM_NOISE_POWER;
  connection->compatibility_mode = FALSE;
  connection->adaptive_operating_rate = TRUE;

  // default initial settings for deltaQ parameters
  connection->frame_error_rate = 0;
  connection->interference_fer = 0;
  connection->num_retransmissions = 0;
  connection->loss_rate = 0;
  connection->delay = 0;
  connection->jitter = 0;

  connection->loss_rate_defined = FALSE;
  connection->delay_defined = FALSE;
  connection->jitter_defined = FALSE;
  connection->bandwidth_defined = FALSE;

  connection->fixed_deltaQ_number = 0;
  connection->fixed_deltaQ_crt = 0;

  /*
     capacity_update_all(&(connection->wimax_capacity), SYS_BW_10,  QPSK_1_8, 
     MIMO_TYPE_SISO);
   */

  // do this at the end since it may cause an error
  return connection_init_standard (connection, standard);
}

// initialize all parameters related to a specific wireless standard;
// return SUCCESS on succes, ERROR on error
int
connection_init_standard (struct connection_class *connection, int standard)
{
  if (standard == WLAN_802_11B)
    {
      connection->standard = standard;
      // we assume now that WLAN adapters start from the lowest rate and go up
      connection->new_operating_rate = B_RATE_1MBPS;
      connection->operating_rate = connection->new_operating_rate;
      connection->bandwidth = b_operating_rates[connection->operating_rate];

      // if no channel was defined choose an appropriate channel
      if (connection->channel == UNDEFINED_VALUE)
	connection->channel = DEFAULT_CHANNEL_BG;
    }
  else if (standard == WLAN_802_11G)
    {
      connection->standard = standard;
      // we assume now that WLAN adapters start from the lowest rate and go up
      connection->new_operating_rate = G_RATE_1MBPS;
      connection->operating_rate = connection->new_operating_rate;
      connection->bandwidth = g_operating_rates[connection->operating_rate];

      // if no channel was defined choose an appropriate channel
      if (connection->channel == UNDEFINED_VALUE)
	connection->channel = DEFAULT_CHANNEL_BG;
    }
  else if (standard == WLAN_802_11A)
    {
      connection->standard = standard;
      // we assume now that WLAN adapters start from the lowest rate and go up
      connection->new_operating_rate = A_RATE_6MBPS;
      connection->operating_rate = connection->new_operating_rate;
      connection->bandwidth = a_operating_rates[connection->operating_rate];

      // if no channel was defined choose an appropriate channel
      if (connection->channel == UNDEFINED_VALUE)
	connection->channel = DEFAULT_CHANNEL_A;
    }
  else if (standard == ETHERNET_10)
    {
      connection->standard = standard;
      connection->new_operating_rate = ETH_RATE_10MBPS;
      connection->operating_rate = connection->new_operating_rate;
      connection->bandwidth = eth_operating_rates[connection->operating_rate];
    }
  else if (standard == ETHERNET_100)
    {
      connection->standard = standard;
      connection->new_operating_rate = ETH_RATE_100MBPS;
      connection->operating_rate = connection->new_operating_rate;
      connection->bandwidth = eth_operating_rates[connection->operating_rate];
    }
  else if (standard == ETHERNET_1000)
    {
      connection->standard = standard;
      connection->new_operating_rate = ETH_RATE_1000MBPS;
      connection->operating_rate = connection->new_operating_rate;
      connection->bandwidth = eth_operating_rates[connection->operating_rate];
    }
  else if (standard == ACTIVE_TAG)
    {
      connection->standard = standard;
      connection->new_operating_rate = 0;
      connection->operating_rate = connection->new_operating_rate;
      connection->bandwidth = 0;	// unknown since depends on each active tag
      //active_tag_operating_rates[connection->operating_rate];
    }
  else if (standard == ZIGBEE)
    {
      connection->standard = standard;
      connection->new_operating_rate = 0;
      connection->operating_rate = connection->new_operating_rate;
      connection->bandwidth =
	zigbee_operating_rates[connection->operating_rate];

      // channel needs to be initialized as well, but it is not used yet
    }
  else if (standard == WIMAX_802_16)
    {
      connection->standard = standard;

      // we assume WiMAX adapters start from the lowest rate/MCS and go up
      connection->new_operating_rate = QPSK_1_8;
      connection->operating_rate = connection->new_operating_rate;
      connection->bandwidth =
	wimax_operating_rates[connection->operating_rate];

      // if no channel was defined choose an appropriate channel
      if (connection->channel == UNDEFINED_VALUE)
	connection->channel = DEFAULT_CHANNEL_WIMAX;
    }
  else
    {
      WARNING ("Connection standard '%d' undefined", standard);
      return ERROR;
    }
  return SUCCESS;
}

// initialize all parameters related to the connection operating rate;
// return SUCCESS on succes, ERROR on error
int
connection_init_operating_rate (struct connection_class *connection,
				int operating_rate)
{
  connection->new_operating_rate = operating_rate;
  connection->operating_rate = operating_rate;

  return SUCCESS;
}

// initialize the local indexes for the from_node, to_node, and
// through_environment of a connection;
// return SUCCESS on succes, ERROR on error
int
connection_init_indexes (struct connection_class *connection,
			 struct scenario_class *scenario)
{
  int i, j;
  int found_nodes = 0;
  struct node_class *node;

  // check if "from_node_index" or "to_node_index" are not initialized
  if (connection->from_node_index == INVALID_INDEX ||
      connection->to_node_index == INVALID_INDEX)
    {
      // try to find the "from_node" and "to_node" in scenario
      for (i = 0; i < scenario->node_number; i++)
	{
	  node = &(scenario->nodes[i]);

	  // try to identify from_node
	  if (strcmp (node->name, connection->from_node) == 0)
	    {
	      connection->from_node_index = i;
	      found_nodes++;

	      // check whether a from_interface was defined
	      if (strcmp (connection->from_interface, DEFAULT_STRING) == 0)
		{
		  // from_interface uses default value => assume interface 0
		  connection->from_interface_index = 0;
		  connection->from_id = node->interfaces[0].id;
		}
	      else		// try to find from_interface among those of the node
		{
		  for (j = 0; j < node->if_num; j++)
		    if (strcmp (node->interfaces[j].name,
				connection->from_interface) == 0)
		      {
			connection->from_interface_index = j;
			connection->from_id = node->interfaces[j].id;
			break;
		      }

		  // if j equals node->if_num, then the from_interface 
		  // could not be found => return ERROR
		  if (j >= node->if_num)
		    {
		      WARNING
			("Connection attribute '%s' with value '%s' does not exist for node '%s'.",
			 CONNECTION_FROM_INTERFACE_STRING,
			 connection->from_interface, connection->from_node);
		      return ERROR;
		    }
		}
	    }

	  // try to identify to_node
	  if (strcmp (node->name, connection->to_node) == 0)
	    {
	      connection->to_node_index = i;
	      found_nodes++;

	      // check whether a to_interface was defined
	      if (strcmp (connection->to_interface, DEFAULT_STRING) == 0)
		{
		  // to_interface uses default value => assume interface 0
		  connection->to_interface_index = 0;
		  connection->to_id = node->interfaces[0].id;
		}
	      else		// try to find to_interface among those of the node
		{
		  for (j = 0; j < node->if_num; j++)
		    if (strcmp (node->interfaces[j].name,
				connection->to_interface) == 0)
		      {
			connection->to_interface_index = j;
			connection->to_id = node->interfaces[j].id;
			break;
		      }

		  // if j equals node->if_num, then the to_interface 
		  // could not be found => return ERROR
		  if (j >= node->if_num)
		    {
		      WARNING
			("Connection attribute '%s' with value '%s' does not exist for node '%s'.",
			 CONNECTION_TO_INTERFACE_STRING,
			 connection->to_interface, connection->to_node);
		      return ERROR;
		    }
		}
	    }

	  if (found_nodes == 2)
	    break;
	}
    }

  // check if "from_node" could not be found
  if (connection->from_node_index == INVALID_INDEX)
    {
      WARNING ("Connection attribute 'from_node' with value '%s' doesn't exist \
in scenario", connection->from_node);
      return ERROR;
    }

  // check if "to_node" could not be found
  if (connection->to_node_index == INVALID_INDEX)
    {
      WARNING ("Connection attribute 'to_node' with value '%s' doesn't exist \
in scenario", connection->to_node);
      return ERROR;
    }

  // check if "through_environment_index" not initialized
  if (connection->through_environment_index == INVALID_INDEX)
    {
      uint32_t through_environment_hash =
	string_hash (connection->through_environment,
		     strlen (connection->through_environment));
      // try to find the through_environment in scenario
      for (i = 0; i < scenario->environment_number; i++)
	{			/*
				   INFO("crt_name=%s crt_hash=%d  through=%s through_hash=%d",
				   scenario->environments[i].name, 
				   scenario->environments[i].name_hash,
				   connection->through_environment,
				   through_environment_hash);
				 */
	  if (scenario->environments[i].name_hash == through_environment_hash)
	    if (strcmp (scenario->environments[i].name,
			connection->through_environment) == 0)
	      {
		connection->through_environment_index = i;
		break;
	      }
	}
    }

  // check if "through_environment" could not be found
  if (connection->through_environment_index == INVALID_INDEX)
    {
      WARNING ("Connection attribute 'through_environment' with value '%s' \
doesn't exist in scenario.\n", connection->through_environment);
      return ERROR;
    }

  return SUCCESS;
}

// print the fields of a connection
// change to abort on error?????????
void
connection_print (struct connection_class *connection)
{
  printf ("  Connection: from_node='%s' from_interface=%d to_node='%s' \
to_interface=%d through_environment='%s'\n", connection->from_node, connection->from_interface_index, connection->to_node, connection->to_interface_index, connection->through_environment);

  if ((connection->standard == WLAN_802_11B) ||
      (connection->standard == WLAN_802_11G) ||
      (connection->standard == WLAN_802_11A))
    {
      printf ("    Properties: packet_size=%d standard='%s' channel=%d \
concurrent_stations=%d interference_noise=%.2f \
RTS_CTS_threshold=%d distance=%.4f Pr=%.2f SNR=%.2f\n", connection->packet_size, connection_standards[connection->standard], connection->channel, connection->concurrent_stations, connection->interference_noise, connection->RTS_CTS_threshold, connection->distance, connection->Pr, connection->SNR);

      printf ("    DeltaQ: FER=%.3f num_retr=%.2f op_rate=%.1fMb/s bw=%.1fMb/s \
loss=%.3f delay=%.2fms jitter=%.2fms\n", connection->frame_error_rate,
	      connection->num_retransmissions, connection_get_operating_rate (connection) / 1e6,
	      connection->bandwidth / 1e6, connection->loss_rate, connection->delay, connection->
	      jitter);
    }
  else if ((connection->standard == ETHERNET_10) ||
	   (connection->standard == ETHERNET_100) ||
	   (connection->standard == ETHERNET_1000))
    {
      printf ("    Properties: packet_size=%d standard='%s'distance=%.4f\n",
	      connection->packet_size,
	      connection_standards[connection->standard],
	      connection->distance);

      printf ("    DeltaQ: FER=%.3f op_rate=%.1fMb/s bw=%.1fMb/s \
loss=%.3f delay=%.2fms jitter=%.2fms\n", connection->frame_error_rate, connection_get_operating_rate (connection) / 1e6, connection->bandwidth / 1e6, connection->loss_rate, connection->delay, connection->jitter);
    }
  else if (connection->standard == ACTIVE_TAG)
    {
      printf ("    Properties: packet_size=%d standard='%s' \
concurrent_stations=%d interference_noise=%.2f distance=%.4f Pr=%.2f \
SNR=%.2f consider_interference=%s\n", connection->packet_size, connection_standards[connection->standard], connection->concurrent_stations, connection->interference_noise, connection->distance, connection->Pr, connection->SNR, (connection->consider_interference == TRUE) ? "true" : "false");

      printf ("    DeltaQ: FER=%.3f interference_fer=%.3f num_retr=%.2f \
op_rate=%.0fb/s bw=%.0fb/s loss=%.3f delay=%.2fms jitter=%.2fms\n", connection->frame_error_rate, connection->interference_fer, connection->num_retransmissions, connection_get_operating_rate (connection), connection->bandwidth, connection->loss_rate, connection->delay, connection->jitter);
    }
  else if (connection->standard == ZIGBEE)
    {
      printf ("    Properties: packet_size=%d standard='%s' \
concurrent_stations=%d interference_noise=%.2f distance=%.4f Pr=%.2f \
SNR=%.2f consider_interference=%s\n", connection->packet_size, connection_standards[connection->standard], connection->concurrent_stations, connection->interference_noise, connection->distance, connection->Pr, connection->SNR, (connection->consider_interference == TRUE) ? "true" : "false");

      printf ("    DeltaQ: FER=%.3f interference_fer=%.3f num_retr=%.2f \
op_rate=%.0fb/s bw=%.0fb/s loss=%.3f delay=%.2fms jitter=%.2fms\n", connection->frame_error_rate, connection->interference_fer, connection->num_retransmissions, connection_get_operating_rate (connection), connection->bandwidth, connection->loss_rate, connection->delay, connection->jitter);
    }
  else
    printf ("    Properties: ERROR: Unknown connection type\n");

  if (connection->fixed_deltaQ_number > 0)
    printf ("    fixed_deltaQ: #%d records\n",
	    connection->fixed_deltaQ_number);
}

// copy the information in connection_src to connection_dst
void
connection_copy (struct connection_class *connection_dst,
		 struct connection_class *connection_src)
{
  int i;

  strncpy (connection_dst->from_node, connection_src->from_node,
	   MAX_STRING - 1);
  connection_dst->from_node_index = connection_src->from_node_index;
  strncpy (connection_dst->from_interface, connection_src->from_interface,
	   MAX_STRING - 1);
  connection_dst->from_interface_index = connection_src->from_interface_index;
  connection_dst->from_id = connection_src->from_id;

  strncpy (connection_dst->to_node, connection_src->to_node, MAX_STRING - 1);
  connection_dst->to_node_index = connection_src->to_node_index;
  strncpy (connection_dst->to_interface, connection_src->to_interface,
	   MAX_STRING - 1);
  connection_dst->to_interface_index = connection_src->to_interface_index;
  connection_dst->to_id = connection_src->to_id;

  strncpy (connection_dst->through_environment,
	   connection_src->through_environment, MAX_STRING - 1);
  connection_dst->through_environment_index =
    connection_src->through_environment_index;

  connection_dst->standard = connection_src->standard;
  connection_dst->channel = connection_src->channel;
  connection_dst->concurrent_stations = connection_src->concurrent_stations;
  connection_dst->interference_noise = connection_src->interference_noise;
  connection_dst->compatibility_mode = connection_src->compatibility_mode;
  connection_dst->RTS_CTS_threshold = connection_src->RTS_CTS_threshold;
  connection_dst->packet_size = connection_src->packet_size;
  connection_dst->consider_interference =
    connection_src->consider_interference;

  // initialize dynamic parameters
  connection_dst->Pr = connection_src->Pr;
  connection_dst->SNR = connection_src->SNR;
  connection_dst->distance = connection_src->distance;

  // operating rate parameters
  connection_dst->operating_rate = connection_src->operating_rate;
  connection_dst->new_operating_rate = connection_src->new_operating_rate;
  connection_dst->adaptive_operating_rate =
    connection_src->adaptive_operating_rate;

  // default initial settings for deltaQ parameters
  connection_dst->frame_error_rate = connection_src->frame_error_rate;
  connection_dst->interference_fer = connection_src->interference_fer;
  connection_dst->num_retransmissions = connection_src->num_retransmissions;
  connection_dst->variable_delay = connection_src->variable_delay;
  connection_dst->loss_rate = connection_src->loss_rate;
  connection_dst->loss_rate_defined = connection_src->loss_rate_defined;
  connection_dst->delay = connection_src->delay;
  connection_dst->delay_defined = connection_src->delay_defined;
  connection_dst->jitter = connection_src->jitter;
  connection_dst->jitter_defined = connection_src->jitter_defined;
  connection_dst->bandwidth = connection_src->bandwidth;
  connection_dst->bandwidth_defined = connection_src->bandwidth_defined;

  // copy fixed_deltaQ properties
  for (i = 0; i < connection_src->fixed_deltaQ_number; i++)
    fixed_deltaQ_copy (&(connection_dst->fixed_deltaQs[i]),
		       &(connection_src->fixed_deltaQs[i]));
  connection_dst->fixed_deltaQ_number = connection_src->fixed_deltaQ_number;
  connection_dst->fixed_deltaQ_crt = connection_src->fixed_deltaQ_crt;

  // no pointers in the capacity structure, so we copy directly from src to dst
  //connection_dst->wimax_capacity = connection_src->wimax_capacity;
}

// return the current operating rate of a connection
double
connection_get_operating_rate (struct connection_class *connection)
{
  if (connection->standard == WLAN_802_11B)
    if ((connection->operating_rate < B_RATE_1MBPS) ||
	(connection->operating_rate > B_RATE_11MBPS))
      {
	WARNING ("Invalid operating rate index '%d' for 802.11b.",
		 connection->operating_rate);
	return 0.0;
      }
    else
      return b_operating_rates[connection->operating_rate];
  else if (connection->standard == WLAN_802_11G)
    if ((connection->operating_rate < G_RATE_1MBPS) ||
	(connection->operating_rate > G_RATE_54MBPS))
      {
	WARNING ("Invalid operating rate index '%d' for 802.11g.",
		 connection->operating_rate);
	return 0.0;
      }
    else
      return g_operating_rates[connection->operating_rate];
  else if (connection->standard == WLAN_802_11A)
    if ((connection->operating_rate < A_RATE_6MBPS) ||
	(connection->operating_rate > A_RATE_54MBPS))
      {
	WARNING ("Invalid operating rate index '%d' for 802.11a.",
		 connection->operating_rate);
	return 0.0;
      }
    else
      return a_operating_rates[connection->operating_rate];
  else if ((connection->standard == ETHERNET_10) ||
	   (connection->standard == ETHERNET_100) ||
	   (connection->standard == ETHERNET_1000))
    if ((connection->operating_rate < ETH_RATE_10MBPS) ||
	(connection->operating_rate > ETH_RATE_1000MBPS))
      {
	WARNING ("Invalid operating rate index '%d' for Ethernet.",
		 connection->operating_rate);
	return 0.0;
      }
    else
      return eth_operating_rates[connection->operating_rate];
  else if (connection->standard == ACTIVE_TAG)
    {
      // operating rate is managed by the tag emulator itself,
      // so we cannot return a useful value
      WARNING
	("The operating rate is not handled by deltaQ for Active Tags.");
      return 0.0;
    }
  else if (connection->standard == ZIGBEE)
    if (connection->operating_rate != 0)
      {
	WARNING ("Invalid operating rate index '%d' for 802.15.4",
		 connection->operating_rate);
	return 0.0;
      }
    else
      return zigbee_operating_rates[connection->operating_rate];
  else if (connection->standard == WIMAX_802_16)
    if ((connection->operating_rate < QPSK_1_8) ||
	(connection->operating_rate > QAM_64_5_6))
      {
	WARNING ("Invalid operating rate index '%d' for 802.16",
		 connection->operating_rate);
	return 0.0;
      }
    else
      {
	return wimax_operating_rates[connection->operating_rate];
	/*
	   struct capacity_class *capacity 
	   = &(((scenario->nodes[connection->from_node_index])
	   .interfaces[connection->from_interface_index]).wimax_params);
	   return (capacity->dl_data_rate + capacity->ul_data_rate);
	 */
      }
  else
    {
      WARNING ("Connection standard '%d' undefined", connection->standard);
      // we use the special value 0.0 to indicate an error
      return 0.0;
    }
}


// compute deltaQ parameters of a connection;
// deltaQ parameters are returned in the corresponding fields of
// the connection object and set the last argument to TRUE if any 
// parameter values were changed, or FALSE otherwise;
// return SUCCESS on succes, ERROR on error
int
connection_do_compute (struct connection_class *connection,
		       struct scenario_class *scenario, int *deltaQ_changed)
{
  double old_loss_rate, old_delay, old_jitter, old_bandwidth;

  // SPECIAL: assign current operating rate according to 
  // the last computed connection->new_operating_rate
  connection->operating_rate = connection->new_operating_rate;

  // save the previous values so that we can tell if they changed
  // after they are recomputed
  old_loss_rate = connection->loss_rate;
  old_delay = connection->delay;
  old_jitter = connection->jitter;
  old_bandwidth = connection->bandwidth;

  if ((connection->standard == WLAN_802_11B) ||
      (connection->standard == WLAN_802_11G) ||
      (connection->standard == WLAN_802_11A))
    {
      if (connection->consider_interference == TRUE)
	{
	  if (wlan_interference (connection, scenario) == ERROR)
	    {
	      WARNING ("Error while computing interference factor (WLAN)");
	      return ERROR;
	    }
	}
      else
	{
	  //reset interference indicators
	  connection->concurrent_stations = 0;
	  connection->interference_noise = MINIMUM_NOISE_POWER;
	}

      if (connection->loss_rate_defined == FALSE)
	// compute loss rate (and FER)
	if (wlan_loss_rate (connection, scenario,
			    &(connection->loss_rate)) == ERROR)
	  {
	    WARNING ("Error while computing loss rate (WLAN)");
	    return ERROR;
	  }

      // compute number of retransmissions
      if (wlan_retransmissions (connection, scenario,
				&(connection->num_retransmissions)) == ERROR)
	{
	  WARNING ("Error while computing number of retransmissions (WLAN)");
	  return ERROR;
	}

      if (connection->adaptive_operating_rate == TRUE)
	// compute operating rate based on FER and ARF algorithm
	if (wlan_operating_rate (connection, scenario,
				 &(connection->new_operating_rate)) == ERROR)
	  {
	    WARNING ("Error while computing operating rate (WLAN)");
	    return ERROR;
	  }

      if (connection->delay_defined == FALSE
	  || connection->jitter_defined == FALSE)
	// compute delay & jitter based on FER
	if (wlan_delay_jitter (connection, scenario,
			       &(connection->variable_delay),
			       &(connection->delay),
			       &(connection->jitter)) == ERROR)
	  {
	    WARNING ("Error while computing delay & jitter (WLAN)");
	    return ERROR;
	  }

      if (connection->bandwidth_defined == FALSE)
	// compute bandwidth based on operating rate, delay and packet size
	if (wlan_bandwidth (connection, scenario,
			    &(connection->bandwidth)) == ERROR)
	  {
	    WARNING ("Error while computing bandwidth (WLAN)");
	    return ERROR;
	  }
    }
  else if ((connection->standard == ETHERNET_10) ||
	   (connection->standard == ETHERNET_100) ||
	   (connection->standard == ETHERNET_1000))
    {
      if (connection->loss_rate_defined == FALSE)
	// compute loss rate
	if (ethernet_loss_rate (connection, scenario,
				&(connection->loss_rate)) == ERROR)
	  {
	    WARNING ("Error while computing loss rate (Ethernet)");
	    return ERROR;
	  }

      if (connection->adaptive_operating_rate == TRUE)
	// compute operating rate
	if (ethernet_operating_rate (connection, scenario,
				     &(connection->new_operating_rate)) ==
	    ERROR)
	  {
	    WARNING ("Error while computing operating rate (Ethernet)");
	    return ERROR;
	  }

      if (connection->delay_defined == FALSE
	  && connection->jitter_defined == FALSE)
	// compute delay & jitter
	if (ethernet_delay_jitter (connection, scenario,
				   &(connection->variable_delay),
				   &(connection->delay),
				   &(connection->jitter)) == ERROR)
	  {
	    WARNING ("Error while computing delay & jitter (Ethernet)");
	    return ERROR;
	  }

      if (connection->bandwidth_defined == FALSE)
	// compute bandwidth based on operating rate, delay and packet size
	if (ethernet_bandwidth (connection, scenario,
				&(connection->bandwidth)) == ERROR)
	  {
	    WARNING ("Error while computing bandwidth (Ethernet)");
	    return ERROR;
	  }
    }
  else if (connection->standard == ACTIVE_TAG)
    {
      if (connection->consider_interference == TRUE)
	{
	  if (active_tag_interference (connection, scenario) == ERROR)
	    {
	      WARNING
		("Error while computing interference factor (ActiveTag)");
	      return ERROR;
	    }
	}
      else
	{
	  //reset interference indicators
	  connection->concurrent_stations = 0;
	  //connection->interference_noise = MINIMUM_ACTIVE_TAG_NOISE_POWER;
	}

      if (connection->loss_rate_defined == FALSE)
	// compute loss rate
	if (active_tag_loss_rate (connection, scenario,
				  &(connection->loss_rate)) == ERROR)
	  {
	    WARNING ("Error while computing loss rate (ActiveTag)");
	    return ERROR;
	  }

      if (connection->adaptive_operating_rate == TRUE)
	// compute operating rate
	if (active_tag_operating_rate (connection, scenario,
				       &(connection->new_operating_rate)) ==
	    ERROR)
	  {
	    WARNING ("Error while computing operating rate (ActiveTag)");
	    return ERROR;
	  }

      if (connection->delay_defined == FALSE
	  && connection->jitter_defined == FALSE)
	// compute delay & jitter
	if (active_tag_delay_jitter (connection, scenario,
				     &(connection->variable_delay),
				     &(connection->delay),
				     &(connection->jitter)) == ERROR)
	  {
	    WARNING ("Error while computing delay & jitter (ActiveTag)");
	    return ERROR;
	  }

      if (connection->bandwidth_defined == FALSE)
	// compute bandwidth based on operating rate, delay and packet size
	if (active_tag_bandwidth (connection, scenario,
				  &(connection->bandwidth)) == ERROR)
	  {
	    WARNING ("Error while computing bandwidth (ActiveTag)");
	    return ERROR;
	  }
    }
  else if (connection->standard == ZIGBEE)
    {
      if (connection->consider_interference == TRUE)
	{
	  if (zigbee_interference (connection, scenario) == ERROR)
	    {
	      WARNING ("Error while computing interference factor (ZigBee)");
	      return ERROR;
	    }
	}
      else
	{
	  //reset interference indicators
	  connection->concurrent_stations = 0;
	  connection->interference_noise = ZIGBEE_MINIMUM_NOISE_POWER;
	}

      if (connection->loss_rate_defined == FALSE)
	// compute loss rate
	if (zigbee_loss_rate (connection, scenario,
			      &(connection->loss_rate)) == ERROR)
	  {
	    WARNING ("Error while computing loss rate (ZigBee)");
	    return ERROR;
	  }

      if (connection->delay_defined == FALSE
	  && connection->jitter_defined == FALSE)
	// compute delay & jitter
	if (zigbee_delay_jitter (connection, scenario,
				 &(connection->variable_delay),
				 &(connection->delay),
				 &(connection->jitter)) == ERROR)
	  {
	    WARNING ("Error while computing delay & jitter (ZigBee)");
	    return ERROR;
	  }

      if (connection->bandwidth_defined == FALSE)
	// compute bandwidth based on operating rate, delay and packet size
	if (zigbee_bandwidth (connection, scenario,
			      &(connection->bandwidth)) == ERROR)
	  {
	    WARNING ("Error while computing bandwidth (ZigBee)");
	    return ERROR;
	  }
    }
  else if (connection->standard == WIMAX_802_16)
    {
      /*
         if (connection->consider_interference == TRUE)
         {
         if (zigbee_interference (connection, scenario) == ERROR)
         {
         WARNING ("Error while computing interference factor (ZigBee)");
         return ERROR;
         }
         }
         else
         {
         //reset interference indicators
         connection->concurrent_stations = 0;
         connection->interference_noise = ZIGBEE_MINIMUM_NOISE_POWER;
         }
       */
      if (connection->loss_rate_defined == FALSE)
	// compute loss rate
	if (wimax_loss_rate (connection, scenario,
			     &(connection->loss_rate)) == ERROR)
	  {
	    WARNING ("Error while computing loss rate (WiMAX)");
	    return ERROR;
	  }

      if (connection->delay_defined == FALSE
	  && connection->jitter_defined == FALSE)
	// compute delay & jitter
	if (wimax_delay_jitter (connection, scenario,
				&(connection->variable_delay),
				&(connection->delay),
				&(connection->jitter)) == ERROR)
	  {
	    WARNING ("Error while computing delay & jitter (WiMAX)");
	    return ERROR;
	  }

      if (connection->bandwidth_defined == FALSE)
	// compute bandwidth based
	if (wimax_bandwidth (connection, scenario,
			     &(connection->bandwidth)) == ERROR)
	  {
	    WARNING ("Error while computing bandwidth (WiMAX)");
	    return ERROR;
	  }
    }
  else
    {
      WARNING ("Connection standard '%d' undefined", connection->standard);
      return ERROR;
    }

  if ((connection->operating_rate != connection->new_operating_rate) ||
      (old_loss_rate != connection->loss_rate) ||
      (old_delay != connection->delay) ||
      (old_jitter != connection->jitter) ||
      (old_bandwidth != connection->bandwidth))
    (*deltaQ_changed) = TRUE;
  else
    (*deltaQ_changed) = FALSE;

  return SUCCESS;
}

// update the state and calculate all deltaQ parameters 
// for the current connection;
// deltaQ parameters are returned in the corresponding fields of
// the connection objects and the last argument is set to TRUE if 
// any parameter values were changed, or to FALSE otherwise;
// return SUCCESS on succes, ERROR on error
int
connection_deltaQ (struct connection_class *connection,
		   struct scenario_class *scenario, int *deltaQ_changed)
{
  // check if "from_node" could not be found
  if (connection->from_node_index == INVALID_INDEX)
    {
      WARNING ("Connection attribute 'from_node'=%s doesn't exist \
in scenario", connection->from_node);
      return ERROR;
    }

  // check if "to_node" could not be found
  if (connection->to_node_index == INVALID_INDEX)
    {
      WARNING ("Connection attribute 'to_node'=%s doesn't exist \
in scenario", connection->to_node);
      return ERROR;
    }

  // check if "through_environment" could not be found
  if (connection->through_environment_index == INVALID_INDEX)
    {
      WARNING ("Connection attribute 'through_environment'=%s \
doesn't exist in scenario.\n", connection->through_environment);
      return ERROR;
    }

  // make calculations for current position of the to_node, 
  // given the state of the from_node and through_environment

  if (scenario->environments
      [connection->through_environment_index].is_dynamic == TRUE)
    {
      INFO ("Updating a dynamic environment...");

      // call update function
      environment_update (&(scenario->environments
			    [connection->through_environment_index]),
			  connection, scenario);
#ifdef MESSAGE_INFO
      environment_print (&(scenario->environments
			   [connection->through_environment_index]));
#endif
    }

  if (connection->standard == WLAN_802_11A ||
      connection->standard == WLAN_802_11B ||
      connection->standard == WLAN_802_11G)
    {
      // update node state with what concerns WLAN communication
      if (wlan_connection_update (connection, scenario) == ERROR)
	{
	  WARNING ("Error while updating connection state");
	  return ERROR;
	}
    }
  else if ((connection->standard == ETHERNET_10) ||
	   (connection->standard == ETHERNET_100) ||
	   (connection->standard == ETHERNET_1000))
    {
      // update connection properties
      if (ethernet_connection_update (connection, scenario) == ERROR)
	{
	  WARNING ("Error while updating connection state");
	  return ERROR;
	}
    }
  else if (connection->standard == ACTIVE_TAG)
    {
      // update node state with what concerns active tag communication
      if (active_tag_connection_update (connection, scenario) == ERROR)
	{
	  WARNING ("Error while updating connection state");
	  return ERROR;
	}
    }
  else if (connection->standard == ZIGBEE)
    {
      // update node state with what concerns ZigBee communication
      if (zigbee_connection_update (connection, scenario) == ERROR)
	{
	  WARNING ("Error while updating connection state");
	  return ERROR;
	}
    }
  else if (connection->standard == WIMAX_802_16)
    {
      // update node state with what concerns WiMAX communication
      if (wimax_connection_update (connection, scenario) == ERROR)
	{
	  WARNING ("Error while updating connection state");
	  return ERROR;
	}
    }
  else
    {
      WARNING ("Connection standard '%d' undefined", connection->standard);
      return ERROR;
    }

  // compute deltaQ for connection
  if (connection_do_compute (connection, scenario, deltaQ_changed) == ERROR)
    {
      WARNING ("Error while computing connection parameters");
      return ERROR;
    }

#ifdef MESSAGE_INFO
  INFO ("Connection updated:");
  // print connection state
  connection_print (connection);
#endif

  return SUCCESS;
}

struct fixed_deltaQ_class *
connection_add_fixed_deltaQ (struct connection_class *connection,
			     struct fixed_deltaQ_class *fixed_deltaQ)
{
  if (connection->fixed_deltaQ_number < MAX_FIXED_DELTAQ)
    {
      if ((connection->fixed_deltaQ_number > 0) &&
	  (connection->fixed_deltaQs
	   [connection->fixed_deltaQ_number - 1].end_time
	   > fixed_deltaQ->start_time))
	{
	  WARNING ("New fixed_deltaQ structure has a start time preceding \
the end time of the previous one.");
	  return NULL;
	}

      connection->fixed_deltaQ_number++;
      fixed_deltaQ_copy
	(&(connection->fixed_deltaQs[connection->fixed_deltaQ_number - 1]),
	 fixed_deltaQ);
      return &(connection->fixed_deltaQs
	       [connection->fixed_deltaQ_number - 1]);
    }
  else
    {
      WARNING ("Maximum number of fixed_deltaQ entries (%d) was reached.",
	       MAX_FIXED_DELTAQ);
      return NULL;
    }
}
