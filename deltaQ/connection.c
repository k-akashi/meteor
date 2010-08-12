
/*
 * Copyright (c) 2006-2009 The StarBED Project  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
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


//////////////////////////////////
// Global variable initialization
//////////////////////////////////

char *connection_standards[]={"802.11b", "802.11g", "802.11a", "Eth-10Mbps",
			      "Eth-10/100 Mbps", "Eth-10/100/1000Mbps",
                              "ActiveTag", "ZigBee"};


/////////////////////////////////////////
// Connection structure functions
/////////////////////////////////////////

// init a connection;
// return SUCCESS on succes, ERROR on error
int connection_init(connection_class *connection, char *from_node, 
		    char *to_node, char *through_environment,
		    int packet_size, int standard, int channel, 
		    int RTS_CTS_threshold, int consider_interference)
{
  strncpy(connection->from_node, from_node, MAX_STRING-1);
  connection->from_node_index = INVALID_INDEX;
  strncpy(connection->to_node, to_node, MAX_STRING-1);
  connection->to_node_index = INVALID_INDEX;
  strncpy(connection->through_environment, through_environment, MAX_STRING-1);
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

  // do this at the end since it may cause an error
  return connection_init_standard(connection, standard);
}

// initialize all parameters related to a specific wireless standard;
// return SUCCESS on succes, ERROR on error
int connection_init_standard(connection_class *connection, int standard)
{
  if(standard==WLAN_802_11B)
    {
      connection->standard = standard;
      // we assume now that WLAN adapters start from the lowest rate and go up
      connection->new_operating_rate = B_RATE_1MBPS;
      connection->operating_rate = connection->new_operating_rate;
      connection->bandwidth = b_operating_rates[connection->operating_rate];

      // if no channel was defined choose an appropriate channel
      if(connection->channel==UNDEFINED_VALUE)
	connection->channel = DEFAULT_CHANNEL_BG;
    }
  else if(standard==WLAN_802_11G)
    {
      connection->standard = standard;
      // we assume now that WLAN adapters start from the lowest rate and go up
      connection->new_operating_rate = G_RATE_1MBPS;
      connection->operating_rate = connection->new_operating_rate;
      connection->bandwidth = g_operating_rates[connection->operating_rate];

      // if no channel was defined choose an appropriate channel
      if(connection->channel==UNDEFINED_VALUE)
	connection->channel = DEFAULT_CHANNEL_BG;
    }
  else if(standard==WLAN_802_11A)
    {
      connection->standard = standard;
      // we assume now that WLAN adapters start from the lowest rate and go up
      connection->new_operating_rate = A_RATE_6MBPS;
      connection->operating_rate = connection->new_operating_rate;
      connection->bandwidth = g_operating_rates[connection->operating_rate];
 
      // if no channel was defined choose an appropriate channel
      if(connection->channel==UNDEFINED_VALUE)
	connection->channel = DEFAULT_CHANNEL_A;
   }
  else if(standard==ETHERNET_10)
    {
      connection->standard = standard;
      connection->new_operating_rate = ETH_RATE_10MBPS;
      connection->operating_rate = connection->new_operating_rate;
      connection->bandwidth = eth_operating_rates[connection->operating_rate];
    }
  else if(standard==ETHERNET_100)
    {
      connection->standard = standard;
      connection->new_operating_rate = ETH_RATE_100MBPS;
      connection->operating_rate = connection->new_operating_rate;
      connection->bandwidth = eth_operating_rates[connection->operating_rate];
    }
  else if(standard==ETHERNET_1000)
    {
      connection->standard = standard;
      connection->new_operating_rate = ETH_RATE_1000MBPS;
      connection->operating_rate = connection->new_operating_rate;
      connection->bandwidth = eth_operating_rates[connection->operating_rate];
    }
  else if(standard==ACTIVE_TAG)
    {
      connection->standard = standard;
      connection->new_operating_rate = 0;
      connection->operating_rate = connection->new_operating_rate;
      connection->bandwidth = 0; // unknown since depends on each active tag
      //active_tag_operating_rates[connection->operating_rate];
    }
  else if(standard==ZIGBEE)
    {
      connection->standard = standard;
      connection->new_operating_rate = 0;
      connection->operating_rate = connection->new_operating_rate;
      connection->bandwidth = 
	zigbee_operating_rates[connection->operating_rate];

      // channel needs to be initialized as well, but it is not used yet

    }
 else
   {
     WARNING("Connection standard '%d' undefined", standard);
     return ERROR;
   }
 return SUCCESS;
}

// initialize the local indexes for the from_node, to_node, and
// through_environment of a connection;
// return SUCCESS on succes, ERROR on error
int connection_init_indexes(connection_class *connection, 
			    scenario_class *scenario)
{
  int i;
  int found_nodes=0;

  // check if "from_node_index" or "to_node_index" are not initialized
  if(connection->from_node_index == INVALID_INDEX ||
     connection->to_node_index == INVALID_INDEX)
    {
      // try to find the "from_node" and "to_node" in scenario
      for(i=0; i<scenario->node_number; i++)
	{
	  if(strcmp(scenario->nodes[i].name, connection->from_node)==0)
	    {
	      connection->from_node_index = i;
	      found_nodes++;
	    }
	  else if(strcmp(scenario->nodes[i].name, connection->to_node)==0)
	    {
	      connection->to_node_index = i;
	      found_nodes++;
	    }
	  if(found_nodes==2)
	    break;
	}
    }

  // check if "from_node" could not be found
  if(connection->from_node_index == INVALID_INDEX) 
    {
      WARNING("Connection attribute 'from_node' with value '%s' doesn't exist \
in scenario", connection->from_node);
      return ERROR;
    }

  // check if "to_node" could not be found
  if(connection->to_node_index == INVALID_INDEX) 
    {
      WARNING("Connection attribute 'to_node' with value '%s' doesn't exist \
in scenario", connection->to_node);
      return ERROR;
    }

  // check if "through_environment_index" not initialized
  if(connection->through_environment_index == INVALID_INDEX)
    {
      uint32_t through_environment_hash=
	string_hash(connection->through_environment, 
		    strlen(connection->through_environment));
      // try to find the through_environment in scenario
      for(i=0; i<scenario->environment_number; i++)
	{/*
	  INFO("crt_name=%s crt_hash=%d  through=%s through_hash=%d",
	       scenario->environments[i].name, 
	       scenario->environments[i].name_hash,
	       connection->through_environment,
	       through_environment_hash);
	 */
	if(scenario->environments[i].name_hash==
	   through_environment_hash)
	  if(strcmp(scenario->environments[i].name, 
		    connection->through_environment)==0)
	    {
	      connection->through_environment_index = i;
	      break;
	    }
	}
    }

  // check if "through_environment" could not be found
  if(connection->through_environment_index == INVALID_INDEX) 
    {
      WARNING("Connection attribute 'through_environment' with value '%s' \
doesn't exist in scenario.\n", connection->through_environment);
      return ERROR;
    }

  return SUCCESS;
}

