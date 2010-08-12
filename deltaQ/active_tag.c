
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
 * File name: active_tag.c
 * Function: Active tag emulation for the deltaQ computation library
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#include <math.h>

#include "message.h"
#include "deltaQ.h"
#include "generic.h"
#include "active_tag.h"

// use this parameter to modify the range of the active tag
// 1.0 => no scaling; <1.0 => range is increased; >1.0 => range is decreased 
// 1.0 => at 3m FER~0.5; 0.5 => at 6m FER~0.5; 0.33 => at 9m FER~0.5
// 0.25 => at 12m FER~0.5; 0.2 => at 15m FER~0.5
//#define DISTANCE_SCALING    0.25 //0.2 //0.25 //0.33333333 //0.5 //1.0

// 1.0 => at 3m FER~0.6; 0.6 => at 5m FER~0.6; 0.3 => at 10m FER~0.6
// 0.2 => at 15m FER~0.6; 0.15 => at 20m FER~0.6
#define DISTANCE_SCALING    0.15 //0.15 //0.2 //0.3 //0.6 //1.0

// the Smart Tag's S-NODE operates at the frequency 303.2 MHz 
// [AYID32305 specifications, page 6]
double active_tag_frequencies[]={303.2e6};

/////////////////////////////////////////////
// Initialize active tag adapter structures according to 
// manufacturer specifications
/////////////////////////////////////////////

// Reference: 
parameters_active_tag s_node =
{
  "S-NODE AYID32305",   // name
  TRUE,                 // use distance model
  {2.4e3},              // operating rate
  {0},                  // Pr_thresholds (from low to high);
                        // not used for distance model
  0.0,                  // Pr_threshold_fer - not used for distance model
  1.0                   // model1_alpha - not used for distance model
};


///////////////////////////////////////
// various functions
///////////////////////////////////////

// print parameters of an active tag model
void active_tag_print_parameters(parameters_active_tag *params)
{
  printf("Active tag model parameters (name='%s')\n", params->name);
  if(params->use_distance_model==TRUE)
    printf("\tuse_distance_model=TRUE\n");
  else
    printf("\tuse_distance_model=FALSE\n");
}

// get a pointer to a parameter structure corresponding to 
// the receiving node;
// return value is this pointer, or NULL if error
parameters_active_tag *active_tag_get_node_adapter(node_class *node)
{
  parameters_active_tag *adapter;

  //  if(node->adapter_type==ORINOCO)
  //	adapter = (&orinoco);
  if(node->adapter_type==S_NODE)
    adapter = (&s_node);
  else
    {
      WARNING("Defined active tag type %d is unknown", node->adapter_type);
      return NULL;
    }

  return adapter;
}


///////////////////////////////////////
// deltaQ parameter computation
///////////////////////////////////////

