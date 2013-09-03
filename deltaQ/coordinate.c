
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
 * File name: coordinate.c
 * Function:  Represent a generic 3D coordinate
 *
 * Author: Razvan Beuran
 *
 * $Id: coordinate.c 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "coordinate.h"
#include "deltaQ.h"

// assign the name 'name to the 3D coordinate 'coordinate', 
// and initialize it using the values c0, c1, and c2
void
coordinate_init (struct coordinate_class *coordinate, const char *name,
		 double c0, double c1, double c2)
{
  if (name == NULL)
    strncpy (coordinate->name, DEFAULT_COORDINATE_NAME, MAX_STRING - 1);
  else
    {
      strncpy (coordinate->name, name, MAX_STRING - 1);
      if (strlen (name) > MAX_STRING - 1)
	coordinate->name[MAX_STRING - 1] = '\0';
    }

  coordinate->name_provided = NAME_WAS_PROVIDED;

  coordinate->c[0] = c0;
  coordinate->c[1] = c1;
  if (MAX_COORDINATES == 3)
    coordinate->c[2] = c2;
  else
    coordinate->c[2] = 0.0;
}

// print coordinate object
void
coordinate_print (const struct coordinate_class *coordinate)
{
  if (MAX_COORDINATES == 3)
    {
      if (coordinate->name_provided == NAME_WAS_PROVIDED)
	printf ("coordinate '%s'=(%.2f, %.2f, %.2f)\n", coordinate->name,
		coordinate->c[0], coordinate->c[1], coordinate->c[2]);
      else
	printf ("coordinate '%s'=(%.2f, %.2f, %.2f)\n",
		DEFAULT_COORDINATE_NAME,
		coordinate->c[0], coordinate->c[1], coordinate->c[2]);
    }
  else
    {
      if (coordinate->name_provided == NAME_WAS_PROVIDED)
	printf ("coordinate '%s'=(%.2f, %.2f)\n", coordinate->name,
		coordinate->c[0], coordinate->c[1]);
      else
	printf ("coordinate '%s'=(%.2f, %.2f)\n", DEFAULT_COORDINATE_NAME,
		coordinate->c[0], coordinate->c[1]);
    }

}

// copy coordinate object coord_src to coord_dest
void
coordinate_copy (struct coordinate_class *coord_dest,
		 const struct coordinate_class *coord_src)
{
  int i;

  for (i = 0; i < MAX_COORDINATES; i++)
    coord_dest->c[i] = coord_src->c[i];
}

// compute the (3D) distance between two coordinates
// (we assume cartesian coordinates);
// return the distance
double
coordinate_distance (const struct coordinate_class *point1,
		     const struct coordinate_class *point2)
{
  struct coordinate_class difference;

  coordinate_vector_difference (&difference, point1, point2);
  return coordinate_vector_magnitude (&difference);
}

// compute the 2D distance between two coordinates
// (we assume cartesian coordinates);
// return the distance
double
coordinate_distance_2D (const struct coordinate_class *point1,
			const struct coordinate_class *point2)
{
  struct coordinate_class difference;

  coordinate_vector_difference_2D (&difference, point1, point2);
  return coordinate_vector_magnitude (&difference);
}

// check whether two coordinates are equal;
// return TRUE if they are equal, FALSE otherwise
int
coordinate_are_equal (const struct coordinate_class *point1,
		      const struct coordinate_class *point2)
{
  struct coordinate_class difference;

  coordinate_vector_difference (&difference, point1, point2);

  if (coordinate_vector_magnitude (&difference) < EPSILON)
    return TRUE;
  else
    return FALSE;
}

/////////////////////////////////////////
// Vector-related functions
/////////////////////////////////////////

// compute the magnitude of a vector represented by 
// the coordinate 'vector';
// return the magnitude
double
coordinate_vector_magnitude (const struct coordinate_class *vector)
{
  int i;
  double sum_squares = 0;

  for (i = 0; i < MAX_COORDINATES; i++)
    sum_squares += (vector->c[i] * vector->c[i]);

  return sqrt (sum_squares);
}

// compute the vector sum of the vectors represented by 
// the coordinates 'vector1' and 'vector'; store the result
// in coordinate 'result'
void
coordinate_vector_sum (struct coordinate_class *result,
		       const struct coordinate_class *vector1,
		       const struct coordinate_class *vector2)
{
  int i;

  for (i = 0; i < MAX_COORDINATES; i++)
    result->c[i] = vector1->c[i] + vector2->c[i];
}

// compute the (3D) vector difference of the vectors represented by 
// the coordinates 'vector1' and 'vector2'; store the result
// in coordinate 'result'
void
coordinate_vector_difference (struct coordinate_class *result,
			      const struct coordinate_class *vector1,
			      const struct coordinate_class *vector2)
{
  int i;

  for (i = 0; i < MAX_COORDINATES; i++)
    result->c[i] = vector1->c[i] - vector2->c[i];
}