// print the fields of a connection
// change to abort on error?????????
void connection_print(connection_class *connection)
{
  printf("  Connection: from_node='%s' to_node='%s' \
through_environment='%s'\n", connection->from_node, connection->to_node, 
	 connection->through_environment);

  if((connection->standard==WLAN_802_11B) ||
     (connection->standard==WLAN_802_11G) ||
     (connection->standard==WLAN_802_11A))
    {
      printf("    Properties: packet_size=%d standard='%s' channel=%d \
concurrent_stations=%d interference_noise=%.2f \
RTS_CTS_threshold=%d distance=%.4f Pr=%.2f SNR=%.2f\n", 
	     connection->packet_size,
	     connection_standards[connection->standard], connection->channel,
	     connection->concurrent_stations, 
	     connection->interference_noise, connection->RTS_CTS_threshold, 
	     connection->distance, connection->Pr, connection->SNR);

      printf("    DeltaQ: FER=%.3f num_retr=%.2f op_rate=%.1fMb/s bw=%.1fMb/s \
loss=%.3f delay=%.2fms jitter=%.2fms\n", connection->frame_error_rate, 
	     connection->num_retransmissions, 
	     connection_operating_rate(connection)/1e6,
	     connection->bandwidth/1e6, connection->loss_rate, 
	     connection->delay, connection->jitter);
    }
  else if ((connection->standard==ETHERNET_10) || 
	   (connection->standard==ETHERNET_100) ||
	   (connection->standard==ETHERNET_1000))
    {
      printf("    Properties: packet_size=%d standard='%s'distance=%.4f\n", 
	     connection->packet_size, 
	     connection_standards[connection->standard], connection->distance);

      printf("    DeltaQ: FER=%.3f op_rate=%.1fMb/s bw=%.1fMb/s \
loss=%.3f delay=%.2fms jitter=%.2fms\n", connection->frame_error_rate, 
	     connection_operating_rate(connection)/1e6,
	     connection->bandwidth/1e6, connection->loss_rate, 
	     connection->delay, connection->jitter);
    }
  else if (connection->standard==ACTIVE_TAG)
    {
      printf("    Properties: packet_size=%d standard='%s' \
concurrent_stations=%d interference_noise=%.2f distance=%.4f Pr=%.2f \
SNR=%.2f consider_interference=%s\n", connection->packet_size, 
	     connection_standards[connection->standard],
	     connection->concurrent_stations, connection->interference_noise, 
	     connection->distance, connection->Pr, connection->SNR,
	     (connection->consider_interference==TRUE)?"true":"false");

      printf("    DeltaQ: FER=%.3f interference_fer=%.3f num_retr=%.2f \
op_rate=%.0fb/s bw=%.0fb/s loss=%.3f delay=%.2fms jitter=%.2fms\n", 
	     connection->frame_error_rate, connection->interference_fer, 
	     connection->num_retransmissions, 
	     connection_operating_rate(connection),
	     connection->bandwidth, connection->loss_rate, 
	     connection->delay, connection->jitter);
    }
  else if (connection->standard==ZIGBEE)
    {
      printf("    Properties: packet_size=%d standard='%s' \
concurrent_stations=%d interference_noise=%.2f distance=%.4f Pr=%.2f \
SNR=%.2f consider_interference=%s\n", connection->packet_size, 
	     connection_standards[connection->standard],
	     connection->concurrent_stations, connection->interference_noise, 
	     connection->distance, connection->Pr, connection->SNR,
	     (connection->consider_interference==TRUE)?"true":"false");

      printf("    DeltaQ: FER=%.3f interference_fer=%.3f num_retr=%.2f \
op_rate=%.0fb/s bw=%.0fb/s loss=%.3f delay=%.2fms jitter=%.2fms\n", 
	     connection->frame_error_rate, connection->interference_fer, 
	     connection->num_retransmissions, 
	     connection_operating_rate(connection),
	     connection->bandwidth, connection->loss_rate, 
	     connection->delay, connection->jitter);
    }
  else
    printf("    Properties: ERROR: Unknown connection type\n");
}

