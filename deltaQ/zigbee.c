
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
 * File name: zigbee.c
 * Function: Model ZigBee communication and compute deltaQ parameters for
 *           connections
 *
 * Author: Razvan Beuran
 *
 * $Id: zigbee.c 152 2013-08-29 23:58:04Z razvan $
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
#include "zigbee.h"


// since MAC emulation is not needed in connection with ORE and JLE,
// we disable this features by default
int enable_MAC_emulation = FALSE;
//int enable_MAC_emulation = TRUE;


//////////////////////////////////////////////////////
// Initialize ZigBee adapter structures according to 
// manufacturer specifications
//////////////////////////////////////////////////////

// Reference: Jennic
struct parameters_zigbee jennic = {
  "JENNIC",			// name
  TRUE,				// use_model1
  {-96},			// Pr_thresholds (from low to high)
  0.01,				// Pr_threshold_fer
  1.0				// model1_alpha
};


/////////////////////////////////////////////
// Arrays of constants for convenient use
/////////////////////////////////////////////

double zigbee_operating_rates[] = { 250e3 };


// print parameters of an 802.11b model
void
print_zigbee_parameters (struct parameters_zigbee *params)
{
  printf ("ZigBee model parameters (name='%s')\n", params->name);
  printf ("\tuse_model1=%d\n", params->use_model1);
}


// update Pr0, which is the power radiated by _this_ interface
// at the distance of 1 m
void
zigbee_interface_update_Pr0 (struct interface_class *interface)
{
  // we use classic signal attenuation equation, and antenna gain
  interface->Pr0 = interface->Pt -
    20 * log10 ((4 * M_PI) / (SPEED_LIGHT / ZIGBEE_FREQUENCY)) +
    interface->antenna_gain;
}

