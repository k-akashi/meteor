
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
 * File name: motion.h
 * Function:  Main header file of the motion modeling library
 *
 * Author: Razvan Beuran
 *
 * $Id: motion.h 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/


#ifndef __MOTION_H
#define __MOTION_H


#include "global.h"
#include "coordinate.h"


////////////////////////////////////////////////
// Global variables
////////////////////////////////////////////////

extern char *motion_types[];


/////////////////////////////////////////
// Motion algorithm constants
/////////////////////////////////////////

#define ACCELERATED_MOTION
#define DESTINATION_DISTANCE_THRESHOLD  0.5

///////////////////////////////////////////
// parameters for behavioral mobility model

// cruising speed in the behavioral paper in m/s
// * 1 m/s => 3.6 km/h => pedestrian*
// 4.17 m/s => 15 km/h => bicycle
// * 5 m/s => 18 km/h => bicycle*
// 11.11 m/s => 40 km/h => car in town (Japan)
// * 12 m/s => 43.2 km/h => car in town* (Japan)
// 16.67 m/s => 60 km/h => car outside town (Japan)
// * 17 m/s => 61.2 km/h => car outside town* (Japan)
#define V0                              1.0

// maximum acceleration and speed (assume max speed is V0)
#define MAX_ACC_MAGN    3.0	//10.0 // ~1G = 9.81 m/s^2
#define MAX_SPEED_MAGN  V0	//2.0 //1.0 //10.0 // 100m => 10 s

// thresholds for various cruising speeds
//* V0=1  => 0.75   1.5   2.0    => pedestrian*
//V0=2    =>  1.5   3.0   4.0      
//V0=3    => 2.25   4.5   6.0
//* V0=5  =>  5.0  10.0  10.0    => bicycle*
//* V0=12 => 12.0  24.0  24.0   => car in town* (Japan)


// threshold starting at which wall rejection force is applied
#define WALL_REJECTION_THRESHOLD        0.75	//0.4 //1.0 //2//1
// threshold at which object avoidance force is applied
#define OBJECT_AVOIDANCE_THRESHOLD      1.5	//0.8 //1.0 //2//1
// threshold at which mutual avoidance force is applied
#define MUTUAL_AVOIDANCE_THRESHOLD      2.0

// not used currently
//#define MIN_RAND_SPEED  -0.2
//#define MAX_RAND_SPEED  0.2


////////////////////////////////////////////
// other parameters
#define MOTION_UNDECIDED                 0
#define MOTION_CLOCKWISE                 1
#define MOTION_COUNTERWISE               2

#define MAX_MOBILITY_RECORDS            2000


/////////////////////////////////////////
// Motion structure definition
/////////////////////////////////////////

struct trace_record_class
{
  double time;
  struct coordinate_class coord;
  double azimuth_orientation;
  double elevation_orientation;
};

struct motion_class
{
  // motion id
  int id;

  // motion type
  int type;

  // name of node to which motion is applied
  char node_name[MAX_STRING];

  // node index in 'scenario' structure;
  // default value (INVALID_INDEX) is replaced during 
  // an initial search for the defined node
  int node_index;

  // speed values on (x,y,z) axes
  struct coordinate_class speed;

  // center values on (x,y) axes for circular motion in x0y plane
  struct coordinate_class center;

  // velocity for circular motion or random-walk motion
  double velocity;

  // velocity limits for random-walk model
  double min_speed, max_speed;

  // total time until next direction change and current time
  // for random-walk model
  double walk_time, current_walk_time;

  // angle of velocity for random-walk model
  double angle;

  // motion start and stop times
  double start_time, stop_time;

  // destination of behavioral or linear motion
  struct coordinate_class destination;

  // time since destination was reached
  double time_since_destination_reached;

  // flag showing whether speed needs to be computed
  // based on the initial position of the node and 
  // its destination
  int speed_from_destination;

  // horizontal and vertical rotation angles
  double rotation_angle_horizontal;
  double rotation_angle_vertical;

  int motion_sense;

  // name of the file containing mobility information
  char mobility_filename[MAX_STRING];

  // flag showing whether the name of the file containing 
  // mobility information has been provided
  int mobility_filename_provided;

  // mobility trace records
  struct trace_record_class trace_records[MAX_MOBILITY_RECORDS];
  int trace_record_number;
  int trace_record_crt;
};


/////////////////////////////////////////
// Motion structure functions
/////////////////////////////////////////

// init a generic motion
void motion_init (struct motion_class *motion, int id, char *node_name,
		  int type,
		  double speed_x, double speed_y, double speed_z,
		  double center_x, double center_y,
		  double velocity, double min_speed, double max_speed,
		  double walk_time, double start_time, double stop_time);

// initialize the local node index for a motion
// return SUCCESS on succes, ERROR on error
int motion_init_index (struct motion_class *motion,
		       struct scenario_class *scenario,
		       int cartesian_coord_syst);

// print the fields of a motion
void motion_print (struct motion_class *motion);

// copy the information in motion_src to motion_dest
void motion_copy (struct motion_class *motion_dest,
		  struct motion_class *motion_src);

// apply the motion to a node from scenario for the specified duration
// return SUCCESS on succes, ERROR on error
int motion_apply (struct motion_class *motion,
		  struct scenario_class *scenario, double motion_current_time,
		  double time_step);

// implement rectilinear motion with speed specified by (speed_x, speed_y,
// speed_z), for duration 'time_step'
void motion_linear (struct motion_class *motion, struct node_class *node,
		    double time_step);

// implement circular motion in x0y plane with respect to center 
// (center_x, center_y), with tangential velocity 'velocity' 
// for duration 'time_step'
void motion_circular (struct motion_class *motion, struct node_class *node,
		      double time_step);

// implement random-walk motion in x0y plane with speed in range 
// [min_speed, max_speed], direction given by 'angle' for at least 
// the total duration of 'walk_time' and with the duration of this 
// step equal to 'time_step'
void motion_random_walk (struct motion_class *motion, struct node_class *node,
			 double time_step);


// NOTE 1: The page indications below refer to the paper
//         "Reconsidering Microscopic Mobility Modeling for
//          Self-Organizing Networks" by F. Legendre et al.

// NOTE 2: everything is done considering the case of 2D

// This function implements
// Rule 1: path following rule towards destination (formula 2, page 7)
int motion_behavioral_path_following (struct motion_class *motion,
				      struct node_class *node,
				      double time_step,
				      struct coordinate_class
				      *a_path_following);

double compute_detour_distance (struct object_class *object, int vertex_index,
				struct coordinate_class destination,
				int go_clockwise);

// NOTE 1: The page indications below refer to the paper
//         "Reconsidering Microscopic Mobility Modeling for
//          Self-Organizing Networks" by F. Legendre et al.

// NOTE 2: everything is done considering the case of 2D

// This function implements
// Rules 2 & 3: wall _and_ obstacle avoidance (formulas 3 & 4, page 7)
int motion_behavioral_object_avoidance (struct motion_class *motion,
					struct scenario_class *scenario,
					struct node_class *node,
					double time_step,
					struct coordinate_class
					*a_wall_rejection,
					struct coordinate_class
					*a_obstacle_avoidance);

int motion_behavioral (struct motion_class *motion,
		       struct scenario_class *scenario,
		       struct node_class *node, double time_step);

void trace_record_copy (struct trace_record_class *dst,
			struct trace_record_class *src);

// compute the relative velocity (absolute value) between two nodes
double motion_relative_velocity (struct scenario_class *scenario,
				 struct node_class *node1,
				 struct node_class *node2);

#endif
