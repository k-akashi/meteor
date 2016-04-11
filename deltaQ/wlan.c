
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
 * File name: wlan.c
 * Function: Model WLAN communication and compute deltaQ parameters for
 *           connections
 *
 * Author: Razvan Beuran
 *
 * $Id: wlan.c 155 2013-09-05 00:27:09Z razvan $
 *
 ***********************************************************************/

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "deltaQ.h"
#include "message.h"
#include "generic.h"
#include "wlan.h"


//////////////////////////////////////////////////
// Initialize WLAN adapter structures according to
// manufacturer specifications
//////////////////////////////////////////////////

// Reference: ORiNOCO 802.11b WLAN adapter
struct parameters_802_11b orinoco = {
  "ORiNOCO 802.11b",		// name
  TRUE,				// use_model1
  {-94, -91, -87, -82}
  ,				// Pr_thresholds (from low to high)
  0.08,				// Pr_threshold_fer
  1.0,				// model1_alpha
  FALSE				// use_model2
};

// Reference: dei80211mr model for NS-2 with acceptable PER set to 1e-4 and
// noise = -100 dBm
struct parameters_802_11b dei80211mr = {
  "NS-2 dei80211mr",		// name
  TRUE,				// use_model1
  {-98, -95, -91, -88}
  ,				// Pr_thresholds (from low to high)
  0.08,				// Pr_threshold_fer
  1.0,				// model1_alpha
  FALSE				// use_model2
};

// Reference: Cisco Aironet 340 WLAN adapter and
//            Intersil HFA3861B DSSS Broadband Processor
//            (used for Cisco Aironet 802.11b)
struct parameters_802_11b cisco_aironet_340 = {
  "Cisco Aironet 340 802.11b",	// name
  TRUE,				// use_model1
  {-90, -88, -87, -83}
  ,				// Pr_thresholds (from low to high)
  0.08,				// Pr_threshold_fer
  1.0,				// model1_alpha
  TRUE,				// use_model2
  {4255.180, 787.4195, 243.0763, 12.44204}
  ,				//model2_a
  {-1.811341, -1.548256, -1.562894, -1.234009}	// model2_b
};

// Reference: Cisco Aironet 802.11a/b/g
struct parameters_802_11b cisco_aironet_abg_b = {
  "Cisco Aironet 802.11a/b/g -- b mode",	// name
  TRUE,				// use_model1
  // Pr_thresholds (from low to high); some values were
  // modified in order to create an increasing order of
  // thresholds as rate increases
  {-94, -93, -92, -90}
  ,				// Pr_thresholds (from low to high)
  0.08,				// Pr_threshold_fer
  1.0,				// model1_alpha
  FALSE				// use_model2
};

// Reference: Cisco Aironet 802.11a/b/g
//            (some values were modified in order to
//            create an increasing order of thresholds
//            as rate increases
struct parameters_802_11g cisco_aironet_abg_bg = {
  "Cisco Aironet 802.11a/b/g -- b/g mode",	// name
  TRUE,				// use_model1
  // Pr_thresholds (from low to high); some values were
  // modified in order to create an increasing order of
  // thresholds as rate increases
  {-94, -93, -92, -91.33 /* modified from -86 */ ,
   -90.67 /* modified from -86 */ , -90, -88 /* modified from -86 */ , -86,
   -84, -80, -75, -71}
  ,				// Pr_thresholds (from low to high)
  0.08,				// Pr_threshold_fer
  0.10,				// Pr_threshold_per
  1.0				// model1_alpha
};

// Reference: Cisco Aironet 802.11a/b/g
//            (used values for frequency range 5.15-5.25 GHz
//            which is employed in Japan)
struct parameters_802_11a cisco_aironet_abg_a = {
  "Cisco Aironet 802.11a/b/g -- a mode",	// name
  TRUE,				// use_model1
  // Pr_thresholds (from low to high); some values were
  // modified in order to create an increasing order of
  // thresholds as rate increases
  {-87, -85.75, -84.5, -83.25 /* last 3 values modified from -87 */ ,
   -82, -79, -74, -72}
  ,
  0.10,				// Pr_threshold_per
  1.0				// model1_alpha
};


/////////////////////////////////////////////
// Arrays of constants for convenient use
/////////////////////////////////////////////

double b_operating_rates[] = { 1e6, 2e6, 5.5e6, 11e6 };

double g_operating_rates[] = { 1e6, 2e6, 5.5e6, 6e6, 9e6, 11e6,
  12e6, 18e6, 24e6, 36e6, 48e6, 54e6
};
double a_operating_rates[] = { 6e6, 9e6, 12e6, 18e6, 24e6, 36e6, 48e6, 54e6 };


///////////////////////////////////////
// Various functions
///////////////////////////////////////

// print parameters of an 802.11b model
void
print_802_11b_parameters (struct parameters_802_11b *params)
{
  printf ("802.11b model parameters (name='%s')\n", params->name);
  printf ("\tuse_model1=%d use_model2=%d\n", params->use_model1,
	  params->use_model2);
}

// print parameters of an 802.11g model
void
print_802_11g_parameters (struct parameters_802_11g *params)
{
  printf ("802.11g model parameters (name='%s')\n", params->name);
  printf ("\tuse_model1=%d\n", params->use_model1);
}

// print parameters of an 802.11a model
void
print_802_11a_parameters (struct parameters_802_11a *params)
{
  printf ("802.11a model parameters (name='%s')\n", params->name);
  printf ("\tuse_model1=%d\n", params->use_model1);
}

/*
// do various post-init configurations;
// return SUCCESS on succes, ERROR on error
int wlan_connection_post_init(struct connection_class *connection)
{
  if(connection->standard==WLAN_802_11B)
    {
      // if no channel was defined choose an appropriate channel
      if(connection->channel==UNDEFINED_VALUE)
	connection->channel = DEFAULT_CHANNEL_BG;
    }
  else if(connection->standard==WLAN_802_11G)
    {
      // if no channel was defined choose an appropriate channel
      if(connection->channel==UNDEFINED_VALUE)
	connection->channel = DEFAULT_CHANNEL_BG;
   }
  else if(connection->standard==WLAN_802_11A)
    {
      // if no channel was defined choose an appropriate channel
      if(connection->channel==UNDEFINED_VALUE)
	connection->channel = DEFAULT_CHANNEL_A;
    }
  else
    {
      WARNING("Connection standard '%d' undefined", connection->standard);
      return ERROR;
    }

  return SUCCESS;
}
*/

///////////////////////////////////////
// deltaQ parameter computation
///////////////////////////////////////

// update Pr0, which is the power radiated by _this_ interface
// at the distance of 1 m
void
wlan_interface_update_Pr0 (struct interface_class *interface)
{
  // we use classic signal attenuation equation, and antenna gain
  interface->Pr0 = interface->Pt
    - 20 * log10 ((4 * M_PI) / (SPEED_LIGHT / FREQUENCY_BG))
    + interface->antenna_gain;

  interface->Pr0_a = interface->Pt
    - 20 * log10 ((4 * M_PI) / (SPEED_LIGHT / FREQUENCY_A))
    + interface->antenna_gain;

}

// update the changing properties of a connection (e.g., relative distance, Pr)
// after the state of the system changed;
// return SUCCESS on succes, ERROR on error
int
wlan_connection_update (struct connection_class *connection,
			struct scenario_class *scenario)
{
  // helping "shortcuts"
  struct node_class *node_rx = &(scenario->nodes[connection->to_node_index]);
  struct node_class *node_tx =
    &(scenario->nodes[connection->from_node_index]);
  struct environment_class *environment =
    &(scenario->environments[connection->through_environment_index]);

  // update distance in function of the new node positions
  connection->distance =
    coordinate_distance (&(node_rx->position), &(node_tx->position));

  // limit distance to prevent numerical errors
  if (connection->distance < MINIMUM_DISTANCE)
    {
      connection->distance = MINIMUM_DISTANCE;
      DEBUG ("Limiting distance between nodes to %.2f m", MINIMUM_DISTANCE);
    }

