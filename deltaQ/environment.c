
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
 * File name: environment.c
 * Function: Source file related to the environment scenario element
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

#include "message.h"
#include "deltaQ.h"

#include "environment.h"

#include "wlan.h"


/////////////////////////////////////////
// Environment structure functions
/////////////////////////////////////////

// init a (static) environment
void environment_init(environment_class *environment, char *name, char *type, 
		      double alpha, double sigma, double W, double noise_power,
		      double x1, double y1, double x2, double y2)
{
  strncpy(environment->name, name, MAX_STRING-1);
  environment->name_hash=string_hash(environment->name, 
				     strlen(environment->name));
  strncpy(environment->type, type, MAX_STRING-1);
  environment->is_dynamic = FALSE;
  environment->num_segments = 1;
  environment->alpha[0] = alpha;
  environment->sigma[0] = sigma;
  environment->W[0] = W;
  environment->noise_power[0] = noise_power;
  environment->length[0] = -1;
}

// init an environment (dynamic version)
void environment_dynamic_init(environment_class *environment, char *name, 
			      char *type, int num_segments, double alpha[], 
			      double sigma[], double W[], double noise_power[],
			      double distance[])
{
  int i;

  strncpy(environment->name, name, MAX_STRING-1);
  environment->name_hash=string_hash(environment->name, 
				     strlen(environment->name));
  strncpy(environment->type, type, MAX_STRING-1);
  environment->is_dynamic = TRUE;

  environment->num_segments = num_segments;

  for(i=0; i<num_segments; i++)
    {
      environment->alpha[i] = alpha[i];
      environment->sigma[i] = sigma[i];
      environment->W[i] = W[i];
      environment->noise_power[i] = noise_power[i];
      environment->length[i] = distance[i];
    }
}

// print the fields of an environment
void environment_print(environment_class *environment)
{
  if(environment->num_segments>0)
    {      
      int i;
      printf("  Environment '%s': type='%s' is_dynamic='%s'\n", 
	     environment->name, environment->type, 
	     (environment->is_dynamic==TRUE)?"true":"false");
      for(i=0; i<environment->num_segments; i++)
	printf("    segment #%d: alpha=%.2f sigma=%.2f W=%.2f \
noise_power=%.2f length=%.2f\n", i, environment->alpha[i], 
	       environment->sigma[i], environment->W[i],
	       environment->noise_power[i], environment->length[i]);
    }
  else 
    printf("  Environment '%s': type='%s' is NOT initialized.\n", 
	   environment->name, environment->type);
}

// copy the information in environment_src to environment_dest
void environment_copy(environment_class *environment_dest, 
		      environment_class * environment_src)
{
  int i;

  strncpy(environment_dest->name, environment_src->name, MAX_STRING-1);
  environment_dest->name_hash = environment_src->name_hash;

  strncpy(environment_dest->type, environment_src->type, MAX_STRING-1);
  environment_dest->is_dynamic = environment_src->is_dynamic;

  environment_dest->num_segments = environment_src->num_segments;

  for(i=0; i<environment_dest->num_segments; i++)
    {
      environment_dest->alpha[i] = environment_src->alpha[i];
      environment_dest->sigma[i] = environment_src->sigma[i];
      environment_dest->W[i] = environment_src->W[i];
      environment_dest->noise_power[i] = environment_src->noise_power[i];
      environment_dest->length[i] = environment_src->length[i];
    }
}

//////////////////////////////////////////
// Dynamic environment functions
// Line intersection code based on a "Graphics Gems"
// article by Mukesh Prasad
// http://tog.acm.org/GraphicsGems/gemsii/xlines.c

// check wheter a and b have the same sign
// (only works for 2's complement representation)

/*
#define SAME_SIGNS( a, b )                 \
		(((long) ((unsigned long) a ^ (unsigned long) b)) >= 0 )
*/

#define SAME_SIGNS( a, b )  ((a<0 && b<0) || (a>0 && b>0))

