
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
 * File name: scenario.c
 * Function: Source file related to the scenario element
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

#include "scenario.h"

#include "wlan.h"
#include "xml_jpgis.h"

/////////////////////////////////////////
// Scenario structure functions
/////////////////////////////////////////

// init a scenario
void scenario_init(scenario_class *scenario)
{
  // 0 number of elements (nodes, objects, environments, motions, connections)
  scenario->node_number = 0;
  scenario->object_number = 0;
  scenario->environment_number = 0;
  scenario->motion_number = 0;
  scenario->connection_number = 0;
}

// print the fields of a scenario
void scenario_print(scenario_class *scenario)
{
  printf("Scenario: node_number=%d object_number=%d environment_number=%d \
motion_number=%d connection_number=%d\n",
	 scenario->node_number, scenario->object_number, 
	 scenario->environment_number, scenario->motion_number, 
	 scenario->connection_number);
}

// add a node to the scenario structure;
// return a pointer to the element on success, NULL on failure
void *scenario_add_node(scenario_class *scenario, node_class *node)
{
  void *return_value;

  // check we can still add a node
  if(scenario->node_number<MAX_NODES)
    {
      node_copy(&(scenario->nodes[scenario->node_number]), node);
      return_value = &(scenario->nodes[scenario->node_number]);
      scenario->node_number++;
    }
  else
    {
      WARNING("Maximum number of nodes in scenario reached (%d)!",
	      MAX_NODES);
      return_value = NULL;
    }

  return return_value;
}

// add an object to the scenario structure;
// return a pointer to the element on success, NULL on failure
void *scenario_add_object(scenario_class *scenario, object_class *object)
{
  void *return_value;

  // check we can still add an object
  if(scenario->object_number<MAX_OBJECTS)
    {
      object_copy(&(scenario->objects[scenario->object_number]), object);
      return_value = &(scenario->objects[scenario->object_number]);
      scenario->object_number++;
    }
  else
    {
      WARNING("Maximum number of objects in scenario reached (%d)!",
	      MAX_OBJECTS);
      return_value = NULL;
    }

  return return_value;
}

// add an environment to the scenario structure;
// return a pointer to the element on success, NULL on failure
void *scenario_add_environment(scenario_class *scenario, 
			       environment_class *environment)
{
  void *return_value;

  // check if we can still add an object
  if(scenario->environment_number<MAX_ENVIRONMENTS)
    {
      environment_copy(&(scenario->environments[scenario->environment_number]),
		       environment);
      return_value = &(scenario->environments[scenario->environment_number]);
      scenario->environment_number++;
    }
  else
    {
      WARNING("Maximum number of environments in scenario reached (%d)!",
	      MAX_ENVIRONMENTS);
      return_value = NULL;
    }

  return return_value;
}

// add a motion to the scenario structure;
// return a pointer to the element on success, NULL on failure
void *scenario_add_motion(scenario_class *scenario, motion_class *motion)
{
  void *return_value;

  // check if we can still add a motion
  if(scenario->motion_number<MAX_MOTIONS)
    {
      motion_copy(&(scenario->motions[scenario->motion_number]), motion);
      return_value = &(scenario->motions[scenario->motion_number]);
      scenario->motion_number++;
    }
  else
    {
      WARNING("Maximum number of motions in scenario reached (%d)!",
	      MAX_MOTIONS);
      return_value = NULL;
    }

  return return_value;
}

