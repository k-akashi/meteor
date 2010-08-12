
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
 * File name: node.c
 * Function: Source file related to the node scenario element
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#include <string.h>
#include <stdio.h>
#include <float.h> // for DBL_MAX

#include "message.h"
#include "deltaQ.h"

#include "node.h"
#include "connection.h"
#include "scenario.h"

#include "wlan.h"
#include "active_tag.h"
#include "zigbee.h"


/////////////////////////////////////////
// Global variables initialization
/////////////////////////////////////////

char *node_types[]={"regular", "access_point"};
char *node_connections[]={"ad_hoc", "infrastructure", "any"};


/////////////////////////////////////////
// Node structure functions
/////////////////////////////////////////

// init a node
void node_init(node_class *node, char *name, int type, int id, char *ssid,
	       int connection, int adapter_type, double antenna_gain,
	       double position_x, double position_y, double position_z, 
	       double Pt, double internal_delay)
{
  strncpy(node->name, name, MAX_STRING-1);
  node->type = type;
  node->id = id;
  strncpy(node->ssid, ssid, MAX_STRING-1);
  node->connection = connection;
  node->adapter_type = adapter_type;
  node->antenna_gain = antenna_gain;
  node->azimuth_orientation = 0;
  node->azimuth_beamwidth = 360;
  node->elevation_orientation = 0;
  node->elevation_beamwidth = 360;
  node->position.c[0] = position_x;
  node->position.c[1] = position_y;
  node->position.c[2] = position_z;
  node->internal_delay = internal_delay;
  node->Pt = Pt;
  node->interference_accounted = FALSE;


  // update the Pr0 field based on Pt
  wlan_node_update_Pr0(node);
  active_tag_node_update_Pr0(node);
  zigbee_node_update_Pr0(node);
}

// print the fields of a node
void node_print(node_class *node)
{
  printf("  Node '%s': type='%s' id=%d ssid='%s' connection='%s' \
adapter_type=%d antenna_gain=%.2f\n",
	 node->name, node_types[node->type], node->id, node->ssid, 
	 node_connections[node->connection], node->adapter_type,
	 node->antenna_gain);
  printf("\tposition=(%.2f, %.2f, %.2f) Pt=%.2f Pr0_a=%.2f Pr0_bg=%.2f \
Pr0_active_tag=%.2f Pr0_zigbee=%.2f\n", 
	 node->position.c[0], node->position.c[1], node->position.c[2],
	 node->Pt, node->Pr0_a, node->Pr0_bg, node->Pr0_active_tag,
	 node->Pr0_zigbee);
}

// copy the information in node_src to node_dest
void node_copy(node_class *node_dest, node_class * node_src)
{
  strncpy(node_dest->name, node_src->name, MAX_STRING-1);
  node_dest->type = node_src->type;
  node_dest->id = node_src->id;
  strncpy(node_dest->ssid, node_src->ssid, MAX_STRING-1);
  node_dest->connection = node_src->connection;
  node_dest->adapter_type = node_src->adapter_type;
  node_dest->antenna_gain = node_src->antenna_gain;
  node_dest->azimuth_orientation = node_src->azimuth_orientation;
  node_dest->azimuth_beamwidth = node_src->azimuth_beamwidth;
  node_dest->elevation_orientation = node_src->elevation_orientation;
  node_dest->elevation_beamwidth = node_src->elevation_beamwidth;

  coordinate_copy(&(node_dest->position),
		  &(node_src->position));
  node_dest->internal_delay = node_src->internal_delay;

  node_dest->Pt = node_src->Pt;
  node_dest->Pr0_a = node_src->Pr0_a;
  node_dest->Pr0_bg = node_src->Pr0_bg;
  node_dest->Pr0_active_tag = node_src->Pr0_active_tag;
  node_dest->Pr0_zigbee = node_src->Pr0_zigbee;

  node_dest->interference_accounted = node_src->interference_accounted;
}