// compute FER corresponding to the current conditions
// for a given operating rate
// return SUCCESS on succes, ERROR on error
int active_tag_fer(connection_class *connection, scenario_class *scenario, 
		 int operating_rate, double *fer)
{
  // we use for the moment a power 4 law to estimate dependency between 
  // FER and distance
  // below are the coefficients and x values to the corresponding powers  

  // POWER 4 GIVES STRANGE RESULTS!!!!!!!!!!!
/*   double a4 = -0.0356; */
/*   double a3 =  0.2808; */
/*   double a2 = -0.5739; */
/*   double a1 =  0.3411; */
/*   double a0 = -0.0076; */
/*   double x1 = connection->distance; */
/*   double x2 = x1 * x1; */
/*   double x3 = x1 * x2; */
/*   double x4 = x2 * x2; */

/*   (*fer) = x4*a4 + x3*a3 + x2*a2 + x1*a1 + a0; */

  // POWER 3 GIVES STRANGE RESULTS!!!!!!!!!!!
/*   double a3 = -0.0042; */
/*   double a2 =  0.1347; */
/*   double a1 = -0.2136; */
/*   double a0 =  0.0459; */
/*   double x1 = connection->distance; */
/*   double x2 = x1 * x1; */
/*   double x3 = x1 * x2; */

/*   (*fer) = x3*a3 + x2*a2 + x1*a1 + a0; */

  double a2 =  0.1096;
  double a1 = -0.1758;
  double a0 =  0.0371;
  double x1 = connection->distance * DISTANCE_SCALING;
  double x2 = x1 * x1;


  environment_class *environment=
    &(scenario->environments[connection->through_environment_index]);

  // check first whether the two nodes are in line of sight;
  // we consider that frame error is 1 otherwise
  if(environment->num_segments>0)
    {
      int seg_i;

      for(seg_i=0; seg_i<environment->num_segments; seg_i++)
	if(environment->W[seg_i]>0)
	  {
	    (*fer)=1.0;
	    DEBUG("Adjusted connection FER = %f (building attenuation)", 
		  (*fer));
	    //fprintf(stderr, "building conflict\n");

	    return SUCCESS;
	  }
    }


  // prevent small error that appears at distance close to 0
  // because of the shape of the power 2 law
  if(connection->distance * DISTANCE_SCALING < 0.5)
    (*fer)=0.0;
  else
    (*fer) = x2*a2 + x1*a1 + a0;
  DEBUG("--- FER (raw value, 4 bytes) = %f\n", (*fer));

  if((*fer) < 0)
    (*fer) = 0;
  else if((*fer) > 1)
    (*fer) = 1;

  // the above FER is computed for 4 byte packets, we
  // need to adjust for current size packets;
  // in addition to the packet load the headers have 6 byte 
  // length, and they must be added in computing FER since
  // any error in the headers will aslo be detected;
  // the effect of 2-bit errors not being detected because of the simple
  // parity checksum is compensated by the shape of the model curve
  // which increases faster with distance than the measured data;
  // measured data showed a slower increase since 2-bit errors were
  // reported as no error
  (*fer) = 1 - pow((1-(*fer)), (6.0+connection->packet_size)/(6.0+4.0));
  DEBUG("Adjusted connection FER = %f", (*fer));

  return SUCCESS;
}

// compute loss rate based on FER
// return SUCCESS on succes, ERROR on error
int active_tag_loss_rate(connection_class *connection, 
		       scenario_class *scenario, double *loss_rate)
{
  if(active_tag_fer(connection, scenario, connection->operating_rate, 
		   &(connection->frame_error_rate))==ERROR)
    {
      WARNING("Error while computing frame error rate");
      return ERROR;
    }

  // loss rate equals FER since there is no retransmission mechanism
  (*loss_rate) = connection->frame_error_rate + connection->interference_fer -
    connection->frame_error_rate * connection->interference_fer;

  if((*loss_rate)>1)
    (*loss_rate)=1;

  return SUCCESS;
}

// compute operating rate based on FER and a model of a rate
// auto-selection mechanism
// return SUCCESS on succes, ERROR on error
int active_tag_operating_rate(connection_class *connection, 
			    scenario_class *scenario, int *operating_rate)
{
  // for Ethernet, operating rate may change if devices with different 
  // capabilities are connected (e.g., 10/100 Mbps with 10 Mbps only);
  // for the moment we don't take this into account
  (*operating_rate) = connection->operating_rate;
  return SUCCESS;
}

// compute delay & jitter and return values in arguments
// return SUCCESS on succes, ERROR on error
int active_tag_delay_jitter(connection_class *connection, 
			  scenario_class *scenario, double *variable_delay,
			  double *delay, double *jitter)
{
  node_class *node_rx=&(scenario->nodes[connection->to_node_index]);
  node_class *node_tx=&(scenario->nodes[connection->from_node_index]);

  // transmission delay is fixed since data size is fixed;
  // according to Nakata-kun's drawing pre-tx time is 2+1=3 ms
  (*variable_delay) = 0;

  // for the moment we consider there is no variation of delay
  (*jitter) = 0;

  // delay is equal to the fixed delay configured for the nodes
  (*delay) =  node_rx->internal_delay + node_tx->internal_delay + 
    (*variable_delay);

  return SUCCESS;
}