// copy the information in connection_src to connection_dest
void connection_copy(connection_class *connection_dest, 
		     connection_class * connection_src)
{
  strncpy(connection_dest->from_node, connection_src->from_node, MAX_STRING-1);
  connection_dest->from_node_index = connection_src->from_node_index;
  strncpy(connection_dest->to_node, connection_src->to_node, MAX_STRING-1);
  connection_dest->to_node_index = connection_src->to_node_index;
  strncpy(connection_dest->through_environment, 
	  connection_src->through_environment, MAX_STRING-1);
  connection_dest->through_environment_index = 
    connection_src->through_environment_index;

  connection_dest->standard = connection_src->standard;
  connection_dest->channel = connection_src->channel;
  connection_dest->concurrent_stations = connection_src->concurrent_stations;
  connection_dest->interference_noise = connection_src->interference_noise;
  connection_dest->compatibility_mode = connection_src->compatibility_mode;
  connection_dest->RTS_CTS_threshold = connection_src->RTS_CTS_threshold;
  connection_dest->packet_size = connection_src->packet_size;
  connection_dest->consider_interference = 
    connection_src->consider_interference;


  // initialize dynamic parameters
  connection_dest->Pr = connection_src->Pr;
  connection_dest->SNR = connection_src->SNR;
  connection_dest->distance = connection_src->distance;

  connection_dest->operating_rate = connection_src->operating_rate;
  connection_dest->new_operating_rate = connection_src->new_operating_rate;

  // default initial settings for deltaQ parameters
  connection_dest->frame_error_rate = connection_src->frame_error_rate;
  connection_dest->interference_fer = connection_src->interference_fer;
  connection_dest->num_retransmissions = connection_src->num_retransmissions;
  connection_dest->variable_delay = connection_src->variable_delay;
  connection_dest->loss_rate = connection_src->loss_rate;
  connection_dest->loss_rate_defined = connection_src->loss_rate_defined;
  connection_dest->delay = connection_src->delay;
  connection_dest->delay_defined = connection_src->delay_defined;
  connection_dest->jitter = connection_src->jitter;
  connection_dest->jitter_defined = connection_src->jitter_defined;
  connection_dest->bandwidth = connection_src->bandwidth;
  connection_dest->bandwidth_defined = connection_src->bandwidth_defined;
}

