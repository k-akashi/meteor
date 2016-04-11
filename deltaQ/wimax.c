
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
 * File name: wimax.c
 * Function: WiMAX emulation for the deltaQ computation library
 *
 * Author: Razvan Beuran
 *
 * $Id: wimax.c 166 2014-02-14 02:03:51Z razvan $
 *
 ***********************************************************************/

#include <math.h>
#include <string.h>

#include "message.h"
#include "deltaQ.h"
#include "wimax.h"


////////////////////////////////////////////////////////////////////////////
// TODO LIST:
// 1) MCS setting may need to be done independently for DL and UL
//    because of restrictions, such as 3/4 coding not being allowed for UL
// 2) Check results against values in Table 3 (page 18) and table 6 (page 26)
//    of "WiMAX Forum - Overview and Performance"
// 3) Include Rx antenna diversity gain for mobile stations with 2 antennas;
//    the gain is given as "3 dB" by "WiMAX Forum - Overview and Performance"
//    (Tables 10, 11 on pages 33, 34)
//    Total diversity gain is 4 dB (R&S "MIMO Performance", page 24)
// 4) Include correct Tx power values for BS and SS (10 W and 200 mW, 
//    respectively based on "WiMAX Forum - Overview and Performance"
//    (Tables 10, 11 on pages 33, 34)
// 5) Matrix A = transmission diversity => increase robustness (range)
//    Matrix B = spatial multiplexing => increase throughput
//    FIXME: Lan's model is for matrix B?!
// 6) Shadowing = large-scale fading <=> difference in Pr in different
//    areas depending on whether the receiver is in the "shadow" or not
//    Fading = small-scale fading <=> fast variations in Pr due to 
//    constructive and destructive effects of multi-path
////////////////////////////////////////////////////////////////////////////


// Ns-3 WiMAX adapter; it needs to be initialized by 
// calling the function "wimax_init_ns3_adapter"
struct parameters_802_16 ns3_wimax;

// variable initialization
// FIXME: What are valid rates for WiMAX?
// We put below the values that correspond to 10 MHz, but they should
// be computed dynamically [Ref: R&S Mobile WiMAX Throughput Measurements pp. 8]
double wimax_operating_rates[] = { 1.75e6, 3.5e6, 7e6, 10.5e6,	// QPSK
  14e6, 18.7e6, 21e6,		//16QAM
  21e6, 28e6, 31.5e6, 35e6	//64QAM
};

#ifdef USE_PAPER_VALUES
static int use_paper_values = 1;
#else
static int use_paper_values = 0;
#endif

// update Pr0, which is the power radiated by _this_ interface

// at the distance of 1 m
void
wimax_interface_update_Pr0 (struct interface_class *interface)
{
  // we use classic signal attenuation equation, and antenna gain
  interface->Pr0 = interface->Pt
    - 20 * log10 ((4 * M_PI) / (SPEED_LIGHT / FREQUENCY_WIMAX))
    + interface->antenna_gain;
}

// get a pointer to a parameter structure corresponding to
// the receiving interface adapter and the connection standard;
// return value is this pointer, or NULL if error
void *
wimax_get_interface_adapter (struct connection_class *connection,
			     struct interface_class *interface)
{
  void *adapter = NULL;

  if (interface->adapter_type == NS3_WIMAX)
    adapter = (&ns3_wimax);
  else
    {
      WARNING ("Defined WiMAX adapter type '%d' is not known",
	       interface->adapter_type);
      return NULL;
    }

  return adapter;
}

// init the Ns-3 WiMAX adapter structure;
// return SUCCESS on succes, ERROR on error
int
wimax_init_ns3_adapter (struct parameters_802_16 *adapter)
{
  // capacity variable
  struct capacity_class capacity;

  int mcs = QPSK_18;

  // init capacity for default BW and all MCSes starting at QPSK_18;
  // needed in order to initialize thresholds 
  if (capacity_update_all (&capacity, WIMAX_DEFAULT_SYS_BW, mcs,
			   MIMO_TYPE_SISO, 1, 1) == ERROR)
    {
      WARNING ("Error updating system bandwidth and MCS for the \
capacity calculation structure.\n");
      return ERROR;
    }

  //printf("WiMAX values: bandwidth = %.2f MCS = %2d: PHY data rate [Mbps]: DL = %5.2f UL = %5.2f\n", WIMAX_DEFAULT_SYS_BW, mcs, capacity.dl_data_rate, capacity.ul_data_rate);

  strncpy (adapter->name, "Ns-3 WiMAX", MAX_STRING);

  int i;
  for (i = 0; i < WIMAX_RATES_NUMBER; i++)
    {
      adapter->Pr_thresholds[i] = wimax_min_threshold (&capacity);
      if (mcs < QAM64_56)
	{
	  mcs++;
	  if (capacity_update_mcs (&capacity, mcs) == ERROR)
	    {
	      printf ("Error updating MCS for the capacity calculation \
structure.\n");
	      return ERROR;
	    }
	  //	  printf("WiMAX values: bandwidth = %.2f MCS = %2d: PHY data rate [Mbps]: DL = %5.2f UL = %5.2f\n", WIMAX_DEFAULT_SYS_BW, mcs, capacity.dl_data_rate, capacity.ul_data_rate);
	}
    }

  // 802.16e-2206 standard mentions that BER after FEC should be less than
  // 1e-6 at the receiver sensitivity power levels, but frame size is not
  // specified so we assume it is PSDU_OFDMA in order to transform BER
  // to FER without losing any generality; the conversion formula is:
  // FER = 1-(1-BER)^(8*PSDU_OFDMA)
  //ns3_wimax->Pr_threshold_fer =1 - pow(1-1e-6, 8*PSDU_OFDMA);

  // we now specify BER directly, but it can also be computed from FER:
  // ber = (1-pow(1-Pr_threshold_fer, 1/(8*(double)frame_size))) 
  adapter->Pr_threshold_ber = 1e-6;

  //wimax_print_adapter(adapter);

  return SUCCESS;
}

