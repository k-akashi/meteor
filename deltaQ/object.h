
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
 * File name: object.h
 * Function:  Header file of object.c
 *
 * Author: Razvan Beuran
 *
 * $Id: object.h 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/


#ifndef __OBJECT_H
#define __OBJECT_H

#include "global.h"
#include "coordinate.h"


#define MAX_VERTICES                   500	//200


////////////////////////////////////////////////
// Global variables
////////////////////////////////////////////////

extern char *object_types[];


////////////////////////////////////////////////
// Object structure definition
////////////////////////////////////////////////

struct object_class
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

  // flag showing whether the coordinates were originally given in the
  // 0xyz metric system
  // if set to FALSE, the coordinates were initially given as 
  // latitude-longitude-altitude, and need to be converted back before
  // being outputted 
  // int is_metric;

  // coordinates of the  of the object
  struct coordinate_class vertices[MAX_VERTICES];

  // number of vertices
  int vertex_number;

  // object height (assuming basis is plane)
  double height;

  // flag showing whether the object should be loaded from a 
  // JPGIS file; in this case any coordinates provided in the
  // QOMET scenario file will be ignored
  int load_from_jpgis_file;

  // flag showing whether the object is a representative for objects
  // that have to be loaded from a region of a JPGIS file; in this case 
  // the object coordinates provided in the QOMET scenario file are used to 
  // define the region
  int load_all_from_region;
};


/////////////////////////////////////////
// Topology object structure functions
/////////////////////////////////////////

// init a topology object
void object_init (struct object_class *object, char *name, char *environment);

// init the coordinates of a rectangular topology object
void object_init_rectangle (struct object_class *object, char *name,
			    char *environment, double x1, double y1,
			    double x2, double y2);

// initialize the local environment index for an object;
// return SUCCESS on succes, ERROR on error
int object_init_index (struct object_class *object,
		       struct scenario_class *scenario);

// print the fields of an object, with coordinates 
// in the (x,y,z) format
void object_print (struct object_class *environment);

// print the fields of an object, with coordinates 
// in the (latitude,longitude,altitude) format
void object_print_blh (struct object_class *object);

// copy the information in object_src to object_dest
void object_copy (struct object_class *object_dest,
		  struct object_class *object_src);

// check whether a newly defined object conflicts with existing ones;
// return TRUE if node is valid, FALSE otherwise
int object_check_valid (struct object_class *objects, int object_number,
			struct object_class *new_object,
			int jpgis_filename_provided);

// find whether a segment defined by (start, end) vectors and an
// object form an intersecttion; the flag 'include_vertices' should
// be set to TRUE if vertices are to be considered valid intersection
// point, or to FALSE otherwise; the flag 'consider_height' should be 
// set to TRUE if segment ends height and object height should be 
// taken into account when intersecting, or to FALSE otherwise;
// return TRUE on true, FALSE on false
int object_intersect (struct coordinate_class *start,
		      struct coordinate_class *end,
		      struct object_class *object, int include_vertices,
		      int consider_height);

#endif