// 2D segment intersection;
// return TRUE if the segments [(xn1,yn1), (xn2,yn2)]
// and [(xo1,yo1), (xo2,yo2)] intersect; 
// if interesect, return the coordinates of interesection in 
// x_intersect and y_intersect
int segment_intersect(double xn1, double yn1, double xn2, double yn2,
		      double xo1, double yo1, double xo2, double yo2,
		      double *x_intersect, double *y_intersect)
{
  // coefficients of line equations for the "n" and "o" segments
  double an, bn, cn, ao, bo, co;
  double rn1, rn2, ro1, ro2;

  double denominator;
  /*
  DEBUG("Intersect [(%.2f,%.2f),(%.2f,%.2f)] and [(%.2f,%.2f),(%.2f,%.2f)]:",
	xn1, yn1, xn2, yn2, xo1, yo1, xo2, yo2);
  */
  ///////////////////////////////////////////
  // first line has equation an*x+bn*y+cn=0 
  an = yn2 - yn1;
  bn = xn1 - xn2;
  cn = xn2 * yn1 - xn1 * yn2;

  // check if signs are incompatible
  ro1 = an * xo1 + bn * yo1 + cn;
  ro2 = an * xo2 + bn * yo2 + cn;

  //if(ro1 !=0 && ro2 !=0 && SAME_SIGNS(ro1, ro2))
  if(fabs(ro1)>=EPSILON && fabs(ro2)>=EPSILON && SAME_SIGNS(ro1, ro2))
    {
      //DEBUG("\tno intersection (ro1=%f, ro2=%f have same sign)", ro1, ro2);
      return FALSE;
    }

  ////////////////////////////////////////////
  // second line has equation ao*x+bo*y+co=0 
  ao = yo2 - yo1;
  bo = xo1 - xo2;
  co = xo2 * yo1 - xo1 * yo2;

  // check if signs are incompatible
  rn1 = ao * xn1 + bo * yn1 + co;
  rn2 = ao * xn2 + bo * yn2 + co;

  //if(rn1 !=0 && rn2 !=0 && SAME_SIGNS(rn1, rn2))
  if(fabs(rn1)>=EPSILON && fabs(rn2)>=EPSILON && SAME_SIGNS(rn1, rn2))
    {
      //DEBUG("\tno intersection (rn1=%f, rn2=%f have same sign)", rn1, rn2);
      return FALSE;
    }

  ////////////////////////////////////////////////////
  // line segments intersect => compute intersection
  denominator = an * bo - ao * bn;

  if(denominator==0)
    {
      //DEBUG("\tno intersection (collinear segments)");
      return FALSE;
    }
  else
    {
      (*x_intersect) = (bn * co - bo * cn)/denominator;
      (*y_intersect) = (ao * cn - an * co)/denominator;

      DEBUG("\tintersection=(%.2f,%.2f)", *x_intersect, *y_intersect);
      return TRUE;
    }
}

// check whether a point is in a 2D object;
// return TRUE if point (xn,yn) is in object (xo1,yo1)x(xo2,yo2)
int point_in_object(double xn, double yn, double xo1, double yo1, 
		    double xo2, double yo2)
{
  if(((xn>=(xo1-EPSILON) && xn<=(xo2+EPSILON)) ||
      (xn>=(xo2-EPSILON) && xn<=(xo1+EPSILON))) &&
     ((yn>=(yo1-EPSILON) && yn<=(yo2+EPSILON)) || 
      (yn>=(yo2-EPSILON) && yn<=(yo1+EPSILON))))
    return TRUE;
  else
    return FALSE;
}

