
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
 * File name: xml_scenario.c
 * Function:  Load an XML scenario into memory in the scenario structure
 *            defined in deltaQ.h
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 141 $
 *   $LastChangedDate: 2009-03-27 17:30:45 +0900 (Fri, 27 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <expat.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "deltaQ.h"
#include "motion.h"
#include "message.h"
#include "wlan.h"
#include "active_tag.h"
#include "zigbee.h"
#include "stack.h"

#include "xml_scenario.h"


////////////////////////////////////////
// XML elements functions
////////////////////////////////////////

// initialize a node object from XML attributes;
// flag 'cartesian_coord_syst' specifies whether
// cartesian coordinates are used or not;
// return SUCCESS on succes, ERROR on error
int xml_node_init(node_class *node, const char **attributes, 
		  int cartesian_coord_syst)
{
  int i;

  // used to store the return value of long_int_value
  long int long_int_result;

  // used to store the return value of double_value
  double double_result;

  // flags showing whether required information was provided
  int name_provided=FALSE, id_provided=FALSE;

  // make sure all parameters are initialized to default values
  node_init(node, DEFAULT_NODE_NAME, DEFAULT_NODE_TYPE, DEFAULT_NODE_ID, 
	    DEFAULT_NODE_SSID, DEFAULT_NODE_CONNECTION, 
	    DEFAULT_NODE_ADAPTER_TYPE, DEFAULT_NODE_ANTENNA_GAIN,
	    DEFAULT_NODE_COORDINATE, DEFAULT_NODE_COORDINATE, 
	    DEFAULT_NODE_COORDINATE, DEFAULT_NODE_PT, 
	    DEFAULT_NODE_INTERNAL_DELAY);

  // parse all possible attributes of a node
  for(i=0; attributes[i]; i+=2)
    if(strcmp(attributes[i], NODE_NAME_STRING)==0)
      {
	strncpy(node->name, attributes[i+1], MAX_STRING-1);
	name_provided = TRUE;
      }
    else if(strcmp(attributes[i], NODE_TYPE_STRING)==0)
      {
	if(strcmp(attributes[i+1], NODE_TYPE_REGULAR_STRING)==0)
	  node->type = REGULAR_NODE;
	else if(strcmp(attributes[i+1], NODE_TYPE_ACCESS_POINT_STRING)==0)
	  node->type = ACCESS_POINT_NODE;
	else
	  {
	    WARNING("Node attribute '%s' is assigned an invalid value ('%s')", 
		    NODE_TYPE_STRING, attributes[i+1]);
	    return ERROR;
	  }
      }
    else if(strcmp(attributes[i], NODE_ID_STRING)==0)
      {
	long_int_result = long_int_value(attributes[i+1]);
	if(long_int_result == LONG_MIN)
	  return ERROR;
	else
	  if((long_int_result>=INT_MIN) && (long_int_result<=INT_MAX))
	    node->id = (int) long_int_result;
	  else
	    {
	      WARNING("Node attribute '%s' has a value outside the 'int' \
range ('%ld')", NODE_ID_STRING, long_int_result);
	      return ERROR;
	    }

	// check the value is correct
	if(node->id<0)
	  {
	    WARNING("Node attribute '%s' must be a positive integer. \
Ignored negative value ('%s')", NODE_ID_STRING, attributes[i+1]);
	    node->id = 0;
	    return ERROR;
	  }
	else
	  id_provided = TRUE;
      }
    else if(strcmp(attributes[i], NODE_SSID_STRING)==0)
	strncpy(node->ssid, attributes[i+1], MAX_STRING-1);
    else if (strcmp(attributes[i], NODE_CONNECTION_STRING)==0)
      {
	if(strcmp(attributes[i+1], NODE_CONNECTION_AD_HOC_STRING)==0)
	  node->connection = AD_HOC_CONNECTION;
	else if(strcmp(attributes[i+1], 
			   NODE_CONNECTION_INFRASTRUCTURE_STRING)==0)
	  node->connection = INFRASTRUCTURE_CONNECTION;
	else if(strcmp(attributes[i+1], NODE_CONNECTION_ANY_STRING)==0)
	  node->connection = ANY_CONNECTION;
	else
	  {
	    WARNING("Node attribute '%s' is assigned an invalid value ('%s')", 
		    NODE_CONNECTION_STRING, attributes[i+1]);
	    return ERROR;
	  }
      }
    else if(strcmp(attributes[i], NODE_ADAPTER_STRING)==0)
      {
	if(strcmp(attributes[i+1], NODE_ADAPTER_ORINOCO_STRING)==0)
	  node->adapter_type = ORINOCO;
	if(strcmp(attributes[i+1], NODE_ADAPTER_DEI80211MR_STRING)==0)
	  node->adapter_type = DEI80211MR;
	else if(strcmp(attributes[i+1], NODE_ADAPTER_CISCO_340_STRING)==0)
	  node->adapter_type = CISCO_340;
	else if(strcmp(attributes[i+1], NODE_ADAPTER_CISCO_ABG_STRING)==0)
	  node->adapter_type = CISCO_ABG;
	else if(strcmp(attributes[i+1], NODE_ADAPTER_JENNIC_STRING)==0)
	  node->adapter_type = JENNIC;
	else if(strcmp(attributes[i+1], NODE_ADAPTER_S_NODE_STRING)==0)
	  node->adapter_type = S_NODE;
	else
	  {
	    WARNING("Node attribute '%s' is assigned an invalid value ('%s')", 
		    NODE_ADAPTER_STRING, attributes[i+1]);
	    return ERROR;
	  }
      }
    else if(strcmp(attributes[i], NODE_ANTENNA_GAIN_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  node->antenna_gain = double_result;
      }
    else if(strcmp(attributes[i], NODE_AZIMUTH_ORIENTATION_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  node->azimuth_orientation = double_result;
      }
    else if(strcmp(attributes[i], NODE_AZIMUTH_BEAMWIDTH_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  node->azimuth_beamwidth = double_result;
      }
    else if(strcmp(attributes[i], NODE_ELEVATION_ORIENTATION_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  node->elevation_orientation = double_result;
      }
    else if(strcmp(attributes[i], NODE_ELEVATION_BEAMWIDTH_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  node->elevation_beamwidth = double_result;
      }
    else if(strcmp(attributes[i], NODE_X_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  node->position.c[0] = double_result;
      }
    else if(strcmp(attributes[i], NODE_Y_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  node->position.c[1] = double_result;
      }
    else if(strcmp(attributes[i], NODE_Z_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  node->position.c[2] = double_result;
      }
    else if(strcmp(attributes[i], NODE_PT_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  node->Pt = double_result;
      }
    else if(strcmp(attributes[i], NODE_INTERNAL_DELAY_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  node->internal_delay = double_result;

	// check the value is correct
	if(node->internal_delay<0)
	  {
	    WARNING("Node attribute '%s' must be a positive double. \
Ignored negative value ('%s')", NODE_INTERNAL_DELAY_STRING, attributes[i+1]);
	    node->internal_delay = 0;
	    return ERROR;
	  }
      }
    else
      {
	WARNING("Unknown XML attribute for element '%s': '%s'", 
		NODE_STRING, attributes[i]);
	return ERROR;
      }

  if(name_provided == FALSE)
    {
      WARNING("Node attribute '%s' is mandatory!", NODE_NAME_STRING);
      return ERROR;
    }

  if(id_provided == FALSE)
    {
      WARNING("Node attribute '%s' is mandatory!", NODE_ID_STRING);
      return ERROR;
    }

  // if coordinate system is not cartesian, we convert
  // latitude & longitude to x & y before storing
  if(cartesian_coord_syst == FALSE)
    {
      coordinate_class point_xyz;
      
      DEBUG("Converting node coordinates from BLH to XYZ...");

      // transform lat & long to x & y
      ll2en(&(node->position), &point_xyz);
		  
      // copy converted data
      coordinate_copy(&(node->position), &point_xyz);
    }

  // update the Pr0 field of the node
  wlan_node_update_Pr0(node);
  active_tag_node_update_Pr0(node);
  zigbee_node_update_Pr0(node);

  return SUCCESS;
}

// initialize a topology object from XML attributes
// flag 'cartesian_coord_syst' specifies whether
// cartesian coordinates are used or not;
// return SUCCESS on succes, ERROR on error
int xml_object_init(object_class *object, const char **attributes,
		    int cartesian_coord_syst)
{
  int i;

  // flags showing whether required information was provided  
  int coordinate_x1_provided = FALSE;
  int coordinate_y1_provided = FALSE;
  int coordinate_x2_provided = FALSE;
  int coordinate_y2_provided = FALSE;

  // used to store the return value of double_value
  double double_result;

  // make sure all parameters are initialized to default values
  /*
  object_init(object, DEFAULT_OBJECT_NAME, DEFAULT_ENVIRONMENT_NAME, 
	      DEFAULT_ENVIRONMENT_COORD_1,  DEFAULT_ENVIRONMENT_COORD_1,  
	      DEFAULT_ENVIRONMENT_COORD_2,  DEFAULT_ENVIRONMENT_COORD_2);
  */

  object_init(object, DEFAULT_OBJECT_NAME, DEFAULT_ENVIRONMENT_NAME);

  // parse all possible attributes of a node
  for (i=0; attributes[i]; i+=2)
    if(strcmp(attributes[i], OBJECT_NAME_STRING)==0)
      strncpy(object->name, attributes[i+1], MAX_STRING-1);
    else if(strcmp(attributes[i], OBJECT_TYPE_STRING)==0)
      {
	if(strcmp(attributes[i+1], OBJECT_TYPE_BUILDING_STRING)==0)
	  object->type = BUILDING_OBJECT;
	else if(strcmp(attributes[i+1], OBJECT_TYPE_ROAD_STRING)==0)
	  object->type = ROAD_OBJECT;
	else
	  {
	    WARNING("Object attribute '%s' is assigned an invalid value \
('%s')", OBJECT_TYPE_STRING, attributes[i+1]);
	    return ERROR;
	  }
      }
    else if(strcmp(attributes[i], OBJECT_MAKE_POLYGON_STRING)==0)
      {
	if(strcmp(attributes[i+1], TRUE_STRING)==0)
	  object->make_polygon=TRUE;
	else if(strcmp(attributes[i+1], FALSE_STRING)==0)
	  object->make_polygon=FALSE;
	else
	  {
	    WARNING("Object attribute '%s' is assigned an invalid value \
('%s')", OBJECT_MAKE_POLYGON_STRING, attributes[i+1]);
	    return ERROR;
	  }
      }
    else if(strcmp(attributes[i], OBJECT_ENVIRONMENT_STRING)==0)
      strncpy(object->environment, attributes[i+1], MAX_STRING-1);
    else if(strcmp(attributes[i], OBJECT_X1_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  {
	   coordinate_x1_provided = TRUE;

	   // add coordinates to object assuming horizontal x axis 
	   // and vertical y axis, and that vertices are numbered 
	   // from bottom-left corner in clockwise manner
	   object->vertex_number = 4;
	   object->vertices[0].c[0] = double_result;
	   object->vertices[1].c[0] = double_result;
	  }
      }
    else if(strcmp(attributes[i], OBJECT_Y1_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  {
	   coordinate_y1_provided = TRUE;

	   // add coordinates to object assuming horizontal x axis 
	   // and vertical y axis, and that vertices are numbered 
	   // from bottom-left corner in clockwise manner
	   object->vertex_number = 4;
	   object->vertices[0].c[1] = double_result;
	   object->vertices[3].c[1] = double_result;
	  }
      }
    else if(strcmp(attributes[i], OBJECT_X2_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  {
	   coordinate_x2_provided = TRUE;

	   // add coordinates to object assuming horizontal x axis 
	   // and vertical y axis, and that vertices are numbered 
	   // from bottom-left corner in clockwise manner
	   object->vertex_number = 4;
	   object->vertices[2].c[0] = double_result;
	   object->vertices[3].c[0] = double_result;

	  }
      }
    else if(strcmp(attributes[i], OBJECT_Y2_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  {
	   coordinate_y2_provided = TRUE;

	   // add coordinates to object assuming horizontal x axis 
	   // and vertical y axis, and that vertices are numbered 
	   // from bottom-left corner in clockwise manner
	   object->vertex_number = 4;
	   object->vertices[1].c[1] = double_result;
	   object->vertices[2].c[1] = double_result;
	  }
      }
    else if(strcmp(attributes[i], OBJECT_HEIGHT_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  object->height = double_result;
      }
    else if(strcmp(attributes[i], OBJECT_LOAD_FROM_JPGIS_FILE_STRING)==0)
      {
	if(strcmp(attributes[i+1], TRUE_STRING)==0)
	  object->load_from_jpgis_file=TRUE;
	else if(strcmp(attributes[i+1], FALSE_STRING)==0)
	  object->load_from_jpgis_file=FALSE;
	else
	  {
	    WARNING("Object attribute '%s' is assigned an invalid value \
('%s')", OBJECT_LOAD_FROM_JPGIS_FILE_STRING, attributes[i+1]);
	    return ERROR;
	  }
      }
    else
      {
	WARNING("Unknown XML attribute for element '%s': '%s'", 
		OBJECT_STRING, attributes[i]);
	return ERROR;
      }

  /*
  // check whether coordinates were provided
  if((coordinate_x1_provided == FALSE || coordinate_y1_provided == FALSE ||
     coordinate_x2_provided == FALSE || coordinate_y2_provided == FALSE) &&
     object->is_3d==FALSE)
    {
      WARNING("Object attributes '%s', '%s', '%s', and '%s' are all \
mandatory!", OBJECT_X1_STRING, OBJECT_Y1_STRING, 
	      OBJECT_X2_STRING, OBJECT_Y2_STRING);
      return ERROR;
    }

  // if coordinate system is not cartesian, we convert
  // latitude & longitude to x & y before storing
  if(object->is_3d==FALSE && cartesian_coord_syst==FALSE)
    {
      coordinate_class point_blh, point_xyz;
      
      // copy coordinates to structure
      point_blh.c[0] = object->x[0];
      point_blh.c[1] = object->y[0];

      // transform lat & long to x & y
      ll2en(&point_blh, &point_xyz);
		    
      // copy converted data
      object->x[0] = point_xyz.c[0];
      object->y[0] = point_xyz.c[1];


      // copy coordinates to structure
      point_blh.c[0] = object->x[1];
      point_blh.c[1] = object->y[1];

      // transform lat & long to x & y
      ll2en(&point_blh, &point_xyz);
		    
      // copy converted data
      object->x[1] = point_xyz.c[0];
      object->y[1] = point_xyz.c[1];
    }
  */

  return SUCCESS;
}

// initialize an environment object from XML attributes
// return SUCCESS on succes, ERROR on error
int xml_environment_init(environment_class *environment, 
			  const char **attributes)
{
  int i;

  // used to store the return value of double_value
  double double_result;

  // flag showing whether required information was provided  
  int name_provided = FALSE;

  // make sure all parameters are initialized to default values
  environment_init(environment, DEFAULT_ENVIRONMENT_NAME, 
		   DEFAULT_ENVIRONMENT_TYPE, DEFAULT_ENVIRONMENT_ALPHA, 
		   DEFAULT_ENVIRONMENT_SIGMA, 
		   DEFAULT_ENVIRONMENT_WALL_ATTENUATION,
		   DEFAULT_ENVIRONMENT_NOISE_POWER,
		   DEFAULT_ENVIRONMENT_COORD_1,  DEFAULT_ENVIRONMENT_COORD_1,  
		   DEFAULT_ENVIRONMENT_COORD_2,  DEFAULT_ENVIRONMENT_COORD_2);
  
  // parse all possible attributes of an environment
  for (i=0; attributes[i]; i+=2)
    if(strcmp(attributes[i], ENVIRONMENT_NAME_STRING)==0)
      {
	strncpy(environment->name, attributes[i+1], MAX_STRING-1);
	environment->name_hash=string_hash(environment->name, 
					   strlen(environment->name));
	name_provided = TRUE;
      }
    else if(strcmp(attributes[i], ENVIRONMENT_TYPE_STRING)==0)
      strncpy(environment->type, attributes[i+1], MAX_STRING-1);
    else if(strcmp(attributes[i], ENVIRONMENT_IS_DYNAMIC_STRING)==0)
      {
	if(strcmp(attributes[i+1], TRUE_STRING)==0)
	  environment->is_dynamic=TRUE;
	else if(strcmp(attributes[i+1], FALSE_STRING)==0)
	  environment->is_dynamic=FALSE;
	else
	  {
	    WARNING("Environment attribute '%s' is assigned an invalid \
value ('%s')", ENVIRONMENT_IS_DYNAMIC_STRING, attributes[i+1]);
	    return ERROR;
	  }
      }
    else if(strcmp(attributes[i], ENVIRONMENT_ALPHA_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  environment->alpha[0] = double_result;

	// check the value is correct
	if(environment->alpha[0]<0)
	  {
	    WARNING("Environment attribute '%s' must be a positive double. \
Ignored negative value ('%s')", ENVIRONMENT_ALPHA_STRING, attributes[i+1]);
	    environment->alpha[0] = 0;
	    return ERROR;
	  }
      }
    else if(strcmp(attributes[i], ENVIRONMENT_SIGMA_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  environment->sigma[0] = double_result;

	// check the value is correct
	if(environment->sigma[0]<0)
	  {
	    WARNING("Environment attribute '%s' must be a positive double. \
Ignored negative value ('%s')", ENVIRONMENT_SIGMA_STRING, attributes[i+1]);
	    environment->sigma[0] = 0;
	    return ERROR;
	  }
      }
    else if(strcmp(attributes[i], ENVIRONMENT_W_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  environment->W[0] = double_result;

	// check the value is correct
	if(environment->W[0]<0)
	  {
	    WARNING("Environment attribute '%s' must be a positive double. \
Ignored negative value ('%s')", ENVIRONMENT_W_STRING, attributes[i+1]);
	    environment->W[0] = 0;
	    return ERROR;
	  }
      }
    else if(strcmp(attributes[i], ENVIRONMENT_NOISE_POWER_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  environment->noise_power[0] = double_result;
      }
    else
      {
	WARNING("Unknown XML attribute for element '%s': '%s'", 
		ENVIRONMENT_STRING, attributes[i]);
	return ERROR;
      }

  if(name_provided == FALSE)
    {
      WARNING("Environment attribute '%s' is mandatory!", 
	      ENVIRONMENT_NAME_STRING);
      return ERROR;
    }

#ifdef MESSAGE_DEBUG
  DEBUG("Creating the following environment:"); 
  environment_print(environment);
#endif

  return SUCCESS;
}

// initialize a motion object from XML attributes
// flag 'cartesian_coord_syst' specifies whether
// cartesian coordinates are used or not;
// return SUCCESS on succes, ERROR on error
int xml_motion_init(motion_class *motion, const char **attributes, 
		    int cartesian_coord_syst)
{
  int i;

  // used to store the return value of double_value
  double double_result;

  // flag showing whether required information was provided  
  int node_name_provided = FALSE;
  int speed_x_provided = FALSE;
  int speed_y_provided = FALSE;
  int speed_z_provided = FALSE;
  int destination_x_provided = FALSE;
  int destination_y_provided = FALSE;
  int destination_z_provided = FALSE;

  int center_x_provided = FALSE;
  int center_y_provided = FALSE;

  // make sure all parameters are initialized to default values
  motion_init(motion, DEFAULT_MOTION_NODE_NAME, DEFAULT_MOTION_TYPE, 
	      DEFAULT_MOTION_SPEED, DEFAULT_MOTION_SPEED, DEFAULT_MOTION_SPEED,
	      DEFAULT_MOTION_CENTER, DEFAULT_MOTION_CENTER,
	      DEFAULT_MOTION_SPEED, DEFAULT_MOTION_SPEED, 
	      DEFAULT_MOTION_MAX_SPEED, DEFAULT_MOTION_WALK_TIME, 
	      DEFAULT_MOTION_TIME, DEFAULT_MOTION_TIME);
  
  // parse all possible attributes of a motion
  for (i=0; attributes[i]; i+=2)
    if(strcmp(attributes[i], MOTION_NODE_NAME_STRING)==0)
      {
	strncpy(motion->node_name, attributes[i+1], MAX_STRING-1);
	node_name_provided = TRUE;
      }
    else if(strcmp(attributes[i], MOTION_TYPE_STRING)==0)
      {
	if(strcmp(attributes[i+1], MOTION_TYPE_LINEAR_STRING)==0)
	  motion->type = LINEAR_MOTION;
	else if(strcmp(attributes[i+1], MOTION_TYPE_CIRCULAR_STRING)==0)
	  motion->type = CIRCULAR_MOTION;
	else if(strcmp(attributes[i+1], MOTION_TYPE_ROTATION_STRING)==0)
	  motion->type = ROTATION_MOTION;
	else if(strcmp(attributes[i+1], MOTION_TYPE_RANDOM_WALK_STRING)==0)
	  motion->type = RANDOM_WALK_MOTION;
	else if(strcmp(attributes[i+1], MOTION_TYPE_BEHAVIORAL_STRING)==0)
	  motion->type = BEHAVIORAL_MOTION;
	else
	  {
	    WARNING("Motion attribute '%s' is assigned an invalid value \
('%s')", MOTION_TYPE_STRING, attributes[i+1]);
	    return ERROR;
	  }
      }
    else if(strcmp(attributes[i], MOTION_CENTER_X_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  {
	    motion->center.c[0] = double_result;
	    center_x_provided = TRUE;
	  }
      }
    else if(strcmp(attributes[i], MOTION_CENTER_Y_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  {
	    motion->center.c[1] = double_result;
	    center_y_provided = TRUE;
	  }
      }
    else if(strcmp(attributes[i], MOTION_SPEED_X_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  {
	    motion->speed.c[0] = double_result;
	    speed_x_provided = TRUE;
	  }
      }
    else if(strcmp(attributes[i], MOTION_SPEED_Y_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  {
	    motion->speed.c[1] = double_result;
	    speed_y_provided = TRUE;
	  }
      }
    else if(strcmp(attributes[i], MOTION_SPEED_Z_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  {
	    motion->speed.c[2] = double_result;
	    speed_z_provided = TRUE;
	  }
      }
    else if(strcmp(attributes[i], MOTION_VELOCITY_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  motion->velocity = double_result;
      }
    else if(strcmp(attributes[i], MOTION_START_TIME_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  motion->start_time = double_result;
      }
    else if(strcmp(attributes[i], MOTION_STOP_TIME_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  motion->stop_time = double_result;
      }
    else if(strcmp(attributes[i], MOTION_MIN_SPEED_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  motion->min_speed = double_result;
      }
    else if(strcmp(attributes[i], MOTION_MAX_SPEED_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  motion->max_speed = double_result;
      }
    else if(strcmp(attributes[i], MOTION_WALK_TIME_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  motion->walk_time = double_result;
      }
    else if(strcmp(attributes[i], MOTION_DESTINATION_X_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  {
	    motion->destination.c[0] = double_result;
	    destination_x_provided = TRUE;
	  }
      }
    else if(strcmp(attributes[i], MOTION_DESTINATION_Y_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  {
	    motion->destination.c[1] = double_result;
	    destination_y_provided = TRUE;
	  }
      }
    else if(strcmp(attributes[i], MOTION_DESTINATION_Z_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  {
	    motion->destination.c[2] = double_result;
	    destination_z_provided = TRUE;
	  }
      }
    else if(strcmp(attributes[i], 
		       MOTION_ROTATION_ANGLE_HORIZONTAL_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  motion->rotation_angle_horizontal = double_result;
      }
    else if(strcmp(attributes[i], 
		       MOTION_ROTATION_ANGLE_VERTICAL_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  motion->rotation_angle_vertical = double_result;
      }
    else 
      {
	WARNING("Unknown XML attribute for element '%s': '%s'", 
		MOTION_STRING, attributes[i]);
	return ERROR;
      }

  if(node_name_provided == FALSE)
    {
      WARNING("Motion attribute 'node_name' is mandatory!");
      return ERROR;
    }

  // speed was not initialized, therefore compute it based on destination
  // and motion time
  if(motion->type == LINEAR_MOTION)
    {
      if(speed_x_provided==FALSE && speed_y_provided==FALSE &&
	 speed_z_provided==FALSE && 
	 destination_x_provided==FALSE && destination_y_provided==FALSE &&
	 destination_z_provided==FALSE)
	{
	  WARNING("For linear motion, either 'speed_x/y/z' or \
'destination_x/y/z' atributes must be initialized");
	  return ERROR;
	}
      else if
	(speed_x_provided==TRUE && speed_y_provided==TRUE &&
	 speed_z_provided==TRUE && 
	 destination_x_provided==TRUE && destination_y_provided==TRUE &&
	  destination_z_provided==TRUE)
	{
	  WARNING("For linear motion, not both 'speed_x/y/z' and \
'destination_x/y/z' can be initialized");
	  return ERROR;
	}
      else if
	(speed_x_provided==FALSE && speed_y_provided==FALSE &&
	 speed_z_provided==FALSE && 
	 destination_x_provided==TRUE && destination_y_provided==TRUE &&
	 destination_z_provided==TRUE)
	motion->speed_from_destination = TRUE;
      else if 
	(speed_x_provided==TRUE && speed_y_provided==TRUE &&
	 speed_z_provided==TRUE &&
	 destination_x_provided==FALSE && destination_y_provided==FALSE &&
	 destination_z_provided==FALSE)
	motion->speed_from_destination = FALSE;
      else
	{
	  WARNING("For linear motion, either 'speed_x/y/z' or \
'destination_x/y/z' have to be provided");
	  return ERROR;
	}
    }
  else if(motion->type == CIRCULAR_MOTION)
    {
      if(center_x_provided==FALSE || center_y_provided==FALSE)
	{
	  WARNING("For circular motion, attributes 'center_x/y' \
must be initialized");
	  return ERROR;
	}
    }
  else if(motion->type == BEHAVIORAL_MOTION)
    {
      if(destination_x_provided==FALSE || destination_y_provided==FALSE ||
	 destination_z_provided==FALSE)
	{
	  WARNING("For behavioral motion, attributes 'destination_x/y/z' \
must be initialized");
	  return ERROR;
	}
      else
	motion->speed_from_destination = TRUE;
    }


  // if coordinate system is not cartesian, and destination will be
  // used to define motion, then we convert latitude & longitude to x
  // & y before storing destination (this has to be done for linear and 
  // behavioral motions)
  if(cartesian_coord_syst==FALSE && motion->speed_from_destination==TRUE)
    {
      coordinate_class point_xyz;
      
      // transform lat & long to x & y
      ll2en(&(motion->destination), &point_xyz);
		    
      // copy converted data
      coordinate_copy(&(motion->destination), &point_xyz);
    }

  // if coordinate system is not cartesian, and the center for circular motion 
  // was used to define motion, then we convert latitude & longitude to x
  // & y before storing the center coordinates
  if(cartesian_coord_syst==FALSE && motion->type==CIRCULAR_MOTION)
    {
      coordinate_class point_xyz;
      
      // transform lat & long to x & y
      ll2en(&(motion->center), &point_xyz);
		    
      // copy converted data
      coordinate_copy(&(motion->center), &point_xyz);
    }


#ifdef MESSAGE_DEBUG
  DEBUG("Creating the following motion:");
  motion_print(motion);
#endif

  return SUCCESS;
}

// initialize a connection object from XML attributes
// return SUCCESS on succes, ERROR on error
int xml_connection_init(connection_class *connection, const char **attributes)
{
  int i;

  // flags showing whether required information was provided  
  int from_node_provided = FALSE;
  int to_node_provided = FALSE;//, to_nodes_provided = FALSE;
  int through_environment_provided = FALSE;

  // used to store the return value of long_int_value
  long int long_int_result;

  // used to store the return value of double_value
  double double_result;

  // make sure all parameters are initialized to default values 
  if(connection_init(connection, DEFAULT_CONNECTION_PARAMETER, 
		     DEFAULT_CONNECTION_PARAMETER, 
		     DEFAULT_CONNECTION_PARAMETER,
		     DEFAULT_CONNECTION_PACKET_SIZE, 
		     DEFAULT_CONNECTION_STANDARD,
		     UNDEFINED_VALUE,// channel value is set later because
		                     // it depends on standard!!!
		     DEFAULT_CONNECTION_RTS_CTS_THRESHOLD,
		     DEFAULT_CONNECTION_CONSIDER_INTERFERENCE)==ERROR)
    return ERROR;

  // parse all possible attributes of a connection
  for (i=0; attributes[i]; i+=2)
    if(strcmp(attributes[i], CONNECTION_FROM_NODE_STRING)==0)
      {
	strncpy(connection->from_node, attributes[i+1], MAX_STRING-1);
	from_node_provided = TRUE;
      }
    else if(strcmp(attributes[i], CONNECTION_TO_NODE_STRING)==0)
      {
	strncpy(connection->to_node, attributes[i+1], MAX_STRING-1);
	to_node_provided = TRUE;
      }
    else if(strcmp(attributes[i], 
		       CONNECTION_THROUGH_ENVIRONMENT_STRING)==0)
      {
	strncpy(connection->through_environment, attributes[i+1], 
		MAX_STRING-1);
	through_environment_provided = TRUE;
      }
    else if(strcmp(attributes[i], CONNECTION_PACKET_SIZE_STRING)==0)
      {
	long_int_result = long_int_value(attributes[i+1]);
	if(long_int_result == LONG_MIN)
	  return ERROR;
	else
	  if((long_int_result>=INT_MIN) && (long_int_result<=INT_MAX))
	    connection->packet_size = (int) long_int_result;
	  else
	    {
	      WARNING("Connection attribute '%s' has a value outside the \
'int' range ('%ld')", CONNECTION_PACKET_SIZE_STRING, long_int_result);
	      return ERROR;
	    }

	// check the value is correct
	if(connection->packet_size<0)
	  {
	    WARNING("Connection attribute '%s' must be a positive integer. \
Ignored negative value ('%s')", CONNECTION_PACKET_SIZE_STRING, 
		    attributes[i+1]);
	    connection->packet_size = DEFAULT_CONNECTION_PACKET_SIZE;
	    return ERROR;
	  }
      }
    else if(strcmp(attributes[i], CONNECTION_STANDARD_STRING)==0)
      {
	if(strcmp(attributes[i+1], CONNECTION_STANDARD_802_11B_STRING)==0)
	  {
	    if(connection_init_standard(connection, WLAN_802_11B)==ERROR)
	      return ERROR;
	  }
	else if(strcmp(attributes[i+1], 
			   CONNECTION_STANDARD_802_11G_STRING)==0)
	  {
	    if(connection_init_standard(connection, WLAN_802_11G)==ERROR)
	      return ERROR;
	  }
	else if(strcmp(attributes[i+1], 
			   CONNECTION_STANDARD_802_11A_STRING)==0)
	  {
	    if(connection_init_standard(connection, WLAN_802_11A)==ERROR)
	      return ERROR;
	  }
	else if(strcmp(attributes[i+1], 
			   CONNECTION_STANDARD_ETH_10_STRING)==0)
	  {
	    if(connection_init_standard(connection, ETHERNET_10)==ERROR)
	      return ERROR;
	  }
	else if(strcmp(attributes[i+1], 
			   CONNECTION_STANDARD_ETH_100_STRING)==0)
	  {
	    if(connection_init_standard(connection, ETHERNET_100)==ERROR)
	      return ERROR;
	  }
	else if(strcmp(attributes[i+1], 
			   CONNECTION_STANDARD_ETH_1000_STRING)==0)
	  {
	    if(connection_init_standard(connection, ETHERNET_1000)==ERROR)
	      return ERROR;
	  }
	else if(strcmp(attributes[i+1], 
			   CONNECTION_STANDARD_ACTIVE_TAG_STRING)==0)
	  {
	    if(connection_init_standard(connection, ACTIVE_TAG)==ERROR)
	      return ERROR;
	  }
	else if(strcmp(attributes[i+1], 
			   CONNECTION_STANDARD_ZIGBEE_STRING)==0)
	  {
	    if(connection_init_standard(connection, ZIGBEE)==ERROR)
	      return ERROR;
	  }
	else
	  {
	    WARNING("Connection attribute '%s' is assigned an invalid value \
('%s')", CONNECTION_STANDARD_STRING, attributes[i+1]);
	    return ERROR;
	  }
      }
    else if(strcmp(attributes[i], CONNECTION_CHANNEL_STRING)==0)
      {
	long_int_result = long_int_value(attributes[i+1]);
	if(long_int_result == LONG_MIN)
	  return ERROR;
	else
	  if((long_int_result>=INT_MIN) && (long_int_result<=INT_MAX))
	    connection->channel = (int) long_int_result;
	  else
	    {
	      WARNING("Connection attribute '%s' has a value outside the \
'int' range ('%ld')", CONNECTION_CHANNEL_STRING, long_int_result);
	      return ERROR;
	    }

	// check the value is correct
	// (channel values should depend on connection standard !!!!!!!!)
	if(connection->channel<MIN_CHANNEL || connection->channel>MAX_CHANNEL)
	  {
	    WARNING("Connection attribute '%s' must be between %d and %d. \
Ignored invalid value ('%s')", CONNECTION_CHANNEL_STRING, MIN_CHANNEL,
		    MAX_CHANNEL, attributes[i+1]);
	    connection->channel = MIN_CHANNEL;//??????????????????????????
	    return ERROR;
	  }
      }
    else if(strcmp(attributes[i], CONNECTION_RTS_CTS_THRESHOLD_STRING)==0)
      {
	long_int_result = long_int_value(attributes[i+1]);
	if(long_int_result == LONG_MIN)
	  return ERROR;
	else
	  if((long_int_result>=INT_MIN) && (long_int_result<=INT_MAX))
	    connection->RTS_CTS_threshold = (int) long_int_result;
	  else
	    {
	      WARNING("Connection attribute '%s' has a value outside the \
'int' range ('%ld')", CONNECTION_RTS_CTS_THRESHOLD_STRING, long_int_result);
	      return ERROR;
	    }

	if(connection->RTS_CTS_threshold<0 || 
	   connection->RTS_CTS_threshold>MAX_PSDU_SIZE_AG)
	  {
	    WARNING("Connection attribute '%s' must be between 0 and %d. \
Ignored invalid value ('%s')", CONNECTION_RTS_CTS_THRESHOLD_STRING, 
		    MAX_PSDU_SIZE_AG, attributes[i+1]);
	    connection->RTS_CTS_threshold = 
	      DEFAULT_CONNECTION_RTS_CTS_THRESHOLD;
	    return ERROR;
	  }
      }
    else if(strcmp(attributes[i], CONNECTION_BANDWIDTH_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  connection->bandwidth = double_result;

	// check if value is correct
	if(connection->bandwidth<0 || connection->bandwidth>1e9)
	  {
	    WARNING("Connection attribute '%s' must be between 0 and 10^9 bps.\
 Ignored invalid value ('%s')", CONNECTION_BANDWIDTH_STRING, attributes[i+1]);
	    connection->bandwidth = 1e9;//??????????????????????????
	    return ERROR;
	  }
	else
	  connection->bandwidth_defined = TRUE;
      }
    else if(strcmp(attributes[i], CONNECTION_LOSS_RATE_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  connection->loss_rate = double_result;

	// check if value is correct
	if(connection->loss_rate<0 || connection->loss_rate>1)
	  {
	    WARNING("Connection attribute '%s' must be between 0 and 1. \
Ignored invalid value ('%s')", CONNECTION_LOSS_RATE_STRING, attributes[i+1]);
	    return ERROR;
	  }
	else
	  connection->loss_rate_defined = TRUE;
      }
    else if(strcmp(attributes[i], CONNECTION_DELAY_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  connection->delay = double_result;

	// check if value is correct
	if(connection->delay<0)
	  {
	    WARNING("Connection attribute '%s' must be positive. Ignored \
invalid value ('%s')", CONNECTION_DELAY_STRING, attributes[i+1]);
	    return ERROR;
	  }
	else
	  {
	    connection->variable_delay = connection->delay; //WHY????????????
	    connection->delay_defined = TRUE;
	  }
      }
    else if(strcmp(attributes[i], CONNECTION_JITTER_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  connection->jitter = double_result;

	// check if value is correct
	if(connection->jitter<0)
	  {
	    WARNING("Connection attribute '%s' must be positive. Ignored \
invalid value ('%s')", CONNECTION_JITTER_STRING, attributes[i+1]);
	    return ERROR;
	  }
	else
	  connection->jitter_defined = TRUE;
      }
    else if(strcmp(attributes[i], 
		       CONNECTION_CONSIDER_INTERFERENCE_STRING)==0)
      {
	if(strcmp(attributes[i+1], TRUE_STRING)==0)
	  connection->consider_interference=TRUE;
	else if(strcmp(attributes[i+1], FALSE_STRING)==0)
	  connection->consider_interference=FALSE;
	else
	  {
	    WARNING("Connection attribute '%s' must be either '%s' or \
'%s'. Ignored unrecognized value ('%s')", 
		    CONNECTION_CONSIDER_INTERFERENCE_STRING, 
		    TRUE_STRING, FALSE_STRING, attributes[i+1]);
	    return ERROR;
	  }
      }
   else 
     {
       WARNING("Unknown XML attribute for element '%s': '%s'",
	       CONNECTION_STRING, attributes[i]);
       return ERROR;
     }
  
  if(from_node_provided == FALSE)
    {
      WARNING("Connection attribute 'from_node' is mandatory!");
      return ERROR;
    }

  if(to_node_provided == FALSE)
    {
      WARNING("Connection attribute 'to_node' is mandatory!");
      return ERROR;
    }

  if(through_environment_provided == FALSE)
    {
      WARNING("Connection attribute 'through_environment' is mandatory!");
      return ERROR;
    }

#ifdef MESSAGE_DEBUG
  DEBUG("Creating the following connection:"); 
  connection_print(connection);
#endif

  return SUCCESS;
}

// initialize an xml_scenario object from XML attributes
// return SUCCESS on succes, ERROR on error
int xml_scenario_init(xml_scenario_class *xml_scenario, 
		      const char **attributes)
{
  int i;

  // used to store the return value of double_value
  double double_result;

  xml_scenario->start_time = 0;
  xml_scenario->duration = 0;
  // use a non-zero value to prevent an infinite loop 
  // in the case of non-initialization by user
  xml_scenario->step = 0.5;

  // by default deltaQ and motion calculation are done 
  // with the same step
  xml_scenario->motion_step_divider = 1.0;

  // default is to use cartesian coordinates
  xml_scenario->cartesian_coord_syst = TRUE;

  // reset flag regarding JPGIS filename
  xml_scenario->jpgis_filename_provided = FALSE;
  
  // no init required for xml_scenario.scenario, or read elements 
  // will be deleted; "scenario_init(&(xml_scenario.scenario))" is 
  // called in the initial part of the main function in "qomet.c"

  // parse all possible attributes of a scenario
  for (i=0; attributes[i]; i+=2)
    if(strcmp(attributes[i], XML_SCENARIO_START_TIME_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  xml_scenario->start_time = double_result;
      }
    else if(strcmp(attributes[i], XML_SCENARIO_DURATION_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  xml_scenario->duration = double_result;

	// check if value is correct
	if(xml_scenario->duration<0)
	  {
	    WARNING("Scenario attribute '%s' must be a positive double. \
Ignored negative value ('%s')", XML_SCENARIO_DURATION_STRING, attributes[i+1]);
	    xml_scenario->duration = 0;
	    return ERROR;
	  }
      }
    else if(strcmp(attributes[i], XML_SCENARIO_STEP_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  xml_scenario->step = double_result;

	// check if value is correct
	if(xml_scenario->step<0)
	  {
	    WARNING("Scenario attribute '%s' must be a positive double. \
Ignored negative value ('%s')", XML_SCENARIO_STEP_STRING, attributes[i+1]);
	    xml_scenario->step = 0.5;
	    return ERROR;
	  }
      }
    else if(strcmp(attributes[i], XML_SCENARIO_MOTION_STEP_DIVIDER_STRING)==0)
      {
	double_result = double_value(attributes[i+1]);
	if(double_result == -HUGE_VAL)
	  return ERROR;
	else
	  xml_scenario->motion_step_divider = double_result;

	// check if value is correct
	if(xml_scenario->step<0)
	  {
	    WARNING("Scenario attribute '%s' must be a positive double. \
Ignored negative value ('%s')", XML_SCENARIO_MOTION_STEP_DIVIDER_STRING, 
		    attributes[i+1]);
	    xml_scenario->motion_step_divider = 1.0;
	    return ERROR;
	  }
      }
    else if(strcmp(attributes[i], XML_SCENARIO_COORD_SYST_STRING)==0)
      {
	if(strcmp(attributes[i+1], 
		      XML_SCENARIO_COORD_SYST_CARTESIAN_STRING)==0)
	  xml_scenario->cartesian_coord_syst = TRUE;
	else if(strcmp(attributes[i+1], 
			   XML_SCENARIO_COORD_SYST_LAT_LON_ALT_STRING)==0)
	  xml_scenario->cartesian_coord_syst = FALSE;
	else
	  {
	    WARNING("Scenario attribute '%s' is assigned an invalid value\
('%s')", XML_SCENARIO_COORD_SYST_STRING, attributes[i+1]);
	    return ERROR;
	  }
      }
    else if(strcmp(attributes[i], XML_SCENARIO_JPGIS_FILENAME_STRING)==0)
      {
	strncpy(xml_scenario->jpgis_filename, attributes[i+1], MAX_STRING-1);
	xml_scenario->jpgis_filename_provided = TRUE;
      }
    else 
      {
	WARNING("Unknown XML attribute for element '%s': '%s'", 
		XML_SCENARIO_STRING, attributes[i]);
	return ERROR;
      }

  return SUCCESS;
}

// print the main properties of an XML scenario
void xml_scenario_print(xml_scenario_class *xml_scenario)
{
  scenario_print(&(xml_scenario->scenario));
  printf("\tstart_time=%.2f duration=%.2f step=%.2f\n", 
	 xml_scenario->start_time, xml_scenario->duration, 
	 xml_scenario->step);
  printf("\tcoordinate system=%s jpgis_filename=%s\n",
	 (xml_scenario->cartesian_coord_syst==TRUE)?
	 XML_SCENARIO_COORD_SYST_CARTESIAN_STRING:
	 XML_SCENARIO_COORD_SYST_LAT_LON_ALT_STRING,
	 (xml_scenario->jpgis_filename_provided==TRUE)?
	 xml_scenario->jpgis_filename:DEFAULT_STRING);
}

/////////////////////////////////////////////////
// Specific XML parsing functions
/////////////////////////////////////////////////

// this function is used whenever a new element is encountered.
// here it is used to read the scenario file, therefore it needs
// to differentiate between the different elements
static void XMLCALL start_element(void *data, const char *element, 
				  const char **attributes)
{
  // get a pointer to user data in our XML object
  xml_scenario_class *xml_scenario =(xml_scenario_class *)data;

  // shortcut to scenario object
  scenario_class *scenario = &(xml_scenario->scenario);

  // temporary storage for new nodes, objects, envrionments,
  // motions, connections
  node_class node;
  object_class object;
  environment_class environment;
  motion_class motion;
  connection_class connection;

  void *element_ptr;


#ifdef MESSAGE_DEBUG
  // print XML tree

  int i;

  DEBUG("Printing XML element...");
  // print depth
  for (i=0; i<xml_depth; i++)
    printf(">");

  printf(">ELEMENT: %s\n", element);

  if(attributes[0]!=NULL)// some attributes  
    {
      printf("\tATTRIBUTES:");
      for (i=0; attributes[i]; i+=2)
	printf(" %s='%s'", attributes[i], attributes[i+1]);
    }
  else // no attributes
    printf("\tNO ATTRIBUTES");
  printf("\n");
#endif

  // initialize internal buffer for character data for each element
  // (otherwise character data is propagated between elements)
  strcpy(xml_scenario->buffer, "");


  // check if we are at top level
  if(xml_depth==0)
    //check we have the right type of document
    if(strcmp(element, XML_SCENARIO_STRING)==0)
      {
	INFO("Parsing QOMET definition XML file...");

	// initialize stack
	stack_init(&(xml_scenario->stack));

	stack_push(&(xml_scenario->stack), xml_scenario, ELEMENT_XML_SCENARIO);
	if(xml_scenario_init(xml_scenario, attributes)==ERROR)
	  xml_scenario->xml_parse_error = TRUE;
      }
    else
      {
	WARNING("Expected top-level entity '%s' not found. XML file \
is not of 'QOMET definition' type!", XML_SCENARIO_STRING);
	xml_scenario->xml_parse_error = TRUE;
      }


  // not top level, so look for element definitions
  else if(strcmp(element, NODE_STRING)==0) // check if node
    {
      DEBUG("Node element found");

      if(xml_node_init(&node, attributes,
		       xml_scenario->cartesian_coord_syst)==ERROR)
	xml_scenario->xml_parse_error = TRUE;
      else 
	if(node_check_valid(scenario->nodes, scenario->node_number,
			    &node)==TRUE)
	  {
	    if((element_ptr=scenario_add_node(scenario, &node))==NULL)
	      xml_scenario->xml_parse_error = TRUE;
	  }
	else
	  xml_scenario->xml_parse_error = TRUE;

      stack_push(&(xml_scenario->stack), element_ptr, ELEMENT_NODE);

    }
  else if (strcmp(element, OBJECT_STRING)==0) // check if topology object
    {
      DEBUG("Object element found");

      if(xml_object_init(&object, attributes,
		       xml_scenario->cartesian_coord_syst)==ERROR)
	xml_scenario->xml_parse_error = TRUE;
      else
	if(object_check_valid(scenario->objects, scenario->object_number,
			      &object, 
			      xml_scenario->jpgis_filename_provided)==TRUE)
	  {
	    if((element_ptr=scenario_add_object(scenario, &object))==NULL)
	      xml_scenario->xml_parse_error = TRUE;
	  }
	else
	  xml_scenario->xml_parse_error = TRUE;

      stack_push(&(xml_scenario->stack), element_ptr, ELEMENT_OBJECT);
    }
  else if (strcmp(element, COORDINATE_STRING)==0) // check if coordinate
    {
      DEBUG("Coordinate element found");

      // coordinates are not stored directly in scenario, therefore
      // a special one unit storage in xml_scenario is used while parsing
      stack_push(&(xml_scenario->stack), &(xml_scenario->coordinate), 
		 ELEMENT_COORDINATE);
    }
  else if (strcmp(element, ENVIRONMENT_STRING)==0) // check if environment
    {
      DEBUG("Environment element found");

      if(xml_environment_init(&environment, attributes)==ERROR)
	xml_scenario->xml_parse_error = TRUE;
      else 
	if(environment_check_valid(scenario->environments, 
				   scenario->environment_number,
				   &environment)==TRUE)
	  {
	    if((element_ptr=
		scenario_add_environment(scenario, &environment))==NULL)
	      xml_scenario->xml_parse_error = TRUE;
	  }
	else
	  xml_scenario->xml_parse_error = TRUE;

      stack_push(&(xml_scenario->stack), element_ptr, ELEMENT_ENVIRONMENT);
    }
  else if (strcmp(element, MOTION_STRING)==0) // check if motion
    {
      DEBUG("Motion element found");

      if(xml_motion_init(&motion, attributes,
		       xml_scenario->cartesian_coord_syst)==ERROR)
	xml_scenario->xml_parse_error = TRUE;
      else
	if((element_ptr=scenario_add_motion(scenario, &motion))==NULL)
	  xml_scenario->xml_parse_error = TRUE;

      stack_push(&(xml_scenario->stack), element_ptr, ELEMENT_MOTION);
    }
  else if (strcmp(element, CONNECTION_STRING)==0) //check if connection
    {
      DEBUG("Connection element found");

      if(xml_connection_init(&connection, attributes)==ERROR)
	xml_scenario->xml_parse_error = TRUE;
      else
	if((element_ptr=scenario_add_connection(scenario, &connection))==NULL)
	  xml_scenario->xml_parse_error = TRUE;

      stack_push(&(xml_scenario->stack), element_ptr, ELEMENT_CONNECTION);
    }
  else // unknown element
    {
      WARNING("Unknown XML element found ('%s')", element);
      xml_scenario->xml_parse_error = TRUE;
    }

  // other actions go BEFORE this
  xml_depth++;
}

// this function is executed when an element ends
static void XMLCALL end_element(void *data, const char *element)
{
  // get a pointer to user data in our XML object
  xml_scenario_class *xml_scenario =(xml_scenario_class *)data;

  element_class *stack_element;

  // other actions go AFTER this
  xml_depth--;

  // pop the element on the stack
  stack_element = stack_pop(&(xml_scenario->stack));

  // if during parsing some error was encountered, ignore
  // further processing
  if(xml_scenario->xml_parse_error==TRUE)
    return;

  if(stack_element == NULL)
    {
      WARNING("Error in stack management");
      xml_scenario->xml_parse_error = TRUE;
      return;
    }
      
  switch(stack_element->element_type)
    {
    case ELEMENT_XML_SCENARIO: INFO("xml_scenario element ended"); break;
    case ELEMENT_NODE:
      {
#ifdef MESSAGE_DEBUG
	node_class *node=(node_class *)stack_element->element;
#endif

	INFO("Node element ended"); 
#ifdef MESSAGE_DEBUG
	DEBUG("Creating the following node:");
	node_print(node);
#endif
	break;
      }
    case ELEMENT_OBJECT:
      {
#ifdef MESSAGE_DEBUG
	object_class *object=(object_class *)stack_element->element;
#endif

	INFO("Object element ended"); 
#ifdef MESSAGE_DEBUG
	DEBUG("Creating the following object:");
	object_print(object);
#endif
	break;
      }
    case ELEMENT_COORDINATE: 
      {
	int count=0;
	coordinate_class *coordinate=
	  (coordinate_class *)stack_element->element;

	// store result of double_value
	double double_result;

	char *token_string, *saved_pointer;
	element_class *stack_top_element;

	INFO("Coordinate element ended"); 
	
	// parse numbers from text (only two coordinates for now)
	while(count<COORDINATE_NUMBER)
	  {
	    if(count == 0)//first time
	      token_string = 
		strtok_r(xml_scenario->buffer, " ", &saved_pointer);
	    else
	      token_string = strtok_r(NULL, " ", &saved_pointer);
	    
	    if(token_string == NULL)
	      break;
	    else
	      {
		double_result = double_value(token_string);
		if(double_result == -HUGE_VAL)
		  {
		    xml_scenario->xml_parse_error = TRUE;
		    break;
		  }
		else
		  if(count==0)
		    coordinate->c[0] = double_result;
		  else if(count==1)
		    coordinate->c[1] = double_result;
		  else
		    coordinate->c[2] = double_result;
		count++;
	      }
	  }

	if(count<2)
	  {
	    WARNING("At least 2 coordinates must be provided. \
Only height/z coordinate are optional");
	    xml_scenario->xml_parse_error = TRUE;
	  }
	else if(count == 2)
	  // if height / z coordinate was not provided,
	  // initialize it to 0
	  coordinate->c[2] = 0;

	// clean up
	strcpy(xml_scenario->buffer, "");

	// try to add this data to the element on top
	// of the stack, which should be an object element
	// pop the element on the stack
	stack_top_element = stack_top(&(xml_scenario->stack));

	if(stack_top_element!=NULL && 
	   stack_top_element->element_type != ELEMENT_OBJECT)
	  {
	    WARNING("Coordinate element not embedded in an object element");
	    xml_scenario->xml_parse_error = TRUE;
	  }
	else
	  {
	    object_class *object=(object_class*)stack_top_element->element;

	    DEBUG("Adding coordinate as vertex %d of object %s",
		  object->vertex_number, object->name);

	    if(object->vertex_number<MAX_VERTICES)
	      {
		coordinate_class point_xyz;

		// if coordinate system is not cartesian we convert
		// latitude & longitude to x & y before storing
		if(xml_scenario->cartesian_coord_syst == FALSE)
		  {
		    ll2en(coordinate, &point_xyz);
		    
		    // store converted data
		    coordinate_copy
		      (&(object->vertices[object->vertex_number]), &point_xyz);
		  }
		else
		  // store coordinates directly
		  coordinate_copy
		    (&(object->vertices[object->vertex_number]), coordinate);

		object->vertex_number++;
	      }
	    else
	      {
		WARNING("Maximum number of vertices in object exceeded (%d)",
			MAX_VERTICES);
		xml_scenario->xml_parse_error = TRUE;
	      }
	  }

	break;
      }
    case ELEMENT_ENVIRONMENT: INFO("Environment element ended"); break;
    case ELEMENT_MOTION: INFO("Motion element ended"); break;
    case ELEMENT_CONNECTION: INFO("Connection element ended"); break;
    default: 
      {
	WARNING("Unknown element ended (element_type=%d)", 
		stack_element->element_type);
	xml_scenario->xml_parse_error = TRUE;
      }
    }

  // check state is correct
  if(xml_depth==0 && !stack_is_empty(&(xml_scenario->stack)))
    {
      WARNING("Internal stack not empty after finishing parsing");
      xml_scenario->xml_parse_error = TRUE;
    }
}

static void XMLCALL character_data (void *data, const XML_Char *string, 
				    int length)
{
  // get a pointer to user data in our XML object
  xml_scenario_class *xml_scenario =(xml_scenario_class *)data;

  /*
  element_class *stack_element;

  // get the top element on the stack
  stack_element = stack_pop(&(xml_scenario->stack));

  switch(stack_element->element_type)
    {
      case ELEMENT_COORDINATE:...
	default: WARNING
    }
  */
  
  // discard end of line characters; is it ok?
  if(string[0]=='\n' && length == 1)
    return;

  if(strlen(xml_scenario->buffer)+length<MAX_STRING)
    {
      //printf("--DEB (bef): '%s'\n", xml_scenario->buffer);
      strncat(xml_scenario->buffer, (char *)string, length);
      //printf("DEB (after): '%s'\n", xml_scenario->buffer);
    }
  else
    {
      WARNING("Cannot parse character data because of buffer overflow");
      xml_scenario->xml_parse_error = TRUE;
    }
}


// parse scenario file and store results in scenario object
// return SUCCESS on succes, ERROR on error
int xml_scenario_parse(FILE *scenario_file, xml_scenario_class *xml_scenario)
{
  int parsing_done;
  int buffer_length;

  // create the parser
  XML_Parser xml_parser = XML_ParserCreate(NULL);
  if(!xml_parser) 
    {
      WARNING("Could not allocate memory for parser");
      return ERROR;
    }

  // set the element handlers 'start' and 'end'
  XML_SetElementHandler(xml_parser, start_element, end_element);

  // set the character handler
  XML_SetCharacterDataHandler(xml_parser, character_data);

  // WHY WAS IT COMMENTED OUT??????
  // TAKE CARE!!!!!!!!!!!!!!!!!!!!!!!!!
  // initialize error status
  xml_scenario->xml_parse_error = FALSE;

  // initialize internal buffer for character data
  // (not really needed because functions using this
  // buffer initialize it as well)
  strcpy(xml_scenario->buffer, "");

  // set the user data pointer to the scenario object
  XML_SetUserData(xml_parser, xml_scenario);

  // parsing loop
  do
    {
      // read one buffer
      buffer_length = (int)fread(xml_buffer, 1, BUFFER_SIZE, scenario_file);
      
      // check for read error
      if(ferror(scenario_file))
	{
	  WARNING("Read error while parsing");
	  return ERROR;
	}
      
      // check for end of file
      parsing_done = feof(scenario_file);

      // check for parse error
      if(XML_Parse(xml_parser, xml_buffer, buffer_length, parsing_done) 
	  == XML_STATUS_ERROR)
	{
	  WARNING("Syntax XML parse error in input file at line #%" 
		  XML_FMT_INT_MOD "u: \"%s\"", 
		  (long unsigned int) XML_GetCurrentLineNumber(xml_parser),
		  XML_ErrorString(XML_GetErrorCode(xml_parser)));
	  return ERROR;
	}
      else if(xml_scenario->xml_parse_error == TRUE)
	{
	  WARNING("User XML parse error in input file (see the WARNING \
messages above)");
	  return ERROR;
	}
    }
  while(parsing_done==0);

  return SUCCESS;
}