// add a connection to the scenario structure;
// return a pointer to the element on success, NULL on failure
// (if more connections are added, a pointer to the last of them is returned)
void *scenario_add_connection(scenario_class *scenario, 
			      connection_class *connection)
{
  void *return_value;

  char string_copy[MAX_STRING];
  int count = 0;

  char *token, *saveptr1;

  environment_class environment;
  char environment_base_name[MAX_STRING];

  int node_i, env_i;
  uint32_t through_environment_hash;

  environment_class *add_env_result;

  // check to see if auto connect must be done are given
  if(strcmp(connection->to_node, 
	    CONNECTION_TO_NODE_AUTO_CONNECT_STRING)==0)
    {
      DEBUG("Auto-connect request detected. The node '%s' will be \
connected to all the nodes that have already been defined. Auto-generating \
connections...", connection->from_node);

      // check first if the environment provided was defined;
      // if so, use it _directly_, otherwise create new environments
      // for each connection (a warning will be issued in this case)
      through_environment_hash=
	string_hash(connection->through_environment, 
		    strlen(connection->through_environment));

      // try to find the through_environment in scenario
      for(env_i=0; env_i<scenario->environment_number; env_i++)
	{
	  if(scenario->environments[env_i].name_hash==
	     through_environment_hash)
	    if(strcmp(scenario->environments[env_i].name, 
		      connection->through_environment)==0)
	      {
		if(scenario->environments[env_i].is_dynamic==TRUE)
		  {
		    WARNING("ERROR: Environment '%s' is dynamic, and cannot be used to \
define multiple connections", connection->through_environment);
		    return NULL;
		  }
		else
		  {
		    connection->through_environment_index = env_i;
		    break;
		  }
	      }
	}

      // environment was not previously defined, a new one must be
      // created now for each connection (dynamic type)
      if(connection->through_environment_index == INVALID_INDEX)
	{
	  fprintf(stderr, "WARNING: environment '%s' has not been defined \
yet. A dynamic environment will be created for each auto-generated \
connection.\n", connection->through_environment);

	  strncpy(environment_base_name, connection->through_environment, 
		  MAX_STRING-1);
	  environment.is_dynamic = TRUE;
	  strncpy(environment.type, "UNKNOWN", MAX_STRING-1);
	  environment.num_segments = 1;
	}

      for(node_i=0; node_i<scenario->node_number; node_i++)
	// check if potential to_node is different from from_node
	if(strcmp(connection->from_node, 
		  (scenario->nodes[node_i]).name)!=0)
	  {
	    DEBUG("Connecting '%s' to '%s'...", connection->from_node,
		  (scenario->nodes[node_i]).name);
	    
	    strncpy(connection->to_node, (scenario->nodes[node_i]).name, 
		    MAX_STRING-1);
	  
	    // check if we can still add a connection
	    if(scenario->connection_number<MAX_CONNECTIONS)
	      {
		count++;
		    
		// if a connection can be successfully added, we also need to 
		// create and add an environment for this connection;
		// the name is built from the name of the environment
		// provided in the connection and of type 'is_dynamic'
		
		if(connection->through_environment_index == INVALID_INDEX)
		  {
		    snprintf(connection->through_environment, MAX_STRING-1, 
			     "%s(%s->%s)", environment_base_name, 
			     connection->from_node, connection->to_node);

		    strncpy(environment.name, connection->through_environment, 
			    MAX_STRING-1);
		    environment.name_hash=
		      string_hash(environment.name, strlen(environment.name));
		  }

		DEBUG("Generating connection from '%s' to '%s' through '%s'",
		      connection->from_node, connection->to_node,
		      connection->through_environment);

		// only need to add environment if it was not previously found
		if(connection->through_environment_index == INVALID_INDEX)
		  add_env_result = 
		    scenario_add_environment(scenario, &environment);

		// try to add the generated environment
		if(add_env_result != NULL || 
		   connection->through_environment_index != INVALID_INDEX)
		  {
		    connection_copy
		      (&(scenario->connections[scenario->connection_number]),
		       connection);
		    return_value = 
		      &(scenario->connections[scenario->connection_number]);
		    scenario->connection_number++;
		    scenario_print(scenario);
		  }
		else
		  {
		    WARNING("Could not add generated environment or \
incorrect environment index initialization");
		    return_value = NULL;
		    break;
		  }
	      }
	    else
	      {
		WARNING("Maximum number of connections in scenario \
reached (%d)!", MAX_CONNECTIONS);
		return_value = NULL;
		break;
	      }
	  }
    }
  // check to see if multiple to_node names are given
  else if(strchr(connection->to_node, ' ')!=NULL)
    {
      DEBUG("Connection to multiple nodes detected. Auto-generating \
connections...");

      // since multiple to_node names were given,
      // we need to parse each of them
      strncpy(string_copy, connection->to_node, MAX_STRING-1);

      // check first if the environment provided was defined;
      // if so, use it _directly_, otherwise create new environments
      // for each connection (a warning will be issued in this case)
      through_environment_hash=
	string_hash(connection->through_environment, 
		    strlen(connection->through_environment));

      // try to find the through_environment in scenario
      for(env_i=0; env_i<scenario->environment_number; env_i++)
	{
	  if(scenario->environments[env_i].name_hash==
	     through_environment_hash)
	    if(strcmp(scenario->environments[env_i].name, 
		      connection->through_environment)==0)
	      {
		if(scenario->environments[env_i].is_dynamic==TRUE)
		  {
		    WARNING("ERROR: Environment '%s' is dynamic, and cannot \
be used to define multiple connections", connection->through_environment);
		    return NULL;
		  }
		else
		  {
		    connection->through_environment_index = env_i;
		    break;
		  }
	      }
	}

      // environment was not previously defined, a new one must be
      // created now for each connection (dynamic type)
      if(connection->through_environment_index == INVALID_INDEX)
	{
	  fprintf(stderr, "WARNING: environment '%s' has not been defined \
yet. A dynamic environment will be created for each auto-generated \
connection.\n", connection->through_environment);

	  strncpy(environment_base_name, connection->through_environment, 
		  MAX_STRING-1);

	  environment.is_dynamic = TRUE;
	  strncpy(environment.type, "UNKNOWN", MAX_STRING-1);
	  environment.num_segments = 1;

	}

      //printf("to_node=%s\n", connection->to_node);

      // try to add one connection for each node in the list to_node
      while(1)
	{
	  if(count == 0)//first time
	    token = strtok_r(string_copy, " ", &saveptr1);
	  else
	    token = strtok_r(NULL, " ", &saveptr1);
	  
	  if(token == NULL)
	    break;
	  //printf("Token=%s string_copy=%s saveptr1=%s\n", 
	  //	 token, string_copy, saveptr1);
	  
	  strncpy(connection->to_node, token, MAX_STRING-1);
	  
	  // check if we can still add a connection
	  if(scenario->connection_number<MAX_CONNECTIONS)
	    {
	      count++;
	      
	      // if a connection can be successfully added, we also need to 
	      // create and add an environment for this connection;
	      // the name is built from the name of the environment
	      // provided in the connection and of type 'is_dynamic'


	      if(connection->through_environment_index == INVALID_INDEX)
		{
		  snprintf(connection->through_environment, MAX_STRING-1, 
			   "%s(%s->%s)", environment_base_name, 
			   connection->from_node, connection->to_node);

		  strncpy(environment.name, connection->through_environment, 
			  MAX_STRING-1);
		  environment.name_hash=string_hash(environment.name, 
						    strlen(environment.name));
		}

	      DEBUG("Generating connection from '%s' to '%s' through '%s'",
		    connection->from_node, connection->to_node,
		    connection->through_environment);
	      
	      // only need to add environment if it was not previously found
	      if(connection->through_environment_index == INVALID_INDEX)
		add_env_result = scenario_add_environment(scenario, 
							  &environment);
	      
	      // try to add the generated environment
	      if(add_env_result != NULL || 
		 connection->through_environment_index != INVALID_INDEX)
		{
		  scenario_print(scenario);
		  connection_copy(&(scenario->
				    connections[scenario->connection_number]),
				  connection);
		  return_value = 
		    &(scenario->connections[scenario->connection_number]);
		  scenario->connection_number++;
		}
	      else
		{
		  WARNING("Could not add generated environment or incorrect \
environment index initialization");
		  return_value = NULL;
		  break;
		}
	    }
	  else
	    {
	      WARNING("Maximum number of connections in scenario reached \
(%d)!", MAX_CONNECTIONS);
	      return_value = NULL;
	      break;
	    }
	}
    }
  // only one to_node seems to be present, 
  // therefore we can directly add the connection
  else
    {
      // check if we can still add a connection
      if(scenario->connection_number<MAX_CONNECTIONS)
	{
	  connection_copy(&(scenario->
			    connections[scenario->connection_number]),
			  connection);
	  return_value = &(scenario->connections[scenario->connection_number]);
	  scenario->connection_number++;
	}
      else
	{
	  WARNING("Maximum number of connections in scenario reached (%d)!",
		  MAX_CONNECTIONS);
	  return_value = NULL;
	}
    }

  return return_value;
}