// compute bandwidth based on operating rate, delay and packet size
// return SUCCESS on succes, ERROR on error
int active_tag_bandwidth(connection_class *connection, 
		       scenario_class *scenario, double *bandwidth)
{
  // for the moment we assume bandwidth is the same with operating rate
  // since the data provided will be identical to that travelling 
  // in the air

  /* // according to Nakata-kun's drawing tx time is 33 ms */
/*   double sending_time = 33; */

/*   // multiply by 1e3 since variable_delay is expressed in ms */
/*   (*bandwidth) =  */
/*     (connection->packet_size * 8 * 1e3) /  */
/*     (connection->variable_delay + sending_time); */

/*   return SUCCESS; */

  parameters_active_tag *adapter;

  adapter = active_tag_get_node_adapter
    (&(scenario->nodes[connection->to_node_index]));

  if(adapter==NULL)
    {
      WARNING("Adapter data cannot be found");
      return ERROR;
    }

  (*bandwidth) = adapter->operating_rates[connection->operating_rate];

  return SUCCESS;
}

// update the changing properties of a connection (e.g., relative distance, Pr)
// after the state of the system changed;
// return SUCCESS on succes, ERROR on error
int active_tag_connection_update(connection_class *connection, 
				 scenario_class *scenario)
{
  // helper "shortcuts"
  node_class *node_rx=&(scenario->nodes[connection->to_node_index]);
  node_class *node_tx=&(scenario->nodes[connection->from_node_index]);
  environment_class *environment=
    &(scenario->environments[connection->through_environment_index]);

  // update distance in function of the new node positions
  connection->distance =
    coordinate_distance(&(node_rx->position), &(node_tx->position));

  // limit distance to prevent numerical errors
  if(connection->distance < MINIMUM_DISTANCE)
    {
      connection->distance = MINIMUM_DISTANCE;
      WARNING("Limiting distance between nodes to %.2f m", MINIMUM_DISTANCE);
    }

  // update Pr in function of the Pr0 of node_tx and distance
  if(environment->num_segments>0)
    {
      // check if using pre-defined or one-segment environment
      if(environment->num_segments==1)
	{
	  double distance;

	  if(environment->length[0]==-1)
	    distance = connection->distance;
	  else
	    distance = environment->length[0];

	  connection->Pr = 
	    node_tx->Pr0_active_tag - 
	    10*environment->alpha[0]*log10(distance) -
	    environment->W[0] + randn(0, environment->sigma[0]) +
	    node_rx->antenna_gain;
	}
      else // dynamic environment
	{
	  int i;
	  double current_distance=0;
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
	  connection->Pr = node_tx->Pr0_active_tag - 
	    10*environment->alpha[0]*log10(environment->length[0]) +
	    node_rx->antenna_gain;

	  W_sum = environment->W[0];

	  sigma_square_sum = environment->sigma[0] * environment->sigma[0];

	  DEBUG("Pr=%f W_s=%.12f s_s_s=%f\n", connection->Pr, 
		 W_sum, sigma_square_sum);

	  // for multiple segments, attenuation parameters must be cumulated
	  for(i=1; i<environment->num_segments; i++)
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
	      current_distance += environment->length[i-1];
	      connection->Pr -= (10*environment->alpha[i]*
				 (log10(current_distance + 
					environment->length[i]) -
				  log10(current_distance)));
	    }

	  connection->Pr -= W_sum;

	  // take into account shadowing
	  connection->Pr += randn(0, sqrt(sigma_square_sum));

	  DEBUG("Pr=%f W_s=%f s_s_s=%f\n", connection->Pr, 
		W_sum, sigma_square_sum);
	}
    }
  else 
    {
      WARNING("Environment %s contains unconsistencies", environment->name);
      return ERROR;
    }
  return SUCCESS;
}

