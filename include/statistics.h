
/*
 * Copyright (c) 2006-2013 The StarBED Project  All rights reserved.
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
 * File name: statistics.h
 * Function: Header file of statistics.c
 *
 * Author: Razvan Beuran
 *
 ***********************************************************************/

#ifndef __STATISTICS_H
#define __STATISTICS_H


#include "deltaQ.h"


/////////////////////////////////////////////
// Basic constants
/////////////////////////////////////////////

#define STATISTICS_PORT           54321
#define STATISTICS_BACKLOG        10
#define STATISTICS_END_EXEC       -1
#define STATISTICS_ADDRESS        "225.1.1.1"

//#define STATISTICS_CONNECTION_STANDARD  WLAN_802_11A
//#define STATISTICS_CONNECTION_STANDARD  WLAN_802_11B
//#define STATISTICS_CONNECTION_STANDARD  WLAN_802_11G


/////////////////////////////////////////////
// Statistics structure
/////////////////////////////////////////////

// statistics class structure
struct stats_class
{
  // total channel utilization of the current node
  float channel_utilization;

  // transmission probability of the current node
  float transmission_probability;

  //channel utilization per node for CWB
  //float channel_utilization_per_source[MAX_NODES_W];
};


/////////////////////////////////////////////
// Statistics functions
/////////////////////////////////////////////

// statistics communication thread that sends traffic statistics
// to other wireconf instances
void *stats_send_thread (void *arg);

// statistics communication thread that listens for 
// traffic statistics sent by other wireconf instances
void *stats_listen_thread (void *arg);

float compute_channel_utilization (struct binary_record_class *binary_record,
                   long long unsigned int delta_pkt_counter,
                   long long unsigned int delta_byte_counter,
                   float time_interval);

float compute_transmission_probability
  (struct binary_record_class *binary_record,
   long long unsigned int delta_pkt_counter,
   long long unsigned int delta_byte_counter, float time_interval,
   float *adjusted_float_delta_pkt_counter1);


int adjust_deltaQ (struct wireconf_class *wireconf,
           struct binary_record_class **binary_records_ucast,
           struct binary_record_class *adjusted_binary_records_ucast,
           int *binary_records_ucast_changed, float *avg_frame_sizes);

float adjust_delay (float delay, float channel_utilization_others);

float adjust_bandwidth (float bandwidth, float channel_utilization_others);

float adjust_loss_rate (float loss_rate, float channel_utilization_others);


// compute collision probability
float compute_collision_probability (struct wireconf_class *wireconf,
                     int rcv_i,
                     struct binary_record_class
                     **binary_records_ucast);

// Lan added on Oct. 01 for computing cwb channel utilization
float compute_cwb_channel_utilization (struct wireconf_class *wireconf,
                       struct binary_record_class
                       *adjusted_records_ucast);

#endif /* !__STATISTICS_H */
