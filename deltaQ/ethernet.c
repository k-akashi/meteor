
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
 * File name: ethernet.c
 * Function: Wired Ethernet emulation for the deltaQ computation library
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#include "message.h"
#include "deltaQ.h"
#include "ethernet.h"

// global variable initialization
double eth_operating_rates[]={10e6, 100e6, 1000e6};


// compute FER corresponding to the current conditions
// for a given operating rate
// return SUCCESS on succes, ERROR on error
int ethernet_fer(connection_class *connection, scenario_class *scenario, 
		 int operating_rate, double *fer)
{
  // for Ethernet we assume the FER caused by BER is 0
  (*fer) = 0;
  return SUCCESS;
}

// compute loss rate based on FER
// return SUCCESS on succes, ERROR on error
int ethernet_loss_rate(connection_class *connection, 
		       scenario_class *scenario, double *loss_rate)
{
  if(ethernet_fer(connection, scenario, connection->operating_rate, 
		  &(connection->frame_error_rate))==ERROR)
    {
      WARNING("Error while computing frame error rate");
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
int ethernet_operating_rate(connection_class *connection, 
			    scenario_class *scenario, int *operating_rate)
{
  // for Ethernet, operating rate may change if devices with different 
  // capabilities are connected (e.g., 10/100 Mbps with 10 Mbps only);
  // for the moment we don't take this into account
  (*operating_rate) = connection->operating_rate;
  return SUCCESS;
}

// compute delay & jitter and return values in arguments
// return SUCCESS on succes, ERROR on error
int ethernet_delay_jitter(connection_class *connection, 
			  scenario_class *scenario, double *variable_delay,
			  double *delay, double *jitter)
{
  node_class *node_rx=&(scenario->nodes[connection->to_node_index]);
  node_class *node_tx=&(scenario->nodes[connection->from_node_index]);

  // for the moment we consider there is no variation of delay
  (*variable_delay) = 0;
  (*jitter) = 0;

  // delay is equal to the fixed delay configured for the nodes
  (*delay) =  node_rx->internal_delay + node_tx->internal_delay;

  return SUCCESS;
}

// compute bandwidth based on operating rate, delay and packet size
// return SUCCESS on succes, ERROR on error
int ethernet_bandwidth(connection_class *connection, 
		       scenario_class *scenario, double *bandwidth)
{
  // for the moment we assume bandwidth is the same with operating rate
  (*bandwidth) = eth_operating_rates[connection->operating_rate];
  return SUCCESS;
}
