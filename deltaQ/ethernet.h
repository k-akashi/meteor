
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
 * File name: ethernet.h
 * Function: Header file of ethernet.c
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
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
int ethernet_fer(connection_class *connection, scenario_class *scenario, 
		 int operating_rate, double *fer);

// compute loss rate based on FER
// return SUCCESS on succes, ERROR on error
int ethernet_loss_rate(connection_class *connection, 
		       scenario_class *scenario, double *loss_rate);

// compute operating rate based on FER and a model of rate
// auto-selection mechanism
// return SUCCESS on succes, ERROR on error
int ethernet_operating_rate(connection_class *connection, 
			    scenario_class *scenario, int *operating_rate);

// compute delay & jitter and return values in arguments
// return SUCCESS on succes, ERROR on error
int ethernet_delay_jitter(connection_class *connection, 
			  scenario_class *scenario, double *variable_delay,
			  double *delay, double *jitter);

// compute bandwidth based on operating rate, delay and packet size
// return SUCCESS on succes, ERROR on error
int ethernet_bandwidth(connection_class *connection, 
		       scenario_class *scenario, double *bandwidth);


#endif
