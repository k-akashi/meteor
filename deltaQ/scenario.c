
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
 * File name: scenario.c
 * Function: Source file related to the scenario element
 *
 * Author: Razvan Beuran
 *
 * $Id: scenario.c 146 2013-06-20 00:50:48Z razvan $
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
void
scenario_init (struct scenario_class *scenario)
{
  // 0 number of elements (nodes, objects, environments, motions, connections)
  scenario->node_number = 0;
  scenario->object_number = 0;
  scenario->environment_number = 0;
  scenario->motion_number = 0;
  scenario->connection_number = 0;
  scenario->interface_number = 0;

  scenario->current_time = 0.0;
}

// print the fields of a scenario
void
scenario_print (struct scenario_class *scenario)
{
  printf ("Scenario: node_number=%d interface_number=%d object_number=%d \
environment_number=%d motion_number=%d connection_number=%d\n", scenario->node_number, scenario->interface_number, scenario->object_number, scenario->environment_number, scenario->motion_number, scenario->connection_number);
}

// add a node to the scenario structure;
// return a pointer to the element on success, NULL on failure
void *
scenario_add_node (struct scenario_class *scenario, struct node_class *node)
{
  void *return_value = NULL;

  // check we can still add a node
  if (scenario->node_number < MAX_NODES)
    {
      // set the node id before copying; ids are assigned automatically
      // in increasing order, hence are the same with the index in the 
      // scenario array
      node->id = scenario->node_number;

      node_copy (&(scenario->nodes[scenario->node_number]), node);
      return_value = &(scenario->nodes[scenario->node_number]);
      scenario->node_number++;
    }
  else
    {
      WARNING ("Maximum number of nodes in scenario reached (%d)!",
	       MAX_NODES);
      return_value = NULL;
    }

  return return_value;
}

// add an object to the scenario structure;
// return a pointer to the element on success, NULL on failure
void *
scenario_add_object (struct scenario_class *scenario,
		     struct object_class *object)
{
  void *return_value = NULL;

  // check we can still add an object
  if (scenario->object_number < MAX_OBJECTS)
    {
      object_copy (&(scenario->objects[scenario->object_number]), object);
      return_value = &(scenario->objects[scenario->object_number]);
      scenario->object_number++;
    }
  else
    {
      WARNING ("Maximum number of objects in scenario reached (%d)!",
	       MAX_OBJECTS);
      return_value = NULL;
    }

  return return_value;
}

// add an environment to the scenario structure;
// return a pointer to the element on success, NULL on failure
void *
scenario_add_environment (struct scenario_class *scenario,
			  struct environment_class *environment)
{
  void *return_value = NULL;

  // check if we can still add an object
  if (scenario->environment_number < MAX_ENVIRONMENTS)
    {
      environment_copy (&
			(scenario->environments
			 [scenario->environment_number]), environment);
      return_value = &(scenario->environments[scenario->environment_number]);
      scenario->environment_number++;
    }
  else
    {
      WARNING ("Maximum number of environments in scenario reached (%d)!",
	       MAX_ENVIRONMENTS);
      return_value = NULL;
    }

  return return_value;
}

// add a motion to the scenario structure;
// return a pointer to the element on success, NULL on failure
void *
scenario_add_motion (struct scenario_class *scenario,
		     struct motion_class *motion)
{
  void *return_value = NULL;

  // check if we can still add a motion
  if (scenario->motion_number < MAX_MOTIONS)
    {
      motion_copy (&(scenario->motions[scenario->motion_number]), motion);
      (scenario->motions[scenario->motion_number]).id =
	scenario->motion_number;
      return_value = &(scenario->motions[scenario->motion_number]);
      scenario->motion_number++;
    }
  else
    {
      WARNING ("Maximum number of motions in scenario reached (%d)!",
	       MAX_MOTIONS);
      return_value = NULL;
    }

  return return_value;
}

// add a connection to the scenario structure;
// return a pointer to the element on success, NULL on failure
// (if more connections are added, a pointer to the last of them is returned)
void *
scenario_add_connection (struct scenario_class *scenario,
			 struct connection_class *connection)
{
  void *return_value = NULL;

