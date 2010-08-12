
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
 * File name: connection.h
 * Function:  Header file of connection.c
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#ifndef __CONNECTION_H
#define __CONNECTION_H


#include "global.h"


//////////////////////////////////
// Global variables
//////////////////////////////////

extern char *connection_standards[];


////////////////////////////////////////////////
// Connection structure definition
////////////////////////////////////////////////

struct connection_class_s
{
  // node from which the connection starts
  char from_node[MAX_STRING];

  // from_node index in 'scenario' structure;
  // default value (INVALID_INDEX) is replaced during 
  // an initial search for the defined node
  int from_node_index;

  // node to which the connection goes
  char to_node[MAX_STRING];

  // to_node index in 'scenario' structure;
  // default value (INVALID_INDEX) is replaced during 
  // an initial search for the defined node
  int to_node_index;

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

  // boolean value set to TRUE for 802.11g receviers 
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

};


/////////////////////////////////////////
// Connection structure functions
/////////////////////////////////////////

// init a connection
// return SUCCESS on succes, ERROR on error
int connection_init(connection_class *connection, char *from_node, 
		    char *to_node, char *through_environment, 
		    int packet_size, int standard, int channel, 
		    int RTS_CTS_threshold, int consider_interference);

// initialize all parameters related to a specific WLAN standard;
// return SUCCESS on succes, ERROR on error
int connection_init_standard(connection_class *connection, int standard);

// initialize the local indexes for the from_node, to_node, and
// through_environment of a connection;
// return SUCCESS on succes, ERROR on error
int connection_init_indexes(connection_class *connection, 
			    scenario_class *scenario);

// print the fields of a connection
void connection_print(connection_class *connection);

// copy the information in connection_src to connection_dest
void connection_copy(connection_class *connection_dest, 
		     connection_class * connection_src);

// return the current operating rate of a connection
double connection_operating_rate(connection_class *connection);

// compute deltaQ parameters of a connection;
// deltaQ parameters are returned in the corresponding fields of
// the connection object and set the last argument to TRUE if any 
// parameter values were changed, or FALSE otherwise;
// return SUCCESS on succes, ERROR on error
int connection_do_compute(connection_class *connection, 
			  scenario_class *scenario, int *deltaQ_changed);

// update the state and calculate all deltaQ parameters 
// for the current connection;
// deltaQ parameters are returned in the corresponding fields of
// the connection objects and the last argument is set to TRUE if 
// any parameter values were changed, or to FALSE otherwise;
// return SUCCESS on succes, ERROR on error
int connection_deltaQ(connection_class *connection, scenario_class *scenario,
		      int *deltaQ_changed);

#endif

