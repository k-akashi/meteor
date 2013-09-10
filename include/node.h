
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
 * File name: node.h
 * Function:  Header file of node.c
 *
 * Author: Razvan Beuran
 *
 * $Id: node.h 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/


#ifndef __NODE_H
#define __NODE_H


#include "global.h"
#include "coordinate.h"
#include "interface.h"


////////////////////////////////////////////////
// Global variables
////////////////////////////////////////////////

extern char *node_types[];
extern char *node_connections[];


////////////////////////////////////////////////
// Node structure definition
////////////////////////////////////////////////

struct node_class
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

  /*
     // code indicating adapter type
     int adapter_type;

     // antenna parameters
     double antenna_gain;               // gain expressed in dBi
     double azimuth_orientation;        // orientation in horizontal plane wrt 0x axis
     double azimuth_beamwidth;  // beamwidth in horizontal plane
     // (angle at which power is halfed <=> 3dB drop)
     double elevation_orientation;      // orientation in vertical plane wrt 0x axis
     double elevation_beamwidth;        // beamwidth in vertical plane
     // (angle at which power is halfed <=> 3dB drop)
   */

  // position of the node
  struct coordinate_class position;

  // internal operation fixed delay for the current node
  double internal_delay;

  /*
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
   */

  struct interface_class interfaces[MAX_INTERFACES];
  int if_num;

  // motion index in 'scenario' structure;
  // default value is INVALID_INDEX for nodes which have no motion
  // associated to them, and is replaced if necessary during 
  // an initial search for the defined motion
  int motion_index;
};


/////////////////////////////////////////
// Node structure functions
/////////////////////////////////////////

// init a node
void node_init (struct node_class *node, char *name, int type, int id,
		char *ssid, int connection, double position_x,
		double position_y, double position_z, double internal_delay);

// print the fields of a node
void node_print (struct node_class *node);

// copy the information in node_src to node_dst
void node_copy (struct node_class *node_dst, struct node_class *node_src);

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

// add an interface to the node;
// return SUCCESS on succes, ERROR on error
int node_add_interface (struct node_class *node,
			struct interface_class *interface);
// init interface indexes of the node
// return SUCCESS on succes, ERROR on error
int node_init_interface_index (struct node_class *node,
			       struct scenario_class *scenario);

#endif