  // update Pr in function of the Pr0 of node_tx and distance
  if (environment->num_segments > 0)
    {
      // check if using pre-defined or one-segment environment
      if (environment->num_segments == 1)
	{
	  double distance;
	  double antenna_dir_attenuation_tx = antenna_directional_attenuation
	    (node_tx,
	     &(node_tx->interfaces[connection->from_interface_index]),
	     node_rx);
	  double antenna_dir_attenuation_rx = antenna_directional_attenuation
	    (node_rx, &(node_rx->interfaces[connection->to_interface_index]),
	     node_tx);

	  if (environment->length[0] == -1)
	    distance = connection->distance;
	  else
	    distance = environment->length[0];

#define INTEROP2014

#ifndef INTEROP2014
	  if (connection->standard == WLAN_802_11A)
	    connection->Pr
	      = node_tx->interfaces[connection->from_interface_index].Pr0_a;
	  else if (connection->standard == WLAN_802_11B ||
		   connection->standard == WLAN_802_11G)
	    connection->Pr
	      = node_tx->interfaces[connection->from_interface_index].Pr0;
	  else
	    {
	      WARNING ("Undefined connection type (%d)",
		       connection->standard);
	      return ERROR;
	    }

#else

	  // for Interop 2014 we decide to adapt the Pr to the one of 
	  // the node with highest transmission power
	  if (connection->standard == WLAN_802_11A)
	    if (node_tx->interfaces[connection->from_interface_index].Pt
		>= node_rx->interfaces[connection->to_interface_index].Pt)
	      connection->Pr
		= node_tx->interfaces[connection->from_interface_index].Pr0_a;
	    else
	      {
		connection->Pr
		  = node_rx->interfaces[connection->to_interface_index].Pr0_a;
		printf("deltaQ: DEBUG: Interop2014: Using Pt of interface %d instead of that of %d because it is larger!\n", node_rx->interfaces[connection->to_interface_index].id, node_tx->interfaces[connection->from_interface_index].id); 
	      }
	  else if (connection->standard == WLAN_802_11B ||
		   connection->standard == WLAN_802_11G)
	    if (node_tx->interfaces[connection->from_interface_index].Pt
		>= node_rx->interfaces[connection->to_interface_index].Pt)
	      connection->Pr
		= node_tx->interfaces[connection->from_interface_index].Pr0;
	    else
	      {
		connection->Pr
		  = node_rx->interfaces[connection->to_interface_index].Pr0;
		printf("deltaQ: DEBUG: Interop2014: Using Pt of interface %d instead of that of %d because it is larger!\n", node_rx->interfaces[connection->to_interface_index].id, node_tx->interfaces[connection->from_interface_index].id); 
	      }
	  else
	    {
	      WARNING ("Undefined connection type (%d)",
		       connection->standard);
	      return ERROR;
	    }

#endif

	  // add the other terms that are independent on standard
	  connection->Pr +=
	    ((node_tx->interfaces[connection->from_interface_index].
	      antenna_gain - antenna_dir_attenuation_tx) -
	     10 * environment->alpha[0] * log10 (distance) -
	     environment->W[0] + randn (0,
					environment->sigma[0]) +
	     (node_rx->interfaces[connection->to_interface_index].
	      antenna_gain - antenna_dir_attenuation_rx));
	}
      else			// dynamic environment
	{
	  int i;
	  double current_distance = 0;
	  double W_sum;
	  double sigma_square_sum;

	  double antenna_dir_attenuation_tx = antenna_directional_attenuation
	    (node_tx,
	     &(node_tx->interfaces[connection->from_interface_index]),
	     node_rx);
	  double antenna_dir_attenuation_rx = antenna_directional_attenuation
	    (node_rx, &(node_rx->interfaces[connection->to_interface_index]),
	     node_tx);

	  // SUMMARY: Cumulated effects of multiple dynamic environments
	  //          are computed as follows:
	  //   - attenuation calculation is based on alpha and
	  //     distance of environments by translating dB to mW
	  //     space and back
	  //   - computation of wall attenuation uses the sum of
	  //     attenuations in dB space
	  //   - computation of global sigma uses property of normal
	  //     distributions

	  // init step 0
	  if (connection->standard == WLAN_802_11A)
	    connection->Pr =
	      node_tx->interfaces[connection->from_interface_index].Pr0_a;
	  else if (connection->standard == WLAN_802_11B
		   || connection->standard == WLAN_802_11G)
	    connection->Pr =
	      node_tx->interfaces[connection->from_interface_index].Pr0;
	  else
	    {
	      WARNING ("Undefined connection type (%d)",
		       connection->standard);
	      return ERROR;
	    }

	  // add the other terms that are independent on standard
	  connection->Pr +=
	    ((node_tx->interfaces[connection->from_interface_index].
	      antenna_gain - antenna_dir_attenuation_tx) -
	     10 * environment->alpha[0] * log10 (environment->length[0]) +
	     (node_rx->interfaces[connection->to_interface_index].
	      antenna_gain - antenna_dir_attenuation_rx));

	  W_sum = environment->W[0];

	  sigma_square_sum = environment->sigma[0] * environment->sigma[0];

	  DEBUG ("Pr=%f W_s=%.12f s_s_s=%f\n", connection->Pr,
		 W_sum, sigma_square_sum);

	  // for multiple segments, attenuation parameters must be cumulated
	  for (i = 1; i < environment->num_segments; i++)
	    {
	      // compute the sum of wall attenuation (in dB)
	      W_sum += environment->W[i];

	      // compute the sum of sigmas (we use the property that the
	      // sum of two normal distributions is a normal distribution
	      // with the variance equal to the variances of the
	      // summed distributions
	      sigma_square_sum +=
		(environment->sigma[i] * environment->sigma[i]);

	      // the formula below takes into account the total distance
	      // between the transmitter and the current point
	      current_distance += environment->length[i - 1];
	      connection->Pr -= (10 * environment->alpha[i] *
				 (log10 (current_distance +
					 environment->length[i]) -
				  log10 (current_distance)));
	    }

	  connection->Pr -= W_sum;

	  // take into account shadowing
	  connection->Pr += randn (0, sqrt (sigma_square_sum));

	  DEBUG ("Pr=%f W_s=%f s_s_s=%f\n", connection->Pr,
		 W_sum, sigma_square_sum);
	}
    }
  else
    {
      WARNING ("Environment %s contains inconsistencies", environment->name);
      return ERROR;
    }

  return SUCCESS;
}

// get a pointer to a parameter structure corresponding to
// the receiving interface adapter and the connection standard;
// return value is this pointer, or NULL if error
void *
wlan_get_interface_adapter (struct connection_class *connection,
			    struct interface_class *interface)
{
  void *adapter = NULL;

  if (connection->standard == WLAN_802_11B)
    {
      if (interface->adapter_type == ORINOCO)
	adapter = (&orinoco);
      else if (interface->adapter_type == DEI80211MR)
	adapter = (&dei80211mr);
      else if (interface->adapter_type == CISCO_340)
	adapter = (&cisco_aironet_340);
      else if (interface->adapter_type == CISCO_ABG)
	adapter = (&cisco_aironet_abg_b);
      else
	{
	  WARNING ("Defined WLAN adapter type %d is incompatible with \
connection standard %d", interface->adapter_type, WLAN_802_11B);
	  return NULL;
	}
    }
  else if (connection->standard == WLAN_802_11G)
    {
      if (interface->adapter_type == CISCO_ABG)
	adapter = (&cisco_aironet_abg_bg);
      else
	{
	  WARNING ("Defined WLAN adapter type %d is incompatible with \
connection standard %d", interface->adapter_type, WLAN_802_11G);
	  return NULL;
	}
    }
  else if (connection->standard == WLAN_802_11A)
    {
      if (interface->adapter_type == CISCO_ABG)
	adapter = (&cisco_aironet_abg_a);
      else
	{
	  WARNING ("Defined WLAN adapter type %d is incompatible with \
connection standard %d", interface->adapter_type, WLAN_802_11A);
	  return NULL;
	}
    }

  return adapter;
}

// do compute channel interference between the current connection
// and a potentially interfering connection 'connection_i';
// return SUCCESS on succes, ERROR on error
int
compute_channel_interference (struct connection_class *connection,
			      struct connection_class *connection_i,
			      struct scenario_class *scenario)
{
  struct connection_class virtual_connection;
  double attenuation = 0;
  int channel_distance;

  void *adapter;

  // do not consider connections between this node and itself
  if (connection_i->from_node_index == connection->to_node_index)
    {
      return TRUE;
    }

  // do not consider again this node if it was processed before
  if (scenario->nodes
      [connection_i->from_node_index].interfaces[connection_i->
						 from_interface_index].
      interference_accounted == TRUE)
    {
      INFO ("Interference with node '%s' already accounted for",
	    connection_i->from_node);
      return TRUE;
    }

  // create a virtual connection that is used only locally
  // the connection is made of connection->to_node (receiver)
  // connection_i->from_node (transmitter) and
  // connection_i->through_environment (later use better
  // environment calculation)
  // all other fields are also inherited from connection_i
  INFO ("Building a virtual connection from '%s' to '%s'",
	connection_i->from_node, connection->to_node);