// update node Pr0, which is the power radiated by _this_ node 
// at the distance of 1 m
void active_tag_node_update_Pr0(node_class *node)
{
  //parameters_active_tag *adapter;

  //adapter = active_tag_get_node_adapter(node);

  // we use classic signal attenuation equation, and antenna gain

  // !!!!!!!!!!!!!!!  BELOW !!!!!!!!!!!!!!!!!!
  // NEED TO USE REAL FREQUENCY BUT ONLY KNOWN WHEN ADAPTER IS DECIDED
  // => use static for now but change in the future

  node->Pr0_active_tag = node->Pt - 
    20 * log10((4 * M_PI) / (SPEED_LIGHT / active_tag_frequencies[0])) +
    node->antenna_gain;
}

// do compute channel interference between the current connection
// and a potentially interfering connection 'connection_i';
// return SUCCESS on succes, ERROR on error
int active_tag_compute_interference(connection_class *connection, 
				   connection_class *connection_i, 
				   scenario_class *scenario)
{
  connection_class virtual_connection;

  // do not consider again this node if it was processed before
  if(scenario->nodes
     [connection_i->from_node_index].interference_accounted == TRUE)
    return TRUE;

  // create a virtual connection that is used only locally
  // the connection is made of connection->to_node (receiver)
  // connection_i->from_node (transmitter) and
  // connection_i->through_environment (later use better
  // environment calculation)
  // all other fields are also inherited from connection_i
  INFO("Building a virtual connection from '%s' to '%s'",
       connection_i->from_node, connection->to_node);

  // mark the interference_accounted flag
  scenario->nodes[connection_i->from_node_index].interference_accounted = TRUE;

  connection_copy(&virtual_connection, connection_i);
  virtual_connection.to_node_index = connection->to_node_index;

#ifdef MESSAGE_DEBUG
  DEBUG("Connections and environments info:"); 
  connection_print(connection);
  connection_print(connection_i);
  connection_print(&virtual_connection);

  environment_print(&(scenario->environments
		      [connection->through_environment_index]));
  environment_print(&(scenario->environments
		      [connection_i->through_environment_index]));
  environment_print(&(scenario->environments
		      [virtual_connection.through_environment_index]));
#endif

  // compute FER for the virtual connection
  active_tag_connection_update(&virtual_connection, scenario);
  active_tag_loss_rate(&virtual_connection, scenario, 
		      &(virtual_connection.loss_rate));
  INFO("Loss rate for virtual connection = %.3f", 
       virtual_connection.loss_rate);

  // interference is considered in the form of additional 
  // error rate induced by other transmitters
  
  connection->interference_fer += (1.0/(9 * 9) * 
				 (1-virtual_connection.frame_error_rate));
  INFO("Resulting interference_fer = %.3f", connection->interference_fer);

  return SUCCESS;
}

// compute the interference caused by other stations
// through overlapping transmissions;
// return SUCCESS on succes, ERROR on error
int active_tag_interference(connection_class *connection, 
			   scenario_class *scenario)
{
  int connection_i;
  //float distance;

  //double rx_power;

  //reset interference indicators for active tag
  connection->interference_fer = 0.0;
  
  // reset interference flags for nodes
  scenario_reset_node_interference_flag(scenario);

  // search connections in scenario that operate on same band
  // and are closely located to the current connection
  for(connection_i=0; connection_i < scenario->connection_number; 
      connection_i++)
    {
      // look only at connections that are not the same with
      // the current one
      if(connection != &(scenario->connections[connection_i]))
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
	  if((connection->standard == ACTIVE_TAG) && 
	     (scenario->connections[connection_i].standard == ACTIVE_TAG))
	    {
	      DEBUG("--------------------------------------------");
	      DEBUG("Interference between active tags '%s' and '%s' detected \
=> determine effects", connection->from_node, 
		    scenario->connections[connection_i].from_node);
	      active_tag_compute_interference
		(connection, &(scenario->connections[connection_i]), 
		 scenario);
	    }
	}
    }

  return SUCCESS;
}
