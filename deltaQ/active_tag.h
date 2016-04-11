
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
 * File name: active_tag.h
 * Function: Header file of active_tag.c
 *
 * Author: Razvan Beuran
 *
 * $Id: active_tag.h 146 2013-06-20 00:50:48Z razvan $
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
struct parameters_active_tag
{
  // name of the active tag
  char name[MAX_STRING];

  int use_distance_model;	// set to TRUE if distance model is used, 
  // to FALSE otherwise
  // constants of model
  double operating_rates[1];	// active tag operating data rate
  float Pr_thresholds[1];	// receive sensitivity thresholds
  float Pr_threshold_fer;	// FER at the Pr-threshold
  float model1_alpha;		// constant of exponential dependency

};


///////////////////////////////////////
// Various functions
///////////////////////////////////////

// print parameters of an active tag model
void active_tag_print_parameters (struct parameters_active_tag *params);

// get a pointer to a parameter structure corresponding to 
// the receiving interface;
// return value is this pointer, or NULL if error
struct parameters_active_tag *active_tag_get_interface_adapter
  (struct interface_class *interface);


////////////////////////////////////////////////
// DeltaQ parameter computation functions
////////////////////////////////////////////////

// compute FER corresponding to the current conditions
// for a given operating rate
// return SUCCESS on succes, ERROR on error
int active_tag_fer (struct connection_class *connection,
		    struct scenario_class *scenario, int operating_rate,
		    double *fer);

// compute loss rate based on FER
// return SUCCESS on succes, ERROR on error
int active_tag_loss_rate (struct connection_class *connection,
			  struct scenario_class *scenario, double *loss_rate);

// compute operating rate based on FER and a model of rate
// auto-selection mechanism
// return SUCCESS on succes, ERROR on error
int active_tag_operating_rate (struct connection_class *connection,
			       struct scenario_class *scenario,
			       int *operating_rate);

// compute delay & jitter and return values in arguments
// return SUCCESS on succes, ERROR on error
int active_tag_delay_jitter (struct connection_class *connection,
			     struct scenario_class *scenario,
			     double *variable_delay, double *delay,
			     double *jitter);

// compute bandwidth based on operating rate, delay and packet size
// return SUCCESS on succes, ERROR on error
int active_tag_bandwidth (struct connection_class *connection,
			  struct scenario_class *scenario, double *bandwidth);

// update the changing properties of a connection (e.g., relative distance, Pr)
// after the state of the system changed;
// return SUCCESS on succes, ERROR on error
int active_tag_connection_update (struct connection_class *connection,
				  struct scenario_class *scenario);

// update Pr0, which is the power radiated by _this_ interface
// at the distance of 1 m
void active_tag_interface_update_Pr0 (struct interface_class *interface);

// do compute channel interference between the current connection
// and a potentially interfering connection 'connection_i';
// return SUCCESS on succes, ERROR on error
int compute_interference (struct connection_class *connection,
			  struct connection_class *connection_i,
			  struct scenario_class *scenario);

// compute the interference caused by other stations
// through overlapping transmissions;
// return SUCCESS on succes, ERROR on error
int active_tag_interference (struct connection_class *connection,
			     struct scenario_class *scenario);
#endif
