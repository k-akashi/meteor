
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
 * File name: environment.h
 * Function:  Header file of environment.c
 *
 * Author: Razvan Beuran
 *
 * $Id: environment.h 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/


#ifndef __ENVIRONMENT_H
#define __ENVIRONMENT_H

#include <stdint.h>

#include "global.h"
#include "generic.h"
#include "connection.h"


// maximum number of segments in a dynamic environment
#define MAX_SEGMENTS            100

#define AWGN_FADING             0
#define RAYLEIGH_FADING         1

////////////////////////////////////////////////
// Environment structure definition
////////////////////////////////////////////////

struct environment_class
{
  // environment name and type
  char name[MAX_STRING];
  uint32_t name_hash;

  char type[MAX_STRING];	// indoor/outdoor; not used yet

  // flag to show whether this environments uses static
  // (pre-defined) values or should be computed dynamically
  // set to TRUE for dynamic computation, to FALSE otherwise
  int is_dynamic;

  // number of segments
  int num_segments;

  // log-distance path-loss model parameters (per segment)
  double alpha[MAX_SEGMENTS];
  double sigma[MAX_SEGMENTS];
  double W[MAX_SEGMENTS];

  // noise power in each segment 
  double noise_power[MAX_SEGMENTS];

  // length of each segment
  double length[MAX_SEGMENTS];

  // type of fading
  int fading;
};


/////////////////////////////////////////
// Environment structure functions
/////////////////////////////////////////

// init an environment
void environment_init (struct environment_class *environment, char *name,
		       char *type, double alpha, double sigma, double W,
		       double noise_power, double x1, double y1, double x2,
		       double y2);

// init an environment (dynamic version)
void environment_dynamic_init (struct environment_class *environment,
			       char *name, char *type, int num_segments,
			       double alpha[], double sigma[], double W[],
			       double noise_power[], double distance[]);

// print the fields of an environment
void environment_print (struct environment_class *environment);


// copy the information in environment_src to environment_dst
void environment_copy (struct environment_class *environment_dst,
		       struct environment_class *environment_src);

// 2D segment intersection;
// return TRUE if the segments [(xn1,yn1), (xn2,yn2)]
// and [(xo1,yo1), (xo2,yo2)] intersect; 
// if they interesect, also return the coordinates of interesection in 
// x_intersect and y_intersect
int segment_intersect (double xn1, double yn1, double xn2, double yn2,
		       double xo1, double yo1, double xo2, double yo2,
		       double *x_intersect, double *y_intersect);

// check whether a point is in a 2D object;
// return TRUE if point (xn,yn) is in object (xo1,yo1)x(xo2,yo2)
int point_in_object (double xn, double yn, double xo1, double yo1,
		     double xo2, double yo2);

// check whether a point is on the edges of a 2D object;
// return TRUE if point (xn,yn) is on at least one edge of 'object'
int point_on_object_edge2d (double xn, double yn,
			    struct object_class *object);

// check whether a point is in a 3D object;
// return TRUE if point (xn,yn) is in object (xo1,yo1)x(xo2,yo2)
int point_in_object3d (double xn, double yn, double zn,
		       struct object_class *object);

// check wether an internal point is on a 2D object edge;
// return TRUE if point (xn,yn) is one one of the 
// edges of object (xo1,yo1)x(xo2,yo2)
int internal_point_on_object_edge (double xn, double yn, double xo1,
				   double yo1, double xo2, double yo2);

// check whether a segment is included in a 2D object;
// return TRUE if segment (xn1,yn1)<->(xn2,yn2) is 
// included in object (xo1,yo1)x(xo2,yo2)
int segment_in_object (double xn1, double yn1, double xn2, double yn2,
		       double xo1, double yo1, double xo2, double yo2);

// check whether a segment is included in a 3D object;
// return TRUE if segment (xn1,yn1,zn1)<->(xn2,yn2,zn2) is 
// included in object (xo1,yo1)x(xo2,yo2)
int segment_in_object3d (double xn1, double yn1, double zn1,
			 double xn2, double yn2, double zn2,
			 struct object_class *object);

// print the contents of an array of intersection points
// or "EMPTY" if no elements are found
void intersection_print (double *intersections_x, double *intersections_y,
			 int number);

// effectively insert the point (new_x, new_y) at position 'index'
// in the interesection points arrays intersections_x and 
// intersections_y (of length 'number');
// return SUCCESS on success, ERROR on error
int intersection_do_insert (double *intersections_x, double *intersections_y,
			    int *number, double new_x, double new_y,
			    int index);

// try to insert the point (new_x, new_y) in the interesection 
// points arrays intersections_x and intersections_y (of length 'number');
// return SUCCESS on success, ERROR on error
int intersection_add_point (double *intersections_x, double *intersections_y,
			    int *number, double new_x, double new_y);

// check whether the point (x,y) is on the segment (sx1,sx2)<->(sx2,sy2)
// return TRUE on if the point is on the segment, FALSE otherwise
int point_on_segment (double x, double y, double sx1, double sy1,
		      double sx2, double sy2);

// update the properties of an environment that is attached to a certain 
// connection, depending on the properties of the scenario
// (position of end-nodes of connection, obstacles, etc.);
// return SUCCESS on succes, ERROR on error
int environment_update (struct environment_class *environment,
			struct connection_class *connection,
			struct scenario_class *scenario);

// check whether a newly defined environment conflicts with existing ones;
// return TRUE if no duplicate environment is found, FALSE otherwise
int environment_check_valid (struct environment_class *environments,
			     int environment_number,
			     struct environment_class *new_environment);

/////////////////////////////////////////////
// Doppler shift
/////////////////////////////////////////////

// compute the SNR decrease due to Doppler shift based on Eq. 13.34 in
// [OFDMA System Analysis pp. 254-258]; we compute the absolute value
// of the change, and since it is a decrease, it should be subtracted from SNR;
// returns the absolute value of the change in SNR
double doppler_snr (double frequency, double subcarrier_spacing,
		    double relative_velocity, double SNR);

/////////////////////////////////////////////
// Rayleigh fading
/////////////////////////////////////////////

// compute FER using a model that integrates FER thresholds and fading models
double environment_fading (struct environment_class *environment,
			   double Pr_threshold_ber, int frame_size,
			   double rx_sensitivity, double Pr);

#endif