// compute the 2D vector difference of the vectors represented by 
// the coordinates 'vector1' and 'vector2'; store the result
// in coordinate 'result'
void
coordinate_vector_difference_2D (struct coordinate_class *result,
				 const struct coordinate_class *vector1,
				 const struct coordinate_class *vector2)
{
  int i;

  for (i = 0; i < MAX_COORDINATES_2D; i++)
    result->c[i] = vector1->c[i] - vector2->c[i];
  for (i = MAX_COORDINATES_2D; i < MAX_COORDINATES; i++)
    result->c[i] = 0;
}

// compute the scalar multiplication of the vector represented by 
// the coordinate 'vector' with scalar 'scalar'; store the result
// in coordinate 'result'
void
coordinate_multiply_scalar (struct coordinate_class *result,
			    const struct coordinate_class *vector,
			    const double scalar)
{
  int i;

  for (i = 0; i < MAX_COORDINATES; i++)
    result->c[i] = vector->c[i] * scalar;
}

// NOTE: The function below works in 3D!!!!!!!!!

// compute the distance between 'point' and a segment defined by
// coordinates 'segment_start' and 'segment_end';
// the value of the distance is stored in 'distance, and
// the coordinate of the intersection of the perpendicular line
// from point to segment in 'intersection';
// return SUCCESS on success, ERROR on error (i.e., distance cannot
// be computed because the perpendicular between the point and segment 
// doesn't fall on the segment)

// NOTE: The code for the 2D version is based on the algorithm
// "Minimum Distance Between a Point and a Line" by P. Bourke (1988)
// http://local.wasp.uwa.edu.au/~pbourke/geometry/pointline/
int
coordinate_distance_to_segment (const struct coordinate_class *point,
				const struct coordinate_class *segment_start,
				const struct coordinate_class *segment_end,
				double *distance,
				struct coordinate_class *intersection)
{
  //struct coordinate_class segment_difference;
  double segment_length;

  // parameter used to determine whether the perpendicular 
  // between the point and segment falls on the segment or not
  double u = 0;

  int i;

  // compute segment length
  segment_length = coordinate_distance (segment_start, segment_end);

  // compute parameter "u"
  for (i = 0; i < MAX_COORDINATES; i++)
    u += ((point->c[i] - segment_start->c[i]) *
	  (segment_end->c[i] - segment_start->c[i]));
  u /= (segment_length * segment_length);

  // if u is not between 0 and 1, it means the perpendicular 
  // doesn't fall on the line segment but outside it
  if (u < 0.0 || u > 1.0)
    return FALSE;

  for (i = 0; i < MAX_COORDINATES; i++)
    intersection->c[i] = segment_start->c[i] +
      u * (segment_end->c[i] - segment_start->c[i]);

  *distance = coordinate_distance (point, intersection);

  //printf("distance=%f\n", *distance);
  //coordinate_print(*intersection, "intersection");

  return TRUE;
}

// NOTE: The function below works in 2D!!!!!!!!!

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
int
coordinate_distance_to_segment_2D (const struct coordinate_class *point,
				   const struct coordinate_class
				   *segment_start,
				   const struct coordinate_class *segment_end,
				   double *distance,
				   struct coordinate_class *intersection)
{
  //struct coordinate_class segment_difference;
  double segment_length;

  // parameter used to determine whether the perpendicular 
  // between the point and segment falls on the segment or not
  double u = 0;

  int i;

  // compute segment length
  segment_length = coordinate_distance_2D (segment_start, segment_end);

  // compute parameter "u"
  for (i = 0; i < MAX_COORDINATES_2D; i++)
    u += ((point->c[i] - segment_start->c[i]) *
	  (segment_end->c[i] - segment_start->c[i]));
  u /= (segment_length * segment_length);

  // if u is not between 0 and 1, it means the perpendicular 
  // doesn't fall on the line segment but outside it
  if (u < 0.0 || u > 1.0)
    return FALSE;

  for (i = 0; i < MAX_COORDINATES_2D; i++)
    intersection->c[i] = segment_start->c[i] +
      u * (segment_end->c[i] - segment_start->c[i]);
  for (i = MAX_COORDINATES_2D; i < MAX_COORDINATES; i++)
    intersection->c[i] = 0;

  *distance = coordinate_distance_2D (point, intersection);

  //printf("distance=%f\n", *distance);
  //coordinate_print(*intersection, "intersection");

  return TRUE;
}

// compute the angle in [0, 2*PI) between a vector and the horizontal axis;
// return the computed angle
double
coordinate_vector_angle_2D (const struct coordinate_class *vector)
{
  int i;
  double sum = 0;
  double angle;

  for (i = 0; i < MAX_COORDINATES_2D; i++)
    sum += vector->c[i] * vector->c[i];

  if (sum < EPSILON)
    angle = 0;
  else
    {
      angle = acos (vector->c[0] / sqrt (sum));
      if (vector->c[1] < 0)
	angle = 2 * M_PI - angle;
    }

  return angle;
}