// check whether a point is in a 3D object;
// return TRUE if point (xn,yn) is in object (xo1,yo1)x(xo2,yo2)
int point_in_object3d(double xn, double yn, double zn, object_class *object)
{
  // long segment to the right used to determine if a point
  // is in an object
  double xn_right = xn+1e6; 

  double x_intersect, y_intersect;
  int vertex_i, vertex_i2;

  int intersection_number = 0;

  // first check whether the segment is higher than the object
  // by checking whether both ends are larger than object height
  if(zn>object->height)
    return FALSE;

  // if z is smaller than the height we should compute 
  // 3D intersection of segment and object; however we assume only
  // a simplified case for the moment, in which a 2D intersection 
  // will suffice (segment is parallel with x0y plane)
  
  // determine intersection points
  // (including the edge [V[N-1],V[0]], even if the object is not 
  // a polygon, so as to avoid strange cases with unaccounted areas)
  DEBUG("Checking point (%f, %f) using a virtual long segment", xn, yn);
  for(vertex_i=0; vertex_i<object->vertex_number; vertex_i++)
    {
      vertex_i2=(vertex_i+1)<object->vertex_number?vertex_i+1:0;

      printf("Object edge (V[%d],V[%d])...\n", vertex_i, vertex_i2);
      // check intersection with each edge
      if(segment_intersect(xn, yn, xn_right, yn,
			   object->vertices[vertex_i].c[0], 
			   object->vertices[vertex_i].c[1], 
			   object->vertices[vertex_i2].c[0], 
			   object->vertices[vertex_i2].c[1], 
			   &x_intersect, &y_intersect)==TRUE)
	    {
	      intersection_number++;
	    }
    }

  // an odd number of intersections means 
  // the point is inside the polygon
  if(intersection_number&1)
    return TRUE;
  else 
    return FALSE;

  /*
 BOOL G_PtInPolygon(POINT *rgpts, WORD wnumpts, POINT ptTest,

                   RECT *prbound)

  {

   RECT   r ;
   POINT  *ppt ;
   WORD   i ;
   POINT  pt1, pt2 ;
   WORD   wnumintsct = 0 ;

   if (!G_PtInPolyRect(rgpts,wnumpts,ptTest,prbound))

   return FALSE ;

   pt1 = pt2 = ptTest ;
   pt2.x = r.right + 50 ;

   // Now go through each of the lines in the polygon and see if it
   // intersects
   for (i = 0, ppt = rgpts ; i < wnumpts-1 ; i++, ppt++)
   {
      if (Intersect(ptTest, pt2, *ppt, *(ppt+1)))
         wnumintsct++ ;
   }

   // And the last line
   if (Intersect(ptTest, pt2, *ppt, *rgpts))
      wnumintsct++ ;

   return (wnumintsct&1) ;

   }
  */
}

// check wether an internal point is on a 2D object edge;
// return TRUE if point (xn,yn) is one one of the 
// edges of object (xo1,yo1)x(xo2,yo2)
int internal_point_on_object_edge(double xn, double yn, double xo1, 
				  double yo1, double xo2, double yo2)
{
  if(fabs(xn-xo1)<EPSILON || fabs(xn-xo2)<EPSILON || 
     fabs(yn-yo1)<EPSILON || fabs(yn-yo2)<EPSILON)
    return TRUE;
  else
    return FALSE;
}

// check whether a segment is included in a 2D object;
// return TRUE if segment (xn1,yn1)<->(xn2,yn2) is 
// included in object (xo1,yo1)x(xo2,yo2)
int segment_in_object(double xn1, double yn1, double xn2, double yn2,
		      double xo1, double yo1, double xo2, double yo2)
{
  if(point_in_object(xn1, yn1, xo1, yo1, xo2, yo2) &&
     point_in_object(xn2, yn2, xo1, yo1, xo2, yo2))
    return TRUE;
  else
    return FALSE;
}

// check whether a segment is included in a 3D object;
// return TRUE if segment (xn1,yn1,zn1)<->(xn2,yn2,zn2) is 
// included in object (xo1,yo1)x(xo2,yo2)
int segment_in_object3d(double xn1, double yn1, double zn1, 
			double xn2, double yn2, double zn2,
			object_class *object)
{
  if(point_in_object3d(xn1, yn1, zn1, object) &&
     point_in_object3d(xn2, yn2, zn2, object))
    return TRUE;
  else
    return FALSE;
}

// print the contents of an array of intersection points
// or "EMPTY" if no elements are found
void intersection_print(double *intersections_x, double *intersections_y, 
			int number)
{
  int i;

  printf("Intersection array (element count=%d): ", number);

  if(number<=0)
    printf("EMPTY\n");
  else
    {
      for(i=0; i<number; i++)
	printf("(%.2f,%.2f) ", intersections_x[i], intersections_y[i]);
      printf("\n");
    }
}

