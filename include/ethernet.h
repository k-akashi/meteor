
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
 * File name: ethernet.h
 * Function: Header file of ethernet.c
 *
 * Author: Razvan Beuran
 *
 * $Id: ethernet.h 145 2013-06-07 01:11:09Z razvan $
 *
 ***********************************************************************/


#ifndef __ETHERNET_H
#define __ETHERNET_H


// constants associated with each operating rate
#define ETH_RATE_10MBPS                    0
#define ETH_RATE_100MBPS                   1
#define ETH_RATE_1000MBPS                  2


// array holding the operating rates associated
// to the different Ethernet operating speeds
extern double eth_operating_rates[];


////////////////////////////////////////////////
// Ethernet related functions
////////////////////////////////////////////////

// compute FER corresponding to the current conditions
// for a given operating rate
// return SUCCESS on succes, ERROR on error
int ethernet_fer (struct connection_class *connection,
		  struct scenario_class *scenario, int operating_rate,
		  double *fer);

// compute loss rate based on FER
// return SUCCESS on succes, ERROR on error
int ethernet_loss_rate (struct connection_class *connection,
			struct scenario_class *scenario, double *loss_rate);

// compute operating rate based on FER and a model of rate
// auto-selection mechanism
// return SUCCESS on succes, ERROR on error
int ethernet_operating_rate (struct connection_class *connection,
			     struct scenario_class *scenario,
			     int *operating_rate);

// compute delay & jitter and return values in arguments
// return SUCCESS on succes, ERROR on error
int ethernet_delay_jitter (struct connection_class *connection,
			   struct scenario_class *scenario,
			   double *variable_delay, double *delay,
			   double *jitter);

// compute bandwidth based on operating rate, delay and packet size
// return SUCCESS on succes, ERROR on error
int ethernet_bandwidth (struct connection_class *connection,
			struct scenario_class *scenario, double *bandwidth);

// update the changing properties of a connection (e.g., relative distance, Pr)
// after the state of the system changed;
// return SUCCESS on succes, ERROR on error
int ethernet_connection_update (struct connection_class *connection,
				struct scenario_class *scenario);

#endif