// return the current operating rate of a connection
double connection_operating_rate(connection_class *connection)
{
  if(connection->standard==WLAN_802_11B)
    return b_operating_rates[connection->operating_rate];
  else if (connection->standard==WLAN_802_11G)
    return g_operating_rates[connection->operating_rate];
  else if (connection->standard==WLAN_802_11A)
    return a_operating_rates[connection->operating_rate];
  else if ((connection->standard==ETHERNET_10) || 
	   (connection->standard==ETHERNET_100) ||
	   (connection->standard==ETHERNET_1000))
    return eth_operating_rates[connection->operating_rate];
  else if (connection->standard==ACTIVE_TAG)
    //return active_tag_operating_rates[connection->operating_rate];
    return 0;// unknown because depends on active tag
  else if (connection->standard==ZIGBEE)
    return zigbee_operating_rates[connection->operating_rate];
  else
    {
      WARNING("Connection standard '%d' undefined", connection->standard);
      // we use the special value 0.0 to indicate an error
      return 0.0;
    }
}


// compute deltaQ parameters of a connection;
// deltaQ parameters are returned in the corresponding fields of
// the connection object and set the last argument to TRUE if any 
// parameter values were changed, or FALSE otherwise;
// return SUCCESS on succes, ERROR on error
int connection_do_compute(connection_class *connection, 
			  scenario_class *scenario, int *deltaQ_changed)
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

  if((connection->standard==WLAN_802_11B) || 
     (connection->standard==WLAN_802_11G) ||
     (connection->standard==WLAN_802_11A))
    {
      if(connection->consider_interference==TRUE)
	if(wlan_interference(connection, scenario)==ERROR)
	  {
	    WARNING("Error while computing interference factor (WLAN)");
	    return ERROR;
	  }

      if(connection->loss_rate_defined==FALSE)
	  // compute loss rate (and FER)
	  if(wlan_loss_rate(connection, scenario, 
			    &(connection->loss_rate))==ERROR)
	    {
	      WARNING("Error while computing loss rate (WLAN)");
	      return ERROR;
	    }

      // compute number of retransmissions
      if(wlan_retransmissions(connection, scenario, 
			      &(connection->num_retransmissions))==ERROR)
	{
	  WARNING("Error while computing number of retransmissions (WLAN)");
	  return ERROR;
	}

      // compute operating rate based on FER and ARF algorithm
      if(wlan_operating_rate(connection, scenario, 
			     &(connection->new_operating_rate))==ERROR)
	{
	  WARNING("Error while computing operating rate (WLAN)");
	  return ERROR;
	}

      if(connection->delay_defined==FALSE || connection->jitter_defined==FALSE)
	// compute delay & jitter based on FER
	if(wlan_delay_jitter(connection, scenario, 
			     &(connection->variable_delay),
			     &(connection->delay), 
			     &(connection->jitter))==ERROR)
	  {
	    WARNING("Error while computing delay & jitter (WLAN)");
	    return ERROR;
	  }

      if(connection->bandwidth_defined==FALSE)
	// compute bandwidth based on operating rate, delay and packet size
	if(wlan_bandwidth(connection, scenario, 
			  &(connection->bandwidth))==ERROR)
	  {
	    WARNING("Error while computing bandwidth (WLAN)");
	    return ERROR;
	  }
    }
  else if((connection->standard==ETHERNET_10) || 
	  (connection->standard==ETHERNET_100) ||
	  (connection->standard==ETHERNET_1000))
    {
      if(connection->loss_rate_defined==FALSE)
	// compute loss rate
	if(ethernet_loss_rate(connection, scenario, 
			      &(connection->loss_rate))==ERROR)
	  {
	    WARNING("Error while computing loss rate (Ethernet)");
	    return ERROR;
	  }

      // compute operating rate
      if(ethernet_operating_rate(connection, scenario, 
				 &(connection->new_operating_rate))==ERROR)
	{
	  WARNING("Error while computing operating rate (Ethernet)");
	  return ERROR;
	}

      if(connection->delay_defined==FALSE && connection->jitter_defined==FALSE)
	// compute delay & jitter
	if(ethernet_delay_jitter(connection, scenario, 
				 &(connection->variable_delay),
				 &(connection->delay), 
				 &(connection->jitter))==ERROR)
	  {
	    WARNING("Error while computing delay & jitter (Ethernet)");
	    return ERROR;
	  }

      if(connection->bandwidth_defined==FALSE)
	// compute bandwidth based on operating rate, delay and packet size
	if(ethernet_bandwidth(connection, scenario, 
			      &(connection->bandwidth))==ERROR)
	  {
	    WARNING("Error while computing bandwidth (Ethernet)");
	    return ERROR;
	  }
    }
  else if(connection->standard==ACTIVE_TAG)
    {
      if(connection->consider_interference==TRUE)
	if(active_tag_interference(connection, scenario)==ERROR)
	  {
	    WARNING("Error while computing interference factor (ActiveTag)");
	    return ERROR;
	  }

      if(connection->loss_rate_defined==FALSE)
	// compute loss rate
	if(active_tag_loss_rate(connection, scenario, 
			       &(connection->loss_rate))==ERROR)
	  {
	    WARNING("Error while computing loss rate (ActiveTag)");
	    return ERROR;
	  }

      // compute operating rate
      if(active_tag_operating_rate(connection, scenario, 
				  &(connection->new_operating_rate))==ERROR)
	{
	  WARNING("Error while computing operating rate (ActiveTag)");
	  return ERROR;
	}

      if(connection->delay_defined==FALSE && connection->jitter_defined==FALSE)
	// compute delay & jitter
	if(active_tag_delay_jitter(connection, scenario, 
				  &(connection->variable_delay),
				  &(connection->delay), 
				  &(connection->jitter))==ERROR)
	  {
	    WARNING("Error while computing delay & jitter (ActiveTag)");
	    return ERROR;
	  }

      if(connection->bandwidth_defined==FALSE)
	// compute bandwidth based on operating rate, delay and packet size
	if(active_tag_bandwidth(connection, scenario, 
			       &(connection->bandwidth))==ERROR)
	  {
	    WARNING("Error while computing bandwidth (ActiveTag)");
	    return ERROR;
	  }
    }
  else if(connection->standard==ZIGBEE)
    {
      if(connection->consider_interference==TRUE)
	if(zigbee_interference(connection, scenario)==ERROR)
	  {
	    WARNING("Error while computing interference factor (ZigBee)");
	    return ERROR;
	  }

      if(connection->loss_rate_defined==FALSE)
	// compute loss rate
	if(zigbee_loss_rate(connection, scenario, 
			    &(connection->loss_rate))==ERROR)
	  {
	    WARNING("Error while computing loss rate (ZigBee)");
	    return ERROR;
	  }

      if(connection->delay_defined==FALSE && connection->jitter_defined==FALSE)
	// compute delay & jitter
	if(zigbee_delay_jitter(connection, scenario, 
			       &(connection->variable_delay),
			       &(connection->delay), 
			       &(connection->jitter))==ERROR)
	  {
	    WARNING("Error while computing delay & jitter (ActiveTag)");
	    return ERROR;
	  }

      if(connection->bandwidth_defined==FALSE)
	// compute bandwidth based on operating rate, delay and packet size
	if(zigbee_bandwidth(connection, scenario, 
			    &(connection->bandwidth))==ERROR)
	  {
	    WARNING("Error while computing bandwidth (ZigBee)");
	    return ERROR;
	  }
    }
  else
    {
      WARNING("Connection standard '%d' undefined", connection->standard);
      return ERROR;
    }

  if((connection->operating_rate != connection->new_operating_rate) ||
     (old_loss_rate != connection->loss_rate) ||
     (old_delay != connection->delay) ||
     (old_jitter != connection->jitter) ||
     (old_bandwidth != connection->bandwidth))
    (*deltaQ_changed)=TRUE;
  else
    (*deltaQ_changed)=FALSE;

  return SUCCESS;
}