// update the changing properties of a connection (e.g., relative distance, Pr)
// after the state of the system changed;
// return SUCCESS on succes, ERROR on error
int
zigbee_connection_update (struct connection_class *connection,
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

	  if (environment->length[0] == -1)
	    distance = connection->distance;
	  else
	    distance = environment->length[0];

	  connection->Pr =
	    node_tx->interfaces[connection->from_interface_index].Pr0 -
	    10 * environment->alpha[0] * log10 (distance) -
	    environment->W[0] + randn (0,
				       environment->sigma[0]) +
	    node_rx->interfaces[connection->to_interface_index].antenna_gain;
	}
      else			// dynamic environment
	{
	  int i;
	  double current_distance = 0;
	  double W_sum;
	  double sigma_square_sum;

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
	  connection->Pr
	    = node_tx->interfaces[connection->from_interface_index].Pr0 -
	    10 * environment->alpha[0] * log10 (environment->length[0]) +
	    node_rx->interfaces[connection->to_interface_index].antenna_gain;

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
zigbee_get_interface_adapter (struct connection_class *connection,
			      struct interface_class *interface)
{
  void *adapter;

  if (interface->adapter_type == JENNIC)
    adapter = (&jennic);
  else
    {
      WARNING ("Defined ZigBee adapter type %d is incompatible with \
connection standard %d", interface->adapter_type, ZIGBEE);
      return NULL;
    }

  return adapter;
}

// do compute channel interference between the current connection
// and a potentially interfering connection 'connection_i';
// return SUCCESS on succes, ERROR on error
int
zigbee_compute_channel_interference (struct connection_class *connection,
				     struct connection_class *connection_i,
				     struct scenario_class *scenario)
{
  struct connection_class virtual_connection;
  double attenuation = 0;
  int channel_distance;

  void *adapter;

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
  zigbee_connection_update (&virtual_connection, scenario);
  INFO ("Pr for virtual connection = %.3f dBm", virtual_connection.Pr);

  ////////////////////////////////////////////////////////////
  // the above power doesn't take into account specific 
  // inter-channel attenuation, so we need to compute this too

  // compute channel distance first, no matter what connection standards are
  channel_distance = (int) fabs (connection->channel - connection_i->channel);

  // check whether the interfering connection is ZigBee
  // ignore others for the moment
  if (connection_i->standard == ZIGBEE)
    {
      INFO ("Interfering connection is ZigBee => DSSS/CCK attenuation model");
      if (channel_distance == 0)
	attenuation = 0;
      else if (channel_distance <= 4)
	attenuation = 10 * log10 ((22.0 - channel_distance * 5) / 22.0);
      else if (channel_distance <= 8)
	attenuation = 10 * log10 ((44.0 - channel_distance * 5) / 44.0) - 30;
      else
	// minimum required attenuation according to the standard
	attenuation = -50.0;
    }

  // add the (negative) attenuation to the received power
  virtual_connection.Pr += attenuation;

  INFO ("Power after attenuation: Pr=%f (inter channel distance=%d)",
	virtual_connection.Pr, channel_distance);

  adapter = zigbee_get_interface_adapter
    (connection,
     (&
      ((scenario->nodes[connection->from_node_index]).
       interfaces[connection->from_interface_index])));

  //printf("ADAPTER=%p\n", adapter);

  // check whether a suitable adapter was found
  if (adapter == NULL)
    {
      WARNING ("No suitable adapter found for connection");
      return ERROR;
    }

  // if the noise connection power is inferior to the sensitivity
  // threshold of the lowest operating rate of the affected connection, 
  // then this power will indeed have effects of noise and induce frame errors
  if (virtual_connection.Pr <
      ((struct parameters_zigbee *) adapter)->Pr_thresholds[0])
    {
      INFO ("Interference from transmission of other stations (noise type)");
      // compute strength of the received power from the 
      // transmitter of the other connection to the 
      // receiver of the current connection
      connection->interference_noise =
	add_powers (connection->interference_noise, virtual_connection.Pr,
		    ZIGBEE_MINIMUM_NOISE_POWER);

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
    }

  return SUCCESS;
}

// compute the interference caused by other stations
// through either concurrent transmission or through noise;
// return SUCCESS on succes, ERROR on error
int
zigbee_interference (struct connection_class *connection,
		     struct scenario_class *scenario)
{
  int connection_i;
  //float distance;

  //double rx_power;

  //reset interference indicators
  connection->concurrent_stations = 0;
  connection->interference_noise = ZIGBEE_MINIMUM_NOISE_POWER;

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
	  if (connection->standard == ZIGBEE &&
	      scenario->connections[connection_i].standard == ZIGBEE)
	    {
	      INFO ("------------------------------------------------");
	      INFO ("Interference ZigBee <-> ZigBee detected => \
determine effects");
	      zigbee_compute_channel_interference
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
zigbee_fer (struct connection_class *connection,
	    struct scenario_class *scenario, int operating_rate, double *fer)
{
  double fer1;

  struct environment_class *environment =
    &(scenario->environments[connection->through_environment_index]);

  struct parameters_zigbee *adapter;
  adapter = zigbee_get_interface_adapter
    (connection,
     (&
      ((scenario->nodes[connection->from_node_index]).
       interfaces[connection->from_interface_index])));

  // MAC overhead duration in us [pp. 138]
  // considered normal address fields have maximum size and no security
  int MAC_Overhead = (2 + 1 + 0 + 8 + 0 + 8 + 0 + 2);
  int header = MAC_Overhead;

  //printf("ADAPTER=%p\n", adapter);

  // check whether a suitable adapter was found
  if (adapter == NULL)
    {
      WARNING ("No suitable adapter found for connection");
      return ERROR;
    }

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
				   ZIGBEE_MINIMUM_NOISE_POWER);
      // limit combined_noise to a minimum standard value;
      // this is equivalent with making sure performance cannot 
      // exceed that obtained in optimum conditions
      if (combined_noise < ZIGBEE_STANDARD_NOISE)
	combined_noise = ZIGBEE_STANDARD_NOISE;

      // compute SNR
      connection->SNR = connection->Pr - combined_noise;
    }
  else
    {
      WARNING ("Environment %s contains inconsistencies", environment->name);
      return ERROR;
    }

  // check whether model parameters were initialized
  if (adapter->use_model1 == TRUE)
    {
      fer1 = adapter->Pr_threshold_fer *
	exp (adapter->model1_alpha *
	     (adapter->Pr_thresholds[operating_rate] -
	      connection->SNR - ZIGBEE_STANDARD_NOISE));

      // limit error rate for numerical reasons
      if (fer1 > MAXIMUM_ERROR_RATE)
	fer1 = MAXIMUM_ERROR_RATE;

      DEBUG
	("fer1=%f Pr_threshold_fer=%f model1_alpha=%f Pr_thresholds[operating_rate]=%f SNR=%f ZIGBEE_STANDARD_NOISE=%f",
	 fer1, adapter->Pr_threshold_fer, adapter->model1_alpha,
	 adapter->Pr_thresholds[operating_rate], connection->SNR,
	 ZIGBEE_STANDARD_NOISE);

      // this FER corresponds to the standard value ZIGBEE_PSDU,
      // therefore needs to be adjusted for different packet sizes

      //fer1 = (fer1 * (connection->packet_size + header)) / ZIGBEE_PSDU; INCORRECT
      fer1 = 1
	- pow (1 - fer1, (connection->packet_size + header) / ZIGBEE_PSDU);

      DEBUG ("after packet adaptation fer1=%f", fer1);
    }
  else
    fer1 = 0;

  // limit error rate for numerical reasons
  if (fer1 > MAXIMUM_ERROR_RATE)
    fer1 = MAXIMUM_ERROR_RATE;

  DEBUG ("FER1=%g", fer1);

  // compute overall FER
  (*fer) = fer1;

  // limit error rate for numerical reasons
  if ((*fer) > MAXIMUM_ERROR_RATE)
    (*fer) = MAXIMUM_ERROR_RATE;

  return SUCCESS;
}

// compute the average number of retransmissions corresponding 
// to the current conditions for ALL frames (including errored ones);
// return SUCCESS on succes, ERROR on error
int
zigbee_retransmissions (struct connection_class *connection,
			struct scenario_class *scenario,
			double *num_retransmissions)
{
  double FER = connection->frame_error_rate;
  int i, r;

  // in this context 'r' is the maximum number of retransmissions,
  // hence the total number of transmission is at most r+1

  r = ZIGBEE_MAX_TRANSMISSIONS - 1;

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
zigbee_loss_rate (struct connection_class *connection,
		  struct scenario_class *scenario, double *loss_rate)
{
  if (zigbee_fer (connection, scenario, connection->operating_rate,
		  &(connection->frame_error_rate)) == ERROR)
    {
      WARNING ("Error while computing frame error rate");
      return ERROR;
    }

  // check if MAC emulation is disabled; if true, then the loss rate
  // is equivalent to FER, unless there is interference which needs to
  // be added to FER in a probabilistic way
  if (enable_MAC_emulation == FALSE)
    {
      // loss rate equals FER since there is no retransmission mechanism
      (*loss_rate) = connection->frame_error_rate
	+ connection->interference_fer
	- connection->frame_error_rate * connection->interference_fer;

      if ((*loss_rate) > 1)
	(*loss_rate) = 1;
    }
  // when emulating the MAC, loss rate must be computed based on FER
  // and the maximum number of retransmissions
  else
    (*loss_rate) = pow (connection->frame_error_rate,
			ZIGBEE_MAX_TRANSMISSIONS);

  return SUCCESS;
}

// calculate PPDU duration in us for ZigBee specified by 'connection';
// the results will be rounded up using ceil function;
// return SUCCESS on succes, ERROR on error
int
zigbee_ppdu_duration (struct connection_class *connection,
		      double *ppdu_duration)
{
  // symbol duration in us [pp. 28] (from symbol rate)
  int symbol_duration = 16;

  int Preamble_duration = 128;	// preamble duration in us [pp. 44]
  int SFD_duration = 32;	// SFD duration in us [pp. 44] (2 symbols)
  int PHR_duration = 32;	// PHR duration in us [pp. 43] (1 octet => 2 symbols)
  // PHY header length in us [pp. 43]
  int PHY_Overhead = Preamble_duration + SFD_duration + PHR_duration;

  // MAC overhead duration in us [pp. 138]
  // considered normal address fields have maximum size and no security
  int MAC_Overhead = (2 + 1 + 0 + 8 + 0 + 8 + 0 + 2) * 2 * symbol_duration;

  // frame payload in us
  int payload_duration = connection->packet_size * 2 * symbol_duration;

  int ACK_duration = 6 * 2 * symbol_duration;	// ACK duration in us [pp. 160]

  int aTurnaroundTime = 12 * symbol_duration;	//[pp. 45]
  int aUnitBackoffPeriod = 20 * symbol_duration;	// [pp. 159]

  // average value [pp.170]
  int delay_to_ACK = aTurnaroundTime + (aUnitBackoffPeriod / 2);

  // SIFS & LIFS duration in us [pp. 30]
  int SIFS = 12 * symbol_duration;
  int LIFS = 40 * symbol_duration;
  int IFS_duration;

  // [pp. 159]
  int aMaxSIFSFrameSize = 18;

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

  // check whether frame (i.e. MPDU) is less than aMaxSIFSFrameSize
  if ((connection->packet_size + (MAC_Overhead / 2 / symbol_duration))
      <= aMaxSIFSFrameSize)
    IFS_duration = SIFS;
  else
    IFS_duration = LIFS;

  // [pp. 170]
  (*ppdu_duration) = PHY_Overhead + MAC_Overhead + payload_duration +
    delay_to_ACK + Preamble_duration + SFD_duration + ACK_duration +
    IFS_duration;

  return SUCCESS;
}

// compute delay & jitter and return values in arguments
// Note: To obtain correct results the call to this function 
// must follow the call to "zigbee_loss_rate()" since it 
// uses the field "frame_error_rate" internally;
// return SUCCESS on succes, ERROR on error
int
zigbee_delay_jitter (struct connection_class *connection,
		     struct scenario_class *scenario, double *variable_delay,
		     double *delay, double *jitter)
{
  // symbol duration in us [pp. 28] (from symbol rate)
  int symbol_duration = 16;
  int aUnitBackoffPeriod = 20 * symbol_duration;	// [pp. 159]

  int slot_time = aUnitBackoffPeriod;

  struct node_class *node_rx = &(scenario->nodes[connection->to_node_index]);
  struct node_class *node_tx =
    &(scenario->nodes[connection->from_node_index]);

  // values for delay and jitter corresponding to 'i' retransmissions (in us)
  // NOTE: the jitter values depend on the average delay, therefore they change
  //       with FER
  double D[ZIGBEE_MAX_TRANSMISSIONS], J[ZIGBEE_MAX_TRANSMISSIONS];

  // used to store temporarily a constant time duration
  // used when calculating D
  double constant_time;

  ////////////////////////////////
  // local constants

  // maximum congestion window (assume no interference)
  // if environment is busy CSMA/CA can be repeated ZIGBEE_MAX_CSMA_BACKOFFS
  // times and CW increaseas up to 5
  //e.g. congestion=0 => CWmax=3; congestion closer to 1 => CWmax = (3+4+5+5+5)
  // very high congestion => failure and retransmit => MODEL!!!!!!!!!!!!!

  int CW[ZIGBEE_MAX_TRANSMISSIONS] = { 3, 3, 3, 3 };

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

  r = ZIGBEE_MAX_TRANSMISSIONS - 1;

  if (zigbee_ppdu_duration (connection, &constant_time) == ERROR)
    {
      WARNING ("Error calculating PPDU duration");
      return ERROR;
    }

  DEBUG ("FRAME DURATION:   %f us", constant_time);

  // compute the average delays for the number of retransmissions "i"
  D[0] = constant_time + CW[0] * slot_time / 2.0;
  for (i = 1; i <= r; i++)
    D[i] = D[i - 1] + constant_time + CW[i] * slot_time / 2.0;

  DEBUG ("__D[0:%d]: %f %f %f %f\n", r, D[0], D[1], D[2], D[3]);

  // compute the weighted mean delay
  D_avg = D[0];

  // only compute the weighted average if MAC emulation is enabled
  if (enable_MAC_emulation == TRUE)
    {
      for (i = 1; i <= r; i++)
	D_avg += D[i] * pow (FER, i);
      D_avg = (1 - FER) / (1 - pow (FER, r + 1)) * D_avg;
    }

  DEBUG ("__D_avg=%f\n", D_avg);

  // compute the average jitter for the number of retransmissions "i"
  for (i = 0; i <= r; i++)
    {
      J[i] = fabs (D[i] - D_avg);
    }

  // adjust for the case of error in the computation
  // after 0 retransmissions (maximum error is 160 us) ????????????
  J[0] = J[0] + slot_time * (CW[0] + 1) / 4;

  DEBUG ("__J[0:%d]: %f %f %f %f\n", r, J[0], J[1], J[2], J[3]);

  // compute the weighted mean jitter
  J_avg = J[0];

  // only compute the weighted average if MAC emulation is enabled
  if (enable_MAC_emulation == TRUE)
    {
      for (i = 1; i <= r; i++)
	{
	  J_avg += J[i] * pow (FER, i);
	}
      J_avg = (1 - FER) / (1 - pow (FER, r + 1)) * J_avg;
    }

  DEBUG ("J_avg=%f\n", J_avg);

  // compute & store the final values of delay & jitter
  if (connection->delay_defined == FALSE)
    {
      (*variable_delay) = D_avg / 1e3;
      (*delay) = node_rx->internal_delay + node_tx->internal_delay +
	(*variable_delay);

      // in case more nodes influence the connection, the delay 
      // must be further increased (and the bandwidth will decrease)
      // as these nodes compete with each other; we use the model of Gupta
      // the number of influences is compute for each connections
      DEBUG ("consider_interference=%d concurrent_stations=%d",
	     connection->consider_interference,
	     connection->concurrent_stations);
      if (connection->consider_interference == TRUE
	  && connection->concurrent_stations > 0)
	{
	  double n = connection->concurrent_stations + 1;
	  (*delay) *= sqrt (n * log2 (n));

	  WARNING ("Delay interference for node with id=%d \
(concurrent_stations=%d)", node_rx->id, (int) (connection->concurrent_stations));
	}
    }

  if (connection->jitter_defined == FALSE)
    {
      (*jitter) = J_avg / 1e3;

      if (connection->consider_interference == TRUE &&
	  connection->concurrent_stations > 0)
	{
	  double n = connection->concurrent_stations + 1;
	  (*jitter) *= sqrt (n * log2 (n));

	  WARNING ("Jitter interference for node with id=%d \
(concurrent_stations=%d)", node_rx->id, (int) (connection->concurrent_stations));
	}
    }

  return SUCCESS;
}

// compute bandwidth based on operating rate, delay and packet size;
// return SUCCESS on succes, ERROR on error
int
zigbee_bandwidth (struct connection_class *connection,
		  struct scenario_class *scenario, double *bandwidth)
{
  (*bandwidth) =
    (connection->packet_size * 8 * 1e3) / connection->variable_delay;

  // use above model to be able to take into account interference
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //(*bandwidth) = zigbee_operating_rates[connection->operating_rate];

  return SUCCESS;
}
