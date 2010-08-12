
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
 * File name: motion.c
 * Function: Main source file of the motion element class
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/

#include <string.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>

#include "deltaQ.h"
#include "motion.h"
#include "message.h"
#include "generic.h"


/////////////////////////////////////////
// Global variable initialization
/////////////////////////////////////////

char *motion_types[]={"linear", "circular", "rotation",
		      "random_walk", "behavioral"};


/////////////////////////////////////////
// Motion structure functions
/////////////////////////////////////////

// init a generic motion
void motion_init(motion_class *motion, char *node_name, int type,
		 double speed_x, double speed_y, double speed_z,
		 double center_x, double center_y,
		 double velocity, double min_speed, double max_speed,
		 double walk_time, double start_time, double stop_time)
{
  // basic fields
  motion->type = type;
  strncpy(motion->node_name, node_name, MAX_STRING-1);
  motion->node_index = INVALID_INDEX;
  motion->start_time = start_time;
  motion->stop_time = stop_time;

  // linear motion fields
  coordinate_init(&(motion->speed), "motion->speed", 
		  speed_x, speed_y, speed_z);

  // rotate motion fields
  coordinate_init(&(motion->center), "motion->center", 
		  center_x, center_y, 0.0);
  motion->velocity = velocity;

  // random walk fields
  motion->min_speed = min_speed;
  motion->max_speed = max_speed;
  motion->walk_time = walk_time;
  motion->current_walk_time = 0;
  // this value signifies unitialized angle and velocity
  motion->angle = DBL_MAX;

  coordinate_init(&(motion->destination), "motion->destination", 
		  0.0, 0.0, 0.0);
  motion->time_since_destination_reached = 0;


  motion->speed_from_destination = FALSE;

  motion->rotation_angle_horizontal = 0;
  motion->rotation_angle_vertical = 0;

  motion->motion_sense = MOTION_UNDECIDED;
}

// initialize the local node index for a motion
// return SUCCESS on succes, ERROR on error
int motion_init_index(motion_class *motion, scenario_class *scenario)
{
  int i;

  if(motion->node_index == INVALID_INDEX)// node_index not initialized
    // try to find corresponding node in scenario
    for(i=0; i<scenario->node_number; i++)
      if(strcmp(scenario->nodes[i].name, motion->node_name)==0)
	{
	  motion->node_index = i;
	  break;
	}

  if(motion->node_index == INVALID_INDEX) // node could not be found
    {
      WARNING("Motion specifies an unexisting node ('%s')", 
	      motion->node_name);
      return ERROR;
    }

  return SUCCESS;
}

// print the fields of a motion
void motion_print(motion_class *motion)
{
  printf("  Motion: node_name='%s' type='%s' speed=(%.2f, %.2f, %.2f) \
destination=(%.2f, %.2f, %.2f) start_time=%.2f stop_time=%.2f\n", 
	 motion->node_name, motion_types[motion->type],
	 motion->speed.c[0], motion->speed.c[1], motion->speed.c[2], 
	 motion->destination.c[0], motion->destination.c[1], 
	 motion->destination.c[2], motion->start_time, motion->stop_time);
}

// copy the information in motion_src to motion_dest
void motion_copy(motion_class *motion_dest, motion_class *motion_src)
{
  // motion type and name
  motion_dest->type = motion_src->type;
  strncpy(motion_dest->node_name, motion_src->node_name, MAX_STRING-1);

  // motion index
  motion_dest->node_index = motion_src->node_index;

  // motion start and stop time
  motion_dest->start_time = motion_src->start_time;
  motion_dest->stop_time = motion_src->stop_time;

  // linear motion fields
  coordinate_copy(&(motion_dest->speed), &(motion_src->speed));

  // rotate motion fields
  coordinate_copy(&(motion_dest->center), &(motion_src->center));
  motion_dest->velocity = motion_src->velocity;

  // random walk fields
  motion_dest->min_speed = motion_src->min_speed;
  motion_dest->max_speed = motion_src->max_speed;
  motion_dest->walk_time = motion_src->walk_time;
  motion_dest->current_walk_time = motion_src->current_walk_time;
  motion_dest->angle = motion_src->angle;

  // behavioral model fields
  coordinate_copy(&(motion_dest->destination), &(motion_src->destination));

  motion_dest->time_since_destination_reached = 
    motion_src->time_since_destination_reached;

  // speed calculation flag
  motion_dest->speed_from_destination = motion_src->speed_from_destination;

  // rotation angles
  motion_dest->rotation_angle_horizontal = 
    motion_src->rotation_angle_horizontal;
  motion_dest->rotation_angle_vertical = 
    motion_src->rotation_angle_vertical;


   motion_dest->motion_sense = motion_src->motion_sense;
}


