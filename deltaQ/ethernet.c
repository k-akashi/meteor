
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
 * File name: ethernet.c
 * Function: Wired Ethernet emulation for the deltaQ computation library
 *
 * Author: Razvan Beuran
 *
 * $Id: ethernet.c 145 2013-06-07 01:11:09Z razvan $
 *
 ***********************************************************************/


#include "message.h"
#include "deltaQ.h"
#include "ethernet.h"

// global variable initialization
double eth_operating_rates[] = { 10e6, 100e6, 1000e6 };


// compute FER corresponding to the current conditions
// for a given operating rate
// return SUCCESS on succes, ERROR on error
int
ethernet_fer (struct connection_class *connection,
	      struct scenario_class *scenario, int operating_rate,
	      double *fer)
{
  // for Ethernet we assume the FER caused by BER is 0
  (*fer) = 0;
  return SUCCESS;
}

// compute loss rate based on FER
// return SUCCESS on succes, ERROR on error
int
ethernet_loss_rate (struct connection_class *connection,
		    struct scenario_class *scenario, double *loss_rate)
{
  if (ethernet_fer (connection, scenario, connection->operating_rate,
		    &(connection->frame_error_rate)) == ERROR)
    {
      WARNING ("Error while computing frame error rate");
      return ERROR;
    }

  // for Ethernet loss rate depends on FER caused by bit errors, 
  // and depends as well on packet size, congestion...
  // for now we only cosnider the dependency on FER
  (*loss_rate) = connection->frame_error_rate;
  return SUCCESS;
}

// compute operating rate based on FER and a model of a rate
// auto-selection mechanism
// return SUCCESS on succes, ERROR on error
int
ethernet_operating_rate (struct connection_class *connection,
			 struct scenario_class *scenario, int *operating_rate)
{
  // for Ethernet, operating rate may change if devices with different 
  // capabilities are connected (e.g., 10/100 Mbps with 10 Mbps only);
  // for the moment we don't take this into account
  (*operating_rate) = connection->operating_rate;
  return SUCCESS;
}

// compute delay & jitter and return values in arguments
// return SUCCESS on succes, ERROR on error
int
ethernet_delay_jitter (struct connection_class *connection,
		       struct scenario_class *scenario,
		       double *variable_delay, double *delay, double *jitter)
{
  struct node_class *node_rx = &(scenario->nodes[connection->to_node_index]);
  struct node_class *node_tx =
    &(scenario->nodes[connection->from_node_index]);

  // for the moment we consider there is no variation of delay
  (*variable_delay) = 0;
  (*jitter) = 0;

  // delay is equal to the fixed delay configured for the nodes
  (*delay) = node_rx->internal_delay + node_tx->internal_delay
    + (*variable_delay);
  return SUCCESS;
}

// compute bandwidth based on operating rate, delay and packet size
// return SUCCESS on succes, ERROR on error
int
ethernet_bandwidth (struct connection_class *connection,
		    struct scenario_class *scenario, double *bandwidth)
{
  // for the moment we assume bandwidth is the same with operating rate
  (*bandwidth) = eth_operating_rates[connection->operating_rate];
  return SUCCESS;
}

// update the changing properties of a connection (e.g., relative distance, Pr)
// after the state of the system changed;
// return SUCCESS on succes, ERROR on error
int
ethernet_connection_update (struct connection_class *connection,
			    struct scenario_class *scenario)
{
  // helper "shortcuts"
  struct node_class *node_rx = &(scenario->nodes[connection->to_node_index]);
  struct node_class *node_tx =
    &(scenario->nodes[connection->from_node_index]);

  // update distance in function of the new node positions
  connection->distance =
    coordinate_distance (&(node_rx->position), &(node_tx->position));

  // limit distance to prevent numerical errors
  if (connection->distance < MINIMUM_DISTANCE)
    {
      connection->distance = MINIMUM_DISTANCE;
      DEBUG ("Limiting distance between nodes to %.2f m", MINIMUM_DISTANCE);
    }

  return SUCCESS;
}