/////////////////////////////////////////////////////
// deltaQ computation top-level functions

// auto connect scenario nodes using the auto_connect_node function;
// return SUCCESS on succes, ERROR on error
// NOTE: This function is still in experimental phase!
int scenario_auto_connect_nodes(scenario_class *scenario)
{
  int node_i;

  // update state of connections according to a given criteria
  // (e.g., frequency in seconds) by erasing all connections,
  // then auto connecting all nodes;

  INFO("Auto-connecting nodes...");

  // reset connections
  // NOTE: more is needed => reset neighbour_number...
  scenario->connection_number = 0;

  // for all the nodes, try to connect them to a suitable
  // neighbouring station (regular or AP)
  for(node_i=0; node_i < scenario->node_number; node_i++)
    if(node_auto_connect(&(scenario->nodes[node_i]), scenario)==ERROR)
      return ERROR;

  INFO("Done: %d connections created", scenario->connection_number);

  return SUCCESS;
}

// auto connect scenario nodes using the auto_connect_node function
// (used for active tag nodes only);
// return SUCCESS on succes, ERROR on error
int scenario_auto_connect_nodes_at(scenario_class *scenario)
{
  int node_i;

  // update state of connections according to a given criteria
  // (e.g., frequency in seconds) by erasing all connections,
  // then auto connecting all nodes;

  // reset connections
  // NOTE: more is needed => reset neighbour_number...
  scenario->connection_number = 0;

  // for all the nodes, try to connect them to a suitable
  // neighbouring node
  for(node_i=0; node_i < scenario->node_number; node_i++)
    if(node_auto_connect_at(&(scenario->nodes[node_i]), scenario)==ERROR)
      return ERROR;

  INFO("Auto-connecting ActiveTag nodes: %d connections created", 
       scenario->connection_number);

  return SUCCESS;
}