  // mark the interference_accounted flag
  scenario->nodes[connection_i->from_node_index].interfaces[connection_i->
							    from_interface_index].
    interference_accounted = TRUE;

  connection_copy (&virtual_connection, connection_i);
  virtual_connection.to_node_index = connection->to_node_index;

#ifdef MESSAGE_DEBUG
  DEBUG ("Connections and environments info:");
  connection_print (connection);
  connection_print (connection_i);
  connection_print (&virtual_connection);

  environment_print
    (&(scenario->environments[connection->through_environment_index]));
  environment_print
    (&(scenario->environments[connection_i->through_environment_index]));
  environment_print
    (&(scenario->environments[virtual_connection.through_environment_index]));
#endif

  // compute Pr for the virtual connection
  wlan_connection_update (&virtual_connection, scenario);
  INFO ("Pr for virtual connection = %.3f dBm", virtual_connection.Pr);

  ////////////////////////////////////////////////////////////
  // the above power doesn't take into account specific
  // inter-channel attenuation, so we need to compute this too

  // compute channel distance first, no matter what connection standards are
  channel_distance = (int) fabs (connection->channel - connection_i->channel);

  // check whether the interfering connection is of type b,
  // or g operating in b mode
  if (connection_i->standard == WLAN_802_11B ||
      (connection_i->standard == WLAN_802_11G &&
       (connection_i->operating_rate == 0 || connection_i->operating_rate == 1
	|| connection_i->operating_rate == 2
	|| connection_i->operating_rate == 5)))
    {
      INFO
	("Interfering connection is 'b'/'g' => DSSS/CCK attenuation model");
      if (channel_distance == 0)
	attenuation = 0;
      else if (channel_distance <= 4)
	attenuation = 10 * log10 ((22.0 - channel_distance * 5) / 22.0);
      else if (channel_distance <= 8)
	attenuation = 10 * log10 ((44.0 - channel_distance * 5) / 44.0) - 30;
      /*
         else if(channel_distance<=13)
         attenuation = 10 * log10((88.0-channel_distance*5)/88.0) - 50;
         else
         {
         WARNING("Channel distance has an illegal value (%d)",
         channel_distance);
         return ERROR;
         }
       */
      else
	// minimum required attenuation according to the standard
	attenuation = -50.0;
    }
  else				// OFDM encoding
    {
      INFO ("Interfering connection is 'a' => OFDM attenuation model");
      if (channel_distance == 0)
	attenuation = 0;
      else if (channel_distance <= 3)
	attenuation = 10 * log10 ((18.0 - channel_distance * 5) / 18.0);
      else if (channel_distance <= 7)
	attenuation = 10 * log10 ((40.0 - channel_distance * 5) / 40.0) - 28;
      /*
         else
         attenuation = 10 * log10((88.0-channel_distance*5)/88.0) - 40;
       */
      else
	// minimum required attenuation according to the standard
	attenuation = -40;
    }

  // add the (negative) attenuation to the received power
  virtual_connection.Pr += attenuation;

  INFO ("Power after attenuation: Pr=%f (inter channel distance=%d)",
	virtual_connection.Pr, channel_distance);

  adapter = wlan_get_interface_adapter
    (connection,
     &((scenario->nodes[connection->from_node_index]).
       interfaces[connection->from_interface_index]));
  //printf("ADAPTER=%p\n", adapter);

  // check whether a suitable adapter was found
  if (adapter == NULL)
    {
      WARNING ("No suitable adapter found for connection");
      return ERROR;
    }

  // for interfaces that are noise sources, add the Pr value to the 
  // interference noise directly
  struct interface_class *interface
    =
    &(scenario->nodes[connection_i->from_node_index].
      interfaces[connection_i->from_interface_index]);

  if (interface->noise_source == TRUE)
    {
      if (scenario->current_time >= interface->noise_start_time &&
	  scenario->current_time < interface->noise_end_time)
	{
	  INFO ("Interference from noise source");

	  // add Pr to interference noise
	  connection->interference_noise =
	    add_powers (connection->interference_noise, virtual_connection.Pr,
			MINIMUM_NOISE_POWER);
	}
    }
  // if the noise connection power is inferior to the sensitivity
  // threshold of the lowest operating rate of the affected connection,
  // then this power will indeed have effects of noise and induce frame errors
  else if ((connection->standard == WLAN_802_11B &&
	    virtual_connection.Pr <
	    ((struct parameters_802_11b *) adapter)->
	    Pr_thresholds[B_RATE_1MBPS])
	   || (connection->standard == WLAN_802_11G
	       && virtual_connection.Pr <
	       ((struct parameters_802_11g *) adapter)->
	       Pr_thresholds[G_RATE_1MBPS])
	   || (connection->standard == WLAN_802_11A
	       && virtual_connection.Pr <
	       ((struct parameters_802_11a *) adapter)->
	       Pr_thresholds[A_RATE_6MBPS]))
    {
      INFO ("Interference from transmission of other stations (noise type)");

      // add Pr to interference noise
      connection->interference_noise =
	add_powers (connection->interference_noise, 
		    virtual_connection.Pr,
		    MINIMUM_NOISE_POWER);

      DEBUG ("Channel noise=%f (inter channel distance=%d)\n",
	     connection->interference_noise, channel_distance);
    }

  // if the noise connection power is superior or equal to the
  // sensitivity threshold of the lowest operating rate, then
  // the corresponding signal will be seen as an 802.11 signal
  // and will trigger the CSMA/CA mechanism of the station;
  // we use the equation derived from Gupta for modelling
  // this effect
  else
    {
      INFO ("Interference between concurrent stations (CSMA/CA resolution)");
      connection->concurrent_stations++;

      // a special case is when the affected station is 'g' and
      // the interfering station is 'b'; in this case the 'g' station
      // will have to operate in compatibility mode
      if (connection->standard == WLAN_802_11G &&
	  connection_i->standard == WLAN_802_11B)
	{
	  INFO
	    ("Interference g <- b => setting compatibility_mode of affected \
connection to TRUE");
	  connection->compatibility_mode = TRUE;
	}
    }

  return SUCCESS;
}

// compute the interference caused by other stations
// through either concurrent transmission or through noise;
// return SUCCESS on succes, ERROR on error
int
wlan_interference (struct connection_class *connection,
		   struct scenario_class *scenario)
{
  int connection_i;
  //float distance;
  //double rx_power;

  //reset interference indicators
  connection->concurrent_stations = 0;
  connection->interference_noise = MINIMUM_NOISE_POWER;

  // reset interference flags
  scenario_reset_node_interference_flag (scenario);

  // search connections in scenario that operate on same band
  // and are closely located to the current connection
  for (connection_i = 0; connection_i < scenario->connection_number;
       connection_i++)
    {
      // look only at connections that are not the same with
      // the current one
      if (connection != &(scenario->connections[connection_i]))
	{
	  //printf("Found a different connection...\n");
	  /*
	     this will be used perhaps later for optimization purposes

	     // measure distance between the current connection and the
	     // connection with index connection_i
	     if(compute_connection_distance
	     (connection, &(scenario->connections[connection_i]),
	     scenario, &distance)==ERROR)
	     {
	     WARNING("Cannot compute distance between connections");
	     return ERROR;
	     }

	     // ignore connection if located at a distance
	     // larger than the interference range
	     if(distance>INTERFERENCE_RANGE)
	     break;
	   */

	  // check if connections are both either 'b'/'g' type or both 'a' type
	  if (((connection->standard == WLAN_802_11B ||
		connection->standard == WLAN_802_11G) &&
	       (scenario->connections[connection_i].standard == WLAN_802_11B
		|| scenario->connections[connection_i].standard ==
		WLAN_802_11G)) || (connection->standard == WLAN_802_11A
				   && scenario->
				   connections[connection_i].standard ==
				   WLAN_802_11A))
	    {
	      INFO ("------------------------------------------------");
	      // interference b-b, g-g, b-g, g-b
	      if (connection->standard == WLAN_802_11B ||
		  connection->standard == WLAN_802_11G)
		INFO ("Interference 'b'/'g' <-> 'b'/'g' detected => \
determine effects");
	      else		// interference a-a
		INFO
		  ("Interference 'a' <-> 'a' detected => determine effects");

	      compute_channel_interference
		(connection, &(scenario->connections[connection_i]),
		 scenario);
	    }
	}
    }

  return SUCCESS;
}

