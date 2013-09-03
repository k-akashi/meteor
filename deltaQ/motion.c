
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
 * File name: motion.c
 * Function: Main source file of the motion element class
 *
 * Author: Razvan Beuran
 *
 * $Id: motion.c 146 2013-06-20 00:50:48Z razvan $
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

char *motion_types[] = { "linear", "circular", "rotation",
  "random_walk", "behavioral", "qualnet"
};


/////////////////////////////////////////
// Motion structure functions
/////////////////////////////////////////

// init a generic motion
void
motion_init (struct motion_class *motion, int id, char *node_name, int type,
	     double speed_x, double speed_y, double speed_z,
	     double center_x, double center_y,
	     double velocity, double min_speed, double max_speed,
	     double walk_time, double start_time, double stop_time)
{
  // basic fields
  motion->id = id;
  motion->type = type;
  strncpy (motion->node_name, node_name, MAX_STRING - 1);
  motion->node_index = INVALID_INDEX;
  motion->start_time = start_time;
  motion->stop_time = stop_time;

  // linear motion fields
  coordinate_init (&(motion->speed), "motion->speed",
		   speed_x, speed_y, speed_z);

  // rotate motion fields
  coordinate_init (&(motion->center), "motion->center",
		   center_x, center_y, 0.0);
  motion->velocity = velocity;

  // random-walk fields
  motion->min_speed = min_speed;
  motion->max_speed = max_speed;
  motion->walk_time = walk_time;
  motion->current_walk_time = 0;
  // this value signifies unitialized angle and velocity
  motion->angle = DBL_MAX;

  coordinate_init (&(motion->destination), "motion->destination",
		   0.0, 0.0, 0.0);
  motion->time_since_destination_reached = 0;


  motion->speed_from_destination = FALSE;

  motion->rotation_angle_horizontal = 0;
  motion->rotation_angle_vertical = 0;

  motion->motion_sense = MOTION_UNDECIDED;

  strcpy (motion->mobility_filename, "N/A");
  motion->mobility_filename_provided = FALSE;
  motion->trace_record_number = 0;
  motion->trace_record_crt = 0;
}

int
motion_load_qualnet_mobility (struct motion_class *motion,
			      struct scenario_class *scenario,
			      int cartesian_coord_syst)
{
  if (motion->mobility_filename_provided == TRUE)
    {
      FILE *mobility_file;
      char line[MAX_STRING];
      int i = 0;
      int line_nr = 0;

      int node_id;
      double time;
      struct coordinate_class read_coord;
      //double c0, c1, c2;
      double azimuth, elevation;

      int scanned_values;

      mobility_file = fopen (motion->mobility_filename, "r");
      if (mobility_file == NULL)
	{
	  WARNING ("Cannot open QualNet mobility file '%s'.",
		   motion->mobility_filename);
	  return ERROR;
	}

      while (fgets (line, MAX_STRING, mobility_file) != NULL)
	{
	  line_nr++;

	  // skip empty lines and comments
	  if ((line[0] == '\n') || (line[0] == '#'))
	    continue;

	  //printf("MF: %s", line);

	  // initialize variable
	  node_id = 0;
	  time = 0.0;
	  read_coord.c[0] = read_coord.c[1] = read_coord.c[2] = 0.0;
	  azimuth = elevation = 0.0;

	  // get new values
	  scanned_values
	    = sscanf (line, "%d %lf (%lf, %lf, %lf) %lf %lf",
		      &node_id, &time, &(read_coord.c[0]), &(read_coord.c[1]),
		      &(read_coord.c[2]), &azimuth, &elevation);
	  if (scanned_values < 5)
	    {
	      WARNING ("Incorrect data on line %d in mobility file '%s': %s",
		       line_nr, motion->mobility_filename, line);
	      WARNING ("Expected format is: <NODE_ID> <TIME> (<COORD1>, \
<COORD2>, <COORD3>) [<AZIMUTH> <ELEVATION>]");
	      WARNING ("NOTE: Only the <AZIMUTH> and <ELEVATION> parameters \
are optional.");

	      fclose (mobility_file);
	      return ERROR;
	    }

	  if (scenario->nodes[motion->node_index].id == node_id)
	    {
	      DEBUG ("MT(%d): %d %lf (%lf, %lf, %lf) %lf %lf",
		     scanned_values, node_id, time, read_coord.c[0],
		     read_coord.c[1], read_coord.c[2], azimuth, elevation);

	      // check for all records except the first one if the new
	      // record time is superior to the last added one, and
	      // take appropriate actions otherwise
	      if (i > 0)
		{
		  // do not add any records with time inferior to that of
		  // the last added record
		  if (time < motion->trace_records[i - 1].time)
		    {
		      WARNING
			("Time %f of new record is inferior to the time of the last added record (%f). Aborting...",
			 time, motion->trace_records[i - 1].time);
		      fclose (mobility_file);
		      return ERROR;
		    }
		  // if a new record has the same time with the last added one,
		  // replace the old record with the new one
		  else if (time == motion->trace_records[i - 1].time)
		    {
		      INFO
			("Time %f of new record equals the time of the last added record (%f). Replacing last record with the new one...",
			 time, motion->trace_records[i - 1].time);
		      i--;
		    }
		}

	      motion->trace_records[i].time = time;

	      // make sure to convert non-Cartesian coordinates
	      if (cartesian_coord_syst == FALSE)
		{
		  struct coordinate_class point_xyz;
		  ll2en (&read_coord, &point_xyz);
		  coordinate_copy (&(motion->trace_records[i].coord),
				   &point_xyz);
		}
	      else
		coordinate_copy (&(motion->trace_records[i].coord),
				 &read_coord);

	      motion->trace_records[i].azimuth_orientation = azimuth;
	      motion->trace_records[i].elevation_orientation = elevation;

	      if (i < MAX_MOBILITY_RECORDS - 1)
		i++;
	      else
		{
		  WARNING
		    ("Maximum number of motion trace records (%d) exceeded.",
		     MAX_MOBILITY_RECORDS);
		  return ERROR;
		}
	    }
	}

      motion->trace_record_number = i;
      DEBUG ("Read %u lines from mobility file. Found %d records for node \
with id '%d'", line_nr, i, scenario->nodes[motion->node_index].id);
      fclose (mobility_file);
    }
  else
    return ERROR;

  return SUCCESS;
}