// perform the necessary initialization before the scenario_deltaQ
// function can be called (mainly connection-related initialization);
// if 'load_from_jpgis_file' flag is set, some object coordinates 
// will be loaded from the JPGIS file 'jpgis_filename';
// return SUCCESS on succes, ERROR on error
int scenario_init_state(scenario_class *scenario, int jpgis_filename_provided,
			char *jpgis_filename, int deltaQ_disabled)
{
  int connection_i; // connection index
  int object_i; // object index
  int motion_i; // motion index
  int num_iterations; // iteration counter
  int deltaQ_changed; // show whether deltaQ was changed or not

  fprintf(stderr, "* Node validation done (%d nodes)\n",
	  scenario->node_number);
  
  fprintf(stderr, "* Environment validation done (%d environments)\n",
	  scenario->environment_number);

  // loop for each object for init
  for(object_i=0; object_i < scenario->object_number; object_i++)
    {
      // do initialization of object indexes
      if(object_init_index(&(scenario->objects[object_i]), 
			   scenario)==ERROR)
	{
	  WARNING("Error while initializing object indexes");
	  return ERROR;
	}
    }

  // load objects from JPGIS file if the filename was provided,
  // and any object was flagged as such
  if(jpgis_filename_provided == TRUE)
    if(xml_jpgis_load_objects(scenario->objects, scenario->object_number,
			      jpgis_filename)==ERROR)
      {
	WARNING("Error while loading object coordinates from JPGIS file '%s'",
		jpgis_filename);
	return ERROR;
      }
  
  // try to merge the objects to form polygons; this may be needed,
  // for example, for the objects that been loaded as polylines 
  // from the JPGIS file; failure to merge a polyline to any of the 
  // current objects will result in an error
  while(1)
    {
      int merge_done = FALSE;
      int tested_objects = 0;

      // loop for each object
      for(object_i=0; object_i < scenario->object_number; object_i++)
	{
	  object_class *crt_object = &(scenario->objects[object_i]);
	  
	  // check that first coordinate and last coordinate 
	  // are equal, which is the convention used in JPGIS
	  // to represent polygons
	  if((fabs(crt_object->vertices[0].c[0]-crt_object->vertices
		   [crt_object->vertex_number-1].c[0]) > EPSILON) ||
	     (fabs(crt_object->vertices[0].c[1]-crt_object->vertices
		   [crt_object->vertex_number-1].c[1]) > EPSILON))
	    {
	      if(crt_object->make_polygon==TRUE)
		{
		  INFO("The polyline object '%s' is considered a polygon; \
no merging necessary", crt_object->name);
		  tested_objects++;
		  continue;
		}
	      else
		WARNING("The object '%s' is not a polygon. Trying to merge...",
			crt_object->name);
	      
	      // try to merge this object with another one
	      merge_done = scenario_try_merge_object(scenario, object_i);

	      if(merge_done==FALSE)
		{
		  WARNING("Unable to merge");

		  scenario_print(scenario);
		  // loop for each object
		  for(object_i=0; object_i < scenario->object_number; 
		      object_i++)
		    object_print(&(scenario->objects[object_i]));

		  return ERROR;
		}
	    }
	  else 
	    tested_objects++;
	}
      if(tested_objects==scenario->object_number)
	break;
    }

  fprintf(stderr, "* Object initialization done (%d objects)\n",
	  scenario->object_number);

  INFO("############################################");
  scenario_print(scenario);
  // loop for each object
  for(object_i=0; object_i < scenario->object_number; object_i++)
    object_print_blh(&(scenario->objects[object_i]));
  INFO("############################################");

  // loop for each motion for init
  for(motion_i=0; motion_i < scenario->motion_number; motion_i++)
    {
      // do initialization of motion indexes
      if(motion_init_index(&(scenario->motions[motion_i]), 
			   scenario)==ERROR)
	{
	  WARNING("Error while initializing motion indexes");
	  return ERROR;
	}
    }

  fprintf(stderr, "* Motion index initialization done (%d motions)\n",
	  scenario->motion_number);

  // init indexes of each connection; since this may take a while
  // for large scenarios, a counter is displayed
  for(connection_i=0; connection_i < scenario->connection_number; 
      connection_i++)
    {
      fprintf(stderr, "Initializing indexes for connection %d (out of %d)\r", 
	      connection_i, scenario->connection_number);

      // do initialization of connection indexes
      if(connection_init_indexes(&(scenario->connections[connection_i]), 
				 scenario)==ERROR)
	{
	  WARNING("Error while initializing connection indexes");
	  return ERROR;
	}
    }

  fprintf(stderr, "* Connection index initialization done (%d connections)\n",
	  scenario->connection_number);

  if(deltaQ_disabled == FALSE)
    {
      // precompute status for each connection; since this may take a while
      // for large scenarios, a counter is displayed
      for(connection_i=0; connection_i < scenario->connection_number; 
	  connection_i++)
	{
	  fprintf(stderr, "Initializing state for connection %d (out of %d)\r",
		  connection_i, scenario->connection_number);
	  
	  // reset number of iterations
	  num_iterations = 0;
	  INFO("--- Initializing connection %d ---", connection_i);

	  do
	    {
	      // compute deltaQ
	      if(connection_deltaQ(&(scenario->connections[connection_i]), 
				   scenario, &deltaQ_changed)==ERROR)
		{
		  WARNING("Error while computing connection deltaQ");
		  return ERROR;
		}
	      
#ifdef MESSAGE_DEBUG
	      DEBUG("Current connection state:");
	      connection_print(&(scenario->connections[connection_i]));
#endif
	      
	      // if deltaQ didn't change with respect to the previous 
	      // iteration then break, else continue
	      if(deltaQ_changed == FALSE)
		break;
	      else
		num_iterations++;
	  
	      // if maximum number of iterations was reached it means 
	      // a steady state cannot be achieved => warning
	      if(num_iterations >= MAXIMUM_PRECOMPUTE)
		{
		  WARNING("Maximum number of iterations (%d) was \
reached during precomputation phase without reaching steady state", 
			  MAXIMUM_PRECOMPUTE);
		  break;
		}
	    }
	  while(1);
	  
	  // print "nice" separator between connections
	  INFO("--- Connection %d initialization finished ---", connection_i);
	}

      fprintf(stderr, "* Connection initialization done (%d \
connections)      \n", scenario->connection_number);
    }

  return SUCCESS;
}