// print the content of a WiMAX adapter structure
void
wimax_print_adapter (struct parameters_802_16 *adapter)
{
  int i;

  printf ("WiMAX adapter '%s'\n", adapter->name);
  printf ("\tPr_thresholds[%d]:", WIMAX_RATES_NUMBER);
  for (i = 0; i < WIMAX_RATES_NUMBER; i++)
    printf (" %.2f", adapter->Pr_thresholds[i]);
  printf ("\n");

  printf ("\tPr_threshold_ber=%f\n", adapter->Pr_threshold_ber);
}

// compute the minimum receiver sensitivity threshold according to Eq. 149b
// in IEEE 802.16e-2005 standard (page 646) (denoted as reference [s]);
// return the threshold
double
wimax_min_threshold (struct capacity_class *capacity)
{
  double SNR_Rx = 1;		// receiver SNR
  double R = REPETITION_FACTOR;	// repetition factor
  double F_S = capacity->sampling_frequency;	// sampling frequency

  // number of used subcarriers (data+pilot)
  // FIXME: should use MIN or MAX of DL and UL (which is correct?)
  // NOTE: all subcarriers represents the worst case!
  double N_Used = capacity->dl_used_subcarriers;

  double N_FFT = capacity->fft_size;	//FFT size
  double ImpLoss = 5;		// implementation loss (values from [s] page 646)
  double NF = 8;		// receiver noise figure (values from [s] page 646)

  // set SNR_Rx depending on MCS based on [s] Table 338 (page 647);
  // extrapolated values assume a linear relationship between coding rate
  // and SNR_Rx which is not necessarily true
  switch (capacity->mcs)
    {
      // QPSK MCS
    case QPSK_18:
      SNR_Rx = 1;
      break;			// extrapolated based on Table 338
    case QPSK_14:
      SNR_Rx = 2;
      break;			// extrapolated based on Table 338
    case QPSK_12:
      SNR_Rx = 5;
      break;			// Table 338
    case QPSK_34:
      SNR_Rx = 8;
      break;			// Table 338

      // QAM-16 MCS
    case QAM16_12:
      SNR_Rx = 10.5;
      break;			// Table 338
    case QAM16_23:
      SNR_Rx = 12.84;
      break;			// extrapolated based on Table 338
    case QAM16_34:
      SNR_Rx = 14;
      break;			// Table 338

      // QAM-64 MCS
    case QAM64_12:
      SNR_Rx = 16;
      break;			// Table 338
    case QAM64_23:
      SNR_Rx = 18;
      break;			// Table 338
    case QAM64_34:
      SNR_Rx = 20;
      break;			// Table 338
    case QAM64_56:
      SNR_Rx = 22;
      break;			// extrapolated based on Table 338

    default:
      WARNING ("Modulation scheme '%d' is not supported", capacity->mcs);
      return DBL_MAX;		// FIXME: what is correct value to return on error?
    }

  // results were validated using the information in "Correction to Rx
  // SNR, Rx sensitivity, and Tx Relative Constellation Error for OFDM and
  // OFDMA systems", 2005-09-22 by comparing the values in deleted
  // Table 337 after adding about 4 dB with the values obtained by using
  // the equation; in particular, for 64 QAM 3/4 3.5MHz the correct value 
  // should be around -76 dB (also given in text) and the equation gives -76.485
  // one more validation even more accurate was done using Tables 1-3 from
  // "802.16e Mobile WiMAX Key RF Receiver Requirements", Microwave Journal:
  //    http://www.microwavejournal.com/articles/print/4774-802-16e-mobile-wimax-key-rf-receiver-requirements
  // NOTE: -114 is said to represent the thermal noise
  return (-114 + SNR_Rx - 10 * log10 (R) + 10 * log10 (F_S * N_Used / N_FFT)
	  + ImpLoss + NF);
}

// compute FER corresponding to the current conditions
// for a given operating rate
// return SUCCESS on succes, ERROR on error
int
wimax_fer (struct connection_class *connection,
	   struct scenario_class *scenario, int operating_rate, double *fer)
{
  double fer_value = 0.0;

  //int MAC_Overhead = 224;     // MAC ovearhed in bits (header + FCS) // FIXME: check
  //int OFDM_Overhead = 22;     // OFDM ovearhed in bits (service field + tail bits)

  int header;			// FIXME: needed?!

  struct environment_class *environment =
    &(scenario->environments[connection->through_environment_index]);

  void *adapter
    = wimax_get_interface_adapter
    (connection,
     &((scenario->nodes[connection->from_node_index]).
       interfaces[connection->from_interface_index]));

  // check whether a suitable adapter was found
  if (adapter == NULL)
    {
      WARNING ("No suitable adapter found for connection");
      return ERROR;
    }

  struct parameters_802_16 *adapter_802_16
    = (struct parameters_802_16 *) adapter;

