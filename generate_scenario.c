
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
 * File name: generate_scenario.c
 * Function: Source file for automatically generating QOMET scenarios
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/

#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "deltaQ.h" 

//#define JPGIS_EXPERIMENT
#define SMALL_EXPERIMENT
//#define LARGE_EXPERIMENT

#define START_AREA_EPSILON       0.1

// JPGIS experiment
#ifdef JPGIS_EXPERIMENT

#define SCENARIO_STEP             0.5

//#define MULT_F                    1e8
//#define SUB_LAT                   3554361000.0
//#define SUB_LON                   13968884000.0

#define NUMBER_NODES              100
#define SCENARIO_DURATION         45.0

#define NUMBER_BUILDINGS          7

char *building_names[]={
  "K18_19217",
  "K18_3887",
  "K18_20127",
  "K18_4579",
  "K18_17893",
  "K18_4019",
  //"K18_4017",//
  "K18_16615"};

#define NUMBER_SOURCES            4
#define NUMBER_DESTINATIONS       1

#define NODE_BASE_NAME            "node"
#define BUILDING_ENV_NAME         "building_env"


#define MOTION_TYPE               "behavioral"

#define MOTION_START_TIME         0.0
#define MOTION_STOP_TIME          SCENARIO_DURATION
#define MOTION_USE_SPEED          FALSE
//#define MOTION_MIN_SPEED          0.0
#define MOTION_MAX_SPEED          2.0
#define MOTION_AVG_SPEED          1.34

#endif


// small experiment
#ifdef SMALL_EXPERIMENT

#define SCENARIO_STEP             0.5
#define MOTION_STEP_DIVIDER       1.0

#define NUMBER_NODES              100
#define SCENARIO_START            -30.0
#define SCENARIO_DURATION         1050.0 //430.0 //230.0

#define NUMBER_BUILDINGS          4
#define NUMBER_SOURCES            4
#define NUMBER_DESTINATIONS       1

#define NODE_BASE_NAME            "node"
#define BUILDING_ENV_NAME         "building_env"

#define MOTION_TYPE               "behavioral"
#define MOTION_START_TIME         0.0
#define MOTION_START_OFFSET       8.0
#define MOTION_STOP_TIME          SCENARIO_DURATION
#define MOTION_USE_SPEED          FALSE
//#define MOTION_MIN_SPEED          0.0
#define MOTION_MAX_SPEED          2.0
#define MOTION_AVG_SPEED          1.34

#endif

// large experiment
#ifdef LARGE_EXPERIMENT

#define SCENARIO_STEP             0.5

#define NUMBER_NODES              1000
#define SCENARIO_DURATION         1500.0

#define NUMBER_BUILDINGS          4
#define NUMBER_SOURCES            4
#define NUMBER_DESTINATIONS       1

#define NODE_BASE_NAME            "node"
#define BUILDING_ENV_NAME         "building_env"

#define MOTION_TYPE               "behavioral"
#define MOTION_START_TIME         0.0
#define MOTION_STOP_TIME          SCENARIO_DURATION
#define MOTION_USE_SPEED          FALSE
//#define MOTION_MIN_SPEED          0.0
#define MOTION_MAX_SPEED          2.0
#define MOTION_AVG_SPEED          1.34

#endif


#define OUT(message...) do {                                        \
    fprintf(stdout, message); fprintf(stdout,"\n");		    \
  } while(0)

// generate an x coordinate located in a given object
// (actually the rectangular region containing the object);
// return the coordinate
double generate_x_in_object(object_class *object)
{
  double min_x=DBL_MAX, max_x=-DBL_MAX;

  int i;

  if(object->vertex_number == 0)
    {
      fprintf(stderr, "Object has no vertices\n");
      return 0.0;
    }
 
  for(i=0; i<object->vertex_number; i++)
    {
      if((object->vertices[i]).c[0]<min_x)
	min_x = object->vertices[i].c[0];
      if((object->vertices[i]).c[0]>max_x)
	max_x = object->vertices[i].c[0];
    }

  //fprintf(stderr, "min_x =%f  max_x= %f\n", min_x, max_x);

  return rand_min_max(min_x, max_x);
}