// compute the deltaQ for all connections of the given scenario;
// deltaQ parameters are returned in the corresponding fields of
// the connection objects;
// return SUCCESS on succes, ERROR on error
int scenario_deltaQ(scenario_class *scenario)
{
  int connection_i, deltaQ_changed;

  // calculate the state for active nodes according to 
  // 'connections' object in 'scenario'
  for(connection_i=0; connection_i < scenario->connection_number; 
      connection_i++)
    {
      fprintf(stderr, "\t\t\tcalculation => connection %d                 \r", 
	      connection_i);
      if(connection_deltaQ(&(scenario->connections[connection_i]), 
			   scenario, &deltaQ_changed)==ERROR)
	return ERROR;
    }

  return SUCCESS;
}

// reset the interference_accounted flag for all nodes
void scenario_reset_node_interference_flag(scenario_class *scenario)
{
  int node_i;

  // for all the nodes, reset interference flag
  for(node_i=0; node_i < scenario->node_number; node_i++)
    scenario->nodes[node_i].interference_accounted = FALSE;
}

// try to merge an object specified by index 'merge_object_i' to other
// objects in scenario; 
// return TRUE if a merge operation was performed, FALSE otherwise
int scenario_try_merge_object(scenario_class *scenario, int merge_object_i)
{
  int crt_object_j;
  object_class *merge_object=&(scenario->objects[merge_object_i]);

  // loop for each object
  for(crt_object_j=0; crt_object_j<scenario->object_number; crt_object_j++)
    {
      object_class *crt_object=&(scenario->objects[crt_object_j]);

      if(merge_object_i == crt_object_j)
	continue;

      object_print(merge_object);
      INFO("Checking object '%s' (index=%d) for merge...", 
	   crt_object->name, crt_object_j);
      object_print(crt_object);

      // first vertex equals last vertex
      if((fabs(merge_object->vertices[0].c[0] -
	       crt_object->vertices[crt_object->vertex_number-1].c[0]) < 
	  EPSILON) &&
	 (fabs(merge_object->vertices[0].c[1] -
	       crt_object->vertices[crt_object->vertex_number-1].c[1]) < 
	  EPSILON))
	{
	  INFO("Merging object '%s' to object '%s' by direct appending...",
	       merge_object->name, crt_object->name);

	  if(scenario_merge_objects(scenario, crt_object_j, 
				    merge_object_i, TRUE) == SUCCESS)
	    return TRUE;
	  else
	    return FALSE;
	}
      // first vertex equals first vertex!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      else if((fabs(merge_object->vertices[0].c[0] - 
		    crt_object->vertices[0].c[0]) < EPSILON) &&
	      (fabs(merge_object->vertices[0].c[1] -
		    crt_object->vertices[0].c[1]) < EPSILON))
	{
	  INFO("Merging object '%s' to object '%s' by direct appending after reversing...",
	       merge_object->name, crt_object->name);

	  if(scenario_merge_objects(scenario, crt_object_j, 
				    merge_object_i, TRUE) == SUCCESS)
	    return TRUE;
	  else
	    return FALSE;
	}
      // last vertex equals first vertex
      else if((fabs(merge_object->vertices[merge_object->vertex_number-1].c[0]-
		    crt_object->vertices[0].c[0]) < EPSILON) &&
	      (fabs(merge_object->vertices[merge_object->vertex_number-1].c[1]-
		    crt_object->vertices[0].c[1]) < EPSILON))
	{
	  INFO("Merging object '%s' to object '%s' by direct appending...",
	       crt_object->name, merge_object->name);
	  if(scenario_merge_objects(scenario, merge_object_i, 
				    crt_object_j, TRUE) == SUCCESS)
	    return TRUE;
	  else
	    return FALSE;
	}
      // last vertex equals last vertex
      else if((fabs(merge_object->vertices[merge_object->vertex_number-1].c[0]-
		    crt_object->vertices[crt_object->vertex_number-1].c[0]) < 
	       EPSILON) &&
	      (fabs(merge_object->vertices[merge_object->vertex_number-1].c[1]-
		    crt_object->vertices[crt_object->vertex_number-1].c[1]) < 
	       EPSILON))
	{
	  INFO("Merging object '%s' to object '%s' by reverse appending...",
	       crt_object->name, merge_object->name);
	  if(scenario_merge_objects(scenario, merge_object_i, 
				    crt_object_j, FALSE) == SUCCESS)
	    return TRUE;
	  else
	    return FALSE;
	}
    }

  return FALSE;
}

