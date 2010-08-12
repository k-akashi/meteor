
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
 * File name: coordinate.h
 * Function:  Header file of coordinate.c
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
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

typedef struct
{
  // name of this coordinate
  char name[MAX_STRING];
  int name_provided;

  // the values of the coordinate; supported choices are:
  //   - 2D or 3D
  //   - cartesian or latitude-longitude-altitude coordinates
  double c[3];//[MAX_COORDINATES]; // don't use constant yet, 
              // since it may have strange effects for objects 
              // which assume 3D coordinates
} coordinate_class;


/////////////////////////////////////////
// Basic functions
/////////////////////////////////////////

// assign the name 'name to the 3D coordinate 'coordinate', 
// and initialize it using the values c0, c1, and c2
void coordinate_init(coordinate_class *coordinate, const char *name,
		     double c0, double c1, double c2);

// print coordinate object
void coordinate_print(const coordinate_class *coordinate);

// copy coordinate object coord_src to coord_dest
void coordinate_copy(coordinate_class *coord_dest, 
		     const coordinate_class *coord_src);

// compute the (3D) distance between two coordinates
// (we assume cartesian coordinates);
// return the distance
double coordinate_distance(const coordinate_class *point1, 
			   const coordinate_class *point2);

// compute the 2D distance between two coordinates
// (we assume cartesian coordinates);
// return the distance
double coordinate_distance_2D(const coordinate_class *point1, 
			      const coordinate_class *point2);

// check whether two coordinates are equal;
// return TRUE if they are equal, FALSE otherwise
int coordinate_are_equal(const coordinate_class *point1, 
			 const coordinate_class *point2);


/////////////////////////////////////////
// Vector-related functions
/////////////////////////////////////////

// compute the magnitude of a vector represented by 
// the coordinate 'vector';
// return the magnitude
double coordinate_vector_magnitude(const coordinate_class *vector);

// compute the vector sum of the vectors represented by 
// the coordinates 'vector1' and 'vector2'; store the result
// in coordinate 'result'
void coordinate_vector_sum(coordinate_class *result, 
			   const coordinate_class *vector1, 
			   const coordinate_class *vector2);

// compute the (3D) vector difference of the vectors represented by 
// the coordinates 'vector1' and 'vector2'; store the result
// in coordinate 'result'
void coordinate_vector_difference(coordinate_class *result, 
				  const coordinate_class *vector1, 
				  const coordinate_class *vector2);

// compute the 2D vector difference of the vectors represented by 
// the coordinates 'vector1' and 'vector2'; store the result
// in coordinate 'result'
void coordinate_vector_difference_2D(coordinate_class *result, 
				     const coordinate_class *vector1, 
				     const coordinate_class *vector2);

// compute the scalar multiplication of the vector represented by 
// the coordinate 'vector' with scalar 'scalar'; store the result
// in coordinate 'result'
void coordinate_multiply_scalar(coordinate_class *result, 
				const coordinate_class *vector,
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
int coordinate_distance_to_segment(const coordinate_class *point, 
				   const coordinate_class *segment_start, 
				   const coordinate_class *segment_end, 
				   double *distance, 
				   coordinate_class *intersection);

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
int coordinate_distance_to_segment_2D(const coordinate_class *point, 
				      const coordinate_class *segment_start, 
				      const coordinate_class *segment_end, 
				      double *distance, 
				      coordinate_class *intersection);


#endif
