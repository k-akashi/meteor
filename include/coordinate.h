
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
 * File name: coordinate.h
 * Function:  Header file of coordinate.c
 *
 * Author: Razvan Beuran
 *
 * $Id: coordinate.h 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/


#ifndef __COORDINATE_H
#define __COORDINATE_H

#include "global.h"

// we now support both 2D and 3D coordinates
// (if this constant is defined as '2' strange effects may occur
// for objects which assume 3D coordinates)
#define MAX_COORDINATES             3

// maximum number of coordinates in 2D
#define MAX_COORDINATES_2D          2

#define DEFAULT_COORDINATE_NAME     "UNKNOWN"

// use this constant for the moment, change in the future!!!!!!!!!!!
#define NAME_WAS_PROVIDED           12345678


////////////////////////////////////////////
// Data structure definition
////////////////////////////////////////////

struct coordinate_class
{
  // name of this coordinate
  char name[MAX_STRING];
  int name_provided;

  // the values of the coordinate; supported choices are:
  //   - 2D or 3D
  //   - cartesian or latitude-longitude-altitude coordinates
  double c[3];			//[MAX_COORDINATES]; // don't use constant yet, 
  // since it may have strange effects for objects 
  // which assume 3D coordinates
};


/////////////////////////////////////////
// Basic functions
/////////////////////////////////////////

// assign the name 'name to the 3D coordinate 'coordinate', 
// and initialize it using the values c0, c1, and c2
void coordinate_init (struct coordinate_class *coordinate, const char *name,
		      double c0, double c1, double c2);

// print coordinate object
void coordinate_print (const struct coordinate_class *coordinate);

// copy coordinate object coord_src to coord_dest
void coordinate_copy (struct coordinate_class *coord_dest,
		      const struct coordinate_class *coord_src);

// compute the (3D) distance between two coordinates
// (we assume cartesian coordinates);
// return the distance
double coordinate_distance (const struct coordinate_class *point1,
			    const struct coordinate_class *point2);

// compute the 2D distance between two coordinates
// (we assume cartesian coordinates);
// return the distance
double coordinate_distance_2D (const struct coordinate_class *point1,
			       const struct coordinate_class *point2);

// check whether two coordinates are equal;
// return TRUE if they are equal, FALSE otherwise
int coordinate_are_equal (const struct coordinate_class *point1,
			  const struct coordinate_class *point2);


/////////////////////////////////////////
// Vector-related functions
/////////////////////////////////////////

// compute the magnitude of a vector represented by 
// the coordinate 'vector';
// return the magnitude
double coordinate_vector_magnitude (const struct coordinate_class *vector);

// compute the vector sum of the vectors represented by 
// the coordinates 'vector1' and 'vector2'; store the result
// in coordinate 'result'
void coordinate_vector_sum (struct coordinate_class *result,
			    const struct coordinate_class *vector1,
			    const struct coordinate_class *vector2);

// compute the (3D) vector difference of the vectors represented by 
// the coordinates 'vector1' and 'vector2'; store the result
// in coordinate 'result'
void coordinate_vector_difference (struct coordinate_class *result,
				   const struct coordinate_class *vector1,
				   const struct coordinate_class *vector2);

// compute the 2D vector difference of the vectors represented by 
// the coordinates 'vector1' and 'vector2'; store the result
// in coordinate 'result'
void coordinate_vector_difference_2D (struct coordinate_class *result,
				      const struct coordinate_class *vector1,
				      const struct coordinate_class *vector2);

// compute the scalar multiplication of the vector represented by 
// the coordinate 'vector' with scalar 'scalar'; store the result
// in coordinate 'result'
void coordinate_multiply_scalar (struct coordinate_class *result,
				 const struct coordinate_class *vector,
				 const double scalar);

// compute the 3D distance between 'point' and a segment defined by
// coordinates 'segment_start' and 'segment_end';
// the value of the distance is stored in 'distance, and
// the coordinate of the intersection of the perpendicular line
// from point to segment in 'intersection';
// return SUCCESS on success, ERROR on error (i.e., distance cannot
// be computed because the point and segment ends are colinear

// NOTE: The code for the 2D version is based on the algorithm
// "Minimum Distance Between a Point and a Line" by P. Bourke (1988)
// http://local.wasp.uwa.edu.au/~pbourke/geometry/pointline/
int coordinate_distance_to_segment (const struct coordinate_class *point,
				    const struct coordinate_class
				    *segment_start,
				    const struct coordinate_class
				    *segment_end, double *distance,
				    struct coordinate_class *intersection);

// compute the 2D distance between 'point' and a segment defined by
// coordinates 'segment_start' and 'segment_end';
// the value of the distance is stored in 'distance, and
// the coordinate of the intersection of the perpendicular line
// from point to segment in 'intersection';
// return SUCCESS on success, ERROR on error (i.e., distance cannot
// be computed because the perpendicular between the point and segment 
// doesn't fall on the segment)

// NOTE: This code is based on the algorithm
// "Minimum Distance Between a Point and a Line" by P. Bourke (1988)
// http://local.wasp.uwa.edu.au/~pbourke/geometry/pointline/
int coordinate_distance_to_segment_2D (const struct coordinate_class *point,
				       const struct coordinate_class
				       *segment_start,
				       const struct coordinate_class
				       *segment_end, double *distance,
				       struct coordinate_class *intersection);

// compute the angle in [0, 2*PI) between a vector and the horizontal axis;
// return the computed angle
double coordinate_vector_angle_2D (const struct coordinate_class *vector);

#endif