// auto-connect a node (from_node) with another one from scenario
// at the moment the only strategy is "highest signal strength"
// depending on the 'node' field 'connection: 
//  - 'infrastructure' => connect to AP
//  - 'ad_hoc' => connect to another regular node
//  - 'auto' => connect to a node of any type 
// return SUCCESS on succes, ERROR on error
// NOTE: This function is still in experimental phase!
int node_auto_connect(node_class *from_node, scenario_class *scenario)
{
  int i;
  double max_Pr = -DBL_MAX;// smallest possible value
  int max_Pr_index = INVALID_INDEX;
  int from_node_index = INVALID_INDEX;

  // temporarily use "0" as default environment index
  int environment_index = 0;

  connection_class *connection;

  // try to add a connection to the scenario
  if(scenario->connection_number < MAX_CONNECTIONS)
    {
      // search from_node_index by going through all nodes
      // (should be optimized)
      for(i=0; i<scenario->node_number; i++)
	if(scenario->nodes[i].id == from_node->id)
	  from_node_index = i;

      if(from_node_index == INVALID_INDEX)
	{
	  WARNING("The node '%s' doesn't exist in scenario", 
		      from_node->name);
	  return ERROR;
	}

      // use the last connection element during searching process
      // by creating a temporary connection
      connection = &(scenario->connections[scenario->connection_number]);

      // make sure all parameters are initialized to default values 
      if(connection_init(connection, DEFAULT_PARAMETER, DEFAULT_PARAMETER,
			 DEFAULT_PARAMETER, DEFAULT_PACKET_SIZE, 
			 DEFAULT_STANDARD, DEFAULT_CHANNEL_BG, 
			 DEFAULT_RTS_CTS_THRESHOLD, TRUE)==ERROR)
	return ERROR;

      // set the 'from_node_index' attribute
      connection->from_node_index = from_node_index;

      // temporarily use "0" as default environment index
      connection->through_environment_index = environment_index;

      // go through all nodes to find the to_node which has the 
      // highest Pr
      for(i=0; i<scenario->node_number; i++)
	// check the node is different than the source one
	if(scenario->nodes[i].id != from_node->id)
	  {
	    int try_connect = FALSE;
	    // depending on the type of the to_node different conditions 
	    // must be checked to decide if connection is permitted

	    // check if the potential to_node is an access point
	    if(scenario->nodes[i].type == ACCESS_POINT_NODE)
	      {
		// only from_nodes with connection "infrastructure" or "any" 
		// can connect to an access point
		if((scenario->nodes[from_node_index].connection == 
		    INFRASTRUCTURE_CONNECTION) || 
		   (scenario->nodes[from_node_index].connection == 
		    ANY_CONNECTION))
		  //the "ssid" must be the same with that of the access point
		  if(strcmp(scenario->nodes[from_node_index].ssid,
				scenario->nodes[i].ssid) == 0)
		    try_connect = TRUE;
	      }
	    // assume the potential to_node is a regular node
	    else
	      {
		// only from_nodes with connection "ad_hoc" or "any"
		// can connect to a regular node
		if((scenario->nodes[from_node_index].connection == 
		    AD_HOC_CONNECTION) || 
		   (scenario->nodes[from_node_index].connection == 
		    ANY_CONNECTION))
		  //the "ssid" must be the same with that of the other node
		  if(strcmp(scenario->nodes[from_node_index].ssid,
				scenario->nodes[i].ssid) == 0)
		    try_connect = TRUE;
	      }

	    // do we still try to connect?	      
	    if(try_connect == TRUE)
	      {
		// set the attribute 'to_node' of the connection temporarily 
		// to the current node
		connection->to_node_index = i;
	    
		// update the connection, so that the field Pr is computed
		if(wlan_connection_update(connection, scenario)==ERROR)
		  return ERROR;

		if(connection->Pr > max_Pr)
		  {
		    max_Pr = connection->Pr;
		    max_Pr_index = i;
		  }
	      }
	  }

      // a suitable node was found
      if(max_Pr_index != INVALID_INDEX)
	{
	  // finalize connection creation
	  strncpy(connection->from_node, 
		  scenario->nodes[from_node_index].name, MAX_STRING-1);
	  strncpy(connection->to_node, 
		  scenario->nodes[max_Pr_index].name, MAX_STRING-1);
	  strncpy(connection->through_environment, 
		  scenario->environments[environment_index].name, 
		  MAX_STRING-1);

#ifdef MESSAGE_INFO
	  connection_print(connection);
#endif

	  scenario->connection_number++;
	}
      else
	WARNING("No connection can be established for node '%s'", 
		from_node->name);
    }
  else
    {
      WARNING("Maximum number of connections in scenario reached (%d)",
	      MAX_CONNECTIONS);
      return ERROR;
    }

  return SUCCESS;
}