/////////////////////////////////////////
// Linear motion functions
/////////////////////////////////////////

// implement rectilinear motion with speed specified by (speed_x, speed_y,
// speed_z), for duration 'time_step'
void motion_linear(motion_class *motion, node_class *node, double time_step)
{
  int i;

  // compute new position if moving with speed (speed_x, speed_y, speed_z)
  // for duration 'time_step'
  for(i=0; i<MAX_COORDINATES; i++)
    node->position.c[i] += (motion->speed.c[i] * time_step);
}

// implement rectilinear motion with speed specified by (speed_x, speed_y,
// speed_z) and acceleration 'acceleration' for duration 'time_step'
void motion_linear_acc(motion_class *motion, coordinate_class acceleration,
		       node_class *node, double time_step)
{
  int i;

  //coordinate_class motion_vector;
  //double motion_magnitude;

  // compute new position if moving with speed (speed_x, speed_y, speed_z)
  // for duration 'time_step';
  // the speed was already updated by this time by the equation
  // v'=v+at, therefore the equation d=vt+(at^2)/2 becomes
  // d=v't-(at^2)/2
  for(i=0; i<MAX_COORDINATES; i++)
    node->position.c[i] += (motion->speed.c[i] * time_step -
			    acceleration.c[i] * time_step * time_step / 2);

  /*
  // the speed was already updated by this time by the equation
  // v'=v+at, therefore the equation d=vt+(at^2)/2 becomes
  // d=v't-(at^2)/2
  for(i=0; i<MAX_COORDINATES; i++)
    motion_vector.c[i] = (motion->speed.c[i] * time_step - 
			  acceleration.c[i] * time_step * time_step / 2);

  motion_magnitude=coordinate_vector_magnitude(&motion_vector);

  if(motion_magnitude>WALL_DISTANCE_THRESHOLD)
    WARNING("Motion magnitude %f exceeds WALL_DISTANCE_THRESHOLD (%f)",
	    motion_magnitude, WALL_DISTANCE_THRESHOLD);

  coordinate_vector_sum(&(node->position), &(node->position), &motion_vector);
  */
}


/////////////////////////////////////////
// Circular motion functions
/////////////////////////////////////////

// implement circular motion in x0y plane with respect to center
// (center_x, center_y), with tangential velocity 'velocity',
// for duration 'time_step'
void motion_circular(motion_class *motion, node_class *node, double time_step)
{
  double radius, delta_alpha, alpha;

  radius = sqrt(pow(node->position.c[0] - motion->center.c[0], 2)+
		pow(node->position.c[1] - motion->center.c[1], 2));
  delta_alpha = motion->velocity * time_step / radius;

  alpha = asin((node->position.c[1]-motion->center.c[1])/radius);

  if((node->position.c[0]-motion->center.c[0])<0 && 
     (node->position.c[1]-motion->center.c[1])>0)
    alpha = M_PI - alpha;
  else if((node->position.c[0]-motion->center.c[0])<0 
	  && (node->position.c[1]-motion->center.c[1])<0)
    alpha = -M_PI - alpha;

  node->position.c[0] = radius * cos(alpha+delta_alpha) + motion->center.c[0];
  node->position.c[1] = radius * sin(alpha+delta_alpha) + motion->center.c[1];
}


/////////////////////////////////////////
// Rotation motion functions
/////////////////////////////////////////

// implement rotation motion of the node for duration 'time_step' 
// (node position doesn't change, only its orientation, 
// and by consequence its antenna orientation)
void motion_rotation(motion_class *motion, node_class *node, double time_step)
{
  // for the moment by rotation motion only the orientation of
  // the antenna changes
  fprintf(stderr, "Applying rotation with angles: horiz=%f vert=%f\n",
	  motion->rotation_angle_horizontal, motion->rotation_angle_vertical);

  // update azimuth orientation and bring the result in 
  // the interval [0,360)
  node->azimuth_orientation += motion->rotation_angle_horizontal;
  if(node->azimuth_orientation>=360)
    node->azimuth_orientation-=360;

  // update elevation orientation and bring the result in 
  // the interval [0,360)
  node->elevation_orientation += motion->rotation_angle_vertical;
  if(node->elevation_orientation>=360)
    node->elevation_orientation-=360;

}


/////////////////////////////////////////
// Random-walk motion functions
/////////////////////////////////////////