// initialize the local node index for a motion
// return SUCCESS on succes, ERROR on error
int
motion_init_index (struct motion_class *motion,
		   struct scenario_class *scenario, int cartesian_coord_syst)
{
  int i;

  if (motion->node_index == INVALID_INDEX)	// node_index not initialized
    // try to find corresponding node in scenario
    for (i = 0; i < scenario->node_number; i++)
      if (strcmp (scenario->nodes[i].name, motion->node_name) == 0)
	{
	  motion->node_index = i;
	  // copy also the motion index to the node 
	  // for use in mobility calculations (e.g., behavioral model)
	  (scenario->nodes[i]).motion_index = motion->id;
	  break;
	}

  if (motion->node_index == INVALID_INDEX)	// node could not be found
    {
      WARNING ("Motion specifies an unexisting node ('%s')",
	       motion->node_name);
      return ERROR;
    }

  if (motion->type == QUALNET_MOTION)
    {
      DEBUG ("Loading QualNet mobility file...");

      if (motion_load_qualnet_mobility
	  (motion, scenario, cartesian_coord_syst) == ERROR)
	return ERROR;
    }

  return SUCCESS;
}

// print the fields of a motion
void
motion_print (struct motion_class *motion)
{
  printf ("  Motion: node_name='%s' type='%s' speed=(%.2f, %.2f, %.2f) \
destination=(%.2f, %.2f, %.2f) start_time=%.2f stop_time=%.2f\n", motion->node_name, motion_types[motion->type], motion->speed.c[0], motion->speed.c[1], motion->speed.c[2], motion->destination.c[0], motion->destination.c[1], motion->destination.c[2], motion->start_time, motion->stop_time);
  printf ("          mobility_filename='%s'\n", motion->mobility_filename);
}

// copy the information in motion_src to motion_dst
void
motion_copy (struct motion_class *motion_dst, struct motion_class *motion_src)
{
  int i;

  // motion id, type and name
  motion_dst->id = motion_src->id;
  motion_dst->type = motion_src->type;
  strncpy (motion_dst->node_name, motion_src->node_name, MAX_STRING - 1);

  // motion index
  motion_dst->node_index = motion_src->node_index;

  // motion start and stop time
  motion_dst->start_time = motion_src->start_time;
  motion_dst->stop_time = motion_src->stop_time;

  // linear motion fields
  coordinate_copy (&(motion_dst->speed), &(motion_src->speed));

  // rotate motion fields
  coordinate_copy (&(motion_dst->center), &(motion_src->center));
  motion_dst->velocity = motion_src->velocity;

  // random-walk fields
  motion_dst->min_speed = motion_src->min_speed;
  motion_dst->max_speed = motion_src->max_speed;
  motion_dst->walk_time = motion_src->walk_time;
  motion_dst->current_walk_time = motion_src->current_walk_time;
  motion_dst->angle = motion_src->angle;

  // behavioral model fields
  coordinate_copy (&(motion_dst->destination), &(motion_src->destination));

  motion_dst->time_since_destination_reached =
    motion_src->time_since_destination_reached;

  // speed calculation flag
  motion_dst->speed_from_destination = motion_src->speed_from_destination;

  // rotation angles
  motion_dst->rotation_angle_horizontal =
    motion_src->rotation_angle_horizontal;
  motion_dst->rotation_angle_vertical = motion_src->rotation_angle_vertical;

  motion_dst->motion_sense = motion_src->motion_sense;

  // motion file name
  strncpy (motion_dst->mobility_filename, motion_src->mobility_filename,
	   MAX_STRING - 1);

  motion_dst->mobility_filename_provided
    = motion_src->mobility_filename_provided;

  for (i = 0; i < motion_src->trace_record_number; i++)
    trace_record_copy (&(motion_dst->trace_records[i]),
		       &(motion_src->trace_records[i]));
  motion_dst->trace_record_number = motion_src->trace_record_number;
  motion_dst->trace_record_crt = motion_src->trace_record_crt;
}


/////////////////////////////////////////
// Linear motion functions
/////////////////////////////////////////

// implement rectilinear motion with speed specified by (speed_x, speed_y,
// speed_z), for duration 'time_step'
void
motion_linear (struct motion_class *motion, struct node_class *node,
	       double time_step)
{
  int i;

  // compute new position if moving with speed (speed_x, speed_y, speed_z)
  // for duration 'time_step'
  for (i = 0; i < MAX_COORDINATES; i++)
    node->position.c[i] += (motion->speed.c[i] * time_step);
}

// implement rectilinear motion with speed specified by (speed_x, speed_y,
// speed_z) and acceleration 'acceleration' for duration 'time_step'
void
motion_linear_acc (struct motion_class *motion,
		   struct coordinate_class acceleration,
		   struct node_class *node, double time_step)
{
  int i;

  //struct coordinate_class motion_vector;
  //double motion_magnitude;

