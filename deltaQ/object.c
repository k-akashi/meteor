
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
 * File name: object.c
 * Function: Source file related to the object scenario element
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 141 $
 *   $LastChangedDate: 2009-03-27 17:30:45 +0900 (Fri, 27 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#include <string.h>
#include <math.h>

#include "message.h"
#include "deltaQ.h"

#include "object.h"


/////////////////////////////////////////
// Global variables initialization
/////////////////////////////////////////

char *object_types[]={"building", "road"};


/////////////////////////////////////////
// Object structure functions
/////////////////////////////////////////

// init a topology object
void object_init(object_class *object, char *name, char *environment)
{
  strncpy(object->name, name, MAX_STRING-1);
  strncpy(object->environment, environment, MAX_STRING-1);
  object->environment_index = INVALID_INDEX;

  object->vertex_number = 0;
  object->make_polygon = TRUE;

  object->height = 0;

  // flag showing whether the coordinates are given in x-y-z metric system
  // if set to FALSE, the coordinates are given as latitude-longitude-altitude
  // and need to be converted before further processing
  object->is_metric = TRUE;

  object->type = BUILDING_OBJECT;
}

// init the coordinates of a rectangular topology object
void object_init_rectangle(object_class *object, char *name, char *environment,
			   double x1, double y1, double x2, double y2)
{
  strncpy(object->name, name, MAX_STRING-1);
  strncpy(object->environment, environment, MAX_STRING-1);
  object->environment_index = INVALID_INDEX;

  object->vertex_number = 4;
  object->vertices[0].c[0] = x1;
  object->vertices[1].c[0] = x1;
  object->vertices[0].c[1] = y1;
  object->vertices[3].c[1] = y1;
  object->vertices[2].c[0] = x2;
  object->vertices[3].c[0] = x2;
  object->vertices[1].c[1] = y2;
  object->vertices[2].c[1] = y2;

  object->make_polygon = TRUE;
}

// initialize the local environment index for an object;
// return SUCCESS on succes, ERROR on error
int object_init_index(object_class *object, scenario_class *scenario)
{
  int i;

  // check if "environment_index" is not initialized
  if(object->environment_index == INVALID_INDEX)
    // try to find the from_node in scenario
    for(i=0; i<scenario->environment_number; i++)
      if(strcmp(scenario->environments[i].name, object->environment)==0)
	{
	  object->environment_index = i;
	  break;
	}

  // check if "environment_index" could not be found
  if(object->environment_index == INVALID_INDEX) 
   {
      WARNING("Object attribute 'environment' with value '%s' doesn't exist \
in scenario", object->environment);
      return ERROR;
   }

  return SUCCESS;
}

// print the fields of an object, with coordinates 
// in the (x,y,z) format
void object_print(object_class *object)
{
  int i;

  printf("  Object '%s': type='%s' environment='%s' vertex_number=%d \
make_polygon=%s height=%.2f load_from_jpgis_file=%s coordinates:\n", 
	 object->name, object_types[object->type], object->environment, 
	 object->vertex_number, (object->make_polygon==TRUE)?"TRUE":"FALSE", 
	 object->height,
	 (object->load_from_jpgis_file==TRUE)?"TRUE":"FALSE");
  if(object->vertex_number==0)
    printf("\tN/A\n");
  else
    for(i=0; i<object->vertex_number; i++)
      printf("\tV[%d](x,y,z)=(%.9f,%.9f,%.9f)\n", i, object->vertices[i].c[0], 
	     object->vertices[i].c[1], object->vertices[i].c[2]);
}

// print the fields of an object, with coordinates 
// in the (latitude,longitude,altitude) format
void object_print_blh(object_class *object)
{
  int i;

  printf("  Object '%s': type='%s' environment='%s' vertex_number=%d \
make_polygon=%s height=%.2f load_from_jpgis_file=%s coordinates:\n", 
	 object->name, object_types[object->type], object->environment, 
	 object->vertex_number, (object->make_polygon==TRUE)?"TRUE":"FALSE", 
	 object->height,
	 (object->load_from_jpgis_file==TRUE)?"TRUE":"FALSE");
  for(i=0; i<object->vertex_number; i++)
    {
      coordinate_class vertex_blh;

      // transform x & y to lat & long
      en2ll(&(object->vertices[i]), &vertex_blh);

      printf("\tV[%d](lat,lon,alt)=(%.9f,%.9f,%.9f)\n", i, 
	     vertex_blh.c[0], vertex_blh.c[1], vertex_blh.c[2]);
    }
}

// copy the information in object_src to object_dest
void object_copy(object_class *object_dest, object_class * object_src)
{
  int i;

  strncpy(object_dest->name, object_src->name, MAX_STRING-1);

  object_dest->type = object_src->type;

  strncpy(object_dest->environment, object_src->environment, MAX_STRING-1);
  object_dest->environment_index = object_src->environment_index;

  object_dest->make_polygon = object_src->make_polygon;
  object_dest->is_metric = object_src->is_metric;

  object_dest->vertex_number = object_src->vertex_number;
  for(i=0; i<object_dest->vertex_number; i++)
    coordinate_copy(&(object_dest->vertices[i]), &(object_src->vertices[i]));
  
  object_dest->height = object_src->height;

  object_dest->load_from_jpgis_file = object_src->load_from_jpgis_file;
}

// check whether a newly defined object conflicts with existing ones;
// return TRUE if node is valid, FALSE otherwise
int object_check_valid(object_class *objects, int object_number, 
		       object_class *new_object, int jpgis_filename_provided)
{
  int i;

  // check whether name was previously used
  for(i=0; i<object_number; i++)
    {
      if(strcmp(objects[i].name, new_object->name)==0)
	{
	  WARNING("Object with name '%s' already defined", new_object->name);
	  return FALSE;
	}
    }

  // check whether JPGIS file was provided 
  if(new_object->load_from_jpgis_file == TRUE &&
     jpgis_filename_provided == FALSE)
    {
      WARNING("Object '%s' is supposed to be loaded from a JPGIS file, \
but no filename was provided in the QOMET scenario", new_object->name);
      return FALSE;
    }

  return TRUE;
}

// find whether a segment defined by (start, end) vectors and an
// object form an intersecttion; the flag 'include_vertices' should
// be set to TRUE if vertices are to be considered valid intersection
// point, or to FALSE otherwise; the flag 'consider_height' should be 
// set to TRUE if segment ends height and object height should be 
// taken into account when intersecting, or to FALSE otherwise;
// return TRUE on true, FALSE on false
int object_intersect(coordinate_class *start, coordinate_class *end, 
		     object_class *object, int include_vertices,
		     int consider_height)
{
  int vertex_i, vertex_i2;

  double x_intersect, y_intersect;

  int last_vertex;

  // if consider_height flag was set to TRUE,
  // first check whether the segment is higher than the object
  // by checking whether both ends are larger than object height
  if(consider_height==TRUE && 
     (start->c[2]>object->height && end->c[2]>object->height))
    return FALSE;

  // if one of the ends is smaller than the height we should compute 
  // 3D intersection of segment and object; however we assume only
  // a simplified case for the moment, in which a 2D intersection 
  // will suffice (segment is parallel with x0y plane)
  
  // determine intersection points
  // (including the edge [V[N-1],V[0]], even if the object is not 
  // a polygon, so as to avoid strange cases with unaccounted areas)
  
  last_vertex = (object->make_polygon==TRUE)?object->vertex_number:
    object->vertex_number-1;

  for(vertex_i=0; vertex_i<last_vertex; vertex_i++)
    {
      vertex_i2=(vertex_i+1)<object->vertex_number?vertex_i+1:0;

      printf("Object edge (V[%d],V[%d])...\n", vertex_i, vertex_i2);
      // check intersection with each edge
      if(segment_intersect(start->c[0], start->c[1], end->c[0], end->c[1],
			   object->vertices[vertex_i].c[0], 
			   object->vertices[vertex_i].c[1], 
			   object->vertices[vertex_i2].c[0], 
			   object->vertices[vertex_i2].c[1], 
			   &x_intersect, &y_intersect)==TRUE)
	{
	  // if 'include_vertices' is FALSE, ignore intersection 
	  // if it is represented by one of the object vertices
	  /*
	  printf("s.x=%f s.y=%f e.x=%f e.y=%f x_i=%f  y_i=%f\n", 
		 start->c[0], start->c[1], end->c[0], end->c[1], 
		 x_intersect, y_intersect);
	  printf("ex-xi=%g ey-yi=%g\n", end->c[0]-x_intersect, 
		 end->c[1]-y_intersect);
	  */
	  if(include_vertices==FALSE && 
	     ((fabs(x_intersect-start->c[0])<EPSILON && 
	       fabs(y_intersect-start->c[1])<EPSILON) ||
	      (fabs(x_intersect-end->c[0])<EPSILON && 
	       fabs(y_intersect-end->c[1])<EPSILON)))
	    {
	      /*
	      printf("Continue s.x=%f s.y=%f e.x=%f e.y=%f x_i=%f  \
y_i=%f\n", start->c[0], start->c[1], end->c[0], end->c[1], 
		     x_intersect, y_intersect);
	      */
	      DEBUG("Intersection ignored because is vertex, and parameter \
'include_vertices' was set to FALSE");
	      continue;
	    }
	  else
	    {
	      /*
	      printf("TRUE: s.x=%f s.y=%f e.x=%f e.y=%f x_i=%f  y_i=%f\n", 
		     start->c[0], start->c[1], end->c[0], end->c[1], 
		     x_intersect, y_intersect);
	      */
	      DEBUG("Found valid intersection: x_i=%f  y_i=%f",
		    x_intersect, y_intersect);
	      return TRUE;
	    }
	}
      else
	DEBUG("No intersection");
    }
  
  printf("FALSE: s.x=%f s.y=%f e.x=%f e.y=%f x_i=%f  y_i=%f\n", 
	 start->c[0], start->c[1], end->c[0], end->c[1], 
	 x_intersect, y_intersect);

  return FALSE;
}