// effectively insert the point (new_x, new_y) at position 'index'
// in the interesection points arrays intersections_x and 
// intersections_y (of length 'number');
// return SUCCESS on success, ERROR on error
int intersection_do_insert(double *intersections_x, double *intersections_y, 
			   int *number, double new_x, double new_y, int index)
{
  int i;

  if((*number)>=(MAX_SEGMENTS+1))
    {
      WARNING("Maximum number of segments (%d) exceeded", MAX_SEGMENTS);
      return ERROR;
    }

  if(index<0)
    {
      WARNING("Unable to insert at negative index (%d)", index);
      return ERROR;
    }

  DEBUG("Inserting point (%.2f,%.2f) at index %d", new_x, new_y, index);

  // make room for new point
  for(i=(*number)-1; i>=index; i--)
    {
      intersections_x[i+1]=intersections_x[i];
      intersections_y[i+1]=intersections_y[i];
    }

  // store new coordinates
  intersections_x[index]=new_x;
  intersections_y[index]=new_y;

  (*number)++;

  return SUCCESS;
}

// try to insert the point (new_x, new_y) in the interesection 
// points arrays intersections_x and intersections_y (of length 'number');
// return SUCCESS on success, ERROR on error
int intersection_add_point(double *intersections_x, double *intersections_y, 
			   int *number, double new_x, double new_y)
{
  int ix, iy;

  if((*number)>=(MAX_SEGMENTS+1))
    {
      WARNING("Maximum number of segments (%d) exceeded", MAX_SEGMENTS);
      return ERROR;
    }

  // empty array => insert at the beginning
  if((*number)==0)
    {
      intersection_do_insert(intersections_x, intersections_y, number, 
			     new_x, new_y, 0);

      return SUCCESS;
    }
    
  for(ix=0; ix<(*number); ix++)
    {
      // if x coordinates are equal, we must check y coordinates
      if(intersections_x[ix]==new_x)
	{
	  for(iy=ix; iy<(*number); iy++)
	    // if y coordinates are also equal, ignore point
	    if(intersections_y[iy]==new_y)
	      {
		WARNING("Ignored identical intersection points");
		return SUCCESS;
	      }
	    // if y coordinate is larger, insert as previous item
	    else if(intersections_y[iy]>new_y)
	      {
		intersection_do_insert(intersections_x, intersections_y, 
				       number, new_x, new_y, iy);
		return SUCCESS;
	      }

	  // search finished without finding a smaller value =>
	  // insert at the end
	  if(iy==(*number))
	    {
	      intersection_do_insert(intersections_x, intersections_y, number, 
				     new_x, new_y, (*number));
	      return SUCCESS;
	    }
	}
      // if x coordinate is larger, insert as previous item
      else if(intersections_x[ix]>new_x)
	{
	  intersection_do_insert(intersections_x, intersections_y, number, 
				 new_x, new_y, ix);
	  return SUCCESS;
	}
    }

  // search finished without finding a smaller value =>
  // insert at the end
  if(ix==(*number))
    {
      intersection_do_insert(intersections_x, intersections_y, number, 
			     new_x, new_y, (*number));
      return SUCCESS;
    }

  return SUCCESS;
}

int point_on_segment(double x, double y, double sx1, double sy1, 
		     double sx2, double sy2)
{

  // check first that point are colinear
  if(fabs(x*(sy1-sy2)+sx1*(sy2-y)+sx2*(y-sy1))<EPSILON)
    //check whether (x,y) is really situated in between the segment ends
    if(((x>=(sx1-EPSILON) && x<=(sx2+EPSILON)) ||
	(x>=(sx2-EPSILON) && x<=(sx1+EPSILON))) &&
       ((y>=(sy1-EPSILON) && y<=(sy2+EPSILON)) || 
	(y>=(sy2-EPSILON) && y<=(sy1+EPSILON))))
      return TRUE;
    else
      return FALSE;
  else
    return FALSE;
}

