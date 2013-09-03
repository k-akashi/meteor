
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
 * File name: interface.c
 * Function: Source file related to the interface scenario element
 *
 * Author: Razvan Beuran
 *
 * $Id: interface.c 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/


#include <string.h>
#include <stdio.h>
#include <float.h>		// for DBL_MAX

#include "message.h"
#include "deltaQ.h"

#include "node.h"
#include "connection.h"
#include "scenario.h"

#include "wlan.h"
#include "active_tag.h"
#include "zigbee.h"
#include "wimax.h"


/////////////////////////////////////////
// Interface structure functions
/////////////////////////////////////////

// init an interface
void
interface_init (struct interface_class *interface, char *name,
		int adapter_type, double antenna_gain,
		double Pt, char *ip_address)
{
  strncpy (interface->name, name, MAX_STRING - 1);
  interface->adapter_type = adapter_type;
  interface->antenna_gain = antenna_gain;
  interface->azimuth_orientation = 0;
  interface->azimuth_beamwidth = 360;
  interface->elevation_orientation = 0;
  interface->elevation_beamwidth = 360;
  interface->antenna_count = 1;
  interface->Pt = Pt;
  strncpy (interface->ip_address, ip_address, IP_ADDR_SIZE - 1);

  interface->interference_accounted = FALSE;

  interface->noise_source = FALSE;
  interface->noise_start_time = 0.0;
  interface->noise_end_time = -0.5;

  // update the Pr0 field based on Pt
  wlan_interface_update_Pr0 (interface);
  /*
     active_tag_node_update_Pr0 (node);
     zigbee_node_update_Pr0 (node);
   */
  wimax_interface_update_Pr0 (interface);
}

// copy the information in interface_src to interface_dst
void
interface_copy (struct interface_class *interface_dst,
		struct interface_class *interface_src)
{
  strncpy (interface_dst->name, interface_src->name, MAX_STRING - 1);
  interface_dst->adapter_type = interface_src->adapter_type;
  interface_dst->antenna_gain = interface_src->antenna_gain;
  interface_dst->azimuth_orientation = interface_src->azimuth_orientation;
  interface_dst->azimuth_beamwidth = interface_src->azimuth_beamwidth;
  interface_dst->elevation_orientation = interface_src->elevation_orientation;
  interface_dst->elevation_beamwidth = interface_src->elevation_beamwidth;
  interface_dst->antenna_count = interface_src->antenna_count;

  interface_dst->Pt = interface_src->Pt;
  interface_dst->Pr0 = interface_src->Pr0;
  interface_dst->Pr0_a = interface_src->Pr0_a;

  strncpy (interface_dst->ip_address, interface_src->ip_address,
	   IP_ADDR_SIZE - 1);

  interface_dst->interference_accounted =
    interface_src->interference_accounted;

  interface_dst->noise_source = interface_src->noise_source;
  interface_dst->noise_start_time = interface_src->noise_start_time;
  interface_dst->noise_end_time = interface_src->noise_end_time;

  // no pointers in the capacity structure, so we copy directly from src to dst
  interface_dst->wimax_params = interface_src->wimax_params;
}

// print the fields of an interface
void
interface_print (struct interface_class *interface)
{
  printf ("    Interface '%s': adapter_type=%d ip_address='%s'\n",
	  interface->name, interface->adapter_type, interface->ip_address);
  printf ("\t antenna_gain=%.2f Pt=%.2f Pr0=%.2f Pr0_a=%.2f\n",
	  interface->antenna_gain, interface->Pt, interface->Pr0,
	  interface->Pr0_a);
}