// compute FER corresponding to the current conditions
// for a given operating rate;
// return SUCCESS on succes, ERROR on error
int
wlan_fer (struct connection_class *connection,
	  struct scenario_class *scenario, int operating_rate, double *fer)
{
  double fer1, fer2;

  int MAC_Overhead = 224;	// MAC ovearhed in bits (header + FCS)
  int OFDM_Overhead = 22;	// OFDM ovearhed in bits (service field + tail bits)

  int header;

#ifndef MODEL1_W_NOISE
  double ber2;
#endif

  //print_802_11b_parameters(adapter_802_11b);
  //print_802_11g_parameters(adapter_802_11g);
  //print_802_11a_parameters(adapter_802_11a);

  struct environment_class *environment =
    &(scenario->environments[connection->through_environment_index]);

  void *adapter;
  adapter = wlan_get_interface_adapter
    (connection,
     &((scenario->nodes[connection->from_node_index]).
       interfaces[connection->from_interface_index]));
  //printf("ADAPTER=%p\n", adapter);

  // check whether a suitable adapter was found
  if (adapter == NULL)
    {
      WARNING ("No suitable adapter found for connection");
      return ERROR;
    }

  if (connection->standard == WLAN_802_11B)
    {
      struct parameters_802_11b *adapter_802_11b =
	(struct parameters_802_11b *) adapter;

      header = MAC_Overhead / 8;

      //print_802_11b_parameters(adapter_802_11b);

#ifndef MODEL1_W_NOISE

      // use model 1 (Pr-threshold based) if enabled
      if (adapter_802_11b->use_model1 == TRUE)
	fer1 = adapter_802_11b->Pr_threshold_fer *
	  exp (adapter_802_11b->model1_alpha *
	       (adapter_802_11b->Pr_thresholds[operating_rate] -
		connection->Pr));
      else
	fer1 = 0;

      // this FER corresponds to the standard value PSDU_DSSS,
      // therefore needs to be adjusted for different packet sizes
      //fer1 = (fer1 * (connection->packet_size + header)) / PSDU_DSSS; INCORRECT
      fer1 = 1
	- pow (1 - fer1, (connection->packet_size + header) / PSDU_DSSS);

      // limit error rate for numerical reasons
      if (fer1 > MAXIMUM_ERROR_RATE)
	fer1 = MAXIMUM_ERROR_RATE;

      // use model 2 (SNR based)
      // BER fitted using a*e^(b*x)
      if (environment->num_segments > 0)
	{
	  // use direct formula for the case of 1 segment
	  if (environment->num_segments == 1)
	    connection->SNR = connection->Pr -
	      wlan_add_powers (environment->noise_power[0],
			       connection->interference_noise);
	  else
	    {
	      // we consider that noise in intermediary environments
	      // has no influence on the receiver since the power of those
	      // noise sources will attenuate; therefore we consider
	      // only the power of the noise source at the receiver
	      // (last index of the array)
	      connection->SNR = connection->Pr -
		wlan_add_powers (environment->noise_power
				 [environment->num_segments - 1],
				 connection->interference_noise);

	      // STATUS: good approximation, more accurate could be done if
	      // we consider the influence of intermediary sources by taking
	      // into account alpha and distance; a sketched implementation
	      // is available below

	      /*
	         int i;
	         double attenuated_noise, noise_power_sum = 0;


	         // compute the sum of noise powers (in mW)
	         // sources with noise power equal to MINIMUM_NOISE_POWER
	         // are excluded from computation so that we don't get
	         // an additive effect for such case
	         for(i=0; i<environment->num_segments; i++)
	         if(environment->noise_power[i] != MINIMUM_NOISE_POWER)
	         noise_power_sum += pow(10, environment->noise_power[i]/10);

	         // if all power sources had minimum level, just substract it,
	         // otherwise compute the dBm value corresponding to the sum
	         if(noise_power_sum == 0)
	         connection->SNR = connection->Pr - MINIMUM_NOISE_POWER;
	         else
	         connection->SNR = connection->Pr - 10 * log10(noise_power_sum);
	       */
	    }
	}
      else
	{
	  WARNING ("Environment %s contains inconsistencies",
		   environment->name);
	  return ERROR;
	}

      // use model 1 (SNR based) if enabled
      if (adapter_802_11b->use_model2 == TRUE)
	ber2 = adapter_802_11b->model2_a[operating_rate] *
	  exp (adapter_802_11b->model2_b[operating_rate] * connection->SNR);
      else
	ber2 = 0;

      // limit error rate for numerical reasons
      if (ber2 > MAXIMUM_ERROR_RATE)
	ber2 = MAXIMUM_ERROR_RATE;

      // compute fer2 from ber2 and packet_size
      fer2 = 1 - pow ((1 - ber2), 8 * connection->packet_size);

      // limit error rate for numerical reasons
      if (fer2 > MAXIMUM_ERROR_RATE)
	fer2 = MAXIMUM_ERROR_RATE;

#else //#ifdef MODEL1_W_NOISE

      // use model 1 (Pr-threshold based) with noise included

      // compute SNR
      if (environment->num_segments > 0)
	{
	  double combined_noise;

	  // compute noise resulting from environment noise plus
	  // interference noise
	  combined_noise = add_powers (environment->noise_power
				       [environment->num_segments - 1],
				       connection->interference_noise,
				       MINIMUM_NOISE_POWER);
	  // limit combined_noise to a minimum standard value;
	  // this is equivalent with making sure performance cannot
	  // exceed that obtained in optimum conditions
	  if (combined_noise < STANDARD_NOISE)
	    combined_noise = STANDARD_NOISE;

	  // compute SNR
	  connection->SNR = connection->Pr - combined_noise;
	}
      else
	{
	  WARNING ("Environment %s contains inconsistencies",
		   environment->name);
	  return ERROR;
	}

      // check whether model parameters were initialized
      if (adapter_802_11b->use_model1 == TRUE)
	{
	  fer1 = adapter_802_11b->Pr_threshold_fer *
	    exp (adapter_802_11b->model1_alpha *
		 (adapter_802_11b->Pr_thresholds[operating_rate] -
		  connection->SNR - STANDARD_NOISE));

	  // this FER corresponds to the standard value PSDU_DSSS,
	  // therefore needs to be adjusted for different packet sizes
	  //fer1 = (fer1 * (connection->packet_size + header)) / PSDU_DSSS; INCORRECT
	  fer1 = 1
	    - pow (1 - fer1, (connection->packet_size + header) / PSDU_DSSS);
	}
      else
	fer1 = 0;

      // limit error rate for numerical reasons
      if (fer1 > MAXIMUM_ERROR_RATE)
	fer1 = MAXIMUM_ERROR_RATE;

      fer2 = 0;

#endif //#ifndef MODEL1_W_NOISE

    }
  else if (connection->standard == WLAN_802_11G)
    {
      struct parameters_802_11g *adapter_802_11g =
	(struct parameters_802_11g *) adapter;

#ifndef MODEL1_W_NOISE

      // use model 1 (Pr-threshold based)
      // for 802.11g only this model is available for the moment

      // for 802.11b operating rates use the same FER, PER for others
      if (adapter_802_11g->use_model1 == TRUE)
	if (operating_rate == 0 || operating_rate == 1 ||
	    operating_rate == 2 || operating_rate == 5)
	  {
	    header = MAC_Overhead / 8;

	    fer1 = adapter_802_11g->Pr_threshold_fer *
	      exp (adapter_802_11g->model1_alpha *
		   (adapter_802_11g->Pr_thresholds
		    [operating_rate] - connection->Pr));

	    // this FER corresponds to the standard value PSDU_DSSS,
	    // therefore needs to be adjusted for different packet sizes
	    //fer1 = (fer1 * (connection->packet_size + header)) / PSDU_DSSS; INCORRECT
	    fer1 = 1
	      - pow (1 - fer1,
		     (connection->packet_size + header) / PSDU_DSSS);
	  }
	else
	  {
	    // add 2 for rounding purposes
	    header = (MAC_Overhead + OFDM_Overhead + 2) / 8;

	    fer1 = adapter_802_11g->Pr_threshold_per *
	      exp (adapter_802_11g->model1_alpha *
		   (adapter_802_11g->Pr_thresholds
		    [operating_rate] - connection->Pr));

	    // this FER corresponds to the standard value PSDU_OFDM,
	    // therefore needs to be adjusted for different packet sizes
	    //fer1 = (fer1 * (connection->packet_size + header)) / PSDU_OFDM; INCORRECT
	    fer1 = 1
	      - pow (1 - fer1,
		     (connection->packet_size + header) / PSDU_OFDM);
	  }
      else
	fer1 = 0;

      // limit error rate for numerical reasons
      if (fer1 > MAXIMUM_ERROR_RATE)
	fer1 = MAXIMUM_ERROR_RATE;

      // the formula below is valid both for the static and dynamic
      // environments given the model we use (see comments in the
      // implementation corresponding to 802.11b above)
      connection->SNR = connection->Pr -
	wlan_add_powers (environment->noise_power
			 [environment->num_segments - 1],
			 connection->interference_noise);

      fer2 = 0;

#else //#ifdef MODEL1_W_NOISE

      // use model 1 (Pr-threshold based) with noise included

      // compute SNR
      if (environment->num_segments > 0)
	{
	  double combined_noise;

	  // compute noise resulting from environment noise plus
	  // interference noise
	  combined_noise = add_powers (environment->noise_power
				       [environment->num_segments - 1],
				       connection->interference_noise,
				       MINIMUM_NOISE_POWER);
	  // limit combined_noise to a minimum standard value;
	  // this is equivalent with making sure performance cannot
	  // exceed that obtained in optimum conditions
	  if (combined_noise < STANDARD_NOISE)
	    combined_noise = STANDARD_NOISE;

	  // compute SNR
	  connection->SNR = connection->Pr - combined_noise;
	}
      else
	{
	  WARNING ("Environment %s contains inconsistencies",
		   environment->name);
	  return ERROR;
	}

      // check whether model parameters were initialized
      if (adapter_802_11g->use_model1 == TRUE)
	if (operating_rate == 0 || operating_rate == 1 ||
	    operating_rate == 2 || operating_rate == 5)
	  {
	    header = MAC_Overhead / 8;

	    fer1 = adapter_802_11g->Pr_threshold_fer *
	      exp (adapter_802_11g->model1_alpha *
		   (adapter_802_11g->Pr_thresholds
		    [operating_rate] - connection->SNR - STANDARD_NOISE));

	    // this FER corresponds to the standard value PSDU_DSSS,
	    // therefore needs to be adjusted for different packet sizes
	    //fer1 = (fer1 * (connection->packet_size + header)) / PSDU_DSSS; INCORRECT
	    fer1 = 1
	      - pow (1 - fer1,
		     (connection->packet_size + header) / PSDU_DSSS);
	  }
	else
	  {
	    // compute Doppler shift only for OFDM operating rates
	    // compute SNR decrease due to Doppler;
	    double relative_velocity
	      = motion_relative_velocity
	      (scenario, &(scenario->nodes[connection->from_node_index]),
	       &(scenario->nodes[connection->to_node_index]));
	    double doppler_snr_value = doppler_snr (FREQUENCY_BG,
						    WIFI_SUBCARRIER_SPACING *
						    1e3,
						    relative_velocity,
						    connection->SNR);
	    /*
	       printf("freq=%.2f GHz  subcarrier_spacing=%.1f Hz  \
	       rel_velocity=%f  SNR=%f  ", FREQUENCY_BG/1e9, WIFI_SUBCARRIER_SPACING*1e3,
	       relative_velocity, connection->SNR);
	       printf("SNR decrease due to Doppler shift = %.3f dB\n", 
	       doppler_snr_value);
	     */
	    // update SNR by subtracting the change due to Doppler shift
	    connection->SNR -= doppler_snr_value;


	    // continue with old code...
	    header = (MAC_Overhead + OFDM_Overhead + 2) / 8;

	    fer1 = adapter_802_11g->Pr_threshold_per *
	      exp (adapter_802_11g->model1_alpha *
		   (adapter_802_11g->Pr_thresholds
		    [operating_rate] - connection->SNR - STANDARD_NOISE));

	    // this FER corresponds to the standard value PSDU_OFDM,
	    // therefore needs to be adjusted for different packet sizes
	    //fer1 = (fer1 * (connection->packet_size + header)) / PSDU_OFDM; INCORRECT
	    fer1 = 1
	      - pow (1 - fer1,
		     (connection->packet_size + header) / PSDU_OFDM);
	  }
      else
	fer1 = 0;

      // limit error rate for numerical reasons
      if (fer1 > MAXIMUM_ERROR_RATE)
	fer1 = MAXIMUM_ERROR_RATE;

      fer2 = 0;

#endif //#ifndef MODEL1_W_NOISE

    }
  else if (connection->standard == WLAN_802_11A)
    {
      struct parameters_802_11a *adapter_802_11a =
	(struct parameters_802_11a *) adapter;

      header = (MAC_Overhead + OFDM_Overhead + 2) / 8;

#ifndef MODEL1_W_NOISE

      // use model 1 (Pr-threshold based)
      // for 802.11a only this model is available for the moment

      if (adapter_802_11a->use_model1 == TRUE)
	{
	  fer1 = adapter_802_11a->Pr_threshold_per *
	    exp (adapter_802_11a->model1_alpha *
		 (adapter_802_11a->Pr_thresholds
		  [operating_rate] - connection->Pr));

	  // this FER corresponds to the standard value PSDU_OFDM,
	  // therefore needs to be adjusted for different packet sizes
	  //fer1 = (fer1 * (connection->packet_size + header)) / PSDU_OFDM; INCORRECT
	  fer1 = 1
	    - pow (1 - fer1, (connection->packet_size + header) / PSDU_OFDM);
	}
      else
	fer1 = 0;

      // limit error rate for numerical reasons
      if (fer1 > MAXIMUM_ERROR_RATE)
	fer1 = MAXIMUM_ERROR_RATE;

      // the formula below is valid both for the static and dynamic
      // environments given the model we use (see comments in the
      // implementation corresponding to 802.11b above)
      connection->SNR = connection->Pr -
	wlan_add_powers (environment->noise_power
			 [environment->num_segments - 1],
			 connection->interference_noise);

      fer2 = 0;

#else //#ifdef MODEL1_W_NOISE

      // use model 1 (Pr-threshold based) with noise included

      // compute SNR
      if (environment->num_segments > 0)
	{
	  double combined_noise;

	  // compute noise resulting from environment noise plus
	  // interference noise
	  combined_noise = add_powers (environment->noise_power
				       [environment->num_segments - 1],
				       connection->interference_noise,
				       MINIMUM_NOISE_POWER);
	  // limit combined_noise to a minimum standard value;
	  // this is equivalent with making sure performance cannot
	  // exceed that obtained in optimum conditions
	  if (combined_noise < STANDARD_NOISE)
	    combined_noise = STANDARD_NOISE;

	  // compute SNR
	  connection->SNR = connection->Pr - combined_noise;
	}
      else
	{
	  WARNING ("Environment %s contains inconsistencies",
		   environment->name);
	  return ERROR;
	}

      // check whether model parameters were initialized
      if (adapter_802_11a->use_model1 == TRUE)
	{

	  // compute Doppler shift only for OFDM operating rates
	  // compute SNR decrease due to Doppler;
	  double relative_velocity
	    = motion_relative_velocity
	    (scenario, &(scenario->nodes[connection->from_node_index]),
	     &(scenario->nodes[connection->to_node_index]));
	  double doppler_snr_value = doppler_snr (FREQUENCY_A,
						  WIFI_SUBCARRIER_SPACING *
						  1e3,
						  relative_velocity,
						  connection->SNR);
	  /*
	     printf("freq=%.2f GHz  subcarrier_spacing=%.1f Hz  \
	     rel_velocity=%f  SNR=%f  ", FREQUENCY_A/1e9, WIFI_SUBCARRIER_SPACING*1e3,
	     relative_velocity, connection->SNR);
	     printf("SNR decrease due to Doppler shift = %.3f dB\n", 
	     doppler_snr_value);
	   */

	  // update SNR by subtracting the change due to Doppler shift
	  connection->SNR -= doppler_snr_value;

	  // continue with old code...
	  fer1 = adapter_802_11a->Pr_threshold_per *
	    exp (adapter_802_11a->model1_alpha *
		 (adapter_802_11a->Pr_thresholds
		  [operating_rate] - connection->SNR - STANDARD_NOISE));

	  // this FER corresponds to the standard value PSDU_OFDM,
	  // therefore needs to be adjusted for different packet sizes
	  //fer1 = (fer1 * (connection->packet_size + header)) / PSDU_OFDM; INCORRECT
	  fer1 = 1
	    - pow (1 - fer1, (connection->packet_size + header) / PSDU_OFDM);
	}
      else
	fer1 = 0;

      // limit error rate for numerical reasons
      if (fer1 > MAXIMUM_ERROR_RATE)
	fer1 = MAXIMUM_ERROR_RATE;

      fer2 = 0;

#endif //#ifndef MODEL1_W_NOISE

    }
  else
    {
      WARNING ("Connection standard '%d' undefined", connection->standard);
      return ERROR;
    }
#ifndef MODEL1_W_NOISE
  DEBUG ("FER1=%g  BER2=%g FER2=%g\n", fer1, ber2, fer2);
#else
  DEBUG ("FER1=%g  FER2=%g\n", fer1, fer2);
#endif