  // compute new position if moving with speed (speed_x, speed_y, speed_z)
  // for duration 'time_step';
  // the speed was already updated by this time by the equation
  // v'=v+at, therefore the equation d=vt+(at^2)/2 becomes
  // d=v't-(at^2)/2
  for (i = 0; i < MAX_COORDINATES; i++)
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
void
motion_circular (struct motion_class *motion, struct node_class *node,
		 double time_step)
{
  double radius, delta_alpha, alpha;

  radius = sqrt (pow (node->position.c[0] - motion->center.c[0], 2) +
		 pow (node->position.c[1] - motion->center.c[1], 2));
  delta_alpha = motion->velocity * time_step / radius;

  alpha = asin ((node->position.c[1] - motion->center.c[1]) / radius);

  if ((node->position.c[0] - motion->center.c[0]) < 0 &&
      (node->position.c[1] - motion->center.c[1]) > 0)
    alpha = M_PI - alpha;
  else if ((node->position.c[0] - motion->center.c[0]) < 0
	   && (node->position.c[1] - motion->center.c[1]) < 0)
    alpha = -M_PI - alpha;

  node->position.c[0] =
    radius * cos (alpha + delta_alpha) + motion->center.c[0];
  node->position.c[1] =
    radius * sin (alpha + delta_alpha) + motion->center.c[1];
}


/////////////////////////////////////////
// Rotation motion functions
/////////////////////////////////////////

// implement rotation motion of the node for duration 'time_step' 
// (node position doesn't change, only its orientation, 
// and by consequence its antenna orientation)
void
motion_rotation (struct motion_class *motion, struct node_class *node,
		 double time_step)
{
  int interf_j;

  // for the moment by rotation motion only the orientation of
  // the antenna changes
  fprintf (stderr, "Applying rotation with angles: horiz=%f vert=%f\n",
	   motion->rotation_angle_horizontal,
	   motion->rotation_angle_vertical);

  // move all interfaces in the same time
  for (interf_j = 0; interf_j < node->interface_number; interf_j++)
    {
      // update azimuth orientation and bring the result in 
      // the interval [0,360)
      node->interfaces[interf_j].azimuth_orientation
	+= motion->rotation_angle_horizontal;
      if (node->interfaces[interf_j].azimuth_orientation >= 360)
	node->interfaces[interf_j].azimuth_orientation -= 360;

      // update elevation orientation and bring the result in 
      // the interval [0,360)
      node->interfaces[interf_j].elevation_orientation
	+= motion->rotation_angle_vertical;
      if (node->interfaces[interf_j].elevation_orientation >= 360)
	node->interfaces[interf_j].elevation_orientation -= 360;
    }
}


/////////////////////////////////////////
// Random-walk motion functions
/////////////////////////////////////////

/*
  Description from "A Survey of Mobility Models for Ad Hoc Network
  Research" by Tracy Camp, Jeff Boleng, Vanessa Davies
  http://www.cse.iitb.ac.in/~varsha/allpapers/wireless/adHocModels/mobilty_models_survey.pdf

  "In this mobility model, an MN moves from its current location to a
  new location by randomly choosing a direction and speed in which to
  travel. The new speed and direction are both chosen from pre-defined
  ranges, [speedmin, speedmax] and [0, 2*PI] respectively. Each
  movement in the Random Walk Mobility Model occurs in either a
  constant time interval t or a constant distance traveled d, at the
  end of which a new direction and speed are calculated. If an MN
  which moves according to this model reaches a simulation boundary,
  it “bounces” off the simulation border with an angle determined by
  the incoming direction. The MN then continues along this new path."

  NOTE: our implementation uses the constant motion time variant;
  moreover the time is split in discrete steps, which are the steps
  with which motion calculation is done.
 
*/

// implement random-walk motion in x0y plane with speed in range 
// [min_speed, max_speed], direction given by 'angle' for at least 
// the total duration of 'walk_time' and with the duration of this 
// step equal to 'time_step'
void
motion_random_walk (struct motion_class *motion, struct node_class *node,
		    double time_step)
{
  // check if we need to make a change in velocity/direction
  // or whether this parameters are uninitialized (2nd condition)
  if (motion->current_walk_time >= motion->walk_time ||
      motion->angle == DBL_MAX)
    {
      // reset current walk time
      motion->current_walk_time = 0;

      // generate a new velocity between min_speed and max_speed
      motion->velocity =
	rand_min_max_inclusive (motion->min_speed, motion->max_speed);

      //generate a new angle
      motion->angle = rand_min_max_inclusive (0, 2 * M_PI);

      // compute speeds in x0y plane
      motion->speed.c[0] = motion->velocity * cos (motion->angle);
      motion->speed.c[1] = motion->velocity * sin (motion->angle);

      DEBUG ("Random Walk: velocity=%f  speed_x=%f  speed_y=%f  angle=%f\n",
	     motion->velocity, motion->speed.c[0], motion->speed.c[1],
	     motion->angle);
    }

  // move node linearly
  motion_linear (motion, node, time_step);

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
int
motion_behavioral_path_following (struct motion_class *motion,
				  struct node_class *node, double time_step,
				  struct coordinate_class *a_path_following)
{
  struct coordinate_class path_vector, path_vector_scaled;
  struct coordinate_class velocity_difference;
  double path_vector_magnitude;

  // scaling constant
  double beta = 1.0;

  printf ("*** Path following computation\n");

  // initialize vectors
  coordinate_init (a_path_following, "a_path_following", 0.0, 0.0, 0.0);
  coordinate_init (&path_vector, "path_vector", 0.0, 0.0, 0.0);
  coordinate_init (&path_vector_scaled, "path_vector_scaled", 0.0, 0.0, 0.0);
  coordinate_init (&velocity_difference, "velocity_difference", 0.0, 0.0,
		   0.0);

  // compute distance to destination
  coordinate_vector_difference_2D (&path_vector,
				   &(motion->destination), &(node->position));
  coordinate_print (&path_vector);
  path_vector_magnitude = coordinate_vector_magnitude (&path_vector);
  printf ("path_vector_magnitude=%f\n", path_vector_magnitude);

  // only try to reach destination if the distance to
  // it exceeds DESTINATION_DISTANCE_THRESHOLD m
  if (path_vector_magnitude > DESTINATION_DISTANCE_THRESHOLD)
    {
#ifdef ACCELERATED_MOTION
      // scale the path vector
      coordinate_multiply_scalar (&path_vector_scaled,
				  &path_vector, V0 / path_vector_magnitude);
      coordinate_print (&path_vector_scaled);

      // compute velocity difference
      coordinate_vector_difference (&velocity_difference,
				    &path_vector_scaled, &(motion->speed));
      coordinate_print (&velocity_difference);

      // scale velocity difference by 'beta' and assign it to result
      coordinate_multiply_scalar (a_path_following,
				  &velocity_difference, beta);
      coordinate_print (a_path_following);
#else
      // scale the path vector
      coordinate_multiply_scalar (&path_vector_scaled,
				  &path_vector, V0 / path_vector_magnitude);
      coordinate_print (&path_vector_scaled);

      // scale path vector again by 'beta' and assign it to result
      coordinate_multiply_scalar (a_path_following,
				  &path_vector_scaled, beta);
      coordinate_print (a_path_following);
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
double
compute_detour_distance (struct object_class *object, int vertex_index,
			 struct coordinate_class destination,
			 int go_clockwise)
{
  int counter, vertex_i, vertex_i2;
  double detour_distance;

  DEBUG ("Compute detour distance");

  // initialize distance
  detour_distance = 0;

  // initialize loop counter
  counter = 0;

  // initialize start vertex
  vertex_i = vertex_index;

  // we consider height is not important, 
  // and all objects must be avoided
  while (counter < object->vertex_number)
    {
      if (go_clockwise == TRUE)
	vertex_i2 =
	  ((vertex_i + 1) < object->vertex_number) ? vertex_i + 1 : 0;
      else
	vertex_i2 =
	  ((vertex_i - 1) >= 0) ? vertex_i - 1 : (object->vertex_number - 1);

      DEBUG ("vertex_i=%d vertex_i2=%d (go_clockwise=%d counter=%d); vertex=",
	     vertex_i, vertex_i2, go_clockwise, counter);
      coordinate_print (&object->vertices[vertex_i]);

      // check if segment made of vertex and destination is in object
      // (if not, the destination is visible)
      if (object_intersect (&(object->vertices[vertex_i]),
			    &destination, object, FALSE, FALSE) == TRUE)
	{
	  DEBUG ("vertex_i2=%d (go_clockwise=%d)", vertex_i2, go_clockwise);

	  detour_distance +=
	    coordinate_distance (&object->vertices[vertex_i],
				 &object->vertices[vertex_i2]);
	  printf ("detour_distance=%f (partial result)\n", detour_distance);
	}
      // destination is visible
      else
	{
	  detour_distance +=
	    coordinate_distance (&(object->vertices[vertex_i]), &destination);
	  DEBUG ("vertex=");
	  coordinate_print (&(object->vertices[vertex_i]));
	  printf ("---- return detour_distance=%f \
(go_clockwise=%d  start vertex=%d  vertex from which object is visible=%d)\n", detour_distance, go_clockwise, vertex_index, vertex_i);

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
int
motion_behavioral_object_avoidance (struct motion_class *motion,
				    struct scenario_class *scenario,
				    struct node_class *node,
				    double time_step,
				    struct coordinate_class *a_wall_rejection,
				    struct coordinate_class
				    *a_obstacle_avoidance)
{
  double gamma = 1.0, gamma2 = 1.0;
  struct coordinate_class intersection;	//, min_intersection;
  double min_distance, distance;
  double min_distance2;

  struct coordinate_class min_vertex;	// used in object avoidance

  struct coordinate_class min_distance_point;

  struct coordinate_class acceleration, acceleration_s;

  int object_i, vertex_i, vertex_i2;

  int do_object_avoidance;
  int avoid_corner;		//, corner_is_edge_start;

  struct object_class *object;

  int last_vertex;

  //int min_dist_vertex;
  int min_dist_vertex_motion_sense = MOTION_UNDECIDED;
  int vertex_motion_sense;


  //////////////////////////////////////////////
  // Object avoidance/rejection computation

  printf ("*** Object avoidance/rejection computation\n");

  // init vectors
  coordinate_init (a_wall_rejection, "a_wall_rejection", 0.0, 0.0, 0.0);
  coordinate_init (&min_distance_point, "min_distance_point", 0.0, 0.0, 0.0);
  coordinate_init (&acceleration, "acceleration", 0.0, 0.0, 0.0);

  // test if any object prevent the node to reach the destination
  for (object_i = 0; object_i < scenario->object_number; object_i++)
    {
      // initialize object pointer
      object = &(scenario->objects[object_i]);
      object_print (object);

      min_distance = DBL_MAX;
      avoid_corner = FALSE;
      do_object_avoidance = FALSE;

      // we consider height is not important, and all objects must be avoided

      // compute distance to all object edges
      last_vertex = (object->make_polygon == TRUE) ? object->vertex_number :
	object->vertex_number - 1;
      for (vertex_i = 0; vertex_i < last_vertex; vertex_i++)
	{
	  vertex_i2 =
	    ((vertex_i + 1) < object->vertex_number) ? vertex_i + 1 : 0;

	  // check distance wrt each edge
	  if (coordinate_distance_to_segment_2D (&(node->position),
						 &(object->vertices
						   [vertex_i]),
						 &(object->vertices
						   [vertex_i2]), &distance,
						 &intersection) == TRUE)
	    {
	      // distance could be calculated => proceed
	      if (min_distance > distance)
		{
		  min_distance = distance;
		  coordinate_copy (&min_distance_point, &intersection);
		  printf ("1. current min_distance=%f min_distance_point=\n",
			  min_distance);
		  coordinate_print (&min_distance_point);
		  //min_dist_vertex=vertex_i;
		}
	    }
	}

      last_vertex = (object->make_polygon == TRUE) ? object->vertex_number :
	object->vertex_number - 1;

      // we consider height is not important, 
      // and all objects must be avoided
      for (vertex_i = 0; vertex_i < last_vertex; vertex_i++)
	{
	  distance = coordinate_distance_2D (&(node->position),
					     &(object->vertices[vertex_i]));

	  // distance could be calculated => proceed
	  if (min_distance > distance)
	    {
	      min_distance = distance;
	      coordinate_copy (&min_distance_point,
			       &(object->vertices[vertex_i]));
	      printf ("2. current min_distance=%f  min_distance_point=\n",
		      min_distance);
	      coordinate_print (&min_distance_point);
	    }
	}

      if (min_distance != DBL_MAX)
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

	  if (min_distance < OBJECT_AVOIDANCE_THRESHOLD)
	    {
	      do_object_avoidance = TRUE;

	      if (min_distance < WALL_REJECTION_THRESHOLD)
		{
		  if (avoid_corner == FALSE)
		    {
		      // use equation of rule 2
		      coordinate_vector_difference_2D (&acceleration,
						       &(node->position),
						       &min_distance_point);
		      DEBUG ("node->position=");
		      coordinate_print (&(node->position));
		      DEBUG ("min_distance_point=");
		      coordinate_print (&min_distance_point);
		      DEBUG ("acceleration=");
		      coordinate_print (&acceleration);

		      // change wrt to paper => use a force inverse 
		      // proportional with distance square (increasing 
		      // as object get closer to wall)
		      if (min_distance == 0)
			min_distance = 0.001;
		      coordinate_init
			(&acceleration_s, "acceleration_s", 0, 0, 0);
		      coordinate_multiply_scalar
			(&acceleration_s, &acceleration,
			 gamma / min_distance / min_distance);
		      coordinate_print (&acceleration_s);

		      coordinate_vector_sum (a_wall_rejection,
					     a_wall_rejection,
					     &acceleration_s);
		      coordinate_print (a_wall_rejection);
		    }
		  else		// this is not used for the moment
		    {
		      INFO ("SPECIAL CORNER AVOIDANCE");

		      // use a random small acceleration to avoid corners
		      coordinate_init (&acceleration_s, "acceleration_s",
				       rand_min_max (-0.1, 0.1),
				       rand_min_max (-0.1, 0.1), 0);

		      coordinate_vector_sum (a_wall_rejection,
					     a_wall_rejection,
					     &acceleration_s);
		      DEBUG ("a_wall_rejection=");
		      coordinate_print (a_wall_rejection);
		    }
		}
	    }
	  else
	    do_object_avoidance = FALSE;
	}
      else
	{
	  WARNING ("Error computing minimum distance");
	  return FALSE;
	}


      /////////////////////////////////////////////////
      // object avoidance computation      
      if (do_object_avoidance == TRUE)
	{
	  double dist_clockwise, dist_counter_clockwise, dist;

	  printf ("*** Object avoidance computation\n");

	  // if wall rejection was enforced, we may need to compute how to 
	  // avoid the object by going around it (Rule 3: obstacle avoidance)

	  // first we need to check if the obstacle is preventing us from
	  // reaching the destination (height is not considered)
	  if (object_intersect (&(node->position), &(motion->destination),
				object, TRUE, FALSE) == FALSE)
	    {
	      // the obstacle doesn't prevent us from reaching the destination
	      // therefore it doesn't have to be avoided; we can skip the
	      // avoidance algorithm
	      DEBUG ("Object doesn't prevent reaching destination");
	      continue;		//skip to next object
	    }
	  else
	    DEBUG ("Object prevents reaching destination. \
Computing avoidance...");

	  min_distance2 = DBL_MAX;

	  last_vertex =
	    (object->make_polygon ==
	     TRUE) ? object->vertex_number : object->vertex_number - 1;

	  for (vertex_i = 0; vertex_i < last_vertex; vertex_i++)
	    {
	      DEBUG ("------- check if vertex %d is visible", vertex_i);
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
	      if (object_intersect (&(node->position),
				    &(object->vertices[vertex_i]),
				    object, FALSE, FALSE) == TRUE)
		{
		  DEBUG ("------- skipping vertex %d because invisible",
			 vertex_i);
		  continue;	//skip to next vertex
		}

	      DEBUG ("---- found candidate=");
	      coordinate_print (&(object->vertices[vertex_i]));

	      // to compute the detour distance we need to compute the sum of 
	      // object edges up to last vertex from where the destination 
	      // is visible; this is done both clockwise and counter-clockwise
	      dist_clockwise =
		compute_detour_distance (object, vertex_i,
					 motion->destination, TRUE);

	      dist_counter_clockwise =
		compute_detour_distance (object, vertex_i,
					 motion->destination, FALSE);
	      /*              
	         if(motion->motion_sense == MOTION_UNDECIDED)
	         {
	       */
	      dist = coordinate_distance (&(node->position),
					  &(object->vertices[vertex_i])) +
		((dist_clockwise < dist_counter_clockwise) ? dist_clockwise :
		 dist_counter_clockwise);

	      vertex_motion_sense =
		(dist_clockwise < dist_counter_clockwise) ?
		MOTION_CLOCKWISE : MOTION_COUNTERWISE;
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

	      printf ("---- possible detour for object='%s' => \
vertex_i=%d dist=%f dist_p_v=%f dist_c=%f dist_cc=%f\n", object->name, vertex_i, dist, coordinate_distance (&(node->position), &(object->vertices[vertex_i])), dist_clockwise, dist_counter_clockwise);

	      if (min_distance2 > dist)
		{
		  min_distance2 = dist;
		  coordinate_copy (&min_vertex,
				   &(object->vertices[vertex_i]));
		  printf ("min_distance2=%f  min_vertex=\n", min_distance2);
		  coordinate_print (&min_vertex);
		  min_dist_vertex_motion_sense = vertex_motion_sense;
		}
	    }

	  // check a distance could be calculated and the object is not too
	  // far away
	  if (min_distance2 != DBL_MAX)
	    {
	      printf ("---- detour for object='%s' => dist=%f vertex=\n",
		      object->name, min_distance2);
	      coordinate_print (&min_vertex);

	      if (motion->motion_sense == MOTION_UNDECIDED)
		motion->motion_sense = min_dist_vertex_motion_sense;

	      // use equation of rule 3
	      coordinate_vector_difference_2D (&acceleration, &min_vertex,
					       &(node->position));
	      DEBUG ("acceleration=");
	      coordinate_print (&acceleration);

	      coordinate_multiply_scalar
		(&acceleration_s, &acceleration,
		 gamma2 / coordinate_vector_magnitude (&acceleration));
	      DEBUG ("acceleration_s=");
	      coordinate_print (&acceleration_s);

	      coordinate_vector_sum (a_obstacle_avoidance,
				     a_obstacle_avoidance, &acceleration_s);
	      DEBUG ("a_obstacle_avoidance=");
	      coordinate_print (a_obstacle_avoidance);
	    }
	  else
	    {
	      WARNING ("Error computing detour distance");
	      return FALSE;
	    }
	}
    }

  return TRUE;
}

void
motion_behavioral_mutual_avoidance (struct motion_class *motion,
				    struct scenario_class *scenario,
				    struct node_class *node,
				    struct coordinate_class
				    *a_mutual_avoidance)
{
  // scaling constant
  double delta = 1.0;

  int node_i;
  struct node_class *node2;
  struct motion_class *motion2;

  struct coordinate_class local_avoidance;
  struct coordinate_class unit_vector;

  double distance;
  double magnitude;
  double angle, angle2, angle_difference;

  printf ("*** Mutual avoidance computation\n");

  // initialize vectors
  coordinate_init (a_mutual_avoidance, "a_mutual_avoidance", 0.0, 0.0, 0.0);
  coordinate_init (&unit_vector, "unit_vector", 0.0, 0.0, 0.0);

  // test if any node is on the path of the current node
  for (node_i = 0; node_i < scenario->node_number; node_i++)
    {
      // initialize object pointer
      node2 = &(scenario->nodes[node_i]);
      if (node2->motion_index != INVALID_INDEX)
	motion2 = &(scenario->motions[node2->motion_index]);
      else
	motion2 = NULL;

      node_print (node2);

      // skip the current node itself
      if (node2->id == node->id)
	continue;

      distance = coordinate_distance (&(node2->position), &(node->position));
      printf ("distance=%f\n", distance);

      // avoid the case when the nodes are overlapped 
      if (distance < EPSILON)
	continue;

      if (distance < MUTUAL_AVOIDANCE_THRESHOLD)
	{
	  magnitude = coordinate_vector_magnitude (&(motion->speed));

	  // if magnitude is zero, don't do avoidance
	  if (magnitude > EPSILON)
	    {
	      angle = coordinate_vector_angle_2D (&(motion->speed));

	      if (motion2 != NULL)
		angle2 = coordinate_vector_angle_2D (&(motion2->speed));
	      else
		angle2 = 0;

	      printf
		("BEHAVIORAL: motion_id=%d angle=%f motion2->id=%d angle2=%f\n",
		 motion->id, angle, (motion2 != NULL) ? motion2->id : -1,
		 angle2);

	      // compute unit vector perpendicular on velocity 
	      if (angle < angle2)
		// vector is rotated clockwise
		coordinate_init (&unit_vector, "unit_vector",
				 motion->speed.c[1], -motion->speed.c[0],
				 motion->speed.c[2]);
	      else
		// vector is rotated counter-clockwise
		coordinate_init (&unit_vector, "unit_vector",
				 -motion->speed.c[1], motion->speed.c[0],
				 motion->speed.c[2]);

	      // normalize unit vector
	      coordinate_multiply_scalar (&unit_vector, &unit_vector,
					  delta / magnitude);

	      // multiply by factor that depends on angle difference
	      angle_difference = fabs (angle - angle2);
	      if (angle_difference > M_PI)
		angle_difference = 2 * M_PI - angle_difference;
	      coordinate_multiply_scalar (&unit_vector, &unit_vector,
					  angle_difference);

	      // multiply by the inverse of distance
	      if (distance < 0.1)
		distance = 0.1;
	      coordinate_multiply_scalar (&unit_vector, &unit_vector,
					  1 / distance);

	    }
	  coordinate_copy (&local_avoidance, &unit_vector);
	  printf ("*** Mutual avoidance activated:\n");
	  coordinate_print (&local_avoidance);

	  coordinate_vector_sum (a_mutual_avoidance, a_mutual_avoidance,
				 &local_avoidance);
	}
    }
}


int
motion_behavioral (struct motion_class *motion,
		   struct scenario_class *scenario, struct node_class *node,
		   double time_step)
{
  struct coordinate_class acceleration;

#ifdef ACCELERATED_MOTION
  struct coordinate_class acceleration_t;
#endif

  struct coordinate_class a_path_following, a_wall_rejection,
    a_obstacle_avoidance, a_mutual_avoidance;

  struct coordinate_class new_speed;

  double acceleration_magnitude, speed_magnitude;

  // NOTE 1: The page indications below refer to the paper
  //         "Reconsidering Microscopic Mobility Modeling for
  //          Self-Organizing Networks" by F. Legendre et al.

  // NOTE 2: everything is done considering the case of 2D

  coordinate_init (&acceleration, "acceleration", 0.0, 0.0, 0.0);

  coordinate_init (&a_path_following, "a_path_following", 0.0, 0.0, 0.0);
  coordinate_init (&a_wall_rejection, "a_wall_rejection", 0.0, 0.0, 0.0);
  coordinate_init (&a_obstacle_avoidance, "a_obstacle_avoidance",
		   0.0, 0.0, 0.0);
  coordinate_init (&a_mutual_avoidance, "a_mutual_avoidance", 0.0, 0.0, 0.0);

  // compute path following acceleration; if destination was reached
  // (return value is FALSE) then skip calculations; else compute 
  // accelerations and move object
  if (motion_behavioral_path_following (motion, node,
					time_step, &a_path_following) == TRUE)
    {
      // compute object rejection and avoidance acceleration
      motion_behavioral_object_avoidance (motion, scenario, node, time_step,
					  &a_wall_rejection,
					  &a_obstacle_avoidance);

      // compute mutual avoidance acceleration
      motion_behavioral_mutual_avoidance (motion, scenario, node,
					  &a_mutual_avoidance);

      printf ("*** Acceleration sum computation\n");

      // if wall rejection or obstacle avoidance forces 
      // are enforced, use them exclusively
      if (coordinate_vector_magnitude (&a_wall_rejection) > EPSILON ||
	  coordinate_vector_magnitude (&a_obstacle_avoidance))
	coordinate_init (&a_path_following, "a_path_following", 0.0, 0.0,
			 0.0);

      /*
         if(coordinate_magnitude(a_obstacle_avoidance)>EPSILON)
         coordinate_init(&a_wall_rejection, 0.0, 0.0, 0.0);
       */

      // sum all acceleration components
      DEBUG ("Adding a_path_following and a_wall_rejection:");
      coordinate_print (&a_path_following);
      coordinate_print (&a_wall_rejection);
      coordinate_vector_sum (&acceleration,
			     &a_path_following, &a_wall_rejection);
      DEBUG ("1. result acceleration=");
      coordinate_print (&acceleration);

      DEBUG ("Adding acceleration and a_obstacle_avoidance:");
      coordinate_print (&acceleration);
      coordinate_print (&a_obstacle_avoidance);
      coordinate_vector_sum (&acceleration,
			     &acceleration, &a_obstacle_avoidance);
      DEBUG ("2. result acceleration=");
      coordinate_print (&acceleration);

      DEBUG ("Adding acceleration and a_mutual_avoidance:");
      coordinate_print (&acceleration);
      coordinate_print (&a_mutual_avoidance);
      coordinate_vector_sum (&acceleration, &acceleration,
			     &a_mutual_avoidance);
      DEBUG ("3. result acceleration=");
      coordinate_print (&acceleration);


      // check that acceleration values don't exceed certain limits
      acceleration_magnitude = coordinate_vector_magnitude (&acceleration);

#ifdef ACCELERATED_MOTION

      if (acceleration_magnitude > MAX_ACC_MAGN)
	{
	  //fprintf(stderr, "Normalized acceleration from %f to %f\n",
	  //  acceleration_magnitude, MAX_ACC_MAGN);
	  coordinate_multiply_scalar (&acceleration, &acceleration,
				      MAX_ACC_MAGN / acceleration_magnitude);
	}

      // To calculate the new speed use formula 
      //        v(t+delta_t) = v(t) + a*delta_t
      // on page 7 of "Modeling Mobility with Behavioral Rules:
      // the Case of Incident and Emergency Situations" by
      // F. Legendre et al.

      // compute acc * delta_t <=> change in speed over time
      coordinate_multiply_scalar (&acceleration_t, &acceleration, time_step);
      DEBUG ("acceleration_t=");
      coordinate_print (&acceleration_t);

      DEBUG ("motion_speed=");
      coordinate_print (&(motion->speed));
      // compute the new speed 
      coordinate_vector_sum (&new_speed, &(motion->speed), &acceleration_t);
      DEBUG ("new_speed (accelerated motion)=");
      coordinate_print (&new_speed);

      speed_magnitude = coordinate_vector_magnitude (&new_speed);
      if (speed_magnitude > MAX_SPEED_MAGN)
	{
	  //fprintf(stderr, "Normalized speed from %f to %f\n",
	  //  speed_magnitude, MAX_SPEED_MAGN);
	  coordinate_multiply_scalar (&new_speed, &new_speed,
				      MAX_SPEED_MAGN / speed_magnitude);
	  // in this case acceleration must be set to 0, since the motion
	  // should be seen as a uniform motion
	  coordinate_init (&acceleration, "acceleration", 0.0, 0.0, 0.0);
	}

#else // do not use accelerated motion but uniform!!!!!!!!!!!!!!!!

      // normalize acceleration to get just its direction
      coordinate_multiply_scalar (&acceleration, &acceleration,
				  1 / acceleration_magnitude);

      // new speed is obtained by multiplying the normalized acceleration 
      // by a constant magnitude (e.g., 0.5 => 0.5 m/s)
      coordinate_multiply_scalar (&new_speed, &acceleration, V0);
      DEBUG ("new_speed (non-accelerated motion)=");
      coordinate_print (&new_speed);
#endif
      /*      
         // check that the node has not reached a dead point
         if(coordinate_magnitude(new_speed)<0.1)
         {
         // use some random values to get out of dead point
         motion->speed_x = rand_min_max (MIN_RAND_SPEED, MAX_RAND_SPEED);
         motion->speed_y = rand_min_max (MIN_RAND_SPEED MAX_RAND_SPEED);
         motion->speed_z = 0.0; // 2D assumption
         }
         else
         {
       */

      // update motion parameters
      coordinate_copy (&(motion->speed), &new_speed);
      //}

#ifdef ACCELERATED_MOTION
      // move node linearly with computed speed and acceleration
      motion_linear_acc (motion, acceleration, node, time_step);
#else
      // move node linearly with computed speed
      motion_linear (motion, node, time_step);
#endif

    }
  else				// stop object
    {
      INFO ("Node '%s' reached destination %f s ago", node->name,
	    motion->time_since_destination_reached);

      // update motion parameters
      coordinate_init (&(motion->speed), "speed", 0.0, 0.0, 0.0);
      coordinate_init (&acceleration, "acceleration", 0.0, 0.0, 0.0);

      motion->time_since_destination_reached += time_step;


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

void
motion_trace (struct motion_class *motion,
	      struct scenario_class *scenario, struct node_class *node,
	      double motion_current_time, double time_step)
{
  DEBUG ("Motion current time = %f", motion_current_time);

  if (motion_current_time
      >= motion->trace_records[motion->trace_record_crt].time)
    if (motion->trace_record_crt < (motion->trace_record_number - 1))
      {
	DEBUG ("Current motion time '%f' >= current trace record time '%f' => \
increment record index", motion_current_time,
	       motion->trace_records[motion->trace_record_crt].time);
	motion->trace_record_crt++;
      }
    else
      {
	DEBUG ("Last trace record (#%d) reached. Doing nothing.",
	       motion->trace_record_crt);
	return;
      }
  else
    DEBUG ("Current motion time '%f' < current trace record time '%f'",
	   motion_current_time,
	   motion->trace_records[motion->trace_record_crt].time);

  DEBUG ("Extrapolating from current position (%f,%f,%f) to (%f,%f,%f)...",
	 node->position.c[0], node->position.c[1], node->position.c[2],
	 motion->trace_records[motion->trace_record_crt].coord.c[0],
	 motion->trace_records[motion->trace_record_crt].coord.c[1],
	 motion->trace_records[motion->trace_record_crt].coord.c[2]);

  motion->speed.c[0]
    = (motion->trace_records[motion->trace_record_crt].coord.c[0]
       - node->position.c[0])
    / (motion->trace_records[motion->trace_record_crt].time -
       motion_current_time);
  motion->speed.c[1]
    = (motion->trace_records[motion->trace_record_crt].coord.c[1]
       - node->position.c[1])
    / (motion->trace_records[motion->trace_record_crt].time -
       motion_current_time);
  motion->speed.c[2]
    = (motion->trace_records[motion->trace_record_crt].coord.c[2]
       - node->position.c[2])
    / (motion->trace_records[motion->trace_record_crt].time -
       motion_current_time);

  DEBUG ("Using linear motion with speed (%f,%f,%f)", motion->speed.c[0],
	 motion->speed.c[1], motion->speed.c[2]);

  motion_linear (motion, node, time_step);
}

// apply the motion to a node from scenario for the specified duration
// return SUCCESS on succes, ERROR on error
int
motion_apply (struct motion_class *motion, struct scenario_class *scenario,
	      double motion_current_time, double time_step)
{
  if (motion->node_index == INVALID_INDEX)	// node could not be found
    {
      WARNING ("Motion specifies an unexisting node ('%s')",
	       motion->node_name);
      return ERROR;
    }
  else
    {
      struct node_class *node = &(scenario->nodes[motion->node_index]);

      // check whether speed needs to be initialized based on destination
      if (motion->type == LINEAR_MOTION && motion->speed_from_destination)
	{
	  struct coordinate_class speed;

	  coordinate_vector_difference (&speed, &(motion->destination),
					&(node->position));

	  DEBUG ("Calculating speed from distance to destination");
	  coordinate_multiply_scalar (&speed, &speed,
				      1 / (motion->stop_time -
					   motion->start_time));

	  coordinate_copy (&(motion->speed), &speed);
	  motion->speed_from_destination = FALSE;
	}

#ifdef MESSAGE_DEBUG
      motion_print (motion);
#endif

      // move node according to the motion type field
      if (motion->type == LINEAR_MOTION)
	motion_linear (motion, node, time_step);
      else if (motion->type == CIRCULAR_MOTION)
	motion_circular (motion, node, time_step);
      else if (motion->type == ROTATION_MOTION)
	motion_rotation (motion, node, time_step);
      else if (motion->type == RANDOM_WALK_MOTION)
	motion_random_walk (motion, node, time_step);
      else if (motion->type == BEHAVIORAL_MOTION)
	motion_behavioral (motion, scenario, node, time_step);
      else if (motion->type == QUALNET_MOTION)
	motion_trace (motion, scenario, node, motion_current_time, time_step);
      else
	{
	  WARNING ("Undefined motion type (%d)", motion->type);
	  return ERROR;
	}

#ifdef MESSAGE_INFO
      INFO ("Moved node position updated:");
      node_print (node);
#endif

    }

  return SUCCESS;
}

void
trace_record_copy (struct trace_record_class *dst,
		   struct trace_record_class *src)
{
  dst->time = src->time;
  coordinate_copy (&(dst->coord), &(src->coord));
  dst->azimuth_orientation = src->azimuth_orientation;
  dst->elevation_orientation = src->elevation_orientation;
}

// compute the relative velocity (absolute value) between two nodes
double
motion_relative_velocity (struct scenario_class *scenario,
			  struct node_class *node1, struct node_class *node2)
{
  struct motion_class *motion1;
  struct motion_class *motion2;

  double relative_velocity = -1.0;

  if (node1->motion_index == INVALID_INDEX)
    motion1 = NULL;
  else
    motion1 = &(scenario->motions[node1->motion_index]);

  if (node2->motion_index == INVALID_INDEX)
    motion2 = NULL;
  else
    motion2 = &(scenario->motions[node2->motion_index]);

  // check whether motion is defined for node 1
  if (motion1 != NULL)
    {
      // defined => check whether it is a not-supported motion type
      if ((motion1->type == CIRCULAR_MOTION)
	  || (motion1->type == ROTATION_MOTION))
	{
	  WARNING ("Motion type '%s' is not supported for relative velocity \
calculation", motion_types[motion1->type]);
	  relative_velocity = -1.0;
	}
      // supported => check node 2
      else
	{
	  // check whether motion is defined for node 2
	  if (motion2 != NULL)
	    {
	      // defined => check if it is a not-supported motion type
	      if ((motion2->type == CIRCULAR_MOTION)
		  || (motion2->type == ROTATION_MOTION))
		{
		  WARNING ("Motion type '%s' is not supported for relative \
velocity calculation", motion_types[motion2->type]);
		  relative_velocity = -1.0;
		}
	      // supported, both nodes move => must calculate the 
	      // relative velocity by subtracting the speed vectors
	      else
		{
		  struct coordinate_class difference;
		  difference.c[0] = motion1->speed.c[0] - motion2->speed.c[0];
		  difference.c[1] = motion1->speed.c[1] - motion2->speed.c[1];
		  difference.c[2] = motion1->speed.c[2] - motion2->speed.c[2];
		  relative_velocity
		    = coordinate_vector_magnitude (&(difference));
		}
	    }
	  // node 2 is static => only node 1 motion matters
	  else
	    relative_velocity =
	      coordinate_vector_magnitude (&(motion1->speed));
	}
    }

  // node 1 is static => check node 2
  else if (motion2 != NULL)
    {
      // check whether it is a non-supported motion type
      if ((motion2->type == CIRCULAR_MOTION)
	  || (motion2->type == ROTATION_MOTION))
	{
	  WARNING ("Motion type '%s' is not supported for relative \
velocity calculation", motion_types[motion2->type]);
	  relative_velocity = -1.0;
	}
      // supported, node 1 is static => only node 2 motion matters
      else
	relative_velocity = coordinate_vector_magnitude (&(motion2->speed));
    }
  else
    // both nodes are static => relative_velocity is 0
    relative_velocity = 0.0;

  return relative_velocity;
}
