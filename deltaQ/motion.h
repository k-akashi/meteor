
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
 * File name: motion.h
 * Function:  Main header file of the motion modeling library
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#ifndef __MOTION_H
#define __MOTION_H


#include "global.h"


////////////////////////////////////////////////
// Global variables
////////////////////////////////////////////////

extern char *motion_types[];


/////////////////////////////////////////
// Motion algorithm constants
/////////////////////////////////////////

#define ACCELERATED_MOTION

#define DESTINATION_DISTANCE_THRESHOLD  0.5
#define WALL_REJECTION_THRESHOLD        0.75 //0.4 //1.0 //2//1
#define OBJECT_AVOIDANCE_THRESHOLD      1.5 //0.8 //1.0 //2//1


#define MIN_RAND_SPEED  -0.2
#define MAX_RAND_SPEED  0.2

// cruising speed in the behavioral paper
#define V0                              1.0

#define MAX_ACC_MAGN    3.0 //10.0 // ~1G = 9.81 m/s^2
#define MAX_SPEED_MAGN  1.0 //10.0 // 100m => 10 s

#define MOTION_UNDECIDED                 0
#define MOTION_CLOCKWISE                 1
#define MOTION_COUNTERWISE               2

/////////////////////////////////////////
// Motion structure definition
/////////////////////////////////////////

struct motion_class_s
{
  // motion type
  int type;

  // name of node to which motion is applied
  char node_name[MAX_STRING];

  // node index in 'scenario' structure;
  // default value (INVALID_INDEX) is replaced during 
  // an initial search for the defined node
  int node_index;

  // speed values on (x,y,z) axes
  coordinate_class speed;

  // center values on (x,y) axes for circular motion in x0y plane
  coordinate_class center;

  // velocity for circular motion or random walk
  double velocity;

  // velocity limits for random walk
  double min_speed, max_speed;

  // total time until next direction change and current time
  // for random walk
  double walk_time, current_walk_time;

  // angle of velocity for random walk
  double angle; 

  // motion start and stop times
  double start_time, stop_time;

  // destination of behavioral or linear motion
  coordinate_class destination;

  // time since destination was reached
  double time_since_destination_reached;

  // flag showing whether speed needs to be computed
  // based on the inistial position of the node and 
  // its destination
  int speed_from_destination;

  // horizontal and vertical rotation angles
  double rotation_angle_horizontal;
  double rotation_angle_vertical;

  int motion_sense;
};

/////////////////////////////////////////
// Motion structure functions
/////////////////////////////////////////

// init a generic motion
void motion_init(motion_class *motion, char *node_name, int type,
		 double speed_x, double speed_y, double speed_z,
		 double center_x, double center_y,
		 double velocity, double min_speed, double max_speed,
		 double walk_time, double start_time, double stop_time);

// initialize the local node index for a motion
// return SUCCESS on succes, ERROR on error
int motion_init_index(motion_class *motion, scenario_class *scenario);

// print the fields of a motion
void motion_print(motion_class *motion);

// copy the information in motion_src to motion_dest
void motion_copy(motion_class *motion_dest, motion_class * motion_src);

// apply the motion to a node from scenario for the specified duration
// return SUCCESS on succes, ERROR on error
int motion_apply(motion_class *motion, scenario_class *scenario, 
		 double time_step);

// implement rectilinear motion with speed specified by (speed_x, speed_y,
// speed_z), for duration 'time_step'
void motion_linear(motion_class *motion, node_class *node, double time_step);

// implement circular motion in x0y plane with respect to center 
// (center_x, center_y), with tangential velocity 'velocity' 
// for duration 'time_step'
void motion_circular(motion_class *motion, node_class *node, double time_step);

// implement random walk motion in x0y plane with speed in range 
// [min_speed, max_speed], direction given by 'angle' for at least 
// the total duration of 'walk_time' and with the duration of this 
// step equal to 'time_step'
void motion_random_walk(motion_class *motion, node_class *node, double time_step);


// NOTE 1: The page indications below refer to the paper
//         "Reconsidering Microscopic Mobility Modeling for
//          Self-Organizing Networks" by F. Legendre et al.

// NOTE 2: everything is done considering the case of 2D

// This function implements
// Rule 1: path following rule towards destination (formula 2, page 7)
int motion_behavioral_path_following(motion_class *motion,
				     node_class *node, double time_step,
				     coordinate_class *a_path_following);

double compute_detour_distance(object_class *object, int vertex_index, 
			       coordinate_class destination, int go_clockwise);

// NOTE 1: The page indications below refer to the paper
//         "Reconsidering Microscopic Mobility Modeling for
//          Self-Organizing Networks" by F. Legendre et al.

// NOTE 2: everything is done considering the case of 2D

// This function implements
// Rules 2 & 3: wall _and_ obstacle avoidance (formulas 3 & 4, page 7)
int motion_behavioral_object_avoidance(motion_class *motion, 
				       scenario_class *scenario, 
				       node_class *node,
				       double time_step, 
				       coordinate_class *a_wall_rejection, 
				       coordinate_class *a_obstacle_avoidance);

int motion_behavioral(motion_class *motion, scenario_class *scenario, 
		      node_class *node, double time_step);


#endif