  // compute header size in bytes
  // FIXME: needed?
  header = 0;			//MAC_Overhead / 8;

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
				   WIMAX_MINIMUM_NOISE_POWER);

      // limit combined_noise to a minimum standard value;
      // this is equivalent with making sure performance cannot
      // exceed that obtained in optimum conditions
      if (combined_noise < WIMAX_STANDARD_NOISE)
	combined_noise = WIMAX_STANDARD_NOISE;

      // compute SNR
      connection->SNR = connection->Pr - combined_noise;
    }
  else
    {
      WARNING ("Environment %s contains inconsistencies", environment->name);
      return ERROR;
    }

  // compute SNR decrease due to Doppler;
  // we do it before computing gain, but is this correct?!
  double relative_velocity 
    = motion_relative_velocity (scenario,
				&(scenario->
				  nodes[connection->
					from_node_index]),
				&(scenario->
				  nodes[connection->
					to_node_index]));
  struct capacity_class *capacity =
    &(((scenario->nodes[connection->from_node_index]).
       interfaces[connection->from_interface_index]).wimax_params);
  capacity->Nt =
    ((scenario->nodes[connection->from_node_index]).
     interfaces[connection->from_interface_index]).antenna_count;
  capacity->Nr =
    ((scenario->nodes[connection->to_node_index]).
     interfaces[connection->to_interface_index]).antenna_count;

  double doppler_snr_value = doppler_snr (FREQUENCY_WIMAX,
					  capacity->subcarrier_spacing * 1e3,
					  relative_velocity, connection->SNR);
  //printf("freq=%.2f GHz  subcarrier_spacing=%.1f Hz  rel_velocity=%f  SNR=%f  ",
  //     FREQUENCY_WIMAX/1e9, capacity->subcarrier_spacing*1e3,
  //     relative_velocity, connection->SNR);
  //printf("SNR decrease due to Doppler shift = %.3f dB\n", doppler_snr_value);

  // update SNR by subtracting the change due to Doppler shift
  connection->SNR -= doppler_snr_value;

  // compute gain for Matrix A operation; for Matrix B there is no SNR gain 
  // but throughput/multiplexing  gain (see function 'capacity_update_mcs')
  if (capacity->mimo_type == MIMO_TYPE_MATRIX_A)
    {
      int Nt = capacity->Nt;	// TX_ANTENNA_COUNT;
      int Nr = capacity->Nr;	// RX_ANTENNA_COUNT; 
      //printf("Connection %d->%d: Nt=%d Nr=%d\n", connection->from_node_index, 
      //         connection->to_node_index, Nt, Nr);

      // array and diversity gains
      double array_gain, diversity_gain;

      // array gain is obtained by coherently combining the energy
      // received by each antenna and leads to a linear increase with 
      // Nr (in W domain) [Ref: Fundamentals of WiMAX, pp. 179-198] =>
      // array gain is given by Nr (use log to convert to dB)
      array_gain = 10 * log10 ((double) (Nr));

      // diversity gain depends on the number of uncorrelated paths
      // between the transmitter and receiver, hence Nt*Nr (=Nd, aka diversity
      // order) as follows: [Ref: Fundamentals of WiMAX, pp. 179-198]
      // - in fully uncorrelated channels change is given by (SNR)^(Nd-1)
      // - in correlated channels change is given by Nd factor

      // we assume below correlated (possibly LOS) channels =>
      // diversity gain is given by the diversity order Nt*Nr (use log to
      // convert to dB)
      diversity_gain = 10 * log10 ((double) (Nt * Nr));

      // for fully uncorrelated channels gain would be given by formula below;
      // FIXME: need to check that SNR is positive!
      // FIXME: how to choose between the formula above and that
      //        below, i.e., how to determine channels are correlated or not?!
      // diversity_gain = (Nt*Nr-1)*connection->SNR;

      // add array and diversity gains to SNR
      connection->SNR += (array_gain + diversity_gain);
    }

  /*  
     // FIXME: check whether model parameters were initialized?!

     // compute FER based on simple exponential model
     fer_value = adapter_802_16->Pr_threshold_fer *
     exp (adapter_802_16->model_alpha *
     (adapter_802_16->Pr_thresholds[operating_rate] -
     (connection->SNR + WIMAX_STANDARD_NOISE)));

     // limit error rate for numerical reasons
     if (fer_value > MAXIMUM_ERROR_RATE)
     fer_value = MAXIMUM_ERROR_RATE;
   */

  fer_value
    = environment_fading (environment, adapter_802_16->Pr_threshold_ber,
			  connection->packet_size + header,
			  adapter_802_16->Pr_thresholds[operating_rate],
			  connection->SNR + WIMAX_STANDARD_NOISE);

  DEBUG ("Pr_threshold_fer=%f model_alpha=%f Pr_thresholds[%d]=%f power=%f \
(SNR=%f) => fer=%f", adapter_802_16->Pr_threshold_fer, adapter_802_16->model_alpha, operating_rate, adapter_802_16->Pr_thresholds[operating_rate], connection->SNR + WIMAX_STANDARD_NOISE, connection->SNR, fer_value);

  // this functionality is now included in environment_fading
  // this FER corresponds to the standard value PSDU_OFDMA,
  // therefore needs to be adjusted for different packet sizes
  // fer_value = 1 - pow (1 - fer_value, (connection->packet_size + header) 
  //                   / PSDU_OFDMA);

  // limit error rate for numerical reasons
  //if (fer_value > MAXIMUM_ERROR_RATE)
  /// fer_value = MAXIMUM_ERROR_RATE;

  DEBUG ("FER=%g\n", fer_value);

  // printf("Pr=%f operating_rate=%d mcs=%d Pr_thresholds[operating_rate]=%f fer=%f\n", connection->Pr, operating_rate, capacity->mcs, adapter_802_16->Pr_thresholds[operating_rate], fer_value); 

  (*fer) = fer_value;

  return SUCCESS;
}

// compute loss rate based on FER
// return SUCCESS on succes, ERROR on error
int
wimax_loss_rate (struct connection_class *connection,
		 struct scenario_class *scenario, double *loss_rate)
{
  if (wimax_fer (connection, scenario, connection->operating_rate,
		 &(connection->frame_error_rate)) == ERROR)
    {
      WARNING ("Error while computing frame error rate");
      return ERROR;
    }

  // as we only emulate the WiMAX PHY, loss rate equals FER
  (*loss_rate) = connection->frame_error_rate;
  return SUCCESS;
}

// compute operating rate based on FER and a model of a rate
// auto-selection mechanism
// return SUCCESS on succes, ERROR on error
int
wimax_operating_rate (struct connection_class *connection,
		      struct scenario_class *scenario, int *operating_rate)
{
  // update operating rate based on MCS value in capacity structure
 struct capacity_class *capacity =
    &(((scenario->nodes[connection->from_node_index]).
       interfaces[connection->from_interface_index]).wimax_params);
  connection->operating_rate = capacity->mcs;

  (*operating_rate) = connection->operating_rate;
  return SUCCESS;
}

// compute delay & jitter and return values in arguments
// return SUCCESS on succes, ERROR on error
int
wimax_delay_jitter (struct connection_class *connection,
		    struct scenario_class *scenario,
		    double *variable_delay, double *delay, double *jitter)
{
  struct node_class *node_rx = &(scenario->nodes[connection->to_node_index]);
  struct node_class *node_tx =
    &(scenario->nodes[connection->from_node_index]);

  // for the moment we consider that the variable delay
  // is constant and equal to the frame duration;
  // also we assume there is no jitter
  // FIXME: is it correct?!
  // delay is actually handled by Ns-3, so we set this to 0
  (*variable_delay) = 0;//WIMAX_FRAME_DURATION;
  (*jitter) = 0;

  // delay is equal to the fixed delay configured for the nodes
  (*delay) = node_rx->internal_delay + node_tx->internal_delay
    + (*variable_delay);

  return SUCCESS;
}