// implement random walk motion in x0y plane with speed in range 
// [min_speed, max_speed], direction given by 'angle' for at least 
// the total duration of 'walk_time' and with the duration of this 
// step equal to 'time_step'
void motion_random_walk(motion_class *motion, node_class *node, 
			double time_step)
{
  // check if we need to make a change in velocity/direction
  // or whether this parameters are uninitialized (2nd condition)
  if(motion->current_walk_time >= motion->walk_time ||
     motion->angle == DBL_MAX)
    {
      // reset current walk time
      motion->current_walk_time = 0;

      // generate a new velocity
      motion->velocity = rand_min_max(motion->min_speed, motion->max_speed);

      //generate a new angle
      motion->angle = rand_min_max(0, 2*M_PI);
    }

  // compute speeds in x0y plane
  motion->speed.c[0] = motion->velocity * cos(motion->angle);
  motion->speed.c[1] = motion->velocity * sin(motion->angle);

  DEBUG("Random Walk: velocity=%f  speed_x=%f  speed_y=%f  angle=%f\n", 
	motion->velocity, motion->speed.c[0], motion->speed.c[1], 
	motion->angle); 

  // move node linearly
  motion_linear(motion, node, time_step);

  // update current walk time
  motion->current_walk_time += time_step;
}


/////////////////////////////////////////
// Behavioral motion functions
/////////////////////////////////////////

// NOTE 1: The page indications below refer to the paper
//         "Reconsidering Microscopic Mobility Modeling for
//          Self-Organizing Networks" by F. Legendre et al.

// NOTE 2: everything is done considering the case of 2D

// This function implements
// Rule 1: path following rule towards destination (formula 2, page 7)
int motion_behavioral_path_following(motion_class *motion,
				     node_class *node, double time_step,
				     coordinate_class *a_path_following)
{
  coordinate_class path_vector, path_vector_scaled;
  coordinate_class velocity_difference;
  double path_vector_magnitude;

  // scaling constant
  double beta = 1.0;

  printf("*** Path following computation\n");

  // initialize vectors
  coordinate_init(a_path_following, "a_path_following", 0.0, 0.0, 0.0);
  coordinate_init(&path_vector, "path_vector", 0.0, 0.0, 0.0);
  coordinate_init(&path_vector_scaled, "path_vector_scaled", 0.0, 0.0, 0.0);
  coordinate_init(&velocity_difference, "velocity_difference", 0.0, 0.0, 0.0);

  // compute distance to destination
  coordinate_vector_difference_2D(&path_vector, 
				  &(motion->destination), &(node->position));
  coordinate_print(&path_vector);
  path_vector_magnitude = coordinate_vector_magnitude(&path_vector);
  printf("path_vector_magnitude=%f\n", path_vector_magnitude);

  // only try to reach destination if the distance to
  // it exceeds DESTINATION_DISTANCE_THRESHOLD m
  if(path_vector_magnitude>DESTINATION_DISTANCE_THRESHOLD)
    {
#ifdef ACCELERATED_MOTION
      // scale the path vector
      coordinate_multiply_scalar(&path_vector_scaled, 
				 &path_vector, V0/path_vector_magnitude);
      coordinate_print(&path_vector_scaled);
      
      // compute velocity difference
      coordinate_vector_difference(&velocity_difference, 
				   &path_vector_scaled, &(motion->speed));
      coordinate_print(&velocity_difference);

      // scale velocity difference by 'beta' and assign it to result
      coordinate_multiply_scalar(a_path_following, 
				 &velocity_difference, beta);
      coordinate_print(a_path_following);
#else
      // scale the path vector
      coordinate_multiply_scalar(&path_vector_scaled, 
				 &path_vector, V0/path_vector_magnitude);
      coordinate_print(&path_vector_scaled);

      // scale path vector again by 'beta' and assign it to result
      coordinate_multiply_scalar(a_path_following, 
				 &path_vector_scaled, beta);
      coordinate_print(a_path_following);
#endif

      return TRUE;
    }
  else
    return FALSE;
}