  // compute overall FER
  (*fer) = fer1 + fer2 - fer1 * fer2;

  // limit error rate for numerical reasons
  if ((*fer) > MAXIMUM_ERROR_RATE)
    (*fer) = MAXIMUM_ERROR_RATE;

  return SUCCESS;
}

// compute the average number of retransmissions corresponding
// to the current conditions for ALL frames (including errored ones);
// return SUCCESS on succes, ERROR on error
int
wlan_retransmissions (struct connection_class *connection,
		      struct scenario_class *scenario,
		      double *num_retransmissions)
{
  double FER = connection->frame_error_rate;
  int i, r;

  // in this context 'r' is the maximum number of retransmissions,
  // hence the total number of transmission can go up to r+1

  // check if RTS_CTS is "enabled"
  if (connection->packet_size > connection->RTS_CTS_threshold)
    r = MAX_TRANSMISSIONS_RTS_CTS - 1;
  else
    r = MAX_TRANSMISSIONS - 1;

  // compute the number of retransmissions as a weighted mean
  (*num_retransmissions) = 0;
  for (i = 1; i <= (r - 1); i++)
    {
      (*num_retransmissions) += i * pow (FER, i);
      DEBUG ("num_retransm[%d]=%f", i, i * pow (FER, i));
    }
  (*num_retransmissions) =
    (1 - FER) * (*num_retransmissions) + r * pow (FER, r);

  return SUCCESS;
}

