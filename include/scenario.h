
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
 * File name: scenario.h
 * Function:  Header file of scenario.c
 *
 * Author: Razvan Beuran
 *
 * $Id: scenario.h 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/


#ifndef __SCENARIO_H
#define __SCENARIO_H

#include "global.h"


////////////////////////////////////////////////
// Scenario data-structure constants
////////////////////////////////////////////////

// NOT USED ANY LONGER
// minimum length of a segment in meters;
// if smaller, segment is ignored
//#define MIN_SEGMENT_LENGTH              0.1 

// minimum distance enforced between nodes when computing
// communciation conditions
#define MINIMUM_DISTANCE                0.1

// value of non-intialized index data
#define INVALID_INDEX                   -1

// node types
#define REGULAR_NODE                    0
#define ACCESS_POINT_NODE               1

// node connection types
#define AD_HOC_CONNECTION               0
#define INFRASTRUCTURE_CONNECTION       1
#define ANY_CONNECTION                  2

// object types
#define ROAD_OBJECT                     0
#define BUILDING_OBJECT                 1

// motion types
#define LINEAR_MOTION                   0
#define CIRCULAR_MOTION                 1
#define ROTATION_MOTION                 2
#define RANDOM_WALK_MOTION              3
#define BEHAVIORAL_MOTION               4
#define QUALNET_MOTION                  5


////////////////////////////////////////////////
// Scenario structure definition
////////////////////////////////////////////////

struct scenario_class
{
  // nodes in scenario, and their number
  struct node_class nodes[MAX_NODES];
  int node_number;

  // topology objects in scenario, and their number
  struct object_class objects[MAX_OBJECTS];
  int object_number;

  // environments in scenario, and their number
  struct environment_class environments[MAX_ENVIRONMENTS];
  int environment_number;

  // motions in scenario, and their number
  struct motion_class motions[MAX_MOTIONS];
  int motion_number;

  // connections in scenario, and their number
  struct connection_class connections[MAX_CONNECTIONS];
  int connection_number;

  // global number of interfaces for all nodes
  int if_num;

  // current execution time of the scenario
  double current_time;
  int own_id;
};


/////////////////////////////////////////
// Scenario structure functions
/////////////////////////////////////////

// init a scenario
void scenario_init (struct scenario_class *scenario);

// print the fields of a scenario
void scenario_print (struct scenario_class *scenario);

// add a node to the scenario structure;
// return a pointer to the element on success, NULL on failure
void *scenario_add_node (struct scenario_class *scenario,
			 struct node_class *node);

// add an object to the scenario structure;
// return a pointer to the element on success, NULL on failure
void *scenario_add_object (struct scenario_class *scenario,
			   struct object_class *object);

// add an environment to the scenario structure;
// return a pointer to the element on success, NULL on failure
void *scenario_add_environment (struct scenario_class *scenario,
				struct environment_class *environment);

// add a motion to the scenario structure;
// return a pointer to the element on success, NULL on failure
void *scenario_add_motion (struct scenario_class *scenario,
			   struct motion_class *motion);

// add a connection to the scenario structure;
// return a pointer to the element on success, NULL on failure
// (if more connections are added, a pointer to the last of them is returned)
void *scenario_add_connection (struct scenario_class *scenario,
			       struct connection_class *connection);

/////////////////////////////////////////////////////
// deltaQ computation top-level functions

// auto connect scenario nodes by using the auto_connect_node function;
// return SUCCESS on succes, ERROR on error
// NOTE: This function is still in experimental phase!
int scenario_auto_connect_nodes (struct scenario_class *scenario);

// auto connect active tag scenario nodes by using the 
// auto_connect_node function;
// return SUCCESS on succes, ERROR on error
int scenario_auto_connect_nodes_at (struct scenario_class *scenario);

// perform the necessary initialization before the scenario_deltaQ
// function can be called (mainly connection-related initialization);
// if 'load_from_jpgis_file' flag is set, some object coordinates 
// will be loaded from the JPGIS file 'jpgis_filename';
// return SUCCESS on succes, ERROR on error
int scenario_init_state (struct scenario_class *scenario,
			 int jpgis_filename_provided, char *jpgis_filename,
			 int cartesian_coord_syst, int deltaQ_disabled);

// compute the deltaQ for all connections of the given scenario;
// deltaQ parameters are returned in the corresponding fields of
// the connection objects;
// return SUCCESS on succes, ERROR on error
int scenario_deltaQ (struct scenario_class *scenario, double current_time);

// reset the interference_accounted flag for all nodes
void scenario_reset_node_interference_flag (struct scenario_class *scenario);


// try to merge an object specified by index 'object_i' to other
// objects in scenario; 
// return TRUE if a merge operation was performed, FALSE otherwise
int scenario_try_merge_object (struct scenario_class *scenario, int object_i);


// merge the object specified by index 'object_i2' to the 
// one specified by index 'object_i1'; if direct_merge is TRUE,
// then direct vertex merging is done, by appending those of 
// object 2 to those of object 1; if direct_merge is FALSE,
// then the order of vertices of object 2 is reversed while merging;
// return SUCCESS on success, ERROR on error
int scenario_merge_objects (struct scenario_class *scenario, int object_i1,
			    int object_i2, int direct_merge);

// remove the object specified by index 'object_i' from the scenario; 
// return SUCCESS on success, ERROR on error
int scenario_remove_object (struct scenario_class *scenario, int object_i);


#endif