// compute detour distance when going around the object 'object' if heading
// toward vertex given by 'vertex_index' when going to 'destination'; 
// detour can be done either in incresing order of vertices or decreasing,
// depending on the parameter go_clockwise (TRUE => increasing order);
// return the detour distance
double compute_detour_distance(object_class *object, int vertex_index, 
			       coordinate_class destination, int go_clockwise)
{
  int counter, vertex_i, vertex_i2;
  double detour_distance;

  DEBUG("Compute detour distance");

  // initialize distance
  detour_distance = 0;

  // initialize loop counter
  counter = 0;

  // initialize start vertex
  vertex_i = vertex_index;

  // we consider height is not important, 
  // and all objects must be avoided
  while(counter<object->vertex_number)
    {
      if(go_clockwise == TRUE)
	vertex_i2 = ((vertex_i+1)<object->vertex_number)?vertex_i+1:0;
      else
	vertex_i2 = ((vertex_i-1)>=0)?vertex_i-1:(object->vertex_number-1);

      DEBUG("vertex_i=%d vertex_i2=%d (go_clockwise=%d counter=%d); vertex=", 
	    vertex_i, vertex_i2, go_clockwise, counter);
      coordinate_print(&object->vertices[vertex_i]);
      
      // check if segment made of vertex and destination is in object
      // (if not, the destination is visible)
      if(object_intersect(&(object->vertices[vertex_i]),
			  &destination, object, FALSE, FALSE) == TRUE)
	{
	  DEBUG("vertex_i2=%d (go_clockwise=%d)", vertex_i2, go_clockwise);
	  
	  detour_distance +=
	    coordinate_distance(&object->vertices[vertex_i], 
				&object->vertices[vertex_i2]);
	  printf("detour_distance=%f (partial result)\n", detour_distance);
	}
      // destination is visible
      else
	{
	  detour_distance += 
	    coordinate_distance(&(object->vertices[vertex_i]), &destination);
	  DEBUG("vertex=");
	  coordinate_print(&(object->vertices[vertex_i]));
	  printf("---- return detour_distance=%f \
(go_clockwise=%d  start vertex=%d  vertex from which object is visible=%d)\n", 
		 detour_distance, go_clockwise, vertex_index, vertex_i);

	  // calculation is finished, we should end the loop
	  break;
	}

      // go to next vertex
      vertex_i = vertex_i2;
      
      // update counter
      counter++;
    }

  return detour_distance;
}

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
				       coordinate_class *a_obstacle_avoidance)
{
  double gamma = 1.0, gamma2 = 1.0;
  coordinate_class intersection;//, min_intersection;
  double min_distance, distance;
  double min_distance2;

  coordinate_class min_vertex; // used in object avoidance

  coordinate_class min_distance_point;

  coordinate_class acceleration, acceleration_s;
  
  int object_i, vertex_i, vertex_i2;

  int do_object_avoidance;
  int avoid_corner;//, corner_is_edge_start;

  object_class *object;

  int last_vertex;

  //int min_dist_vertex;
  int min_dist_vertex_motion_sense;
  int vertex_motion_sense;


  //////////////////////////////////////////////
  // Object avoidance/rejection computation

  printf("*** Object avoidance/rejection computation\n");

  // init vectors
  coordinate_init(a_wall_rejection, "a_wall_rejection", 0.0, 0.0, 0.0);
  coordinate_init(&min_distance_point, "min_distance_point", 0.0, 0.0, 0.0);
  coordinate_init(&acceleration, "acceleration", 0.0, 0.0, 0.0);

  // test if any object prevent the node to reach the destination
  for(object_i=0; object_i<scenario->object_number; object_i++)
    {
      // initialize object pointer
      object = &(scenario->objects[object_i]);
      object_print(object);

      min_distance = DBL_MAX;
      avoid_corner = FALSE;
      do_object_avoidance = FALSE;

      // we consider height is not important, and all objects must be avoided

      // compute distance to all object edges
      last_vertex = (object->make_polygon==TRUE)?object->vertex_number:
	object->vertex_number-1;
      for(vertex_i=0; vertex_i<last_vertex; vertex_i++)
	{
	  vertex_i2=((vertex_i+1)<object->vertex_number)?vertex_i+1:0;
	      
	  // check distance wrt each edge
	  if(coordinate_distance_to_segment_2D(&(node->position), 
					       &(object->vertices[vertex_i]), 
					       &(object->vertices[vertex_i2]),
					       &distance, &intersection)==TRUE)
	    {
	      // distance could be calculated => proceed
	      if(min_distance>distance)
		{
		  min_distance = distance;
		  coordinate_copy(&min_distance_point, &intersection);
		  printf("1. current min_distance=%f min_distance_point=\n", 
			 min_distance);
		  coordinate_print(&min_distance_point);
		  //min_dist_vertex=vertex_i;
		}
	    }
	}

      last_vertex = (object->make_polygon==TRUE)?object->vertex_number:
	object->vertex_number-1;

      // we consider height is not important, 
      // and all objects must be avoided
      for(vertex_i=0; vertex_i<last_vertex; vertex_i++)
	{
	  distance = coordinate_distance_2D(&(node->position), 
					    &(object->vertices[vertex_i]));
	  
	  // distance could be calculated => proceed
	  if(min_distance>distance)
	    {
	      min_distance = distance;
	      coordinate_copy(&min_distance_point, 
			      &(object->vertices[vertex_i]));
	      printf("2. current min_distance=%f  min_distance_point=\n", 
		     min_distance);
	      coordinate_print(&min_distance_point);
	    }
	}

      if(min_distance!=DBL_MAX)
	{
	  /*
	  if((fabs(intersection.c[0]-
		   (object->vertices[min_dist_vertex]).c[0])<EPSILON && 
	      fabs(intersection.c[1]-
		   (object->vertices[min_dist_vertex]).c[1])<EPSILON))
	    {
	      avoid_corner = TRUE;
	      corner_is_edge_start = TRUE;
	    }
	  else if((fabs(intersection.c[0]-
			(object->vertices[min_dist_vertex]).c[0])<EPSILON && 
		   fabs(intersection.c[1]-
			(object->vertices[min_dist_vertex]).c[1])<EPSILON))
	    {
	      avoid_corner = TRUE;
	      corner_is_edge_start = FALSE;
	    }
	  */

	  if(min_distance < OBJECT_AVOIDANCE_THRESHOLD)
	    {
	      do_object_avoidance = TRUE;

	      if(min_distance < WALL_REJECTION_THRESHOLD)
		{
		  if(avoid_corner==FALSE)
		    {
		      // use equation of rule 2
		      coordinate_vector_difference_2D(&acceleration, 
						      &(node->position), 
						      &min_distance_point);
		      DEBUG("node->position=");
		      coordinate_print(&(node->position));
		      DEBUG("min_distance_point=");
		      coordinate_print(&min_distance_point);
		      DEBUG("acceleration=");
		      coordinate_print(&acceleration);
		      
		      // change wrt to paper => use a force inverse 
		      // proportional with distance square (increasing 
		      // as object get closer to wall)
		      if(min_distance==0)
			min_distance=0.001;
		      coordinate_init
			(&acceleration_s, "acceleration_s", 0,0,0);
		      coordinate_multiply_scalar
			(&acceleration_s, &acceleration, 
			 gamma/min_distance/min_distance);
		      coordinate_print(&acceleration_s);
		      
		      coordinate_vector_sum(a_wall_rejection, 
					    a_wall_rejection, &acceleration_s);
		      coordinate_print(a_wall_rejection);
		    }
		  else // this is not used for the moment
		    {
		      INFO("SPECIAL CORNER AVOIDANCE");
		      
		      // use a random small acceleration to avoid corners
		      coordinate_init(&acceleration_s, "acceleration_s", 
				      rand_min_max(-0.1, 0.1), 
				      rand_min_max(-0.1, 0.1), 0);
		      
		      coordinate_vector_sum(a_wall_rejection, 
					    a_wall_rejection, &acceleration_s);
		      DEBUG("a_wall_rejection=");
		      coordinate_print(a_wall_rejection);
		    }
		}
	    }
	  else
	    do_object_avoidance = FALSE;
	}
      else
	{
	  WARNING("Error computing minimum distance");
	  return FALSE;
	}


      /////////////////////////////////////////////////
      // object avoidance computation      
      if(do_object_avoidance == TRUE)
	{
	  double dist_clockwise, dist_counter_clockwise, dist;

	  printf("*** Object avoidance computation\n");

	  // if wall rejection was enforced, we may need to compute how to 
	  // avoid the object by going around it (Rule 3: obstacle avoidance)

	  // first we need to check if the obstacle is preventing us from
	  // reaching the destination (height is not considered)
	  if(object_intersect(&(node->position), &(motion->destination), 
			      object, TRUE, FALSE)==FALSE)
	    {
	      // the obstacle doesn't prevent us from reaching the destination
	      // therefore it doesn't have to be avoided; we can skip the
	      // avoidance algorithm
	      DEBUG("Object doesn't prevent reaching destination");
	      continue;//skip to next object
	    }
	  else
	    DEBUG("Object prevents reaching destination. \
Computing avoidance...");	      

	  min_distance2 = DBL_MAX;

	  last_vertex = (object->make_polygon==TRUE)?object->vertex_number:
	    object->vertex_number-1;

	  for(vertex_i=0; vertex_i<last_vertex; vertex_i++)
	    {
	      DEBUG("------- check if vertex %d is visible", vertex_i);
	      /*
	      if(coordinate_are_equal(&min_distance_point, 
				      &(object->vertices[vertex_i]))==TRUE)
		{
		  DEBUG("------- skipping vertex %d because equal to the \
vertex used in wall avoidance", vertex_i);
		  continue;
		}
	      */  
	      
	      // check if segment made of position and vertex is in object,
	      // therefore invisible
	      if(object_intersect(&(node->position), 
				  &(object->vertices[vertex_i]), 
				  object, FALSE, FALSE) == TRUE)
		{
		  DEBUG("------- skipping vertex %d because invisible", 
			vertex_i);
		  continue;//skip to next vertex
		}

	      DEBUG("---- found candidate=");
	      coordinate_print(&(object->vertices[vertex_i]));
	      
	      // to compute the detour distance we need to compute the sum of 
	      // object edges up to last vertex from where the destination 
	      // is visible; this is done both clockwise and counter-clockwise
	      dist_clockwise = 
		compute_detour_distance(object, vertex_i, 
					motion->destination, TRUE);

	      dist_counter_clockwise = 
		compute_detour_distance(object,	vertex_i, 
					motion->destination, FALSE);
	      /*	      
	      if(motion->motion_sense == MOTION_UNDECIDED)
		{
	      */
		  dist = coordinate_distance(&(node->position), 
					     &(object->vertices[vertex_i])) + 
		    ((dist_clockwise<dist_counter_clockwise)?dist_clockwise:
		     dist_counter_clockwise);
		  
		  vertex_motion_sense = 
		    (dist_clockwise<dist_counter_clockwise)?
		    MOTION_CLOCKWISE:MOTION_COUNTERWISE;
	      /*
		}
	      else if(motion->motion_sense == MOTION_CLOCKWISE)
		{
		  dist = coordinate_distance(&(node->position), 
					     &(object->vertices[vertex_i])) + 
		    dist_clockwise;
		  
		  vertex_motion_sense = MOTION_CLOCKWISE;
		}
	      else if(motion->motion_sense == MOTION_COUNTERWISE)
		{
		  dist = coordinate_distance(&(node->position), 
					     &(object->vertices[vertex_i])) + 
		    dist_counter_clockwise;
		  
		  vertex_motion_sense = MOTION_COUNTERWISE;
		}
	      else
		{
		  WARNING("Motion sense is unknown");
		  return FALSE;
		}
	      */
	      
	      printf("---- possible detour for object='%s' => \
vertex_i=%d dist=%f dist_p_v=%f dist_c=%f dist_cc=%f\n", 
		     object->name, vertex_i, dist, 
		     coordinate_distance(&(node->position), 
					 &(object->vertices[vertex_i])), 
		     dist_clockwise, dist_counter_clockwise);
	      
	      if(min_distance2>dist)
		{
		  min_distance2 = dist;
		  coordinate_copy(&min_vertex, &(object->vertices[vertex_i]));
		  printf("min_distance2=%f  min_vertex=\n", min_distance2);
		  coordinate_print(&min_vertex);
		  min_dist_vertex_motion_sense = vertex_motion_sense;
		}
	    }

	  // check a distance could be calculated and the object is not too
	  // far away
	  if(min_distance2 != DBL_MAX)
	    {
	      printf("---- detour for object='%s' => dist=%f vertex=\n", 
		     object->name, min_distance2);
	      coordinate_print(&min_vertex);

	      if(motion->motion_sense == MOTION_UNDECIDED)
		motion->motion_sense = min_dist_vertex_motion_sense;

	      // use equation of rule 3
	      coordinate_vector_difference_2D(&acceleration, &min_vertex, 
					      &(node->position));
	      DEBUG("acceleration=");
	      coordinate_print(&acceleration);

	      coordinate_multiply_scalar
		(&acceleration_s, &acceleration,
		 gamma2/coordinate_vector_magnitude(&acceleration));
	      DEBUG("acceleration_s=");
	      coordinate_print(&acceleration_s);

	      coordinate_vector_sum(a_obstacle_avoidance, 
			 a_obstacle_avoidance, &acceleration_s);
	      DEBUG("a_obstacle_avoidance=");
	      coordinate_print(a_obstacle_avoidance);
	    }
	  else
	    {
	      WARNING("Error computing detour distance");
	      return FALSE;
	    }
 	}
    }
      
  return TRUE;
}