// compute loss rate based on FER;
// return SUCCESS on succes, ERROR on error
int
wlan_loss_rate (struct connection_class *connection,
		struct scenario_class *scenario, double *loss_rate)
{
  if (wlan_fer (connection, scenario, connection->operating_rate,
		&(connection->frame_error_rate)) == ERROR)
    {
      WARNING ("Error while computing frame error rate");
      return ERROR;
    }

  if (wlan_do_compute_loss_rate (connection, loss_rate) == ERROR)
    {
      WARNING ("Error while computing loss rate");
      return ERROR;
    }

  return SUCCESS;
}

// compute loss rate based on FER (we assume FER was
// already calculated);
// return SUCCESS on succes, ERROR on error
int
wlan_do_compute_loss_rate (struct connection_class *connection,
			   double *loss_rate)
{
  // check whether RTS/CTS is enabled
  if (connection->packet_size > connection->RTS_CTS_threshold)
    (*loss_rate) =
      pow (connection->frame_error_rate, MAX_TRANSMISSIONS_RTS_CTS);
  else
    (*loss_rate) = pow (connection->frame_error_rate, MAX_TRANSMISSIONS);

  return SUCCESS;
}

// compute operating rate based on FER and a model of the ARF mechanism;
// return SUCCESS on succes, ERROR on error
int
wlan_operating_rate (struct connection_class *connection,
		     struct scenario_class *scenario, int *operating_rate)
{
  // ARF model: in function of the current conditions trigger rate changes
  // at the next iteration

  // rate changes downwards if the probability of losing two consecutive frames
  // exceeds a predefined threshold
  if (connection->frame_error_rate * connection->frame_error_rate >
      ARF_FER_DOWN_THRESHOLD)
    {
      if ((connection->standard == WLAN_802_11B) ||
	  (connection->standard == WLAN_802_11G) ||
	  (connection->standard == WLAN_802_11A))
	{
	  (*operating_rate) = connection->operating_rate;

	  // use this loop for fast adaptation to network conditions
	  // when they vary suddenly
	  while (1)
	    {
	      double fer_lower_rate;

	      // check if we are not already operating at the minimum rate
	      if ((connection->standard == WLAN_802_11B &&
		   (*operating_rate) > B_RATE_1MBPS) ||
		  (connection->standard == WLAN_802_11G &&
		   (*operating_rate) > G_RATE_1MBPS) ||
		  (connection->standard == WLAN_802_11A &&
		   (*operating_rate) > A_RATE_6MBPS))
		{
		  (*operating_rate)--;

		  // compute fer at lower rate
		  if (wlan_fer (connection, scenario, (*operating_rate),
				&fer_lower_rate) == ERROR)
		    return ERROR;

		  // we stop the iterations when we reach an operating rate
		  // that ensures proper operation
		  if (fer_lower_rate * fer_lower_rate <
		      ARF_FER_DOWN_THRESHOLD)
		    break;
		}
	      else
		// end loop when reaching minimum operating rate
		break;
	    }
	}
      else
	{
	  WARNING ("Connection standard '%d' undefined",
		   connection->standard);
	  return ERROR;
	}
    }
  // rate changes upwards if the probability of successfully sending 10
  // consecutive frames exceeds a predefined threshold
  else if (pow (1 - connection->frame_error_rate, 10) > ARF_FER_UP_THRESHOLD)
    {
      if ((connection->standard == WLAN_802_11B) ||
	  (connection->standard == WLAN_802_11G) ||
	  (connection->standard == WLAN_802_11A))
	{
	  // check if we are not already operating at the maximum rate
	  // of the connection standard
	  if ((connection->standard == WLAN_802_11B &&
	       connection->operating_rate < B_RATE_11MBPS) ||
	      (connection->standard == WLAN_802_11G &&
	       connection->operating_rate < G_RATE_54MBPS) ||
	      (connection->standard == WLAN_802_11A &&
	       connection->operating_rate < A_RATE_54MBPS))
	    {
	      double fer_higher_rate;

	      // according to ARF, the rate is increased only if
	      // the first frame (probe) is not lost
	      if (wlan_fer
		  (connection, scenario, connection->operating_rate + 1,
		   &fer_higher_rate) == ERROR)
		return ERROR;
	      if (fer_higher_rate < ARF_FER_KEEP_THRESHOLD)
		(*operating_rate) = connection->operating_rate + 1;
	    }
	}
      else
	{
	  WARNING ("Connection standard '%d' undefined",
		   connection->standard);
	  return ERROR;
	}
    }

  return SUCCESS;
}

