
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
 * File name: connection.h
 * Function:  Header file of connection.c
 *
 * Author: Razvan Beuran
 *
 * $Id: connection.h 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/


#ifndef __CONNECTION_H
#define __CONNECTION_H


#include "global.h"
#include "fixed_deltaQ.h"

//////////////////////////////////
// Global variables
//////////////////////////////////

extern char *connection_standards[];


////////////////////////////////////////////////
// Connection structure definition
////////////////////////////////////////////////

struct connection_class
{
  // node from which the connection starts
  char from_node[MAX_STRING];

  // from_node index in 'scenario' structure;
  // default value (INVALID_INDEX) is replaced during 
  // an initial search for the defined node
  int from_node_index;

  // interface from which the connection starts
  char from_interface[MAX_STRING];

  // from_interface index in 'node' structure;
  // default value (0) can be replaced during 
  // an initial search for the defined interface
  int from_interface_index;

  // global id of the sender
  int from_id;

  // node to which the connection goes
  char to_node[MAX_STRING];

  // to_node index in 'scenario' structure;
  // default value (INVALID_INDEX) is replaced during 
  // an initial search for the defined node
  int to_node_index;

  // interface to which the connection goes
  char to_interface[MAX_STRING];

  // to_interface index in 'node' structure;
  // default value (0) can be replaced during 
  // an initial search for the defined interface
  int to_interface_index;

  // global id of the receiver
  int to_id;

  // environment through which the connection takes place
  char through_environment[MAX_STRING];

  // through_environment index in 'scenario' structure;
  // default value (INVALID_INDEX) is replaced during 
  // an initial search for the defined node
  int through_environment_index;

  // connection standard (802.11a/b/g)
  int standard;

  // connection channel 
  int channel;

  // value of noise level in dB caused to the receiver of this
  // connection by transmitters of other connections
  double interference_noise;

  // number of concurrent stations that influence the activity
  // of the current receiver by CSMA/CA mechanism
  int concurrent_stations;

  // boolean value set to TRUE for 802.11g receivers 
  // if 802.11b stations are present in its neighbourhood,
  // case which requires operation in compatibility mode;
  // set to FALSE otherwise
  int compatibility_mode;

  // packet size above which RTS/CTS mechanism is enabled;
  // using a value of MAX_PSDU_SIZE effectively disables
  // the RTS/CTS mechanism
  int RTS_CTS_threshold;

  // the average packet size transmitted through this connection
  int packet_size;

  // received power by 'to_node' in dBm
  double Pr;

  // SNR for signal received by 'to_node' in dB
  double SNR;

  //distance between receiver and transmitter
  double distance;

  // current operating rate
  int operating_rate;

  // next operating rate after rate adaptation calculation
  int new_operating_rate;

  // set to TRUE if operating rate is to be adaptive;
  // FALSE otherwise
  int adaptive_operating_rate;

  // average frame error rate
  double frame_error_rate;

  // additional frame error rate induced by 
  // overlapping transmitters (active tag case)
  double interference_fer;

  // set to TRUE if interference between nodes is to be considered
  // (only for the case of active tags for the moment); FALSE otherwise
  int consider_interference;

  // average number of retransmissions (in addition to the initial 
  // transmission)
  double num_retransmissions;

  // the variable (WLAN dependent) component of the delay
  double variable_delay;

  // deltaQ parameters for the connection
  double loss_rate, delay, jitter, bandwidth;

  // boolean flags indicating whether respective deltaQ 
  // parameters were pre-defined
  int loss_rate_defined, delay_defined, jitter_defined, bandwidth_defined;

  struct fixed_deltaQ_class fixed_deltaQs[MAX_FIXED_DELTAQ];
  int fixed_deltaQ_number;
  int fixed_deltaQ_crt;

  //struct capacity_class wimax_capacity;
};


/////////////////////////////////////////
// Connection structure functions
/////////////////////////////////////////

// init a connection
// return SUCCESS on succes, ERROR on error
int connection_init (struct connection_class *connection, char *from_node,
		     char *to_node, char *through_environment,
		     int packet_size, int standard, int channel,
		     int RTS_CTS_threshold, int consider_interference);

// initialize all parameters related to a specific WLAN standard;
// return SUCCESS on succes, ERROR on error
int connection_init_standard (struct connection_class *connection,
			      int standard);

// initialize all parameters related to the connection operating rate;
// return SUCCESS on succes, ERROR on error
int connection_init_operating_rate (struct connection_class *connection,
				    int operating_rate);

// initialize the local indexes for the from_node, to_node, and
// through_environment of a connection;
// return SUCCESS on succes, ERROR on error
int connection_init_indexes (struct connection_class *connection,
			     struct scenario_class *scenario);

// print the fields of a connection
void connection_print (struct connection_class *connection);

// copy the information in connection_src to connection_dest
void connection_copy (struct connection_class *connection_dest,
		      struct connection_class *connection_src);

// return the current operating rate of a connection
double connection_get_operating_rate (struct connection_class *connection);

// compute deltaQ parameters of a connection;
// deltaQ parameters are returned in the corresponding fields of
// the connection object and set the last argument to TRUE if any 
// parameter values were changed, or FALSE otherwise;
// return SUCCESS on succes, ERROR on error
int connection_do_compute (struct connection_class *connection,
			   struct scenario_class *scenario,
			   int *deltaQ_changed);

// update the state and calculate all deltaQ parameters 
// for the current connection;
// deltaQ parameters are returned in the corresponding fields of
// the connection objects and the last argument is set to TRUE if 
// any parameter values were changed, or to FALSE otherwise;
// return SUCCESS on succes, ERROR on error
int connection_deltaQ (struct connection_class *connection,
		       struct scenario_class *scenario, int *deltaQ_changed);

struct fixed_deltaQ_class *connection_add_fixed_deltaQ
  (struct connection_class *connection,
   struct fixed_deltaQ_class *fixed_deltaQ);

#endif