// update the properties of an environment that is attached to a certain 
// connection, depending on the properties of the scenario
// (position of end-nodes of connection, obstacles, etc.);
// return SUCCESS on succes, ERROR on error
int environment_update(environment_class *environment, 
		       connection_class *connection, scenario_class *scenario)
{
  int object_index, env_index;
  int vertex_i, vertex_i2;

  int i;

  double x_intersect, y_intersect;
  double intersections_x[MAX_SEGMENTS+1];
  double intersections_y[MAX_SEGMENTS+1];
  int intersections_objects[MAX_SEGMENTS+1];
  double length;

  int number = 0;

  int increasing;

  node_class *from_node, *to_node; // shortcuts

  from_node = &(scenario->nodes[connection->from_node_index]);
  to_node = &(scenario->nodes[connection->to_node_index]);

  if(environment->is_dynamic==FALSE)
    {
      WARNING("Trying to update a non-dynamic environment ('%s')",
	      environment->name);
      return ERROR;
    }

#ifdef MESSAGE_DEBUG
  DEBUG("Array before adding 'from_node':");
  intersection_print(intersections_x, intersections_y, number);
#endif

  // add 'from_node' position
  intersection_add_point(intersections_x, intersections_y, &number,
			 (from_node->position).c[0],
			 (from_node->position).c[1]);
#ifdef MESSAGE_DEBUG
  DEBUG("Array before adding 'to_node':");
  intersection_print(intersections_x, intersections_y, number);
#endif

  // add 'to_node' position
  intersection_add_point(intersections_x, intersections_y, &number,
			 (to_node->position).c[0],
			 (to_node->position).c[1]);

#ifdef MESSAGE_DEBUG
  DEBUG("Array's final state:");
  intersection_print(intersections_x, intersections_y, number);
#endif

  if(number==1)
    {
      WARNING("No intersections found. Possbile node overlap. \
Using local environment of receiver node");

      // add the same point again so that a segment is formed
      intersection_do_insert(intersections_x, intersections_y, &number, 
			     intersections_x[number-1], 
			     intersections_y[number-1], 0);
      intersection_print(intersections_x, intersections_y, number);

      DEBUG("step 1");
    }

  /////////////////////////////////
  // determine intersection points
  for(object_index=0; object_index<scenario->object_number; object_index++)
    {
      int height_check_needed=FALSE;

      // first check whether the segment is higher than the object
      // by checking whether both ends are larger than object height
      if(from_node->position.c[2]>scenario->objects[object_index].height && 
	 to_node->position.c[2]>scenario->objects[object_index].height)
	continue;

      // check now whether at least one segment end is higher than the object
      if((from_node->position.c[2]>=scenario->objects[object_index].height && 
	  to_node->position.c[2]<scenario->objects[object_index].height) ||
	 (from_node->position.c[2]<scenario->objects[object_index].height && 
	  to_node->position.c[2]>=scenario->objects[object_index].height))
	height_check_needed = TRUE;

      // if one of the ends is smaller than the height we should compute 
      // 3D intersection of segment and object; however we assume only
      // a simplified case for the moment, in which a 2D intersection 
      // will suffice (segment is parallel with x0y plane)
      
      // determine intersection points
      // (including the edge [V[N-1],V[0]], even if the object is not 
      // a polygon, so as to avoid strange cases with unaccounted areas)
      for(vertex_i=0; vertex_i<
	    scenario->objects[object_index].vertex_number; vertex_i++)
	{
	  vertex_i2=
	    ((vertex_i+1)<scenario->objects[object_index].vertex_number)?
	    vertex_i+1:0;
	      
	  printf("Object edge (V[%d],V[%d])...\n", vertex_i, vertex_i2);
	  // check intersection with each edge
	  if(segment_intersect
	     (from_node->position.c[0], from_node->position.c[1],
	      to_node->position.c[0], to_node->position.c[1],
	      scenario->objects[object_index].vertices[vertex_i].c[0],
	      scenario->objects[object_index].vertices[vertex_i].c[1],
	      scenario->objects[object_index].vertices[vertex_i2].c[0], 
	      scenario->objects[object_index].vertices[vertex_i2].c[1], 
	      &x_intersect, &y_intersect)==TRUE)
	    {
	      // check whether we need to check the height as well
	      if(height_check_needed == TRUE)
		{
		  double min_height = 
		    (from_node->position.c[2]<to_node->position.c[2])?
		    from_node->position.c[2]:to_node->position.c[2];
		  double height_diff = 
		    fabs(from_node->position.c[2]-to_node->position.c[2]);
		  double total_length = 
		    sqrt(pow(from_node->position.c[0]-
			     to_node->position.c[0], 2)+
			 pow(from_node->position.c[1]-
			     to_node->position.c[1], 2));
		  double angle = atan(height_diff/total_length);
		  
		  double min_height_intersection1, min_height_intersection2;
		  double distance_to_intersection1, distance_to_intersection2;

		  DEBUG("from_node<->to_node (I): \
total_length=%f height_diff=%f angle=%f", total_length, height_diff, angle);

		  distance_to_intersection1 =
		    sqrt(pow(from_node->position.c[0]-x_intersect, 2)+
			 pow(from_node->position.c[1]-y_intersect, 2));
		  min_height_intersection1 = 
		    distance_to_intersection1*tan(angle) + min_height;

		  distance_to_intersection2 =
		    sqrt(pow(to_node->position.c[0]-x_intersect, 2)+
			 pow(to_node->position.c[1]-y_intersect, 2));
		  min_height_intersection2 = 
		    distance_to_intersection2*tan(angle) + min_height;

		  DEBUG("distance_to_intersection1=%f \
min_height_intersection1=%f distance_to_intersection2=%f \
min_height_intersection2=%f", 
			distance_to_intersection1, min_height_intersection1,
			distance_to_intersection2, min_height_intersection2);

		  // if the minimum height at which the intersection point 
		  // can be located is larger then object height, 
		  // then this intersection point can be discarded
		  if(min_height_intersection1>
		     scenario->objects[object_index].height &&
		     min_height_intersection2>
		     scenario->objects[object_index].height)
		    {
		      DEBUG("Ignored intersection point due to height \
differences");
		      continue;
		    }
		}

	      intersection_add_point(intersections_x, intersections_y, 
				     &number, x_intersect, y_intersect);
#ifdef MESSAGE_DEBUG
	      DEBUG("Added intersection point to array...");
	      intersection_print(intersections_x, intersections_y, number);
#endif
	      
	    }
	}
    }

  printf("Number of intersection points to process=%d\n", number);

  /////////////////////////////////////////////////////////
  // determine the object which corresponds to each segment
  for(i=0; i<number-1; i++)
    {
      printf("Processing intersection point %d\n", i);
      intersections_objects[i]=INVALID_INDEX;
      for(object_index=0; object_index<scenario->object_number; object_index++)
	{
	  object_print(&(scenario->objects[object_index]));
	  if(segment_in_object3d(intersections_x[i], intersections_y[i], 0,
				 intersections_x[i+1], intersections_y[i+1], 
				 0, &(scenario->objects[object_index]))==TRUE)
	    {
	      DEBUG("Segment [(%.2f,%.2f),(%.2f,%.2f)] in object '%s'",
		    intersections_x[i], intersections_y[i],
		    intersections_x[i+1], intersections_y[i+1],
		    scenario->objects[object_index].name);
	      
	      // this is the first object containing this segment
	      if(intersections_objects[i]==INVALID_INDEX)
		intersections_objects[i]=object_index;
	      // this segment is in multiple objects
	      // we consider that the "smallest" object is the one that
	      // determines the segment properties
	      else
		{
		  int vertex_number_in=0;
		  
		  for(vertex_i=0; 
		      vertex_i<scenario->objects[object_index].vertex_number; 
		      vertex_i++)
		    if(point_in_object3d
		       (scenario->objects[object_index].
			vertices[vertex_i].c[0],
			scenario->objects[object_index].
			vertices[vertex_i].c[1],
			scenario->objects[object_index].
			vertices[vertex_i].c[2],
			&(scenario->objects[intersections_objects[i]]))==TRUE)
		      vertex_number_in++;

		  // check if all points were included
		  if(vertex_number_in == 
		     scenario->objects[object_index].vertex_number)
		    // new object is included in previous object, so we change
		    // the saved index
		    {
		      DEBUG("Latest object is included in the previously \
found one => update inclusion information");
		      intersections_objects[i]=object_index;
		    }
		}
	    }
	}
    }

  ///////////////////////////////////
  // update environment fields
  env_index = 0;

  printf("Proc update=%d\n", number);

  if(intersections_x[0]==from_node->position.c[0] && 
     intersections_y[0]==from_node->position.c[1])
    {
      // we start from index 0 and go towards the end
      increasing = TRUE;
    }
  else if(intersections_x[0]==to_node->position.c[0] && 
     intersections_y[0]==to_node->position.c[1])
    {
      // start from index number-1 and go towards the beginning
      increasing = TRUE; // CHECK LOGIC!!!!  SAME AS ABOVE???????
    }
  else
    WARNING("ERROR regarding segment set ends");

  for(i=0; i<number-1; i++)
    {
      length =
	sqrt(pow(intersections_x[i] - intersections_x[i+1], 2)+
	     pow(intersections_y[i] - intersections_y[i+1], 2));

      INFO("Intersection segment index = %d", i);
      if(length<EPSILON)
	{
	  WARNING("Segment too short. Assigning length %.2f m", 
		  MINIMUM_DISTANCE);
	  environment->length[env_index] = MINIMUM_DISTANCE;
	  INFO("1len=%f", environment->length[env_index]);
	}
      else
	{
	  // since objects are in 3D, we use the angle of the segment 
	  // made by the end points and the horizontal plane to adjust 
	  // segment length
	  
	  // check first whether the height is the same
	  if(fabs(from_node->position.c[2]-to_node->position.c[2])<EPSILON)
	    environment->length[env_index] = length;
	  else
	    {
	      double min_height = (from_node->position.c[2]<
				   to_node->position.c[2])?
		from_node->position.c[2]:to_node->position.c[2];
	      double height_diff = 
		fabs(from_node->position.c[2]-to_node->position.c[2]);
	      double total_length = 
		sqrt(pow(from_node->position.c[0]-to_node->position.c[0], 2)+
		     pow(from_node->position.c[1]-to_node->position.c[1], 2));

	      double angle = atan(height_diff/total_length);

	      double min_height_intersection1, min_height_intersection2;
	      double distance_to_intersection1, distance_to_intersection2;

	      distance_to_intersection1 =
		sqrt(pow(from_node->position.c[0]-x_intersect, 2)+
		     pow(from_node->position.c[1]-y_intersect, 2));
	      min_height_intersection1 = 
		distance_to_intersection1*tan(angle) + min_height;

	      distance_to_intersection2 =
		sqrt(pow(to_node->position.c[0]-x_intersect, 2)+
		     pow(to_node->position.c[1]-y_intersect, 2));
	      min_height_intersection2 = 
		distance_to_intersection2*tan(angle) + min_height;

	      DEBUG("from_node<->to_node (II): total_length=%f \
height_diff==%f angle=%f", total_length, height_diff, angle);

	      // check that segment is fully contained in object's volume
	      if(min_height_intersection1<=
		 scenario->objects[intersections_objects[i]].height &&
		 min_height_intersection2<=
		 scenario->objects[intersections_objects[i]].height)
		environment->length[env_index]=length/cos(angle);

	      // otherwise one height is larger, so some scaling is needed
	      // (we ignore the case when both heights are larger, since
	      // those intersection points were already excluded)
	      else
		{
		  double min_height_intersection = 
		    (min_height_intersection1<min_height_intersection2)?
		    min_height_intersection1:min_height_intersection2;

		  environment->length[env_index]=
		    min_height_intersection/sin(angle);
		}
	    }
	}


      if(intersections_objects[i]!=INVALID_INDEX)
	{
	  object_class *object = 
	    &(scenario->objects[intersections_objects[i]]);
	  int obj_env_index = object->environment_index;
	  int num_ends_on_edges = 0;
	  
	  DEBUG("Copying parameters from environment %d", 
		obj_env_index);
	  
	  environment->alpha[env_index] =  
	    scenario->environments[obj_env_index].alpha[0];
	  environment->sigma[env_index] = 
	    scenario->environments[obj_env_index].sigma[0];
	  
	  INFO("2len=%f", environment->length[env_index]);

	  if(environment->length[env_index] != MINIMUM_DISTANCE)
	    {
	      // there are three cases for segment position wrt
	      // objects, and three realistic W computation alternatives:
	      // 1. segment is fully included in an object
	      //    => W_segment = 0 (no object walls interfere)
	      // 2. segment has one end on an object edge
	      //    => W_segment = W_object / 2 (1 object wall interferes)
	      // 3. segment has both ends on an object edges
	      //    => W_segment = W_object (2 object walls interferes)
	      for(vertex_i=0; vertex_i<object->vertex_number; vertex_i++)
		{
		  vertex_i2 = ((vertex_i+1)<object->vertex_number)?
		    vertex_i+1:0;
		  
		  printf("Object edge (V[%d],V[%d])...\n", vertex_i, 
			 vertex_i2);
		  
		  if(point_on_segment
		     (intersections_x[i],intersections_y[i],
		      object->vertices[vertex_i].c[0], 
		      object->vertices[vertex_i].c[1],
		      object->vertices[vertex_i2].c[0], 
		      object->vertices[vertex_i2].c[1]))
		    num_ends_on_edges++;

		  if(point_on_segment
		     (intersections_x[i+1],intersections_y[i+1],
		      object->vertices[vertex_i].c[0], 
		      object->vertices[vertex_i].c[1],
		      object->vertices[vertex_i2].c[0], 
		      object->vertices[vertex_i2].c[1]))
		    num_ends_on_edges++;
		}
				  
	      DEBUG("Number of segment ends on object edges = %d",
		    num_ends_on_edges); 
	  
	      if(num_ends_on_edges==0) // case 1.
		{
		  DEBUG("Segment in object. Ignoring walls");
		  environment->W[env_index] = 0;
		}
	      else if(num_ends_on_edges==1) // case 2.
		{
		  DEBUG("Segment has one end in object. \
Using wall attenuation x 1");
		  environment->W[env_index] = 
		    scenario->environments[obj_env_index].W[0];
		}
	      else // num_ends_on_edges==2 (case 3.), therefore
		{
		  DEBUG("Segment has both ends on object edges. \
Using wall attenuation x 2");
		  // multiply wall attenuation by 2
		  environment->W[env_index] = 
		    scenario->environments[obj_env_index].W[0] * 2;
		}
	    }
	  else
	    // for minimum distance segments use W = 0 since they are 
	    // virtual segments, even if located on an edge
	    environment->W[env_index] = 0;

	  environment->noise_power[env_index] = 
	    scenario->environments[obj_env_index].noise_power[0];
	}
      else
	{
	  WARNING("No environment information found. Using default \
parameters: alpha=%.2f, sigma=%.2f, W=%.2f, noise_power=%.2f", 
		  DEFAULT_ALPHA, DEFAULT_SIGMA, 
		  DEFAULT_W, DEFAULT_NOISE_POWER);
	  
	  environment->alpha[env_index] = DEFAULT_ALPHA;
	  environment->sigma[env_index] = DEFAULT_SIGMA;
	  environment->W[env_index] = DEFAULT_W;
	  environment->noise_power[env_index] = DEFAULT_NOISE_POWER;
	}
      env_index++;
    }

  // update field in environment structure
  environment->num_segments = env_index;

  return SUCCESS;
}

// check whether a newly defined environment conflicts with existing ones;
// return TRUE if no duplicate environment is found, FALSE otherwise
int environment_check_valid(environment_class *environments, 
			    int environment_number, 
			    environment_class *new_environment)
{
  int i;
  for(i=0; i<environment_number; i++)
    {
      if(strcmp(environments[i].name, new_environment->name)==0)
	{
	  WARNING("Environment with name '%s' already defined", 
		  new_environment->name);
	  return FALSE;
	}
    }

  return TRUE;
}
