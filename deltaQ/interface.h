
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
 * File name: interface.h
 * Function:  Header file of interface.c
 *
 * Author: Razvan Beuran
 *
 * $Id: interface.h 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/


#ifndef __INTERFACE_H
#define __INTERFACE_H


#include "global.h"
#include "wimax.h"

////////////////////////////////////////////////
// Interface structure constants
////////////////////////////////////////////////

#define MAX_INTERFACES                  10
#define MAX_ANTENNA_COUNT               4

////////////////////////////////////////////////
// Interface structure definition
////////////////////////////////////////////////

struct interface_class
{
  // interface name
  char name[MAX_STRING];

  // interface id (global value for all interfaces of all nodes)
  int id;

  // code indicating adapter type
  int adapter_type;

  // antenna parameters
  double antenna_gain;		// gain expressed in dBi
  double azimuth_orientation;	// orientation in horizontal plane wrt 0x axis
  double azimuth_beamwidth;	// beamwidth in horizontal plane
  // (angle at which power is halfed <=> 3dB drop)
  double elevation_orientation;	// orientation in vertical plane wrt 0x axis
  double elevation_beamwidth;	// beamwidth in vertical plane
  // (angle at which power is halfed <=> 3dB drop)

  // number of antennas for MIMO systems
  // (we assume they all use the same parameters)
  int antenna_count;

  // transmitted power in dBm
  double Pt;

  //char management_ip[IP_ADDR_SIZE]; // should be at the node level?!
  char ip_address[IP_ADDR_SIZE];

  // received power at distance 1 m when _this_ interface transmits
  // using the 802.11b/g, active tag, ZigBee or WiMAX mode
  double Pr0;

  // received power at distance 1 m when _this_ interface transmits
  // using the 802.11a mode
  double Pr0_a;

  // set to TRUE if interference caused by this interface
  // was already accounted; FALSE otherwise
  int interference_accounted;

  struct capacity_class wimax_params;

  // flag indicating whether this interface should be considered as a
  // noise source
  int noise_source;

  // start and end time of noise source
  double noise_start_time, noise_end_time;
};


/////////////////////////////////////////
// Interface structure functions
/////////////////////////////////////////

// init an interface
void interface_init (struct interface_class *interface, char *name,
		     int adapter_type, double antenna_gain,
		     double Pt, char *ip_address);

// copy the information in interface_src to interface_dst
void interface_copy (struct interface_class *interface_dst,
		     struct interface_class *interface_src);

// print the fields of an interface
void interface_print (struct interface_class *interface);

/*
// auto-connect a node (from_node) with another one from scenario
// at the moment the only strategy is "highest signal strength"
// depending on the 'node' field 'connection: 
//  - 'infrastructure' => connect to AP
//  - 'ad_hoc' => connect to another regular node
//  - 'auto' => connect to a node of any type
// return SUCCESS on succes, ERROR on error
// NOTE: This function is still in experimental phase!
int node_auto_connect (struct node_class *from_node,
		       struct scenario_class *scenario);

// auto-connect an active tag node (from_node) with other 
// similar nodes from scenario;
// return SUCCESS on succes, ERROR on error
int node_auto_connect_at (struct node_class *from_node,
			  struct scenario_class *scenario);

// check whether a newly defined node conflicts with existing ones;
// return TRUE if node is valid, FALSE otherwise
int node_check_valid (struct node_class *nodes, int node_number,
		      struct node_class *new_node);
*/

#endif