// generate a y coordinate located in a given object;
// return the coordinate
double generate_y_in_object(object_class *object)
{
  double min_y=DBL_MAX, max_y=-DBL_MAX;

  int i;

  if(object->vertex_number == 0)
    {
      fprintf(stderr, "Object has no vertices\n");
      return 0.0;
    }
 
  for(i=0; i<object->vertex_number; i++)
    {
      if((object->vertices[i]).c[1]<min_y)
	min_y = object->vertices[i].c[1];
      if((object->vertices[i]).c[1]>max_y)
	max_y = object->vertices[i].c[1];
    }

  //fprintf(stderr, "min_y =%f  max_y= %f\n", min_y, max_y);

  return rand_min_max(min_y, max_y);
}


// NOT USED YET!!!!!!!!!!!!!!!!
// and pass it back as a function parameter;
// return SUCCESS on success, ERROR on error
int generate_point_in_object3d(object_class *object, coordinate_class *point)
{
  int vertex_i;

  // for the moment we return the point which is the average of the
  // base shape vertices (internal point for convex polygons) 
  // and located at height/2; 
  // more complex algorithms will be implemented in the future
  // (see http://local.wasp.uwa.edu.au/~pbourke/geometry/insidepoly/)
  
  point->c[0] = 0;
  point->c[1] = 0;
  point->c[2] = 0;
  for(vertex_i=0; vertex_i<object->vertex_number; vertex_i++)
    {
      point->c[0] += object->vertices[vertex_i].c[0];
      point->c[1] += object->vertices[vertex_i].c[1];
    }
  
  point->c[0] /= object->vertex_number;
  point->c[1] /= object->vertex_number;
  point->c[2] = object->height / 2;
  
  return SUCCESS;
}

