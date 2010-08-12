
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
 * File name: object.h
 * Function:  Header file of object.c
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#ifndef __OBJECT_H
#define __OBJECT_H

#include "coordinate.h"

#define MAX_VERTICES                    35


////////////////////////////////////////////////
// Global variables
////////////////////////////////////////////////

extern char *object_types[];


////////////////////////////////////////////////
// Object structure definition
////////////////////////////////////////////////

struct object_class_s
{
  // object name
  char name[MAX_STRING];

  // object type
  int type;

  // environment associated to the object
  // (name and index in scenario)
  char environment[MAX_STRING];
  int environment_index;

  // flag showing whether the base is a polygon;
  // if set to FALSE, the base is a polyline
  int make_polygon;

  // flag showing whether the coordinates are given in x-y-z metric system
  // if set to FALSE, the coordinates are given as latitude-longitude-altitude
  // and need to be converted before further processing
  int is_metric;

  // coordinates of the  of the object
  coordinate_class vertices[MAX_VERTICES];

  // number of vertices
  int vertex_number;

  // object height (assuming basis is plane)
  double height;

  // flag showing whether the object should be loaded from a 
  // JPGIS file; in this case any coordinates provided in the
  // QOMET scenario file will be ignored
  int load_from_jpgis_file;
};


/////////////////////////////////////////
// Topology object structure functions
/////////////////////////////////////////

// init a topology object
void object_init(object_class *object, char *name, char *environment);

// init the coordinates of a rectangular topology object
void object_init_rectangle(object_class *object, char *name, char *environment,
			   double x1, double y1, double x2, double y2);

// initialize the local environment index for an object;
// return SUCCESS on succes, ERROR on error
int object_init_index(object_class *object, scenario_class *scenario);

// print the fields of an object, with coordinates 
// in the (x,y,z) format
void object_print(object_class *environment);

// print the fields of an object, with coordinates 
// in the (latitude,longitude,altitude) format
void object_print_blh(object_class *object);

// copy the information in object_src to object_dest
void object_copy(object_class *object_dest, object_class * object_src);

// check whether a newly defined object conflicts with existing ones;
// return TRUE if node is valid, FALSE otherwise
int object_check_valid(object_class *objects, int object_number, 
		       object_class *new_object, int jpgis_filename_provided);

// find whether a segment defined by (start, end) vectors and an
// object form an intersecttion; the flag 'include_vertices' should
// be set to TRUE if vertices are to be considered valid intersection
// point, or to FALSE otherwise; the flag 'consider_height' should be 
// set to TRUE if segment ends height and object height should be 
// taken into account when intersecting, or to FALSE otherwise;
// return TRUE on true, FALSE on false
int object_intersect(coordinate_class *start, coordinate_class *end, 
		     object_class *object, int include_vertices,
		     int consider_height);

#endif