// compute bandwidth based on operating rate, delay and packet size
// return SUCCESS on succes, ERROR on error
int
wimax_bandwidth (struct connection_class *connection,
		 struct scenario_class *scenario, double *bandwidth)
{
  struct capacity_class *capacity
    =
    &(((scenario->nodes[connection->from_node_index]).
       interfaces[connection->from_interface_index]).wimax_params);
  capacity->Nt =
    ((scenario->nodes[connection->from_node_index]).
     interfaces[connection->from_interface_index]).antenna_count;
  capacity->Nr =
    ((scenario->nodes[connection->to_node_index]).
     interfaces[connection->to_interface_index]).antenna_count;
  capacity_update_mimo (capacity, capacity->mimo_type);

  // we assume that the base station has the highest index, hence
  // low-to-high index connections are uplink
  // FIXME: implement a more generic method?
  if(connection->from_node_index<connection->to_node_index)
    (*bandwidth) = capacity->ul_data_rate;
  else
    (*bandwidth) = capacity->dl_data_rate;
  /*
  printf("bandwidth = %.2f MCS = %2d: PHY data rate [Mbps]: DL = %5.2f UL = %5.2f\n", capacity->system_bandwidth, capacity->mcs, capacity->dl_data_rate, capacity->ul_data_rate);
  */
  return SUCCESS;
}

// update the changing properties of a connection (e.g., relative distance, Pr)
// after the state of the system changed;
// return SUCCESS on succes, ERROR on error
int
wimax_connection_update (struct connection_class *connection,
			 struct scenario_class *scenario)
{
  // helper "shortcuts"
  struct node_class *node_rx = &(scenario->nodes[connection->to_node_index]);
  struct node_class *node_tx =
    &(scenario->nodes[connection->from_node_index]);
  struct environment_class *environment =
    &(scenario->environments[connection->through_environment_index]);

  //connection->operating_rate = node_tx->interfaces[0].wimax_params.mcs;
  //printf("tx.mcs=%d rx.mcs=%d => operating_rate=%d\n",node_tx->interfaces[0].wimax_params.mcs, node_rx->interfaces[0].wimax_params.mcs,connection->operating_rate );

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

