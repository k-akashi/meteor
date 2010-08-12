
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
 * File name: node.h
 * Function:  Header file of node.c
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#ifndef __NODE_H
#define __NODE_H


#include "global.h"
#include "coordinate.h"


////////////////////////////////////////////////
// Global variables
////////////////////////////////////////////////

extern char *node_types[];
extern char *node_connections[];


////////////////////////////////////////////////
// Node structure definition
////////////////////////////////////////////////

struct node_class_s
{
  // node name
  char name[MAX_STRING];

  // node type (regular, access point)
  int type;

  // node id
  int id;

  // SSID for this node
  char ssid[MAX_STRING];

  // connection type
  int connection;

  // code indicating adapter type
  int adapter_type; 

  // antenna parameters
  double antenna_gain; // gain expressed in dBi
  double azimuth_orientation; // orientation in horizontal plane wrt 0x axis
  double azimuth_beamwidth; // beamwidth in horizontal plane
                            // (angle at which power is halfed <=> 3dB drop)
  double elevation_orientation; // orientation in vertical plane wrt 0x axis
  double elevation_beamwidth; // beamwidth in vertical plane
                              // (angle at which power is halfed <=> 3dB drop)
 
  // position of the node
  coordinate_class position;

  // internal operation fixed delay for the current node
  double internal_delay;

  // transmitted power in dBm
  double Pt;

  // received power at distance 1 m when _this_ node transmits
  // for a and b/g standars, respectively
  double Pr0_a, Pr0_bg;

  // received power at distance 1 m when _this_ node transmits
  // for Smart Tag
  double Pr0_active_tag;

  // received power at distance 1 m when _this_ node transmits
  // for ZigBee
  double Pr0_zigbee;

  // set to TRUE if interference caused by this node
  // was already accounted; FALSE otherwise
  int interference_accounted;

};


/////////////////////////////////////////
// Node structure functions
/////////////////////////////////////////

// init a node
void node_init(node_class *node, char *name, int type, int id, char *ssid,
	       int connection, int adapter_type, double antenna_gain,
	       double position_x, double position_y, double position_z, 
	       double Pt, double internal_delay);

// print the fields of a node
void node_print(node_class *node);

// copy the information in node_src to node_dest
void node_copy(node_class *node_dest, node_class * node_src);

// auto-connect a node (from_node) with another one from scenario
// at the moment the only strategy is "highest signal strength"
// depending on the 'node' field 'connection: 
//  - 'infrastructure' => connect to AP
//  - 'ad_hoc' => connect to another regular node
//  - 'auto' => connect to a node of any type
// return SUCCESS on succes, ERROR on error
// NOTE: This function is still in experimental phase!
int node_auto_connect(node_class *from_node, scenario_class *scenario);

// auto-connect an active tag node (from_node) with other 
// similar nodes from scenario;
// return SUCCESS on succes, ERROR on error
int node_auto_connect_at(node_class *from_node, scenario_class *scenario);

// check whether a newly defined node conflicts with existing ones;
// return TRUE if node is valid, FALSE otherwise
int node_check_valid(node_class *nodes, int node_number, 
		     node_class *new_node);

#endif