// calculate PPDU duration in us for 802.11 WLAN specified by 'connection';
// the results will be rounded up using ceil function;
// return SUCCESS on succes, ERROR on error
int
wlan_ppdu_duration (struct connection_class *connection,
		    double *ppdu_duration, int *slot_time)
{
  int MAC_Overhead = 224;	// MAC ovearhed in bits (header + FCS)
  int OFDM_Overhead = 22;	// OFDM ovearhed in bits (service field + tail bits)
  int Frame_Body = connection->packet_size * 8;	// frame payload in bits

  int ACK_Size = 112;		// ACK payload in bits
  int RTS_Size = 160;		// RTS payload in bits
  int CTS_Size = 112;		// CTS payload in bits

  int SIFS, DIFS;		// SIFS & DIFS duration in us
  int PHY_Overhead;		// PHY header length in us
  int Signal_Extension = 6;	// special field used by 802.11g
  int Symbol_Duration = 4;	// symbol duration in OFDM encoding

  // rate at which ACK/RTS/CTS frames are sent
  double basic_rate_bg = 1e6;
  double basic_rate_a = 6e6;

  // number of data bits per OFDM symbol; '0' was assigned for
  // non-OFDM data rates in 802.11g
  int g_OFDM_data[] = { 0, 0, 0, 24, 36, 0, 48, 72, 96, 144, 192, 216 };
  int a_OFDM_data[] = { 24, 36, 48, 72, 96, 144, 192, 216 };

  // Basic delay calculation rules:
  // 1) normal operation
  //    time_frame = DIFS + PPDU + SIFS + ACK
  //    PPDU = PLCP_preamble + PLCP_header + PSDU_MPDU
  //    PSDU_MPDU = MAC_header + LLC + SNAP + IP_datagram
  //

  // 802.11b constants
  // DIFS = 50 us
  // SIFS = 10 us
  // PLCP_preamble + PLCP_header = 192 us (long preamble)
  //                             = 96 us (short preamble -- non-standard)
  // ACK = 14 bytes
  // optimize execution time by precomputing some values


  if (connection->standard == WLAN_802_11A)
    {
      // NOTE: The results obained with the code below were checked
      // using the Excel file "80211_TransmissionTime.xls"

      int data_symbols, ACK_symbols;
      SIFS = SIFS_802_11A;
      (*slot_time) = SLOT_802_11A;

      // compute DIFS
      DIFS = SIFS + 2 * (*slot_time);

      PHY_Overhead = 20;

      data_symbols =
	(int) ceil ((double) (OFDM_Overhead + MAC_Overhead + Frame_Body) /
		    a_OFDM_data[connection->operating_rate]);
      ACK_symbols =
	(int) ceil ((double) (OFDM_Overhead + ACK_Size) /
		    a_OFDM_data[connection->operating_rate]);

      (*ppdu_duration) = DIFS + PHY_Overhead +
	data_symbols * Symbol_Duration +
	SIFS + PHY_Overhead + ACK_symbols * Symbol_Duration;

      DEBUG ("Results: D%d P%d ds%d SE%d S%d P%d As%d SE%d",
	     DIFS, PHY_Overhead, data_symbols, Signal_Extension,
	     SIFS, PHY_Overhead, ACK_symbols, Signal_Extension);

      // if RTS_CTS is enabled, RTS, CTS and 2 SIFS must be added
      if (connection->packet_size > connection->RTS_CTS_threshold)
	(*ppdu_duration) += ((RTS_Size + PHY_Overhead) * 1e6 / basic_rate_a +
			     (CTS_Size + PHY_Overhead) * 1e6 / basic_rate_a +
			     2 * SIFS);
    }
  else if (connection->standard == WLAN_802_11B)
    {
      // NOTE: The results obained with the code below were checked
      // using the Excel file "80211_TransmissionTime.xls"
      // 1 us differences at 5.5 and 11 Mb/s are caused by our
      // use of 'ceil' as specified by 802.11b standard, p.16

      // use values corresponding to 802.11b
      SIFS = SIFS_802_11B;
      (*slot_time) = SLOT_802_11B;

      // compute DIFS
      DIFS = SIFS + 2 * (*slot_time);

#ifdef USE_SHORT_PREAMBLE
      if (connection->operating_rate == B_RATE_1MBPS)
	PHY_Overhead = PHY_OVERHEAD_802_11BG_LONG;
      else
	PHY_Overhead = PHY_OVERHEAD_802_11BG_SHORT;
#else
      PHY_Overhead = PHY_OVERHEAD_802_11BG_LONG;
#endif

      (*ppdu_duration) = DIFS + PHY_Overhead +
	ceil ((MAC_Overhead + Frame_Body) *
	      1e6 / b_operating_rates[connection->operating_rate]) + SIFS
	// apparently ACK is sent at the same rate with data!!!!!
	+ PHY_Overhead
	//+ ceil((PHY_Overhead + ACK_Size)*1e6/basic_rate_bg);
	+ ceil ((ACK_Size) *
		1e6 / b_operating_rates[connection->operating_rate]);

      DEBUG
	("DIFS=%d PHY_Overhead=%d PHY_frame=%f SIFS=%d PHY_Overhead=%d ACK_frame=%f",
	 DIFS, PHY_Overhead,
	 ceil ((MAC_Overhead +
		Frame_Body) * 1e6 /
	       b_operating_rates[connection->operating_rate]), SIFS,
	 PHY_Overhead,
	 ceil ((ACK_Size) * 1e6 /
	       b_operating_rates[connection->operating_rate]));

      // if RTS_CTS is enabled, RTS, CTS and 2 SIFS must be added
      if (connection->packet_size > connection->RTS_CTS_threshold)
	(*ppdu_duration) += ((RTS_Size + PHY_Overhead) * 1e6 / basic_rate_bg +
			     (CTS_Size + PHY_Overhead) * 1e6 / basic_rate_bg +
			     2 * SIFS);
    }
  else if (connection->standard == WLAN_802_11G)
    {
      // NOTE: The results obained with the code below were checked 
      // using the Excel file "80211_TransmissionTime.xls"
      // 1 us differences at some rates are caused by our 
      // use of 'ceil' as specified by 802.11b standard, p.16

      // use values corresponding to 802.11g
      SIFS = SIFS_802_11G;

      // if g station is not in compatibility mode, use short slot,
      // otherwise use long slot
      if (connection->compatibility_mode == FALSE)
	(*slot_time) = SLOT_802_11G_SHORT;
      else
	(*slot_time) = SLOT_802_11G_LONG;

      // compute DIFS
      DIFS = SIFS + 2 * (*slot_time);

      // the code below can be used for ERP-DSSS operating rates
      // (1, 2, 5.5 and 11 Mb/s)
      if (connection->operating_rate == 0 || connection->operating_rate == 1
	  || connection->operating_rate == 2
	  || connection->operating_rate == 5)
	{
#ifdef USE_SHORT_PREAMBLE
	  if (connection->operating_rate == G_RATE_1MBPS)
	    PHY_Overhead = PHY_OVERHEAD_802_11BG_LONG;
	  else
	    PHY_Overhead = PHY_OVERHEAD_802_11BG_SHORT;
#else
	  PHY_Overhead = PHY_OVERHEAD_802_11BG_LONG;
#endif

	  (*ppdu_duration) = DIFS + PHY_Overhead +
	    ceil ((MAC_Overhead + Frame_Body) *
		  1e6 / g_operating_rates[connection->operating_rate]) + SIFS
	    // apparently ACK is sent at the same rate with data!!!!!
	    + PHY_Overhead
	    //+ ceil((PHY_Overhead + ACK_Size)*1e6/basic_rate_bg);
	    + ceil ((ACK_Size) *
		    1e6 / g_operating_rates[connection->operating_rate]);
	}
      else			// ERP-OFDM rates: 6, 9, 12, 18, 24, 36, 48, 54 Mb/s
	{
	  int data_symbols, ACK_symbols;
	  PHY_Overhead = 20;

	  data_symbols =
	    (int) ceil ((double) (OFDM_Overhead + MAC_Overhead + Frame_Body) /
			g_OFDM_data[connection->operating_rate]);
	  ACK_symbols =
	    (int) ceil ((double) (OFDM_Overhead + ACK_Size) /
			g_OFDM_data[connection->operating_rate]);

	  (*ppdu_duration) = DIFS + PHY_Overhead +
	    data_symbols * Symbol_Duration + Signal_Extension +
	    SIFS + PHY_Overhead +
	    ACK_symbols * Symbol_Duration + Signal_Extension;
	}

      // if RTS_CTS is enabled, RTS, CTS and 2 SIFS must be added
      if (connection->packet_size > connection->RTS_CTS_threshold)
	(*ppdu_duration) += ((RTS_Size + PHY_Overhead) * 1e6 / basic_rate_bg +
			     (CTS_Size + PHY_Overhead) * 1e6 / basic_rate_bg +
			     2 * SIFS);
      // if RTS_CTS is not enabled and the g connection operates 
      // in compatibility mode, we need to send CTS-to-self (CTS + SIFS)
      else if (connection->compatibility_mode == TRUE)
	(*ppdu_duration) += ((CTS_Size + PHY_Overhead) * 1e6 / basic_rate_bg +
			     SIFS);
    }
  else
    {
      WARNING ("Undefined connection type (%d)", connection->standard);
      return ERROR;
    }

  return SUCCESS;
}

// compute delay & jitter and return values in arguments
// Note: To obtain correct results the call to this function 
// must follow the call to "wlan_loss_rate()" since it 
// uses the field "frame_error_rate" internally;
// return SUCCESS on succes, ERROR on error
int
wlan_delay_jitter (struct connection_class *connection,
		   struct scenario_class *scenario, double *variable_delay,
		   double *delay, double *jitter)
{
  struct node_class *node_rx = &(scenario->nodes[connection->to_node_index]);
  struct node_class *node_tx =
    &(scenario->nodes[connection->from_node_index]);

  // mean weighted delay and jitter (in us)
  double D_avg, J_avg;

  // we assume in here that total channel utilization by others is 0.0
  wlan_do_compute_delay_jitter (connection, &D_avg, &J_avg, 0.0);

  // compute & store the final values of delay & jitter
  if (connection->delay_defined == FALSE)
    {
      (*variable_delay) = D_avg;

      // in case more nodes influence the connection, the delay 
      // must be further increased (and the bandwidth will decrease)
      // as these nodes compete with each other; we use the model of Gupta
      // the number of influences is compute for each connections
      if (connection->consider_interference == TRUE &&
	  connection->concurrent_stations > 0)
	{
	  double n = connection->concurrent_stations + 1;
	  (*variable_delay) *= sqrt (n * log2 (n));

	  DEBUG ("Delay interference for node with id=%d (\
concurrent_stations=%d)", node_rx->id, (int) (connection->concurrent_stations));
	}

      (*delay) = node_rx->internal_delay + node_tx->internal_delay +
	(*variable_delay);
    }

  if (connection->jitter_defined == FALSE)
    {
      (*jitter) = J_avg;

      if (connection->consider_interference == TRUE &&
	  connection->concurrent_stations > 0)
	{
	  double n = connection->concurrent_stations + 1;
	  (*jitter) *= sqrt (n * log2 (n));

	  DEBUG ("Jitter interference for node with id=%d (\
concurrent_stations=%d)", node_rx->id, (int) (connection->concurrent_stations));
	}
    }

  return SUCCESS;
}

