
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
 * File name: active_tag.h
 * Function: Header file of active_tag.c
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#ifndef __ACTIVE_TAG_H
#define __ACTIVE_TAG_H


// defines for supported active tags
#define S_NODE                         0  


// the Smart Tag's S-NODE operates at the frequency 303.2 MHz 
// (value is defined in 'active_tag.c')
// [AYID32305 specifications, page 6]
extern double active_tag_frequencies[];


// data structure to hold active tag parameters
typedef struct
{
  // name of the active tag
  char name[MAX_STRING];

  int use_distance_model;     // set to TRUE if distance model is used, 
                              // to FALSE otherwise
  // constants of model
  double operating_rates[1];  // active tag operating data rate
  float Pr_thresholds[1]; // receive sensitivity thresholds
  float Pr_threshold_fer; // FER at the Pr-threshold
  float model1_alpha;     // constant of exponential dependency

} parameters_active_tag;


////////////////////////////////////////////////
// Ethernet related functions
////////////////////////////////////////////////

// compute FER corresponding to the current conditions
// for a given operating rate
// return SUCCESS on succes, ERROR on error
int active_tag_fer(connection_class *connection, scenario_class *scenario, 
		 int operating_rate, double *fer);

// compute loss rate based on FER
// return SUCCESS on succes, ERROR on error
int active_tag_loss_rate(connection_class *connection, 
		       scenario_class *scenario, double *loss_rate);

// compute operating rate based on FER and a model of rate
// auto-selection mechanism
// return SUCCESS on succes, ERROR on error
int active_tag_operating_rate(connection_class *connection, 
			    scenario_class *scenario, int *operating_rate);

// compute delay & jitter and return values in arguments
// return SUCCESS on succes, ERROR on error
int active_tag_delay_jitter(connection_class *connection, 
			  scenario_class *scenario, double *variable_delay,
			  double *delay, double *jitter);

// compute bandwidth based on operating rate, delay and packet size
// return SUCCESS on succes, ERROR on error
int active_tag_bandwidth(connection_class *connection, 
		       scenario_class *scenario, double *bandwidth);

// update the changing properties of a connection (e.g., relative distance, Pr)
// after the state of the system changed;
// return SUCCESS on succes, ERROR on error
int active_tag_connection_update(connection_class *connection, 
				scenario_class *scenario);

// update node Pr0, which is the power radiated by _this_ node 
// at the distance of 1 m
void active_tag_node_update_Pr0(node_class *node);

// do compute channel interference between the current connection
// and a potentially interfering connection 'connection_i';
// return SUCCESS on succes, ERROR on error
int compute_interference(connection_class *connection, 
			 connection_class *connection_i, 
			 scenario_class *scenario);

// compute the interference caused by other stations
// through overlapping transmissions;
// return SUCCESS on succes, ERROR on error
int active_tag_interference(connection_class *connection, 
			   scenario_class *scenario);
#endif