// merge the object specified by index 'object_i2' to the 
// one specified by index 'object_i1'; if direct_merge is TRUE,
// then direct vertex merging is done, by appending those of 
// object 2 to those of object 1; if direct_merge is FALSE,
// then the order of vertices of object 2 is reversed while merging;
// return SUCCESS on success, ERROR on error
int scenario_merge_objects(scenario_class *scenario, int object_i1, 
			   int object_i2, int direct_merge)
{
  int i;
  object_class *object1 = &(scenario->objects[object_i1]);
  object_class *object2 = &(scenario->objects[object_i2]);

  // check if direct vertex merging was requested
  if(direct_merge == TRUE)
    {
      // if first vertex equals first vertex, then the object1
      // vertex order must first be reversed
      if((fabs(object1->vertices[0].c[0] - 
	       object2->vertices[0].c[0]) < EPSILON) &&
	 (fabs(object1->vertices[0].c[1] - 
	       object2->vertices[0].c[1]) < EPSILON))
	{
	  coordinate_class aux_coord;
	  INFO("@@@@@@@@@@@@@@@@@@@@ Reversing");
	  //object_print(object1);
 
	  for(i=0; i<object1->vertex_number/2; i++)
	    {
	      coordinate_copy(&aux_coord, &(object1->vertices[i]));
	      coordinate_copy
		(&(object1->vertices[i]), 
		 &(object1->vertices[object1->vertex_number-1-i]));
	      coordinate_copy(&(object1->vertices[object1->vertex_number-1-i]),
			      &aux_coord);
	      //INFO("-- after step %d (exchanging %d and %d)", i,
	      //   i, object1->vertex_number-1-i);
	      //object_print(object1);
	    }
	  //INFO("-- final");
	  //object_print(object1);
	}

      // append vertices of object2 in direct order
      for(i=1; i<object2->vertex_number; i++)
	if(object1->vertex_number<MAX_VERTICES)
	  {
	    coordinate_copy(&(object1->vertices[object1->vertex_number]),
			    &(object2->vertices[i]));
	    object1->vertex_number++;
	  }
	else
	  {
	    WARNING("Maximum number of vertices (%d) exceeded", MAX_VERTICES);
	    return ERROR;
	  }
    }
  else // reverse vertex merging
    {
      for(i=object2->vertex_number-2; i>=0; i--)
	if(object1->vertex_number<MAX_VERTICES)
	  {
	    coordinate_copy(&(object1->vertices[object1->vertex_number]),
			    &(object2->vertices[i]));
	    object1->vertex_number++;
	  }
	else
	  {
	    WARNING("Maximum number of vertices (%d) exceeded", MAX_VERTICES);
	    return ERROR;
	  }
    }
  strncat(object1->name, object2->name, MAX_STRING-strlen(object1->name)-1);
  object_print(object1);

  scenario_remove_object(scenario, object_i2);

  return SUCCESS;
}

// remove the object specified by index 'object_i' from the scenario; 
// return SUCCESS on success, ERROR on error
int scenario_remove_object(scenario_class *scenario, int object_i)
{
  int i;

  if(object_i<0 || object_i>(scenario->object_number-1))
    {
      WARNING("Object index %d is invalid", object_i);
      return FALSE;
    }
  else
    {
      for(i=object_i; i<(scenario->object_number-1); i++)
	object_copy(&(scenario->objects[i]),&(scenario->objects[i+1])); 
      scenario->object_number--;
    }

  return TRUE;
}