// compute variable delay & jitter and return values in arguments
// Note: To obtain correct results, the call to this function 
// must follow the call to "wlan_loss_rate()" since it 
// uses the field "frame_error_rate" internally;
// return SUCCESS on succes, ERROR on error
int
wlan_do_compute_delay_jitter (struct connection_class *connection,
			      double *avg_delay, double *avg_jitter,
			      float total_channel_utilization_others)
{
  // duration of a contention slot in us
  int slot_time;

  // congestion window values (different for 802.11 b and g)
  int CW[MAX_TRANSMISSIONS];

  // values for delay and jitter corresponding to 'i' retransmissions (in us)
  // NOTE: the jitter values depend on the average delay, therefore they change
  //       with FER
  double D[MAX_TRANSMISSIONS], J[MAX_TRANSMISSIONS];

  // used to store temporarily a constant time duration
  // used when calculating D
  double constant_time;

  ////////////////////////////////
  // local constants

  // maximum congestion window for 802.11b
  int CW_b[MAX_TRANSMISSIONS] = { 31, 63, 127, 255, 511, 1023, 1023 };
  // maximum congestion window for 802.11g
  int CW_ag[MAX_TRANSMISSIONS] = { 15, 31, 63, 127, 255, 511, 1023 };

  // loop variable
  int i;

  // number of retransmissions
  int r;

  // local storage for "connection->frame_error_rate"
  double FER;

  // mean weighted delay and jitter (in us)
  double D_avg, J_avg;

  // preliminary initializations
  FER = connection->frame_error_rate;

  // for b connections, or g connections is compatibility mode
  // use b-type congestion window
  if (connection->standard == WLAN_802_11B ||
      (connection->standard == WLAN_802_11G &&
       connection->compatibility_mode == TRUE))
    {
      for (i = 0; i < MAX_TRANSMISSIONS; i++)
	CW[i] = CW_b[i];
    }
  // for a connections, or g connections not in compatibility mode
  // use a/g-type congestion window
  else if (connection->standard == WLAN_802_11A ||
	   (connection->standard == WLAN_802_11G &&
	    connection->compatibility_mode == FALSE))
    {
      for (i = 0; i < MAX_TRANSMISSIONS; i++)
	CW[i] = CW_ag[i];
    }
  else
    {
      WARNING ("Connection standard '%d' undefined", connection->standard);
      return ERROR;
    }

  // in this context 'r' is the maximum number of retransmissions,
  // hence the total number of transmission can go up to r+1

  // check if RTS_CTS is used (depending on packet size and threshold)
  if (connection->packet_size > connection->RTS_CTS_threshold)
    r = MAX_TRANSMISSIONS_RTS_CTS - 1;
  else
    r = MAX_TRANSMISSIONS - 1;

  if (wlan_ppdu_duration (connection, &constant_time, &slot_time) == ERROR)
    {
      WARNING ("Error calculating PPDU duration");
      return ERROR;
    }

  DEBUG ("FRAME DURATION:   %f us", constant_time);

  // compute the average delays for the number of retransmissions "i"
  /*  STANDARD
     D[0] = constant_time + CW[0] * slot_time / 2.0;
     for (i = 1; i <= r; i++)
     D[i] = D[i - 1] + constant_time + CW[i] * slot_time / 2.0;
   */
  /*  WRONG MODEL
     D[0] = constant_time + (CW[0] * slot_time / 2.0) 
     / (1 - total_channel_utilization_others);
     for (i = 1; i <= r; i++)
     D[i] = D[i - 1] + constant_time + (CW[i] * slot_time / 2.0)
     / (1 - total_channel_utilization_others);
   */
  //  GOOD MODEL
  D[0] = (constant_time + (CW[0] * slot_time / 2.0))
    / (1 - total_channel_utilization_others);
  for (i = 1; i <= r; i++)
    D[i] = D[i - 1] + (constant_time + (CW[i] * slot_time / 2.0))
      / (1 - total_channel_utilization_others);

  DEBUG ("__D: %f %f %f %f %f %f %f", D[0], D[1], D[2], D[3],
	 D[4], D[5], D[6]);

  // compute the weighted mean delay
  D_avg = D[0];
  for (i = 1; i <= r; i++)
    D_avg += D[i] * pow (FER, i);
  D_avg = (1 - FER) / (1 - pow (FER, r + 1)) * D_avg;


  DEBUG ("__D_avg=%f", D_avg);

  // compute the average jitter for the number of retransmissions "i"
  for (i = 0; i <= r; i++)
    {
      J[i] = fabs (D[i] - D_avg);
    }

  // adjust for the case of error in the computation
  // after 0 retransmissions (maximum error is 160 us)
  J[0] = J[0] + slot_time * (CW[0] + 1) / 4;

  DEBUG ("__J: %f %f %f %f %f %f %f", J[0], J[1], J[2], J[3],
	 J[4], J[5], J[6]);

  // compute the weighted mean jitter
  J_avg = J[0];
  for (i = 1; i <= r; i++)
    {
      J_avg += J[i] * pow (FER, i);
    }
  J_avg = (1 - FER) / (1 - pow (FER, r + 1)) * J_avg;

  DEBUG ("J_avg=%f", J_avg);

  /*
     // METHOD 2 (more accurate) is sketched below
     // compute the average jitter for the number of retransmissions "i"
     for(i=0; i<7; i++)
     {
     int k;

     if((D[i]-CW[i]*slot_time/2.0)>D_avg || 
     (D[i]+CW[i]*slot_time/2.0<D_avg)) 
     J[i]=fabs(D[i]-D_avg);
     else
     {
     k = floor((D_avg-D[i]+CW[i]*slot_time/2.0)/slot_time);
     printf("k=%d\n", k);
     J[i]=((2*k+1-CW[i])*(D_avg-D[i]) + slot_time*(k+1)*(CW[i]-k))
     /(CW[i]+1);
     }
     }
     printf("J: %f %f %f %f %f %f %f\n", 
     J[0], J[1], J[2], J[3], J[4], J[5], J[6]);
   */

  // convert values from us to ms
  (*avg_delay) = D_avg / 1e3;
  (*avg_jitter) = J_avg / 1e3;

  return SUCCESS;
}

// compute bandwidth based on operating rate, delay and packet size;
// return SUCCESS on succes, ERROR on error
int
wlan_bandwidth (struct connection_class *connection,
		struct scenario_class *scenario, double *bandwidth)
{
  return wlan_do_compute_bandwidth (connection, bandwidth);
}

// do compute bandwidth based on  delay and packet size;
// return SUCCESS on succes, ERROR on error
int
wlan_do_compute_bandwidth (struct connection_class *connection,
			   double *bandwidth)
{
  // multiply by 1e3 since variable_delay is expressed in ms
  (*bandwidth) =
    (connection->packet_size * 8 * 1e3) / connection->variable_delay;

  return SUCCESS;
}

// used in wireconf/statistics.c
int
wlan_operating_rate_index (struct connection_class *connection, float op_rate)
{
  int i;

  if (connection->standard == WLAN_802_11A)
    {
      for (i = A_RATE_6MBPS; i <= A_RATE_54MBPS; i++)
	if (a_operating_rates[i] == op_rate)
	  return i;
    }
  else if (connection->standard == WLAN_802_11B)
    {
      for (i = B_RATE_1MBPS; i <= B_RATE_11MBPS; i++)
	if (b_operating_rates[i] == op_rate)
	  return i;
    }
  else if (connection->standard == WLAN_802_11G)
    {
      for (i = G_RATE_1MBPS; i <= G_RATE_54MBPS; i++)
	if (g_operating_rates[i] == op_rate)
	  return i;
    }
  else
    DEBUG ("Unknown WLAN connections standard: %d", connection->standard);

  // if successful, this function wil return above,
  // hence here only error outcome is possible
  DEBUG ("Unable to determine operating rate index");
  return -1;
}
