
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
 * File name: environment.h
 * Function:  Header file of environment.c
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#ifndef __ENVIRONMENT_H
#define __ENVIRONMENT_H

#include <stdint.h>

#include "global.h"
#include "generic.h"


// maximum number of segments in a dynamic environment
#define MAX_SEGMENTS                    100


////////////////////////////////////////////////
// Environment structure definition
////////////////////////////////////////////////

struct environment_class_s
{
  // environment name and type
  char name[MAX_STRING];
  uint32_t name_hash;

  char type[MAX_STRING];// indoor/outdoor; not used yet

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

};


/////////////////////////////////////////
// Environment structure functions
/////////////////////////////////////////

// init an environment
void environment_init(environment_class *environment, char *name, char *type, 
		      double alpha, double sigma, double W, 
		      double noise_power,
		      double x1, double y1, double x2, double y2);

// init an environment (dynamic version)
void environment_dynamic_init(environment_class *environment, char *name, 
			      char *type, int num_segments, double alpha[], 
			      double sigma[], double W[], double noise_power[],
			      double distance[]);

// print the fields of an environment
void environment_print(environment_class *environment);


// copy the information in environment_src to environment_dest
void environment_copy(environment_class *environment_dest, 
		      environment_class * environment_src);

// 2D segment intersection;
// return TRUE if the segments [(xn1,yn1), (xn2,yn2)]
// and [(xo1,yo1), (xo2,yo2)] intersect; 
// if they interesect, also return the coordinates of interesection in 
// x_intersect and y_intersect
int segment_intersect(double xn1, double yn1, double xn2, double yn2,
		      double xo1, double yo1, double xo2, double yo2,
		      double *x_intersect, double *y_intersect);

// check whether a point is in a 2D object;
// return TRUE if point (xn,yn) is in object (xo1,yo1)x(xo2,yo2)
int point_in_object(double xn, double yn, double xo1, double yo1, 
		    double xo2, double yo2);

// check whether a point is in a 3D object;
// return TRUE if point (xn,yn) is in object (xo1,yo1)x(xo2,yo2)
int point_in_object3d(double xn, double yn, double zn, object_class *object);

// check wether an internal point is on a 2D object edge;
// return TRUE if point (xn,yn) is one one of the 
// edges of object (xo1,yo1)x(xo2,yo2)
int internal_point_on_object_edge(double xn, double yn, double xo1, 
			 double yo1, double xo2, double yo2);

// check whether a segment is included in a 2D object;
// return TRUE if segment (xn1,yn1)<->(xn2,yn2) is 
// included in object (xo1,yo1)x(xo2,yo2)
int segment_in_object(double xn1, double yn1, double xn2, double yn2,
		      double xo1, double yo1, double xo2, double yo2);

// check whether a segment is included in a 3D object;
// return TRUE if segment (xn1,yn1,zn1)<->(xn2,yn2,zn2) is 
// included in object (xo1,yo1)x(xo2,yo2)
int segment_in_object3d(double xn1, double yn1, double zn1, 
			double xn2, double yn2, double zn2,
			object_class *object);

// print the contents of an array of intersection points
// or "EMPTY" if no elements are found
void intersection_print(double *intersections_x, double *intersections_y, 
			int number);

// effectively insert the point (new_x, new_y) at position 'index'
// in the interesection points arrays intersections_x and 
// intersections_y (of length 'number');
// return SUCCESS on success, ERROR on error
int intersection_do_insert(double *intersections_x, double *intersections_y, 
			   int *number, double new_x, double new_y, int index);

// try to insert the point (new_x, new_y) in the interesection 
// points arrays intersections_x and intersections_y (of length 'number');
// return SUCCESS on success, ERROR on error
int intersection_add_point(double *intersections_x, double *intersections_y, 
			   int *number, double new_x, double new_y);

// update the properties of an environment that is attached to a certain 
// connection, depending on the properties of the scenario
// (position of end-nodes of connection, obstacles, etc.);
// return SUCCESS on succes, ERROR on error
int environment_update(environment_class *environment, 
		       connection_class *connection, scenario_class *scenario);

// check whether a newly defined environment conflicts with existing ones;
// return TRUE if no duplicate environment is found, FALSE otherwise
int environment_check_valid(environment_class *environments, 
			    int environment_number, 
			    environment_class *new_environment);

#endif
