
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
 * File name: scenario.h
 * Function:  Header file of scenario.c
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#ifndef __SCENARIO_H
#define __SCENARIO_H

#include "global.h"


////////////////////////////////////////////////
// Scenario data-structure constants
////////////////////////////////////////////////

// maximum data sizes constants
#define MAX_NODES                       500
#define MAX_OBJECTS                     500
#define MAX_ENVIRONMENTS                250000
#define MAX_MOTIONS                     500
#define MAX_CONNECTIONS                 250000

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
#define BUILDING_OBJECT                 0
#define ROAD_OBJECT                     1

// motion types
#define LINEAR_MOTION                   0
#define CIRCULAR_MOTION                 1
#define ROTATION_MOTION                 2
#define RANDOM_WALK_MOTION              3
#define BEHAVIORAL_MOTION               4


////////////////////////////////////////////////
// Scenario structure definition
////////////////////////////////////////////////

struct scenario_class_s
{
  // nodes in scenario, and their number
  node_class nodes[MAX_NODES];
  int node_number;

  // topology objects in scenario, and their number
  object_class objects[MAX_OBJECTS];
  int object_number;

  // environments in scenario, and their number
  environment_class environments[MAX_ENVIRONMENTS];
  int environment_number;

  // motions in scenario, and their number
  motion_class motions[MAX_MOTIONS];
  int motion_number;

  // connections in scenario, and their number
  connection_class connections[MAX_CONNECTIONS];
  int connection_number;

};


/////////////////////////////////////////
// Scenario structure functions
/////////////////////////////////////////

// init a scenario
void scenario_init(scenario_class *scenario);

// print the fields of a scenario
void scenario_print(scenario_class *scenario);

// add a node to the scenario structure;
// return a pointer to the element on success, NULL on failure
void *scenario_add_node(scenario_class *scenario, node_class *node);

// add an object to the scenario structure;
// return a pointer to the element on success, NULL on failure
void *scenario_add_object(scenario_class *scenario, object_class *object);

// add an environment to the scenario structure;
// return a pointer to the element on success, NULL on failure
void *scenario_add_environment(scenario_class *scenario, 
			       environment_class *environment);

// add a motion to the scenario structure;
// return a pointer to the element on success, NULL on failure
void *scenario_add_motion(scenario_class *scenario, motion_class *motion);

// add a connection to the scenario structure;
// return a pointer to the element on success, NULL on failure
// (if more connections are added, a pointer to the last of them is returned)
void *scenario_add_connection(scenario_class *scenario, 
			      connection_class *connection);

/////////////////////////////////////////////////////
// deltaQ computation top-level functions

// auto connect scenario nodes by using the auto_connect_node function;
// return SUCCESS on succes, ERROR on error
// NOTE: This function is still in experimental phase!
int scenario_auto_connect_nodes(scenario_class *scenario);

// auto connect active tag scenario nodes by using the 
// auto_connect_node function;
// return SUCCESS on succes, ERROR on error
int scenario_auto_connect_nodes_at(scenario_class *scenario);

// perform the necessary initialization before the scenario_deltaQ
// function can be called (mainly connection-related initialization);
// if 'load_from_jpgis_file' flag is set, some object coordinates 
// will be loaded from the JPGIS file 'jpgis_filename';
// return SUCCESS on succes, ERROR on error
int scenario_init_state(scenario_class *scenario, int jpgis_filename_provided,
			char *jpgis_filename, int deltaQ_disabled);

// compute the deltaQ for all connections of the given scenario;
// deltaQ parameters are returned in the corresponding fields of
// the connection objects;
// return SUCCESS on succes, ERROR on error
int scenario_deltaQ(scenario_class *scenario);

// reset the interference_accounted flag for all nodes
void scenario_reset_node_interference_flag(scenario_class *scenario);


// try to merge an object specified by index 'object_i' to other
// objects in scenario; 
// return TRUE if a merge operation was performed, FALSE otherwise
int scenario_try_merge_object(scenario_class *scenario, int object_i);


// merge the object specified by index 'object_i2' to the 
// one specified by index 'object_i1'; if direct_merge is TRUE,
// then direct vertex merging is done, by appending those of 
// object 2 to those of object 1; if direct_merge is FALSE,
// then the order of vertices of object 2 is reversed while merging;
// return SUCCESS on success, ERROR on error
int scenario_merge_objects(scenario_class *scenario, int object_i1, 
			   int object_i2, int direct_merge);

// remove the object specified by index 'object_i' from the scenario; 
// return SUCCESS on success, ERROR on error
int scenario_remove_object(scenario_class *scenario, int object_i);


#endif