int motion_behavioral(motion_class *motion, scenario_class *scenario, 
		      node_class *node, double time_step)
{
  coordinate_class acceleration;

#ifdef ACCELERATED_MOTION
  coordinate_class acceleration_t;
#endif

  coordinate_class a_path_following, a_wall_rejection, 
    a_obstacle_avoidance, a_mutual_avoidance;

  coordinate_class new_speed;

  double acceleration_magnitude, speed_magnitude;
  
  // NOTE 1: The page indications below refer to the paper
  //         "Reconsidering Microscopic Mobility Modeling for
  //          Self-Organizing Networks" by F. Legendre et al.

  // NOTE 2: everything is done considering the case of 2D

  coordinate_init(&acceleration, "acceleration", 0.0, 0.0, 0.0);

  coordinate_init(&a_path_following, "a_path_following", 0.0, 0.0, 0.0);
  coordinate_init(&a_wall_rejection, "a_wall_rejection", 0.0, 0.0, 0.0);
  coordinate_init(&a_obstacle_avoidance, "a_obstacle_avoidance", 
		  0.0, 0.0, 0.0);
  coordinate_init(&a_mutual_avoidance, "a_mutual_avoidance", 0.0, 0.0, 0.0);

  // compute path following acceleration; if destination was reached
  // (return value is FALSE) then skip calculations; else compute 
  // accelerations and move object
  if(motion_behavioral_path_following(motion, node, 
				      time_step, &a_path_following)==TRUE)
    {
      // compute object avoidance acceleration
      motion_behavioral_object_avoidance(motion, scenario, node, time_step, 
					 &a_wall_rejection, 
					 &a_obstacle_avoidance);

      printf("*** Acceleration sum computation\n");

      // if wall rejection or obstacle avoidance forces 
      // are enforced, use them exclusively
      if(coordinate_vector_magnitude(&a_wall_rejection)>EPSILON ||
	 coordinate_vector_magnitude(&a_obstacle_avoidance))
	coordinate_init(&a_path_following, "a_path_following", 0.0, 0.0, 0.0);

      /*
      if(coordinate_magnitude(a_obstacle_avoidance)>EPSILON)
	coordinate_init(&a_wall_rejection, 0.0, 0.0, 0.0);
      */

      // sum all acceleration components
      DEBUG("Adding a_path_following and a_wall_rejection:");
      coordinate_print(&a_path_following);
      coordinate_print(&a_wall_rejection);
      coordinate_vector_sum(&acceleration, 
			    &a_path_following, &a_wall_rejection);
      DEBUG("1. result acceleration=");
      coordinate_print(&acceleration);

      DEBUG("Adding acceleration and a_obstacle_avoidance:");
      coordinate_print(&acceleration);
      coordinate_print(&a_obstacle_avoidance);
      coordinate_vector_sum(&acceleration, 
			    &acceleration, &a_obstacle_avoidance);
      DEBUG("2. result acceleration=");
      coordinate_print(&acceleration);

      DEBUG("Adding acceleration and a_mutual_avoidance:");
      coordinate_print(&acceleration);
      coordinate_print(&a_mutual_avoidance);
      coordinate_vector_sum(&acceleration, &acceleration, &a_mutual_avoidance);
      DEBUG("3. result acceleration=");
      coordinate_print(&acceleration);

      
      // check that acceleration values don't exceed certain limits
      acceleration_magnitude = coordinate_vector_magnitude(&acceleration);
      
#ifdef ACCELERATED_MOTION

      if(acceleration_magnitude > MAX_ACC_MAGN)
	{
	  //fprintf(stderr, "Normalized acceleration from %f to %f\n",
	  //  acceleration_magnitude, MAX_ACC_MAGN);
	  coordinate_multiply_scalar(&acceleration, &acceleration, 
				     MAX_ACC_MAGN/acceleration_magnitude);
	}
      
      // To calculate the new speed use formula 
      //        v(t+delta_t) = v(t) + a*delta_t
      // on page 7 of "Modeling Mobility with Behavioral Rules:
      // the Case of Incident and Emergency Situations" by
      // F. Legendre et al.

      // compute acc * delta_t <=> change in speed over time
      coordinate_multiply_scalar(&acceleration_t, &acceleration, time_step);
      DEBUG("acceleration_t=");
      coordinate_print(&acceleration_t);
      
      DEBUG("motion_speed=");
      coordinate_print(&(motion->speed));
      // compute the new speed 
      coordinate_vector_sum(&new_speed, &(motion->speed), &acceleration_t);
      DEBUG("new_speed (accelerated motion)=");
      coordinate_print(&new_speed);

      speed_magnitude = coordinate_vector_magnitude(&new_speed);
      if(speed_magnitude > MAX_SPEED_MAGN)
	{
	  //fprintf(stderr, "Normalized speed from %f to %f\n",
	  //  speed_magnitude, MAX_SPEED_MAGN);
	  coordinate_multiply_scalar(&new_speed, &new_speed, 
				     MAX_SPEED_MAGN/speed_magnitude);
	  // in this case acceleration must be set to 0, since the motion
	  // should be seen as a uniform motion
	  coordinate_init(&acceleration, "acceleration", 0.0, 0.0, 0.0);
	}

#else // do not use accelerated motion but uniform!!!!!!!!!!!!!!!!

      // normalize acceleration to get just its direction
      coordinate_multiply_scalar(&acceleration, &acceleration, 
				 1/acceleration_magnitude);

      // new speed is obtained by multiplying the normalized acceleration 
      // by a constant magnitude (e.g., 0.5 => 0.5 m/s)
      coordinate_multiply_scalar(&new_speed, &acceleration, V0);
      DEBUG("new_speed (non-accelerated motion)=");
      coordinate_print(&new_speed);
#endif      
      /*      
      // check that the node has not reached a dead point
      if(coordinate_magnitude(new_speed)<0.1)
	{
	  // use some random values to get out of dead point
	  motion->speed_x = MIN_RAND_SPEED + (rand()/((double)RAND_MAX+1.0)) * 
	    (MAX_RAND_SPEED - MIN_RAND_SPEED);
	  motion->speed_y = MIN_RAND_SPEED + (rand()/((double)RAND_MAX+1.0)) * 
	    (MAX_RAND_SPEED - MIN_RAND_SPEED);
	  motion->speed_z = 0.0; // 2D assumption
	}
      else
	{
      */

      // update motion parameters
      coordinate_copy(&(motion->speed), &new_speed);
	  //}

#ifdef ACCELERATED_MOTION
      // move node linearly with computed speed and acceleration
      motion_linear_acc(motion, acceleration, node, time_step);
#else
      // move node linearly with computed speed
      motion_linear(motion, node, time_step);
#endif

    }
  else // stop object
    {
      INFO("Node '%s' reached destination %f s ago", node->name,
	   motion->time_since_destination_reached);

      // update motion parameters
      coordinate_init(&(motion->speed), "speed", 0.0, 0.0, 0.0);
      coordinate_init(&acceleration, "acceleration", 0.0, 0.0, 0.0);

      motion->time_since_destination_reached+=time_step;


      // SPECIAL CODE for PLT collaboration!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      /*
      if(motion->time_since_destination_reached > 18)
	{
	  INFO("Resetting node '%s' position to a predefined place",
	       node->name);

	  coordinate_init(&(node->position), "position", 
			  10000.0, 10000.0, 0.0);
	}
      */
      // END SPECIAL CODE
    }

  return SUCCESS;
}