// auto-connect a node (from_node) with another one from scenario
// (used for active_tags only currently);
// return SUCCESS on succes, ERROR on error
// NOTE: This function is still in experimental phase!
int node_auto_connect_at(node_class *from_node, scenario_class *scenario)
{
  int i;

  int from_node_index = INVALID_INDEX;
  // temporarily use "0" as default environment index
  int environment_index = 0;
  
  int created_connections=0;

  connection_class *candidate_connection;

  // check that node is active tag
  if(from_node->type != ACTIVE_TAG)
    {
      WARNING("Node '%s' is not of type active tag", from_node->name);
      return SUCCESS;
    }

  // search from_node_index by going through all nodes
  // (should be optimized)
  for(i=0; i<scenario->node_number; i++)
    if(scenario->nodes[i].id == from_node->id)
      from_node_index = i;

  if(from_node_index == INVALID_INDEX)
    {
      WARNING("The node '%s' doesn't exist in scenario", 
	      from_node->name);
      return ERROR;
    }

  // go through all nodes to find the to_node 
  // nodes that fullfil the connectivity condition
  for(i=0; i<scenario->node_number; i++)
    {

      // check the node is different than the source one
      if(scenario->nodes[i].id != from_node->id)
	{
	  if(scenario->connection_number < MAX_CONNECTIONS)
	    {
	      DEBUG("Building a candidate connection from '%s' to '%s'",
		    from_node->name, scenario->nodes[i].name);

	      // use the last connection element during searching process
	      // by creating a temporary connection
	      candidate_connection = 
		&(scenario->connections[scenario->connection_number]);

	      // make sure all parameters are initialized to default values 
	      if(connection_init(candidate_connection, DEFAULT_PARAMETER, 
				 DEFAULT_PARAMETER,
				 DEFAULT_PARAMETER, 7, 
				 ACTIVE_TAG, DEFAULT_CHANNEL_BG, 
				 DEFAULT_RTS_CTS_THRESHOLD, FALSE)==ERROR)
		return ERROR;

	      // set the 'from_node_index' attribute
	      candidate_connection->from_node_index = from_node_index;

	      // set the attribute 'to_node' of the connection temporarily 
	      // to the current node
	      candidate_connection->to_node_index = i;

	      // set the 'through_environment_index' attribute
	      candidate_connection->through_environment_index = 
		environment_index;

	      // update the connection, so that the fields is computed
	      if(active_tag_connection_update(candidate_connection, 
					     scenario)==ERROR)
		return ERROR;

	      active_tag_loss_rate(candidate_connection, scenario, 
		      &(candidate_connection->loss_rate));

#ifdef AUTO_CONNECT_BASED_ON_FER
	      // if the frame error rate is not 1, the candidate
	      // connection should be saved
	      if(candidate_connection->frame_error_rate < 1.0)
		{
#endif
		  // finalize connection creation
		  strncpy(candidate_connection->from_node, 
			  scenario->nodes[from_node_index].name, 
			  MAX_STRING-1);
		  strncpy(candidate_connection->to_node, 
			  scenario->nodes[i].name, MAX_STRING-1);
		  strncpy(candidate_connection->through_environment, 
			  scenario->environments[environment_index].name, 
			  MAX_STRING-1);

#ifdef MESSAGE_DEBUG
		  connection_print(candidate_connection);
#endif

		  created_connections++;
		  scenario->connection_number++;

#ifdef AUTO_CONNECT_BASED_ON_FER
		}
#endif
	    }
	  else
	    {
	      WARNING("Maximum number of connections in scenario reached (%d)",
		      MAX_CONNECTIONS);
	      return ERROR;
	    }
	}
    }

  if(created_connections!=0)
    DEBUG("Auto-connect node '%s': created %d connections", 
	  from_node->name, created_connections);
  else
    DEBUG("Auto-connect node '%s': no connection established", 
	  from_node->name);

  return SUCCESS;
}

// check whether a newly defined node conflicts with existing ones;
// return TRUE if node is valid, FALSE otherwise
int node_check_valid(node_class *nodes, int node_number, 
		     node_class *new_node)
{
  int i;
  for(i=0; i<node_number; i++)
    {
      if(strcmp(nodes[i].name, new_node->name)==0)
	{
	  WARNING("Node with name '%s' already defined", new_node->name);
	  return FALSE;
	}

      if(nodes[i].id == new_node->id)
	{
	  WARNING("Node with id '%d' already defined", new_node->id);
	  return FALSE;
	}
    }

  return TRUE;
}