	  connection->Pr
	    = node_tx->interfaces[connection->from_interface_index].Pr0;

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
	  connection->Pr =
	    node_tx->interfaces[connection->from_interface_index].Pr0;

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


////////////////////////////////////////////////
// Capacity calculation functions
////////////////////////////////////////////////

// In what follows we use values obtained or derived from these references:
// [1] C. So-In, R. Jain, A-K Tamimi, "Capacity Evaluation for IEEE 802.16e 
//     Mobile WiMAX", Journal of Computer Systems, Networks and Comm.
// [2] Rohde & Schwarz, "Mobile WiMAX Throughput Measurements",
//     Application Note
// [3] Agilent Technologies, "Mobile WiMAX PHY Layer (RF) Operation and 
//     Measurement", Application Note
// [4] H. Yaghoobi, "Scalable OFDMA Physical Layer in IEEE 802.16 Wireless MAN",
//     Intel Technology Journal
// [5] WiMAX Forum Mobile System Profile rev. 1.2.2 (2006-11-17)
// [6] Rohde & Schwarz, "Mobile WiMAX MIMO Multipath Performance
//     Measurements", Application Note

// update the entire capacity calculation structure, 
// including parameters that depend on system bandwidth;
// return SUCCESS on success, ERROR on error
int
capacity_update_all (struct capacity_class *capacity,
		     double system_bandwidth, int mcs, int mimo_type,
		     int Nt, int Nr)
{
  // system bandwidth in MHz
  //printf("capacity_update_all: bw=%.2f mcs=%d\n", system_bandwidth, mcs);

  capacity->system_bandwidth = system_bandwidth;

  capacity->Nt = Nt;
  capacity->Nr = Nr;

  /////////////////////////////////////////////////
  // set parameters that depend on system bandwidth

  if (capacity->system_bandwidth == SYS_BW_1)
    {
      capacity->sampling_factor = SAMPLING_FACTOR_28_25;	// [1] Table 1
      capacity->fft_size = 128;	// [1] Table 1 (formula in [2] Fig. 2)
      capacity->dl_signaling_overhead = DL_PREAMBLE_OVERHEAD + DL_ADDITIONAL_OVERHEAD_16;	// FIXME: guessed from [2] Table 2
      capacity->ttg_rtg_duration = 1.61;

      // total symbols = 47 (probably OK, from calculation)
      if (use_paper_values == TRUE)
	{
	  // values based on total symbols = 47 and [1] Sec. 3, paragraph 2
	  capacity->dl_symbols = 29;	// probably OK
	  capacity->ul_symbols = 18;	// probably OK
	}
      else
	{
	  // values based on total symbols = 47 and max values in [2] Table 2
	  capacity->dl_symbols = 35;	// probably OK
	  capacity->ul_symbols = 21;	// probably OK
	}
    }
  else if (capacity->system_bandwidth == SYS_BW_5)
    {
      capacity->sampling_factor = SAMPLING_FACTOR_28_25;	// [1] Table 1
      capacity->fft_size = 512;	// [1] Table 1 (formula in [2] Fig. 2)
      capacity->dl_signaling_overhead = DL_PREAMBLE_OVERHEAD + DL_ADDITIONAL_OVERHEAD_16;	// [2] Table 2
      capacity->ttg_rtg_duration = 1.61;

      // total symbols = 47 (calculation and [3] Table 2)
      if (use_paper_values == TRUE)
	{
	  // values based on total symbols = 47 and [1] Sec. 3, paragraph 2
	  capacity->dl_symbols = 29;
	  capacity->ul_symbols = 18;
	}
      else
	{
	  // max DL & UL values in [2] Table 2
	  capacity->dl_symbols = 35;
	  capacity->ul_symbols = 21;
	}
    }
  else if (capacity->system_bandwidth == SYS_BW_10)
    {
      capacity->sampling_factor = SAMPLING_FACTOR_28_25;	// [1] Table 1
      capacity->fft_size = 1024;	// [1] Table 1 (formula in [2] Fig. 2) 
      capacity->dl_signaling_overhead = DL_PREAMBLE_OVERHEAD + DL_ADDITIONAL_OVERHEAD_12;	// [2] Table 2
      capacity->ttg_rtg_duration = 1.61;

      // total symbols = 47 (calculation and [3] Table 2)
      if (use_paper_values == TRUE)
	{
	  // [1] Sec. 3, paragraph 2
	  capacity->dl_symbols = 29;
	  capacity->ul_symbols = 18;
	}
      else
	{
	  // max DL & UL values in [2] Table 2
	  capacity->dl_symbols = 35;
	  capacity->ul_symbols = 21;
	}
    }
  else if (capacity->system_bandwidth == SYS_BW_20)
    {
      capacity->sampling_factor = SAMPLING_FACTOR_28_25;	// [1] Table 1
      capacity->fft_size = 2048;	// [1] Table 1 (formula in [2] Fig. 2)
      capacity->dl_signaling_overhead = DL_PREAMBLE_OVERHEAD + DL_ADDITIONAL_OVERHEAD_12;	// FIXME: guessed from [2] Table 2
      capacity->ttg_rtg_duration = 1.61;

      // total symbols = 47 (probably OK, from calculation)
      if (use_paper_values == TRUE)
	{
	  // values based on total symbols = 47 and [1] Sec. 3, paragraph 2
	  capacity->dl_symbols = 29;	// probably OK
	  capacity->ul_symbols = 18;	// probably OK
	}
      else
	{
	  // values based on total symbols = 47 and max values in [2] Table 2
	  capacity->dl_symbols = 35;	// probably OK
	  capacity->ul_symbols = 21;	// probably OK
	}
    }
  else if (capacity->system_bandwidth == SYS_BW_3)
    {
      capacity->sampling_factor = SAMPLING_FACTOR_8_7;	// [1] Table 1
      capacity->fft_size = 512;	// [1] Table 1 (formula in [2] Fig. 2)
      capacity->dl_signaling_overhead = DL_PREAMBLE_OVERHEAD + DL_ADDITIONAL_OVERHEAD_16;	// [2] Table 2
      capacity->ttg_rtg_duration = 1.72;

      // total symbols = 33 (probably OK, from calculation)
      if (use_paper_values == TRUE)
	{
	  // the values below are those we calculated to be the closest to 
	  // 2:1 ratio with sum 33, with dl_symbols of form 2k+1 and 
	  // ul_symbols of form 3j ([1] Sec. 3, paragraph 2);
	  // possible values according to [5] Table 10
	  capacity->dl_symbols = 21;
	  capacity->ul_symbols = 12;
	}
      else
	{
	  // max DL & UL values in [2] Table 2
	  capacity->dl_symbols = 24;
	  capacity->ul_symbols = 15;
	}
    }
  else if (capacity->system_bandwidth == SYS_BW_7)
    {
      capacity->sampling_factor = SAMPLING_FACTOR_8_7;	// [1] Table 1
      capacity->fft_size = 1024;	// [1] Table 1 (formula in [2] Fig. 2)
      capacity->dl_signaling_overhead = DL_PREAMBLE_OVERHEAD + DL_ADDITIONAL_OVERHEAD_12;	// [2] Table 2
      capacity->ttg_rtg_duration = 1.72;

      // total symbols = 33 (calculation and [3] Table 2)
      if (use_paper_values == TRUE)
	{
	  // the values below are those we calculated to be the closest to 
	  // 2:1 ratio with sum 33, with dl_symbols of form 2k+1 and 
	  // ul_symbols of form 3j ([1] Sec. 3, paragraph 2);
	  // possible values according to [5] Table 10
	  capacity->dl_symbols = 21;
	  capacity->ul_symbols = 12;
	}
      else
	{
	  // max DL & UL values in [2] Table 2
	  capacity->dl_symbols = 24;
	  capacity->ul_symbols = 15;
	}
    }
  else if (capacity->system_bandwidth == SYS_BW_8)
    {
      capacity->sampling_factor = SAMPLING_FACTOR_8_7;	// [1] Table 1
      capacity->fft_size = 1024;	// [1] Table 1 (formula in [2] Fig. 2)
      capacity->dl_signaling_overhead = DL_PREAMBLE_OVERHEAD + DL_ADDITIONAL_OVERHEAD_12;	// [2] Table 2
      capacity->ttg_rtg_duration = 1.40;

      // total symbols = 42 ([3] Table 2)
      if (use_paper_values == TRUE)
	{
	  // the values below are those we calculated to be the closest to 
	  // 2:1 ratio with sum 42, with dl_symbols of form 2k+1 and 
	  // ul_symbols of form 3j ([1] Sec. 3, paragraph 2);
	  // possible values according to [5] Table 10
	  capacity->dl_symbols = 27;
	  capacity->ul_symbols = 15;
	}
      else
	{
	  // max DL & UL values in [2] Table 2
	  capacity->dl_symbols = 30;
	  capacity->ul_symbols = 18;
	}
    }
  else
    {
      WARNING ("System bandwidth = %.2f is not supported.", system_bandwidth);
      return ERROR;
    }

  // UL signaling overhead is always the same ([2] Table 2)
  capacity->ul_signaling_overhead = UL_CONTROL_SYMBOLS;


  //////////////////////////////////////////////////
  // compute basic parameters (cf. [1] Table 1)

  // sampling_frequency in MHz, mainly depends on sampling factor ([2] Fig. 2)
  capacity->sampling_frequency
    =
    floor (capacity->sampling_factor * capacity->system_bandwidth * 1e6 /
	   8000) * 8000 / 1e6;

  // frequency sampling time ([1] Table 1);
  // divide by 1e3 to represent value in nanoseconds;
  // we use floor to match results in [1] Table 1
  // capacity->sample_time = floor( (1/capacity->sampling_frequency)*1e3);
  capacity->sample_time = 1 / capacity->sampling_frequency * 1e3;

  // subcarrier spacing ([2] Table 2);
  // multiply by 1e3 to get results in kHz;
  // we round result to 2 decimal places to match results in [1] Table 1
  //capacity->subcarrier_spacing = 
  //  floor (capacity->sampling_frequency * 1e3 / capacity->fft_size*100)/100;
  capacity->subcarrier_spacing =
    capacity->sampling_frequency * 1e3 / capacity->fft_size;

  // useful (data) symbol time ([1] Table 1);
  // multiply by 1e3 to have results in us;
  // we round result to 1 decimal place to match results in [1] Table 1
  //capacity->data_symbol_time 
  //  = floor ((1/capacity->subcarrier_spacing)*1e3 *10)/10; 
  capacity->data_symbol_time = 1 / capacity->subcarrier_spacing * 1e3;

  // guard time is used to avoid symbol interference caused by 
  // multi-path fading ([1] Table 1)
  capacity->guard_time = capacity->data_symbol_time * CYCLIC_PREFIX_RATIO_G;

  // OFDMA symbol duration ([1] Table 1)
  capacity->OFDMA_symbol_time = capacity->data_symbol_time
    + capacity->guard_time;

  // total number of symbols per frame after subtracting 
  // time reserved for TTG/RTG
  capacity->total_symbols
    = floor (FRAME_DURATION / capacity->OFDMA_symbol_time
	     - capacity->ttg_rtg_duration);

#ifdef DL_SYMBOL_RATIO
  if (1)
    {
      capacity->dl_symbols = (int) floor(capacity->total_symbols*DL_SYMBOL_RATIO);
      capacity->ul_symbols = (int) floor(capacity->total_symbols*(1-DL_SYMBOL_RATIO));
      //printf("Update DL and UL symbol count: dl_symbols=%d ul_symbols=%d (DL_SYMBOL_RATIO=%.2f)\n", capacity->dl_symbols, capacity->ul_symbols, DL_SYMBOL_RATIO);
    }
#endif

  ////////////////////////////////////////////
  // compute subcarrier values ([1] Table 2])

  // an empirical formula can deduced based on [1] Table 2 values,
  // but it does not provide the correct results in all cases,
  // so we use directly the values instead; for reference, the formula is:
  // guard_subcarriers = floor (capacity->fft_size * 0.179)

  // initialize number of DL and UL guard subcarriers
  if (capacity->system_bandwidth == SYS_BW_1)
    {
      capacity->dl_guard_subcarriers = 43;	// [1] Table 2
      capacity->ul_guard_subcarriers = 31;	// [1] Table 2
    }
  else if (capacity->system_bandwidth == SYS_BW_5)
    {
      capacity->dl_guard_subcarriers = 91;	// [1] Table 2
      capacity->ul_guard_subcarriers = 103;	// [1] Table 2
    }
  else if (capacity->system_bandwidth == SYS_BW_10)
    {
      capacity->dl_guard_subcarriers = 183;	// [1] Table 2
      capacity->ul_guard_subcarriers = 183;	// [1] Table 2
    }
  else if (capacity->system_bandwidth == SYS_BW_20)
    {
      capacity->dl_guard_subcarriers = 367;	// [1] Table 2
      capacity->ul_guard_subcarriers = 367;	// [1] Table 2
    }
  else if (capacity->system_bandwidth == SYS_BW_3)
    {
      // FIXME: adjusted to match results in [2] Table 5
      capacity->dl_guard_subcarriers = 150;

      // applied empirical formula given that FFT=512 => same with 5 MHz case;
      // results validated using [2] Table 5
      capacity->ul_guard_subcarriers = 103;
    }
  else if (capacity->system_bandwidth == SYS_BW_7)
    {
      // FIXME: adjusted to match results in [2] Table 5
      capacity->dl_guard_subcarriers = 255;

      // applied empirical formula given that FFT=1024 => same with 10 MHz case;
      // results validated using [2] Table 5
      capacity->ul_guard_subcarriers = 183;
    }
  else if (capacity->system_bandwidth == SYS_BW_8)
    {
      // FIXME: adjusted to match results in [2] Table 5
      capacity->dl_guard_subcarriers = 230;

      // applied empirical formula given that FFT=1024 => same with 10 MHz case;
      // results validated using [2] Table 5
      capacity->ul_guard_subcarriers = 183;
    }
  else
    {
      WARNING ("System bandwidth = %.2f is not supported.",
	       capacity->system_bandwidth);
      return ERROR;
    }

  // total number of subcarriers is equal with FFT size,
  // an is composed of used subcarriers and guard subcarriers
  capacity->dl_used_subcarriers = capacity->fft_size
    - capacity->dl_guard_subcarriers;

  // formula deduced based on [1] Sec. 2, paragraph 6, and verified using 
  // the values in Table 2; the following were considered:
  // * 1 DC subcarrier is sent without power;
  // * there are 28 subcarriers in a subchannel
  // * on average there are 4 pilots in a subchannel (8 pilots / 2 symbol time)
  capacity->dl_pilot_subcarriers
    = (int) ((capacity->dl_used_subcarriers - DC_SUBCARRIER)
	     / DL_SUBCARRIERS_PER_SLOT)
    * (DL_PILOTS_PER_SLOT / DL_SYMBOLS_PER_SLOT);

  // used subcarriers = data subcarriers + pilot subcarriers;
  // 1 DC subcarrier is sent without power
  capacity->dl_data_subcarriers
    = (capacity->dl_used_subcarriers - capacity->dl_pilot_subcarriers)
    - DC_SUBCARRIER;

  // total number of subcarriers is equal with FFT size,
  // an is composed of used subcarriers and guard subcarriers
  capacity->ul_used_subcarriers = capacity->fft_size
    - capacity->ul_guard_subcarriers;

  // formula deduced based on [1] Sec. 2, paragraph 6, and verified using 
  // the values in Table 2; the following were considered:
  // * 1 DC subcarrier is sent without power;
  // * there are 24 subcarriers in a subchannel
  // * on average there are 8 pilots in a subchannel (24 pilots / 3 symbol time)
  capacity->ul_pilot_subcarriers
    = (int) ((capacity->ul_used_subcarriers - DC_SUBCARRIER)
	     / UL_SUBCARRIERS_PER_SLOT)
    * (UL_PILOTS_PER_SLOT / UL_SYMBOLS_PER_SLOT);

  // used subcarriers = data subcarriers + pilot subcarriers;
  // 1 DC subcarrier is sent without power      
  capacity->ul_data_subcarriers
    = (capacity->ul_used_subcarriers - capacity->ul_pilot_subcarriers)
    - DC_SUBCARRIER;


  /////////////////////////////////////////////////////////////
  // compute slot values ([1] Table 4)

  // number of slots per frame formula deduced based on [1] Table 4
  // (another method can be used to calculate the number of slots in 
  //  DL subframe is = subchannel * (total symbols excluding preamble /2), 
  // because 28/2=14)
  capacity->dl_slots =
    (capacity->dl_data_subcarriers + capacity->dl_pilot_subcarriers)
    * (capacity->dl_symbols - capacity->dl_signaling_overhead)
    / (DL_SUBCARRIERS_PER_SLOT * DL_SYMBOLS_PER_SLOT);

  // number of slots per frame
  // another method can be used to calculate the number of slots in UL
  // subframe is = subchannel * (total symbols excluding reserved /3),
  // because 15/3=5 symbols in slots
  capacity->ul_slots =
    (capacity->ul_data_subcarriers + capacity->ul_pilot_subcarriers)
    * (capacity->ul_symbols - capacity->ul_signaling_overhead)
    / (UL_SUBCARRIERS_PER_SLOT * UL_SYMBOLS_PER_SLOT);

  // store MIMO type
  capacity->mimo_type = mimo_type;

  // compute noise power using the formula: 
  // P_dBm = -174 + 10*log10(delta_f) (with delta_f expressed in Hertz)
  // [Wikipedia: Johnson-Nyquist noise]
  // FIXME: the most accurate results would be obtained if noise power
  // would be computed depending on the number of carriers that are actually
  // used instead of the entire system bandwidth
  capacity->thermal_noise_power
    = -174 + 10 * log10 (capacity->system_bandwidth * 1e6);

  // in the end, update the MCS-dependent parameters as well and return
  return capacity_update_mcs (capacity, mcs);
}

// update the MIMO-dependent parameters capacity calculation structure,
// which effectively recomputes the DL and UL data rates;
// return SUCCESS on success, ERROR on error
int
capacity_update_mimo (struct capacity_class *capacity, int mimo_type)
{
  capacity->mimo_type = mimo_type;

  // in the end, update the MCS-dependent parameters as well and return
  // FIXME: optimize to recompute only for matrix B, the only MIMO type 
  // which affect data rate directly?!
  return capacity_update_mcs (capacity, capacity->mcs);
}

// update the MCS-dependent parameters capacity calculation structure,
// which effectively recomputes the DL and UL data rates;
// return SUCCESS on success, ERROR on error
int
capacity_update_mcs (struct capacity_class *capacity, int mcs)
{
  // MCS dependent part starts here
  // check whether modulation scheme is supported
  if (mcs < QPSK_18 || mcs > QAM64_56)
    {
      WARNING ("Modulation scheme '%d' is not supported", mcs);
      return ERROR;
    }
  else
    capacity->mcs = mcs;


  //////////////////////////////////////////
  // Downlink computation

  // number of bytes per slot
  capacity->dl_bytes_per_slot = capacity_bytes_per_slot (capacity, TRUE);
  if (capacity->dl_bytes_per_slot == -1)
    return ERROR;

  // maximum data rate in bps given a certain frame duration
  capacity->dl_data_rate =
    (capacity->dl_bytes_per_slot * 8 * capacity->dl_slots)
    * (1e6 / FRAME_DURATION);


  //////////////////////////////////////////
  // Uplink computation

  // number of bytes per slot
  capacity->ul_bytes_per_slot = capacity_bytes_per_slot (capacity, FALSE);
  if (capacity->ul_bytes_per_slot == -1)
    return ERROR;

  // maximum data rate in bps given a certain frame duration
  capacity->ul_data_rate =
    (capacity->ul_bytes_per_slot * 8 * capacity->ul_slots)
    * (1e6 / FRAME_DURATION);

  // for matrix B MIMO, DL data rate is doubled [6] page 5, paragraph 3
  // (in [6] the doubling is done before taking into account MCS, 
  // but since MCS only implies a multiplication with a constant,
  // doubling can also be done at the end);
  // actually the multiplication factor is min(Nt,Nr) = 2 for 2x2 systems
  // Tx and Rx antenna count
  // Tx and Rx antenna count
  int Nt = capacity->Nt;	// TX_ANTENNA_COUNT;
  int Nr = capacity->Nr;	// RX_ANTENNA_COUNT; 

  if (capacity->mimo_type == MIMO_TYPE_MATRIX_B)
    capacity->dl_data_rate *= ((Nt < Nr) ? Nt : Nr);

  // in case repetition factor is enabled, it will contribute to a 
  // proportional decrease in data rate; 
  // note that repetition coding is only possible for QPSK rates!
  // FIXME: make repetition factor dynamic not predefined constant
  if (mcs >= QPSK_18 && mcs <= QPSK_34)
    if (REPETITION_FACTOR > 1)
      {
	capacity->dl_data_rate /= (double) REPETITION_FACTOR;
	capacity->ul_data_rate /= (double) REPETITION_FACTOR;
      }

  return SUCCESS;
}

// print capacity calculation structure
void
capacity_print (struct capacity_class *capacity)
{
  printf ("------------------------ TABLE 1 -----------------------\n");
  printf ("System bandwidth [MHz]\t\t\t\t%.2f\n", capacity->system_bandwidth);
  printf ("Sampling factor\t\t\t\t\t%.2f\n", capacity->sampling_factor);
  printf ("Sampling frequency (F_s) [MHz]\t\t\t%.1f\n",
	  capacity->sampling_frequency);
  printf ("Sample time (1/F_s) [ns]\t\t\t%.0f\n", capacity->sample_time);
  printf ("FFT size (N_FFT)\t\t\t\t%d\n", capacity->fft_size);
  printf ("Subcarrier spacing (Delta_f) [kHz]\t\t%.2f\n",
	  capacity->subcarrier_spacing);
  printf ("Useful symbol time (T_b = 1/Delta_f) [us])\t%.1f\n",
	  capacity->data_symbol_time);
  printf ("Guard time (T_g = T_b/8) [us]\t\t\t%.1f\n", capacity->guard_time);
  printf ("OFDMA symbol time (T_s = T_b + T_g) [us]\t%.1f\n",
	  capacity->OFDMA_symbol_time);
  printf ("------------------------- TABLE 2 ----------------------\n");
  printf ("(a) Downlink\n");
  printf ("System bandwidth [MHz]\t\t\t\t%.2f\n", capacity->system_bandwidth);
  printf ("FFT size (N_FFT)\t\t\t\t%d\n", capacity->fft_size);
  printf ("number of guard subcarriers\t\t\t%d\n",
	  capacity->dl_guard_subcarriers);
  printf ("number of used subcarriers\t\t\t%d\n",
	  capacity->dl_used_subcarriers);
  printf ("number of pilot subcarriers\t\t\t%d\n",
	  capacity->dl_pilot_subcarriers);
  printf ("number of data subcarriers\t\t\t%d\n",
	  capacity->dl_data_subcarriers);
  printf ("(b) Uplink\n");
  printf ("System bandwidth [MHz]\t\t\t\t%.2f\n", capacity->system_bandwidth);
  printf ("FFT size (N_FFT)\t\t\t\t%d\n", capacity->fft_size);
  printf ("number of guard subcarriers\t\t\t%d\n",
	  capacity->ul_guard_subcarriers);
  printf ("number of used subcarriers\t\t\t%d\n",
	  capacity->ul_used_subcarriers);
  printf ("number of pilot subcarriers\t\t\t%d\n",
	  capacity->ul_pilot_subcarriers);
  printf ("number of data subcarriers\t\t\t%d\n",
	  capacity->ul_data_subcarriers);
  printf ("------------------------- OTHER ------------------------\n");
  printf ("Total symbols per frame = %d\n", capacity->total_symbols);
  printf ("DL: Number of symbols = %d\n", capacity->dl_symbols);
  printf ("UL: Number of symbols = %d\n", capacity->ul_symbols);
  printf ("DL: Number of slots = %d\n", capacity->dl_slots);
  printf ("UL: Number of slots = %d\n", capacity->ul_slots);
  printf ("DL: Bytes per slot = %.2f\n", capacity->dl_bytes_per_slot);
  printf ("UL: Bytes per slot = %.2f\n", capacity->ul_bytes_per_slot);
  printf ("DL: Data rate = %.2f Mbps\n", capacity->dl_data_rate / 1e6);
  printf ("UL: Data rate = %.2f Mbps\n", capacity->ul_data_rate / 1e6);
  printf ("Total data rate = %.2f Mbps\n", (capacity->dl_data_rate
					    + capacity->ul_data_rate) / 1e6);
  printf ("MIMO type: %d\n", capacity->mimo_type);
  printf ("Thermal noise power: %f\n", capacity->thermal_noise_power);
  printf ("--------------------------------------------------------\n");
}

// initialize subcarrier-based information;
// return SUCCESS on success, ERROR on error
int
capacity_subcarriers (struct capacity_class *capacity)
{
  return SUCCESS;
}

// compute number of bytes per slot depending on modulation coding schemes
// according to the following formula:
//   bits per symbol x coding rate x 48 data carriers and symbols per slot / 8;
// specify whether it's for downlink or not by setting the value of 'downlink';
// returns number of bytes per slot
double
capacity_bytes_per_slot (struct capacity_class *capacity, int downlink)
{
  double bytes_per_symbol;
  double data_bits_per_symbol;

  // compute the number of bits per symbol for each MCS;
  // formula from [1] Sec. 3, paragraph 4:
  // bytes_per_slot = bits_per_symbol * coding_rate * 48 / 8 bits,
  // where 48 is the number of data subcarriers and symbols per slot
  // values for each MCS from [1] Table 3;
  // we denote "bits_per_symbol * coding_rate" by "data_bits"
  switch (capacity->mcs)
    {

      // QPSK MCS
    case QPSK_18:
      data_bits_per_symbol = 2 * 0.125;
      break;
    case QPSK_14:
      data_bits_per_symbol = 2 * 0.250;
      break;
    case QPSK_12:
      data_bits_per_symbol = 2 * 0.500;
      break;
    case QPSK_34:
      data_bits_per_symbol = 2 * 0.750;
      break;

      // QAM-16 MCS
    case QAM16_12:
      data_bits_per_symbol = 4 * 0.500;
      break;
    case QAM16_23:
      data_bits_per_symbol = 4 * 0.667;
      break;
    case QAM16_34:
      data_bits_per_symbol = 4 * 0.750;
      break;

      // QAM-64 MCS
    case QAM64_12:
      data_bits_per_symbol = 6 * 0.500;
      break;
    case QAM64_23:
      data_bits_per_symbol = 6 * 0.667;
      break;
    case QAM64_34:
      data_bits_per_symbol = 6 * 0.750;
      break;
    case QAM64_56:
      data_bits_per_symbol = 6 * 0.833;
      break;

    default:
      WARNING ("Modulation scheme '%d' is not supported", capacity->mcs);
      return -1.0;
    }

  // with the default parameters, both for DL and for UL the number of
  // subcarriers x symbols per slot used for data is 48, but we write it
  // below with the corresponding formulas in case values change
  if (downlink == TRUE)
    bytes_per_symbol = data_bits_per_symbol
      * (DL_SUBCARRIERS_PER_SLOT * DL_SYMBOLS_PER_SLOT -
	 DL_PILOTS_PER_SLOT) / 8;
  else
    bytes_per_symbol = data_bits_per_symbol
      * (UL_SUBCARRIERS_PER_SLOT * UL_SYMBOLS_PER_SLOT -
	 UL_PILOTS_PER_SLOT) / 8;

  // for MCS=QPSK_18, the result 1.5 is expected, otherwise round the result
  // to closest integer to compensate for numerical errors
  if (capacity->mcs != QPSK_18)
    bytes_per_symbol = round (bytes_per_symbol);

  return bytes_per_symbol;
}