// main function
int main()
{
  node_class nodes[NUMBER_NODES];

#ifndef JPGIS_EXPERIMENT
  object_class buildings[NUMBER_BUILDINGS];
#endif

  object_class start_areas[NUMBER_SOURCES];
  object_class destination_areas[NUMBER_DESTINATIONS];

  environment_class building_env;

  int i;
  time_t crt_time;
  char *ctime_result;

  char node_name[MAX_STRING];
  int nodes_per_area[]={0, 0, 0, 0};


  ///////////////////////////////////////////
  // Initialize structures
  //////////////////////////////////////////

#ifdef JPGIS_EXPERIMENT

  // initialize start areas
  object_init_rectangle(&(start_areas[0]), "NO_NAME", "NO_ENVIRONMENT",
	      35.543585961, 139.688818869, 
	      35.544054230, 139.688828869);
  object_init_rectangle(&(start_areas[1]), "NO_NAME", "NO_ENVIRONMENT",
	      35.544044230, 139.688818869, 
	      35.544054230, 139.689377519);
  object_init_rectangle(&(start_areas[2]), "NO_NAME", "NO_ENVIRONMENT",
	      35.543585961, 139.689367519, 
	      35.544054230, 139.689377519);
  object_init_rectangle(&(start_areas[3]), "NO_NAME", "NO_ENVIRONMENT",
	      35.543585961, 139.688818869, 
	      35.543595961, 139.689377519);

  // initialize destination area
  object_init_rectangle(&(destination_areas[0]), "NO_NAME", "NO_ENVIRONMENT",
	      35.54381,139.68908, 35.54383, 139.68910);
#endif

#ifdef SMALL_EXPERIMENT
  // initialize objects
  object_init_rectangle(&(buildings[0]), "bldg0", BUILDING_ENV_NAME,
	      10.0, 10.0, 95.0, 95.0);
  object_init_rectangle(&(buildings[1]), "bldg1", BUILDING_ENV_NAME,
	      10.0, 105.0, 95.0, 190.0);
  object_init_rectangle(&(buildings[2]), "bldg2", BUILDING_ENV_NAME,
	      105.0, 105.0, 190.0, 190.0);
  object_init_rectangle(&(buildings[3]), "bldg3", BUILDING_ENV_NAME,
	      105.0, 10.0, 190.0, 95.0);

  // initialize start areas
  object_init_rectangle(&(start_areas[0]), "start0", "NO_ENVIRONMENT",
	      0.0, 10.0, 10.0, 190.0);
  object_init_rectangle(&(start_areas[1]), "start1", "NO_ENVIRONMENT",
	      0.0, 190.0, 200.0, 200.0);
  object_init_rectangle(&(start_areas[2]), "start2", "NO_ENVIRONMENT",
	      190.0, 10.0, 200.0, 190.0);
  object_init_rectangle(&(start_areas[3]), "start3", "NO_ENVIRONMENT",
	      0.0, 0.0, 200.0, 10.0);

  // initialize destination area
  object_init_rectangle(&(destination_areas[0]), "dest", "NO_ENVIRONMENT",
	      95, 95, 105, 105);
#endif

#ifdef LARGE_EXPERIMENT
  // initialize objects
  object_init_rectangle(&(buildings[0]), "bldg0", BUILDING_ENV_NAME,
	      100.0, 100.0, 950.0, 950.0);
  object_init_rectangle(&(buildings[1]), "bldg1", BUILDING_ENV_NAME,
	      100.0, 1050.0, 950.0, 1900.0);
  object_init_rectangle(&(buildings[2]), "bldg2", BUILDING_ENV_NAME,
	      1050.0, 1050.0, 1900.0, 1900.0);
  object_init_rectangle(&(buildings[3]), "bldg3", BUILDING_ENV_NAME,
	      1050.0, 100.0, 1900.0, 950.0);

  // initialize start areas
  object_init_rectangle(&(start_areas[0]), "start0", "NO_ENVIRONMENT",
	      0.0, 100.0, 100.0, 1900.0);
  object_init_rectangle(&(start_areas[1]), "start1", "NO_ENVIRONMENT",
	      0.0, 1900.0, 2000.0, 2000.0);
  object_init_rectangle(&(start_areas[2]), "start2", "NO_ENVIRONMENT",
	      1900.0, 100.0, 2000.0, 1900.0);
  object_init_rectangle(&(start_areas[3]), "start3", "NO_ENVIRONMENT",
	      0.0, 0.0, 2000.0, 100.0);

  // initialize destination area
  object_init_rectangle(&(destination_areas[0]), "dest", "NO_ENVIRONMENT",
	      950, 950, 1050, 1050);
#endif

  // initialize nodes (depends on source and destination areas)
  for(i=0; i<NUMBER_NODES; i++)
    {
      int area_id;

      // create node name
      snprintf(node_name, MAX_STRING-1, "%s%03d", NODE_BASE_NAME, i);

      // first quarter of nodes starts from start_areas[0]
      if(i<NUMBER_NODES/4)
	area_id = 0;
      // second quarter of nodes starts from start_areas[1]
      else if(i<NUMBER_NODES/2)
	area_id = 1;
      // third quarter of nodes starts from start_areas[2]
      else if(i<3*NUMBER_NODES/4)
	area_id = 2;
      // forth quarter of nodes starts from start_areas[3]
      else
	area_id = 3;

      nodes_per_area[area_id]++;

      node_init(&(nodes[i]), node_name, 0, i, "NO_SSID", 0, 0, 0,
		generate_x_in_object(&(start_areas[area_id])), 
		generate_y_in_object(&(start_areas[area_id])), 
		0.0, 0, 0);
    }

  // print initialization summary
  fprintf(stderr, "Nodes initialized as follows: area0 -> %d nodes; \
area1 -> %d nodes; area2 -> %d nodes; area3 -> %d nodes\n", 
	  nodes_per_area[0], nodes_per_area[1], 
	  nodes_per_area[2], nodes_per_area[3]);

  // initialize environments
  environment_init(&building_env, BUILDING_ENV_NAME, "NO_TYPE", 
		   2.0, 0.0, 0.0, -100.0, 0, 0, 0, 0);
  

  ///////////////////////////////////////////
  // Create output
  //////////////////////////////////////////

  //output scenario header
  time(&crt_time);
  ctime_result = ctime(&crt_time);
  // replace the ending '\n' by '\0' for nicer output
  ctime_result[strlen(ctime_result)-1]='\0';
  OUT("\n<!-- Scenario file for QOMET v1.3; auto-generated on %s -->",
        ctime_result);

#ifdef JPGIS_EXPERIMENT
  // output scenario parameters
  OUT("\n<qomet_scenario duration=\"%.3f\" step=\"%.3f\" \
coordinate_system=\"lat_lon_alt\" \
jpgis_filename=\"kawasaki-area-1_utf8.xml\">", 
	 SCENARIO_DURATION, SCENARIO_STEP);

  // output building objects
  OUT("\n<!-- Objects -->");
  for(i=0; i<NUMBER_BUILDINGS; i++)
    {
      OUT("  <object name=\"%s\" environment=\"%s\" \
load_from_jpgis_file=\"true\"/>", building_names[i],BUILDING_ENV_NAME);
    }

#else

  // output scenario parameters
  OUT("\n<qomet_scenario  start_time=\"%.3f\" duration=\"%.3f\" step=\"%.3f\" \
motion_step_divider=\"%.3f\">", SCENARIO_START, SCENARIO_DURATION, 
      SCENARIO_STEP, MOTION_STEP_DIVIDER);

  // output building objects
  OUT("\n<!-- Objects -->");
  for(i=0; i<NUMBER_BUILDINGS; i++)
    {
      OUT("  <object name=\"%s\" environment=\"%s\" x1=\"%.3f\" y1=\"%.3f\" \
x2=\"%.3f\" y2=\"%.3f\"/>", buildings[i].name, buildings[i].environment,
	  buildings[i].vertices[0].c[0], buildings[i].vertices[0].c[1], 
	  buildings[i].vertices[2].c[0], buildings[i].vertices[2].c[1]);
    }

#endif


  // output environments
  OUT("\n<!-- Environments -->");
  OUT("  <environment name=\"%s\" is_dynamic=\"false\" alpha=\"%.3f\" \
sigma=\"%.3f\" W=\"%.3f\" noise_power=\"%.3f\"/>", building_env.name,
      building_env.alpha[0], building_env.sigma[0], building_env.W[0], 
      building_env.noise_power[0]);
  OUT("  <environment name=\"env_outdoor\" is_dynamic=\"true\"/>");

  // output nodes
  OUT("\n<!-- Nodes -->");
  for(i=0; i<NUMBER_NODES; i++)
    {
      OUT("  <node name=\"%s\" id=\"%d\" adapter=\"s_node\" \
x=\"%.9f\" y=\"%.9f\" z=\"%.9f\"/>", nodes[i].name, nodes[i].id, 
	  nodes[i].position.c[0], nodes[i].position.c[1], 
	  nodes[i].position.c[2]);
    }

  // output motions
  OUT("\n<!-- Motions -->");
  for(i=0; i<NUMBER_NODES; i++)
    {
      if(strcmp(MOTION_TYPE, "linear")==0)
	{
	  if(MOTION_USE_SPEED == TRUE)
	    {
	      OUT("  <motion node_name=\"%s\" type=\"%s\" speed_x=\"%.3f\" \
speed_y=\"%.3f\" speed_z=\"%.3f\" start_time=\"%.3f\" stop_time=\"%.3f\"/>", 
		  nodes[i].name, MOTION_TYPE, 
		  rand_min_max(-MOTION_MAX_SPEED, MOTION_MAX_SPEED),
		  rand_min_max(-MOTION_MAX_SPEED, MOTION_MAX_SPEED), 
		  0.0, MOTION_START_TIME, MOTION_STOP_TIME);
	    }
	  // use destination to specify motion
	  else
	    {
	      OUT("  <motion node_name=\"%s\" type=\"%s\" \
destination_x=\"%.9f\" destination_y=\"%.9f\" destination_z=\"%.9f\" \
start_time=\"%.3f\" stop_time=\"%.3f\"/>", 
		  nodes[i].name, MOTION_TYPE, 
		  generate_x_in_object(&(destination_areas[0])),
		  generate_y_in_object(&(destination_areas[0])),
		  0.0, MOTION_START_TIME, MOTION_STOP_TIME);
	    }
	}
      else if(strcmp(MOTION_TYPE, "behavioral")==0)
	{
	  OUT("  <motion node_name=\"%s\" type=\"%s\" velocity=\"%.3f\" \
destination_x=\"%.9f\" destination_y=\"%.9f\" destination_z=\"%.9f\"	\
start_time=\"%.3f\" stop_time=\"%.3f\"/>", 
	      nodes[i].name, MOTION_TYPE, MOTION_AVG_SPEED,
	      generate_x_in_object(&(destination_areas[0])),
	      generate_y_in_object(&(destination_areas[0])),
	      0.0, MOTION_START_TIME+MOTION_START_OFFSET*i, MOTION_STOP_TIME);
	}
      else
	fprintf(stderr, "Don't know how to generate '%s' motion\n", 
		MOTION_TYPE);
    }

  // output connections
  OUT("\n<!-- Connections -->");
  for(i=0; i<NUMBER_NODES; i++)
    {
      OUT("  <connection from_node=\"%s\" to_node=\"auto_connect\" \
through_environment=\"env_outdoor\" standard=\"active_tag\" packet_size=\"7\" \
consider_interference=\"false\" />", 
	  nodes[i].name);
    }

  // output scenario end tag
  OUT("\n</qomet_scenario>");


  return SUCCESS;
}