// apply the motion to a node from scenario for the specified duration
// return SUCCESS on succes, ERROR on error
int motion_apply(motion_class *motion, scenario_class *scenario, 
		 double time_step)
{
  if(motion->node_index == INVALID_INDEX) // node could not be found
    {
      WARNING("Motion specifies an unexisting node ('%s')", 
	      motion->node_name);
      return ERROR;
    }
  else
    {
      node_class *node = &(scenario->nodes[motion->node_index]);

      // check whether speed needs to be initialized based on destination
      if(motion->type == LINEAR_MOTION && motion->speed_from_destination)
	{
	  coordinate_class speed;

	  coordinate_vector_difference(&speed, &(motion->destination), 
				       &(node->position));

	  DEBUG("Calculating speed from distance to destination");
	  coordinate_multiply_scalar(&speed, &speed, 
				 1/(motion->stop_time-motion->start_time));
	  
	  coordinate_copy(&(motion->speed), &speed);
	  motion->speed_from_destination = FALSE;
	}

      motion_print(motion);

      // move node according to the motion type field
      if(motion->type == LINEAR_MOTION)
      	motion_linear(motion, node, time_step);
      else if(motion->type == CIRCULAR_MOTION)
	motion_circular(motion, node, time_step);
      else if(motion->type == ROTATION_MOTION)
	motion_rotation(motion, node, time_step);
      else if(motion->type == RANDOM_WALK_MOTION)
	motion_random_walk(motion, node, time_step);
      else if(motion->type == BEHAVIORAL_MOTION)
	motion_behavioral(motion, scenario, node, time_step);
      else
	{
	  WARNING("Undefined motion type (%d)", motion->type);
	  return ERROR;
	}

#ifdef MESSAGE_INFO
      INFO("Moved node position updated:");
      node_print(node);
#endif

    }

  return SUCCESS;
}