/*
// auto-connect a node (from_node) with another one from scenario
// at the moment the only strategy is "highest signal strength"
// depending on the 'node' field 'connection: 
//  - 'infrastructure' => connect to AP
//  - 'ad_hoc' => connect to another regular node
//  - 'auto' => connect to a node of any type 
// return SUCCESS on succes, ERROR on error
// NOTE: This function is still in experimental phase!
int
node_auto_connect (struct node_class *from_node,
		   struct scenario_class *scenario)
{
  int i;
  double max_Pr = -DBL_MAX;	// smallest possible value
  int max_Pr_index = INVALID_INDEX;
  int from_node_index = INVALID_INDEX;

  // temporarily use "0" as default environment index
  int environment_index = 0;

  struct connection_class *connection;

  // try to add a connection to the scenario
  if (scenario->connection_number < MAX_CONNECTIONS)
    {
      // search from_node_index by going through all nodes
      // (should be optimized)
      for (i = 0; i < scenario->node_number; i++)
	if (scenario->nodes[i].id == from_node->id)
	  from_node_index = i;

      if (from_node_index == INVALID_INDEX)
	{
	  WARNING ("The node '%s' doesn't exist in scenario",
		   from_node->name);
	  return ERROR;
	}

      // use the last connection element during searching process
      // by creating a temporary connection
      connection = &(scenario->connections[scenario->connection_number]);

      // make sure all parameters are initialized to default values 
      if (connection_init (connection, DEFAULT_PARAMETER, DEFAULT_PARAMETER,
			   DEFAULT_PARAMETER, DEFAULT_PACKET_SIZE,
			   DEFAULT_STANDARD, DEFAULT_CHANNEL_BG,
			   DEFAULT_RTS_CTS_THRESHOLD, TRUE) == ERROR)
	return ERROR;

      // set the 'from_node_index' attribute
      connection->from_node_index = from_node_index;

      // temporarily use "0" as default environment index
      connection->through_environment_index = environment_index;

      // go through all nodes to find the to_node which has the 
      // highest Pr
      for (i = 0; i < scenario->node_number; i++)
	// check the node is different than the source one
	if (scenario->nodes[i].id != from_node->id)
	  {
	    int try_connect = FALSE;
	    // depending on the type of the to_node different conditions 
	    // must be checked to decide if connection is permitted

	    // check if the potential to_node is an access point
	    if (scenario->nodes[i].type == ACCESS_POINT_NODE)
	      {
		// only from_nodes with connection "infrastructure" or "any" 
		// can connect to an access point
		if ((scenario->nodes[from_node_index].connection ==
		     INFRASTRUCTURE_CONNECTION) ||
		    (scenario->nodes[from_node_index].connection ==
		     ANY_CONNECTION))
		  //the "ssid" must be the same with that of the access point
		  if (strcmp (scenario->nodes[from_node_index].ssid,
			      scenario->nodes[i].ssid) == 0)
		    try_connect = TRUE;
	      }
	    // assume the potential to_node is a regular node
	    else
	      {
		// only from_nodes with connection "ad_hoc" or "any"
		// can connect to a regular node
		if ((scenario->nodes[from_node_index].connection ==
		     AD_HOC_CONNECTION) ||
		    (scenario->nodes[from_node_index].connection ==
		     ANY_CONNECTION))
		  //the "ssid" must be the same with that of the other node
		  if (strcmp (scenario->nodes[from_node_index].ssid,
			      scenario->nodes[i].ssid) == 0)
		    try_connect = TRUE;
	      }

	    // do we still try to connect?            
	    if (try_connect == TRUE)
	      {
		// set the attribute 'to_node' of the connection temporarily 
		// to the current node
		connection->to_node_index = i;

		// update the connection, so that the field Pr is computed
		if (wlan_connection_update (connection, scenario) == ERROR)
		  return ERROR;

		if (connection->Pr > max_Pr)
		  {
		    max_Pr = connection->Pr;
		    max_Pr_index = i;
		  }
	      }
	  }

      // a suitable node was found
      if (max_Pr_index != INVALID_INDEX)
	{
	  // finalize connection creation
	  strncpy (connection->from_node,
		   scenario->nodes[from_node_index].name, MAX_STRING - 1);
	  strncpy (connection->to_node,
		   scenario->nodes[max_Pr_index].name, MAX_STRING - 1);
	  strncpy (connection->through_environment,
		   scenario->environments[environment_index].name,
		   MAX_STRING - 1);

#ifdef MESSAGE_INFO
	  connection_print (connection);
#endif

	  scenario->connection_number++;
	}
      else
	WARNING ("No connection can be established for node '%s'",
		 from_node->name);
    }
  else
    {
      WARNING ("Maximum number of connections in scenario reached (%d)",
	       MAX_CONNECTIONS);
      return ERROR;
    }

  return SUCCESS;
}

// auto-connect a node (from_node) with another one from scenario
// (used for active_tags only currently);
// return SUCCESS on succes, ERROR on error
// NOTE: This function is still in experimental phase!
int
node_auto_connect_at (struct node_class *from_node,
		      struct scenario_class *scenario)
{
  int i;

  int from_node_index = INVALID_INDEX;
  // temporarily use "0" as default environment index
  int environment_index = 0;

  int created_connections = 0;

  struct connection_class *candidate_connection;

  // check that node is active tag
  if (from_node->type != ACTIVE_TAG)
    {
      WARNING ("Node '%s' is not of type active tag", from_node->name);
      return SUCCESS;
    }

  // search from_node_index by going through all nodes
  // (should be optimized)
  for (i = 0; i < scenario->node_number; i++)
    if (scenario->nodes[i].id == from_node->id)
      from_node_index = i;

  if (from_node_index == INVALID_INDEX)
    {
      WARNING ("The node '%s' doesn't exist in scenario", from_node->name);
      return ERROR;
    }

  // go through all nodes to find the to_node 
  // nodes that fullfil the connectivity condition
  for (i = 0; i < scenario->node_number; i++)
    {

      // check the node is different than the source one
      if (scenario->nodes[i].id != from_node->id)
	{
	  if (scenario->connection_number < MAX_CONNECTIONS)
	    {
	      DEBUG ("Building a candidate connection from '%s' to '%s'",
		     from_node->name, scenario->nodes[i].name);

	      // use the last connection element during searching process
	      // by creating a temporary connection
	      candidate_connection =
		&(scenario->connections[scenario->connection_number]);

	      // make sure all parameters are initialized to default values 
	      if (connection_init (candidate_connection, DEFAULT_PARAMETER,
				   DEFAULT_PARAMETER,
				   DEFAULT_PARAMETER, 7,
				   ACTIVE_TAG, DEFAULT_CHANNEL_BG,
				   DEFAULT_RTS_CTS_THRESHOLD, FALSE) == ERROR)
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
	      if (active_tag_connection_update (candidate_connection,
						scenario) == ERROR)
		return ERROR;

	      active_tag_loss_rate (candidate_connection, scenario,
				    &(candidate_connection->loss_rate));

#ifdef AUTO_CONNECT_BASED_ON_FER
	      // if the frame error rate is not 1, the candidate
	      // connection should be saved
	      if (candidate_connection->frame_error_rate < 1.0)
		{
#endif
		  // finalize connection creation
		  strncpy (candidate_connection->from_node,
			   scenario->nodes[from_node_index].name,
			   MAX_STRING - 1);
		  strncpy (candidate_connection->to_node,
			   scenario->nodes[i].name, MAX_STRING - 1);
		  strncpy (candidate_connection->through_environment,
			   scenario->environments[environment_index].name,
			   MAX_STRING - 1);

#ifdef MESSAGE_DEBUG
		  connection_print (candidate_connection);
#endif

		  created_connections++;
		  scenario->connection_number++;

#ifdef AUTO_CONNECT_BASED_ON_FER
		}
#endif
	    }
	  else
	    {
	      WARNING
		("Maximum number of connections in scenario reached (%d)",
		 MAX_CONNECTIONS);
	      return ERROR;
	    }
	}
    }

  if (created_connections != 0)
    DEBUG ("Auto-connect node '%s': created %d connections",
	   from_node->name, created_connections);
  else
    DEBUG ("Auto-connect node '%s': no connection established",
	   from_node->name);

  return SUCCESS;
}

// check whether a newly defined node conflicts with existing ones;
// return TRUE if node is valid, FALSE otherwise
int
node_check_valid (struct node_class *nodes, int node_number,
		  struct node_class *new_node)
{
  int i;
  for (i = 0; i < node_number; i++)
    {
      if (strcmp (nodes[i].name, new_node->name) == 0)
	{
	  WARNING ("Node with name '%s' already defined", new_node->name);
	  return FALSE;
	}

      if (nodes[i].id == new_node->id)
	{
	  WARNING ("Node with id '%d' already defined", new_node->id);
	  return FALSE;
	}
    }

  return TRUE;
}
*/