  char string_copy[MAX_STRING];
  int count = 0;

  char *token, *saveptr1;

  struct environment_class environment;
  char environment_base_name[MAX_STRING];

  int node_i, env_i;
  uint32_t through_environment_hash;

  struct environment_class *add_env_result = NULL;

  // check to see if auto connect must be done are given
  if (strcmp (connection->to_node,
	      CONNECTION_TO_NODE_AUTO_CONNECT_STRING) == 0)
    {
      DEBUG ("Auto-connect request detected. The node '%s' will be \
connected to all the nodes that have already been defined. Auto-generating \
connections...", connection->from_node);

      // check first if the environment provided was defined;
      // if so, use it _directly_, otherwise create new environments
      // for each connection (a warning will be issued in this case)
      through_environment_hash =
	string_hash (connection->through_environment,
		     strlen (connection->through_environment));

      // try to find the through_environment in scenario
      for (env_i = 0; env_i < scenario->environment_number; env_i++)
	{
	  if (scenario->environments[env_i].name_hash ==
	      through_environment_hash)
	    if (strcmp (scenario->environments[env_i].name,
			connection->through_environment) == 0)
	      {
		/*
		   if (scenario->environments[env_i].is_dynamic == TRUE)
		   {
		   WARNING ("ERROR: Environment '%s' is dynamic, and cannot be used to \
		   define multiple connections", connection->
		   through_environment);
		   return NULL;
		   }
		   else
		   {
		 */
		connection->through_environment_index = env_i;
		break;
		/*
		   }
		 */
	      }
	}

      // environment was not previously defined, a new one must be
      // created now for each connection (dynamic type)
      if (connection->through_environment_index == INVALID_INDEX)
	{
	  fprintf (stderr, "WARNING: environment '%s' has not been defined \
yet. A dynamic environment will be created for each auto-generated \
connection.\n", connection->through_environment);

	  strncpy (environment_base_name, connection->through_environment,
		   MAX_STRING - 1);
	  environment.is_dynamic = TRUE;
	  strncpy (environment.type, "UNKNOWN", MAX_STRING - 1);
	  environment.num_segments = 1;
	}

      for (node_i = 0; node_i < scenario->node_number; node_i++)
	// check if potential to_node is different from from_node
	if (strcmp (connection->from_node,
		    (scenario->nodes[node_i]).name) != 0)
	  {
	    DEBUG ("Connecting '%s' to '%s'...", connection->from_node,
		   (scenario->nodes[node_i]).name);

	    strncpy (connection->to_node, (scenario->nodes[node_i]).name,
		     MAX_STRING - 1);

	    // check if we can still add a connection
	    if (scenario->connection_number < MAX_CONNECTIONS)
	      {
		count++;

		// if a connection can be successfully added, we also need to 
		// create and add an environment for this connection;
		// the name is built from the name of the environment
		// provided in the connection and of type 'is_dynamic'

		if (connection->through_environment_index == INVALID_INDEX)
		  {
		    snprintf (connection->through_environment, MAX_STRING - 1,
			      "%s(%s->%s)", environment_base_name,
			      connection->from_node, connection->to_node);

		    strncpy (environment.name,
			     connection->through_environment, MAX_STRING - 1);
		    environment.name_hash =
		      string_hash (environment.name,
				   strlen (environment.name));

		    environment.is_dynamic = TRUE;

		  }

		DEBUG ("Generating connection from '%s' to '%s' through '%s'",
		       connection->from_node, connection->to_node,
		       connection->through_environment);

		// only need to add environment if it was not previously found
		if (connection->through_environment_index == INVALID_INDEX)
		  add_env_result =
		    scenario_add_environment (scenario, &environment);

		// try to add the generated environment
		if (add_env_result != NULL ||
		    connection->through_environment_index != INVALID_INDEX)
		  {
		    connection_copy
		      (&(scenario->connections[scenario->connection_number]),
		       connection);
		    return_value =
		      &(scenario->connections[scenario->connection_number]);
		    scenario->connection_number++;
#ifdef MESSAGE_DEBUG
		    scenario_print (scenario);
#endif
		  }
		else
		  {
		    WARNING ("Could not add generated environment or \
incorrect environment index initialization");
		    return_value = NULL;
		    break;
		  }
	      }
	    else
	      {
		WARNING ("Maximum number of connections in scenario \
reached (%d)!", MAX_CONNECTIONS);
		return_value = NULL;
		break;
	      }
	  }
    }
  // check to see if multiple to_node names are given
  else if (strchr (connection->to_node, ' ') != NULL)
    {
      DEBUG ("Connection to multiple nodes detected. Auto-generating \
connections...");

      // since multiple to_node names were given,
      // we need to parse each of them
      strncpy (string_copy, connection->to_node, MAX_STRING - 1);

      // check first if the environment provided was defined;
      // if so, use it _directly_, otherwise create new environments
      // for each connection (a warning will be issued in this case)
      through_environment_hash =
	string_hash (connection->through_environment,
		     strlen (connection->through_environment));

      // try to find the through_environment in scenario
      for (env_i = 0; env_i < scenario->environment_number; env_i++)
	{
	  if (scenario->environments[env_i].name_hash ==
	      through_environment_hash)
	    if (strcmp (scenario->environments[env_i].name,
			connection->through_environment) == 0)
	      {
		if (scenario->environments[env_i].is_dynamic == TRUE)
		  {
		    WARNING ("ERROR: Environment '%s' is dynamic, and cannot \
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
      if (connection->through_environment_index == INVALID_INDEX)
	{
	  fprintf (stderr, "WARNING: environment '%s' has not been defined \
yet. A dynamic environment will be created for each auto-generated \
connection.\n", connection->through_environment);

	  strncpy (environment_base_name, connection->through_environment,
		   MAX_STRING - 1);

	  environment.is_dynamic = TRUE;
	  strncpy (environment.type, "UNKNOWN", MAX_STRING - 1);
	  environment.num_segments = 1;

	}

      //printf("to_node=%s\n", connection->to_node);

      // try to add one connection for each node in the list to_node
      while (1)
	{
	  if (count == 0)	//first time
	    token = strtok_r (string_copy, " ", &saveptr1);
	  else
	    token = strtok_r (NULL, " ", &saveptr1);

	  if (token == NULL)
	    break;
	  //printf("Token=%s string_copy=%s saveptr1=%s\n", 
	  //     token, string_copy, saveptr1);

	  strncpy (connection->to_node, token, MAX_STRING - 1);

	  // check if we can still add a connection
	  if (scenario->connection_number < MAX_CONNECTIONS)
	    {
	      count++;

	      // if a connection can be successfully added, we also need to 
	      // create and add an environment for this connection;
	      // the name is built from the name of the environment
	      // provided in the connection and of type 'is_dynamic'


	      if (connection->through_environment_index == INVALID_INDEX)
		{
		  snprintf (connection->through_environment, MAX_STRING - 1,
			    "%s(%s->%s)", environment_base_name,
			    connection->from_node, connection->to_node);

		  strncpy (environment.name, connection->through_environment,
			   MAX_STRING - 1);
		  environment.name_hash = string_hash (environment.name,
						       strlen
						       (environment.name));
		}

	      DEBUG ("Generating connection from '%s' to '%s' through '%s'",
		     connection->from_node, connection->to_node,
		     connection->through_environment);

	      // only need to add environment if it was not previously found
	      if (connection->through_environment_index == INVALID_INDEX)
		add_env_result = scenario_add_environment (scenario,
							   &environment);

	      // try to add the generated environment
	      if (add_env_result != NULL ||
		  connection->through_environment_index != INVALID_INDEX)
		{
#ifdef MESSAGE_DEBUG
		  scenario_print (scenario);
#endif
		  connection_copy (&
				   (scenario->connections
				    [scenario->connection_number]),
				   connection);
		  return_value =
		    &(scenario->connections[scenario->connection_number]);
		  scenario->connection_number++;
		}
	      else
		{
		  WARNING ("Could not add generated environment or incorrect \
environment index initialization");
		  return_value = NULL;
		  break;
		}
	    }
	  else
	    {
	      WARNING ("Maximum number of connections in scenario reached \
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
      if (scenario->connection_number < MAX_CONNECTIONS)
	{
	  connection_copy (&
			   (scenario->connections
			    [scenario->connection_number]), connection);
	  return_value =
	    &(scenario->connections[scenario->connection_number]);
	  scenario->connection_number++;
	}
      else
	{
	  WARNING ("Maximum number of connections in scenario reached (%d)!",
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
int
scenario_auto_connect_nodes (struct scenario_class *scenario)
{
  int node_i;

  // update state of connections according to a given criteria
  // (e.g., frequency in seconds) by erasing all connections,
  // then auto connecting all nodes;

  INFO ("Auto-connecting nodes...");

  // reset connections
  // NOTE: more is needed => reset neighbour_number...
  scenario->connection_number = 0;

  // for all the nodes, try to connect them to a suitable
  // neighbouring station (regular or AP)
  for (node_i = 0; node_i < scenario->node_number; node_i++)
    if (node_auto_connect (&(scenario->nodes[node_i]), scenario) == ERROR)
      return ERROR;

  INFO ("Done: %d connections created", scenario->connection_number);

  return SUCCESS;
}

// auto connect scenario nodes using the auto_connect_node function
// (used for active tag nodes only);
// return SUCCESS on succes, ERROR on error
int
scenario_auto_connect_nodes_at (struct scenario_class *scenario)
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
  for (node_i = 0; node_i < scenario->node_number; node_i++)
    if (node_auto_connect_at (&(scenario->nodes[node_i]), scenario) == ERROR)
      return ERROR;

  INFO ("Auto-connecting ActiveTag nodes: %d connections created",
	scenario->connection_number);

  return SUCCESS;
}

// perform the necessary initialization before the scenario_deltaQ
// function can be called (mainly connection-related initialization);
// if 'load_from_jpgis_file' flag is set, some object coordinates 
// will be loaded from the JPGIS file 'jpgis_filename';
// return SUCCESS on succes, ERROR on error
int
scenario_init_state (struct scenario_class *scenario,
		     int jpgis_filename_provided, char *jpgis_filename,
		     int cartesian_coord_syst, int deltaQ_disabled)
{

  int node_i;			// node index
  int object_i;			// object index
  int connection_i;		// connection index
  int motion_i;			// motion index
  int num_iterations;		// iteration counter
  int deltaQ_changed;		// show whether deltaQ was changed or not

  // initialize interface indexes for each node
  for (node_i = 0; node_i < scenario->node_number; node_i++)
    {
      // do initialization of object indexes
      if (node_init_interface_index (&(scenario->nodes[node_i]),
				     scenario) == ERROR)
	{
	  WARNING ("Error while initializing node interface indexes");
	  return ERROR;
	}
    }
  DEBUG ("* Node validation and initialization done (%d nodes)\n",
	 scenario->node_number);

  // nothing to be done for environment validation...
  DEBUG ("* Environment validation and initialization done (%d environments)\n",
	 scenario->environment_number);

  // loop for each object for init
  for (object_i = 0; object_i < scenario->object_number; object_i++)
    {
      // do initialization of object indexes
      if (object_init_index (&(scenario->objects[object_i]),
			     scenario) == ERROR)
	{
	  WARNING ("Error while initializing object indexes");
	  return ERROR;
	}
    }

  // load objects from JPGIS file if the filename was provided,
  // and any object was flagged as such
  if (jpgis_filename_provided == TRUE)
    if (xml_jpgis_load_objects (scenario, scenario->objects,
				scenario->object_number,
				jpgis_filename) == ERROR)
      {
	WARNING
	  ("Error while loading object coordinates from JPGIS file '%s'",
	   jpgis_filename);
	return ERROR;
      }

  // check that all objects were loaded from the JPGIS file and
  // remove region place-holder objects after JPGIS objects were added (if any)
  DEBUG ("Check objects in scenario");
  for (object_i = 0; object_i < scenario->object_number; object_i++)
    {
      if (scenario->objects[object_i].load_all_from_region == TRUE)
	{
	  DEBUG
	    ("Remove region place-holder object '%s' after adding JPGIS objects",
	     scenario->objects[object_i].name);
	  scenario_remove_object (scenario, object_i);
	}
      else if (scenario->objects[object_i].load_from_jpgis_file == TRUE)
	{
	  WARNING ("Object '%s' could not be loaded from JPGIS file '%s'",
		   scenario->objects[object_i].name, jpgis_filename);
	  return ERROR;
	}
    }

  // try to merge the objects to form polygons; this may be needed,
  // for example, for the objects that been loaded as polylines 
  // from the JPGIS file; failure to merge a polyline to any of the 
  // current objects will result in an error
  while (1)
    {
      int merge_done = FALSE;
      int tested_objects = 0;

      // loop for each object
      for (object_i = 0; object_i < scenario->object_number; object_i++)
	{
	  struct object_class *crt_object = &(scenario->objects[object_i]);

	  // check that first coordinate and last coordinate 
	  // are equal, which is the convention used in JPGIS
	  // to represent polygons
	  if ((fabs (crt_object->vertices[0].c[0] - crt_object->vertices
		     [crt_object->vertex_number - 1].c[0]) > EPSILON) ||
	      (fabs (crt_object->vertices[0].c[1] - crt_object->vertices
		     [crt_object->vertex_number - 1].c[1]) > EPSILON))
	    {
	      if (crt_object->make_polygon == TRUE)
		{
		  INFO ("The polyline object '%s' is considered a polygon; \
no merging necessary", crt_object->name);
		  tested_objects++;
		  continue;
		}
	      else
		DEBUG
		  ("The object '%s' is not a polygon. Trying to merge...",
		   crt_object->name);

	      // try to merge this object with another one
	      merge_done = scenario_try_merge_object (scenario, object_i);

	      if (merge_done == FALSE)
		{
		  DEBUG ("Unable to merge, removing object '%s'...",
			 scenario->objects[object_i].name);
		  scenario_remove_object (scenario, object_i);

#ifdef MESSAGE_DEBUG
		  scenario_print (scenario);

		  // loop for each object
		  for (object_i = 0; object_i < scenario->object_number;
		       object_i++)
		    object_print (&(scenario->objects[object_i]));
#endif
		  //              return ERROR;
		}
	    }
	  else
	    tested_objects++;
	}
      if (tested_objects == scenario->object_number)
	break;
    }

  DEBUG ("* Object validation and initialization done (%d objects)\n",
	 scenario->object_number);

#ifdef MESSAGE_DEBUG

  DEBUG ("############################################");

  scenario_print (scenario);

  // loop for each object
  for (object_i = 0; object_i < scenario->object_number; object_i++)
    object_print_blh (&(scenario->objects[object_i]));

  DEBUG ("############################################");
#endif

  // loop for each motion for init
  for (motion_i = 0; motion_i < scenario->motion_number; motion_i++)
    {
      // do initialization of motion indexes
      if (motion_init_index (&(scenario->motions[motion_i]),
			     scenario, cartesian_coord_syst) == ERROR)
	{
	  WARNING ("Error while initializing motion indexes");
	  return ERROR;
	}
    }

  DEBUG ("* Motion validation and initialization done (%d motions)\n",
	 scenario->motion_number);

  // init indexes of each connection; since this may take a while
  // for large scenarios, a counter is displayed
  for (connection_i = 0; connection_i < scenario->connection_number;
       connection_i++)
    {
      /* Commented out to speed up processing for large scenarios
         fprintf (stderr, "Initializing indexes for connection %d (out of %d)\r",
         connection_i, scenario->connection_number);
       */

      // do initialization of connection indexes
      if (connection_init_indexes (&(scenario->connections[connection_i]),
				   scenario) == ERROR)
	{
	  WARNING ("Error while initializing connection indexes");
	  return ERROR;
	}
    }

  if (deltaQ_disabled == FALSE)
    {
      // precompute status for each connection; since this may take a while
      // for large scenarios, a counter is displayed
      for (connection_i = 0; connection_i < scenario->connection_number;
	   connection_i++)
	{
	  /*  Commented out to speed up processing for large scenarios
	     fprintf (stderr,
	     "Initializing state for connection %d (out of %d)\r",
	     connection_i, scenario->connection_number);
	   */

	  // reset number of iterations
	  num_iterations = 0;
	  INFO ("--- Initializing connection %d ---", connection_i);

	  do
	    {
	      // compute deltaQ
	      if (connection_deltaQ (&(scenario->connections[connection_i]),
				     scenario, &deltaQ_changed) == ERROR)
		{
		  WARNING ("Error while computing connection deltaQ");
		  return ERROR;
		}

#ifdef MESSAGE_DEBUG
	      DEBUG ("Current connection state:");
	      connection_print (&(scenario->connections[connection_i]));
#endif

	      // if deltaQ didn't change with respect to the previous 
	      // iteration then break, else continue
	      if (deltaQ_changed == FALSE)
		break;
	      else
		num_iterations++;

	      // if maximum number of iterations was reached it means 
	      // a steady state cannot be achieved => warning
	      if (num_iterations >= MAXIMUM_PRECOMPUTE)
		{
		  // FIXME: It appears that the maximum number of 
		  // iterations is sometimes exceeded even though
		  // the operating rate is stable because the deltaQ
		  // parameters are not. Cause is not yet known.
		  DEBUG ("Maximum number of iterations (%d) was \
reached during precomputation phase without reaching steady state", MAXIMUM_PRECOMPUTE);
		  break;
		}
	    }
	  while (1);

	  // print "nice" separator between connections
	  INFO ("--- Connection %d initialization finished ---",
		connection_i);
	}

      DEBUG ("* Connection validation and initialization done (%d \
connections)      \n", scenario->connection_number);
    }

  return SUCCESS;
}


// compute the deltaQ for all connections of the given scenario;
// deltaQ parameters are returned in the corresponding fields of
// the connection objects;
// return SUCCESS on succes, ERROR on error
int
scenario_deltaQ (struct scenario_class *scenario, double current_time)
{
  int connection_i, deltaQ_changed;
  struct connection_class *connection;
  struct fixed_deltaQ_class *fixed_deltaQ;

  // calculate the state for active nodes according to 
  // 'connections' object in 'scenario'
  for (connection_i = 0; connection_i < scenario->connection_number;
       connection_i++)
    {
      /*  Commented out to speed up processing for large scenarios
         fprintf (stderr,
         "\t\t\tcalculation => connection %d                 \r",
         connection_i);
       */
      connection = &(scenario->connections[connection_i]);

      // check whether a fixed_deltaQ structure can be applied
      if (connection->fixed_deltaQ_number > 0)
	{
	  fixed_deltaQ = &(connection->fixed_deltaQs
			   [connection->fixed_deltaQ_crt]);

	  // advance to next record if needed
	  if (fixed_deltaQ->end_time <= current_time)
	    if (connection->fixed_deltaQ_crt
		< (connection->fixed_deltaQ_number - 1))
	      {
		connection->fixed_deltaQ_crt++;
		fixed_deltaQ = &(connection->fixed_deltaQs
				 [connection->fixed_deltaQ_crt]);
	      }

	  DEBUG ("fixed_deltaQ: fixed_deltaQ_number=%d fixed_deltaQ_crt=%d \
current_time=%.2f", connection->fixed_deltaQ_number, connection->fixed_deltaQ_crt, current_time);

	  if (fixed_deltaQ->start_time <= current_time)
	    {
	      connection->bandwidth_defined = TRUE;
	      connection->bandwidth = fixed_deltaQ->bandwidth;
	      connection->loss_rate_defined = TRUE;
	      connection->loss_rate = fixed_deltaQ->loss_rate;
	      connection->delay_defined = TRUE;
	      connection->delay = fixed_deltaQ->delay;
	      connection->jitter_defined = TRUE;
	      connection->jitter = fixed_deltaQ->jitter;
	    }
	  else
	    {
	      connection->bandwidth_defined = FALSE;
	      connection->loss_rate_defined = FALSE;
	      connection->delay_defined = FALSE;
	      connection->jitter_defined = FALSE;
	    }
	}

      if (connection_deltaQ
	  (&(scenario->connections[connection_i]), scenario,
	   &deltaQ_changed) == ERROR)
	return ERROR;
    }

  return SUCCESS;
}

// reset the interference_accounted flag for all nodes
void
scenario_reset_node_interference_flag (struct scenario_class *scenario)
{
  int node_i, interf_j;
  struct node_class *node;

  // for all the nodes and all the interfaces, reset interference flag
  for (node_i = 0; node_i < scenario->node_number; node_i++)
    {
      node = &(scenario->nodes[node_i]);
      for (interf_j = 0; interf_j < node->interface_number; interf_j++)
	node->interfaces[interf_j].interference_accounted = FALSE;
    }
}

// try to merge an object specified by index 'merge_object_i' to other
// objects in scenario; 
// return TRUE if a merge operation was performed, FALSE otherwise
int
scenario_try_merge_object (struct scenario_class *scenario,
			   int merge_object_i)
{
  int crt_object_j;
  struct object_class *merge_object = &(scenario->objects[merge_object_i]);

  // loop for each object
  for (crt_object_j = 0; crt_object_j < scenario->object_number;
       crt_object_j++)
    {
      struct object_class *crt_object = &(scenario->objects[crt_object_j]);

      if (merge_object_i == crt_object_j)
	continue;

#ifdef MESSAGE_DEBUG
      object_print (merge_object);

      DEBUG ("Checking object '%s' (index=%d) for merge...",
	     crt_object->name, crt_object_j);
      object_print (crt_object);
#endif

      // first vertex equals last vertex
      if ((fabs (merge_object->vertices[0].c[0] -
		 crt_object->vertices[crt_object->vertex_number - 1].c[0]) <
	   EPSILON) &&
	  (fabs (merge_object->vertices[0].c[1] -
		 crt_object->vertices[crt_object->vertex_number - 1].c[1]) <
	   EPSILON))
	{
	  INFO ("Merging object '%s' to object '%s' by direct appending...",
		merge_object->name, crt_object->name);

	  if (scenario_merge_objects (scenario, crt_object_j,
				      merge_object_i, TRUE) == SUCCESS)
	    return TRUE;
	  else
	    return FALSE;
	}
      // first vertex equals first vertex!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      else if ((fabs (merge_object->vertices[0].c[0] -
		      crt_object->vertices[0].c[0]) < EPSILON) &&
	       (fabs (merge_object->vertices[0].c[1] -
		      crt_object->vertices[0].c[1]) < EPSILON))
	{
	  INFO
	    ("Merging object '%s' to object '%s' by direct appending after reversing...",
	     merge_object->name, crt_object->name);

	  if (scenario_merge_objects (scenario, crt_object_j,
				      merge_object_i, TRUE) == SUCCESS)
	    return TRUE;
	  else
	    return FALSE;
	}
      // last vertex equals first vertex
      else
	if ((fabs
	     (merge_object->vertices[merge_object->vertex_number - 1].c[0] -
	      crt_object->vertices[0].c[0]) < EPSILON)
	    &&
	    (fabs
	     (merge_object->vertices[merge_object->vertex_number - 1].c[1] -
	      crt_object->vertices[0].c[1]) < EPSILON))
	{
	  INFO ("Merging object '%s' to object '%s' by direct appending...",
		crt_object->name, merge_object->name);
	  if (scenario_merge_objects (scenario, merge_object_i,
				      crt_object_j, TRUE) == SUCCESS)
	    return TRUE;
	  else
	    return FALSE;
	}
      // last vertex equals last vertex
      else
	if ((fabs
	     (merge_object->vertices[merge_object->vertex_number - 1].c[0] -
	      crt_object->vertices[crt_object->vertex_number - 1].c[0]) <
	     EPSILON)
	    &&
	    (fabs
	     (merge_object->vertices[merge_object->vertex_number - 1].c[1] -
	      crt_object->vertices[crt_object->vertex_number - 1].c[1]) <
	     EPSILON))
	{
	  INFO ("Merging object '%s' to object '%s' by reverse appending...",
		crt_object->name, merge_object->name);
	  if (scenario_merge_objects (scenario, merge_object_i,
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
int
scenario_merge_objects (struct scenario_class *scenario, int object_i1,
			int object_i2, int direct_merge)
{
  int i;
  struct object_class *object1 = &(scenario->objects[object_i1]);
  struct object_class *object2 = &(scenario->objects[object_i2]);

  // check if direct vertex merging was requested
  if (direct_merge == TRUE)
    {
      // if first vertex equals first vertex, then the object1
      // vertex order must first be reversed
      if ((fabs (object1->vertices[0].c[0] -
		 object2->vertices[0].c[0]) < EPSILON) &&
	  (fabs (object1->vertices[0].c[1] -
		 object2->vertices[0].c[1]) < EPSILON))
	{
	  struct coordinate_class aux_coord;
	  INFO ("@@@@@@@@@@@@@@@@@@@@ Reversing");
	  //object_print(object1);

	  for (i = 0; i < object1->vertex_number / 2; i++)
	    {
	      coordinate_copy (&aux_coord, &(object1->vertices[i]));
	      coordinate_copy
		(&(object1->vertices[i]),
		 &(object1->vertices[object1->vertex_number - 1 - i]));
	      coordinate_copy (&
			       (object1->vertices
				[object1->vertex_number - 1 - i]),
			       &aux_coord);
	      //INFO("-- after step %d (exchanging %d and %d)", i,
	      //   i, object1->vertex_number-1-i);
	      //object_print(object1);
	    }
	  //INFO("-- final");
	  //object_print(object1);
	}

      // append vertices of object2 in direct order
      for (i = 1; i < object2->vertex_number; i++)
	if (object1->vertex_number < MAX_VERTICES)
	  {
	    coordinate_copy (&(object1->vertices[object1->vertex_number]),
			     &(object2->vertices[i]));
	    object1->vertex_number++;
	  }
	else
	  {
	    WARNING ("Maximum number of vertices (%d) exceeded",
		     MAX_VERTICES);
	    return ERROR;
	  }
    }
  else				// reverse vertex merging
    {
      for (i = object2->vertex_number - 2; i >= 0; i--)
	if (object1->vertex_number < MAX_VERTICES)
	  {
	    coordinate_copy (&(object1->vertices[object1->vertex_number]),
			     &(object2->vertices[i]));
	    object1->vertex_number++;
	  }
	else
	  {
	    WARNING ("Maximum number of vertices (%d) exceeded",
		     MAX_VERTICES);
	    return ERROR;
	  }
    }
  strncat (object1->name, object2->name,
	   MAX_STRING - strlen (object1->name) - 1);

#ifdef MESSAGE_DEBUG
  object_print (object1);
#endif

  scenario_remove_object (scenario, object_i2);

  return SUCCESS;
}

// remove the object specified by index 'object_i' from the scenario; 
// return SUCCESS on success, ERROR on error
int
scenario_remove_object (struct scenario_class *scenario, int object_i)
{
  int i;

  if (object_i < 0 || object_i > (scenario->object_number - 1))
    {
      WARNING ("Object index %d is invalid", object_i);
      return FALSE;
    }
  else
    {
      for (i = object_i; i < (scenario->object_number - 1); i++)
	object_copy (&(scenario->objects[i]), &(scenario->objects[i + 1]));
      scenario->object_number--;
    }

  return TRUE;
}
