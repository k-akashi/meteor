
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
 * File name: zigbee.h
 * Function:  Header file of zigbee.c
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/

#ifndef __ZIGBEE_H
#define __ZIGBEE_H

#include "deltaQ.h"


////////////////////////////////////////////////
// NOTE: Page references in this file are given
//       with respect to IEEE Std 802.15.4-2006
////////////////////////////////////////////////


/////////////////////////////////////////////
// Generic ZigBee constants
/////////////////////////////////////////////

// ZigBee channels in 2.4GHz band have center frequencies from 
// 2.405 to 2.480 GHz (16 channels numbered from 11 to 26) [pp. 29]
// (we use here the average frequency)
#define ZIGBEE_FREQUENCY                    2.4425e9

// length of "standard" PSDU frames (used for FER calculation) [pp. 31]
// PSDU = PLCP (Physical Layer Convergence Protocol) SDU (Service Data Unit)
// Note: PSDU is the same with MPDU (MAC Protocol Data Unit)
#define ZIGBEE_PSDU                       20

// maximum number of transmissions of a frame before it is 
// considered lost (without and with RTS/CTS mechanism, respectively)
// [pp. 164] => max retransmissions = 3
#define ZIGBEE_MAX_TRANSMISSIONS        4

// maximum number of backoffs for CSMA/CA before declaring channel access
// failure [pp. 163] macMaxCSMABackoffs = 4 +1 because it can be equal
#define ZIGBEE_MAX_CSMA_BACKOFFS        5

// minimum and maximum value of a channel identifier for 2.4 GHz band [pp. 29]
#define ZIGBEE_MIN_CHANNEL                     11
#define ZIGBEE_MAX_CHANNEL                     26

// duration of a slot in 802.11a [us]
//#define SLOT_ZIGBEE                    9

// SIFS duration in us for 802.11b
//#define SIFS_802_11B                    10

// long 802.11b/g PHY header length in us (long preamble)
//#define PHY_OVERHEAD_802_11BG_LONG      192

// thermal noise level associated to Pr-threshold 
// determining technique; for 7 MHz frequency band 
// of ZigBee [pp. 49] it is equal to -105 dB
// Formula: P_dBm = -174 + 10*log10(delta_f) [Wikipedia: Johnson-Nyquist noise]
#define ZIGBEE_STANDARD_NOISE                  -105.0
// WHAT IS REL. TO MINIMUM_NOISE_POWER ????


////////////////////////////////////////////////
// ZigBee model parameters
////////////////////////////////////////////////

// constants associated with each operating rate
#define ZIGBEE_RATE                     0

// number of operating rates of 802.11b
#define ZIGBEE_RATES_NUMBER             1

// data structure to hold ZigBee parameters
// Note: later all structures may be unified
typedef struct
{
  // name of the adapter
  char name[MAX_STRING];

  // constants of model 1 (Pr-threshold based model)
  int use_model1;         // set to TRUE if model 1 is used, FALSE otherwise
  float Pr_thresholds[ZIGBEE_RATES_NUMBER]; // receive sensitivity thresholds
  float Pr_threshold_fer; // FER at the Pr-threshold (valid for DSSS 
                          // encoding both in 802.11b and g)
  float model1_alpha;     // constant of exponential dependency

} parameters_zigbee;


/////////////////////////////////////////////
// Arrays of constants for convenient use
/////////////////////////////////////////////

extern double zigbee_operating_rates[];


/////////////////////////////////////////////
// Default values
/////////////////////////////////////////////

// default values for some connection parameters ????????????????????
#define ZIGBEE_DEFAULT_PARAMETER               "UNKNOWN"
#define ZIGBEE_DEFAULT_PACKET_SIZE             20
#define ZIGBEE_DEFAULT_STANDARD                ZIGBEE
#define ZIGBEE_DEFAULT_CHANNEL                 10
#define ZIGBEE_DEFAULT_ADAPTER_TYPE            JENNIC

// default values for some environment parameters
// (also used when no environment information can be found
// in environment_update)
#define ZIGBEE_DEFAULT_ALPHA                   2.0
#define ZIGBEE_DEFAULT_SIGMA                   0.0
#define ZIGBEE_DEFAULT_W                       0.0
#define ZIGBEE_DEFAULT_NOISE_POWER             -105.0

// defines for supported WLAN adapters
#define JENNIC                          0  


////////////////////////////////////////////////
// Special constants
////////////////////////////////////////////////

// various constants
#define ZIGBEE_MINIMUM_NOISE_POWER             -105
#define MAXIMUM_ERROR_RATE              0.999999999

// maximum number of iterations when trying to determine 
// the steady state of a connection during init phase
#define MAXIMUM_PRECOMPUTE              13 //?????????????

// print parameters of an 802.11b model
void print_zigbee_parameters(parameters_zigbee *params);

// update node Pr0, which is the power radiated by _this_ node 
// at the distance of 1 m
void zigbee_node_update_Pr0(node_class *node);

// update the changing properties of a connection (e.g., relative distance, Pr)
// after the state of the system changed;
// return SUCCESS on succes, ERROR on error
int zigbee_connection_update(connection_class *connection, 
			   scenario_class *scenario);

// get a pointer to a parameter structure corresponding to 
// the receiving node adapter and the connection standard;
// return value is this pointer
void *zigbee_get_node_adapter(connection_class *connection, node_class *node);

// do compute channel interference between the current connection
// and a potentially interfering connection 'connection_i';
// return SUCCESS on succes, ERROR on error
int zigbee_compute_channel_interference(connection_class *connection, 
				 connection_class *connection_i, 
				 scenario_class *scenario);

// compute the interference caused by other stations
// through either concurrent transmission or through noise;
// return SUCCESS on succes, ERROR on error
int zigbee_interference(connection_class *connection, 
		      scenario_class *scenario);

// compute FER corresponding to the current conditions
// for a given operating rate;
// return SUCCESS on succes, ERROR on error
int zigbee_fer(connection_class *connection, scenario_class *scenario, 
	     int operating_rate, double *fer);

// compute the average number of retransmissions corresponding 
// to the current conditions for ALL frames (including errored ones);
// return SUCCESS on succes, ERROR on error
int zigbee_retransmissions(connection_class *connection, 
			 scenario_class *scenario, 
			 double *num_retransmissions);

// compute loss rate based on FER;
// return SUCCESS on succes, ERROR on error
int zigbee_loss_rate(connection_class *connection, 
		   scenario_class *scenario, double *loss_rate);

// calculate PPDU duration in us for 802.11 WLAN specified by 'connection';
// the results will be rounded up using ceil function;
// return SUCCESS on succes, ERROR on error
int zigbee_ppdu_duration(connection_class *connection, double *ppdu_duration);

// compute delay & jitter and return values in arguments
// Note: To obtain correct results the call to this function 
// must follow the call to "zigbee_loss_rate()" since it 
// uses the field "frame_error_rate" internally;
// return SUCCESS on succes, ERROR on error
int zigbee_delay_jitter(connection_class *connection, 
		      scenario_class *scenario, double *variable_delay,
		      double *delay, double *jitter);

// compute bandwidth based on operating rate, delay and packet size;
// return SUCCESS on succes, ERROR on error
int zigbee_bandwidth(connection_class *connection, 
		   scenario_class *scenario, double *bandwidth);


#endif