// update the state and calculate all deltaQ parameters 
// for the current connection;
// deltaQ parameters are returned in the corresponding fields of
// the connection objects and the last argument is set to TRUE if 
// any parameter values were changed, or to FALSE otherwise;
// return SUCCESS on succes, ERROR on error
int connection_deltaQ(connection_class *connection, scenario_class *scenario,
		      int *deltaQ_changed)
{
  // check if "from_node" could not be found
  if(connection->from_node_index == INVALID_INDEX) 
   {
      WARNING("Connection attribute 'from_node'=%s doesn't exist \
in scenario", connection->from_node);
      return ERROR;
   }

  // check if "to_node" could not be found
  if(connection->to_node_index == INVALID_INDEX) 
    {
      WARNING("Connection attribute 'to_node'=%s doesn't exist \
in scenario", connection->to_node);
      return ERROR;
    }

  // check if "through_environment" could not be found
  if(connection->through_environment_index == INVALID_INDEX) 
    {
      WARNING("Connection attribute 'through_environment'=%s \
doesn't exist in scenario.\n", connection->through_environment);
      return ERROR;
    }

  // make calculations for current position of the to_node, 
  // given the state of the from_node and through_environment

  if(scenario->environments
     [connection->through_environment_index].is_dynamic==TRUE)
    {
      INFO("Updating a dynamic environment...");

      // call update function
      environment_update(&(scenario->environments
			   [connection->through_environment_index]),
			 connection,
			 scenario);
#ifdef MESSAGE_INFO
      environment_print(&(scenario->environments
			  [connection->through_environment_index]));
#endif
    }

  if(connection->standard == WLAN_802_11A ||
     connection->standard == WLAN_802_11B ||
     connection->standard == WLAN_802_11G)
    {
      // update node state with what concerns WLAN communication
      if(wlan_connection_update(connection, scenario)==ERROR)
	{
	  WARNING("Error while updating connection state");
	  return ERROR;
	}
    }
  else if ((connection->standard==ETHERNET_10) || 
	   (connection->standard==ETHERNET_100) ||
	   (connection->standard==ETHERNET_1000))
    //eth_connection_update(connection, scenario);
    // the function above is not defined yet, so for the 
    // moment we just do nothing
    ;
  else if (connection->standard==ACTIVE_TAG)
    {
      // update node state with what concerns active tag communication
      if(active_tag_connection_update(connection, scenario)==ERROR)
	{
	  WARNING("Error while updating connection state");
	  return ERROR;
	}
    }
  else if (connection->standard==ZIGBEE)
    {
      // update node state with what concerns ZigBee communication
      if(zigbee_connection_update(connection, scenario)==ERROR)
	{
	  WARNING("Error while updating connection state");
	  return ERROR;
	}
    }
  else
    {
      WARNING("Connection standard '%d' undefined", connection->standard);
      return ERROR;
    }

  // compute deltaQ for connection
  if(connection_do_compute(connection, scenario, deltaQ_changed)==ERROR)
    {
      WARNING("Error while computing connection parameters");
      return ERROR;
    }

#ifdef MESSAGE_INFO
  INFO("Connection updated:");
  // print connection state
  connection_print(connection);
#endif

  return SUCCESS;
}

