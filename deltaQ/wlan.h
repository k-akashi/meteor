
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
 * File name: wlan.h
 * Function:  Header file of wlan.c
 *
 * Author: Razvan Beuran
 *
 * $Id: wlan.h 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/


#ifndef __WLAN_H
#define __WLAN_H

#include "deltaQ.h"


/////////////////////////////////////////////
// Fundamental defines
/////////////////////////////////////////////

// use this in order to enable the use of model 1 modified by the 
// addition of the fact that noise (background noise and interference noise)
// is taken into account; model 2 will be ignored in this case;
// this constant is defined by default since v1.2, and this behaviour
// will be exclusive starting with v1.3
#define MODEL1_W_NOISE


/////////////////////////////////////////////
// Generic 802.11 constants
/////////////////////////////////////////////

// 802.11b/g operate in the frequency band of 2.4 to 2.4897 GHz
// (we use here the average frequency)
#define FREQUENCY_BG                    2.4448e9

// 802.11a operates in the frequency band of 5.15 to 5.85 GHz (with gaps)
// we currently only model the band 5.15 to 5.25 GHz, available in Japan
// (we use here the average frequency)
#define FREQUENCY_A                     5.2e9

// length of "standard" PSDU frames (used for FER calculation)
// PSDU = PLCP (Physical Layer Convergence Protocol) SDU (Service Data Unit)
// Note: PSDU is the same with MPDU (MAC Protocol Data Unit)
#define PSDU_DSSS                       1024
#define PSDU_OFDM                       1000

// subcarrier spacing for 802.11a/g in kHz;
// given by 1/symbol_time = 1/3.2 us [Ref: OFDM at morse.colorado.edu]
#define WIFI_SUBCARRIER_SPACING         312.5

// maximum number of transmissions of a frame before it is 
// considered lost (without and with RTS/CTS mechanism, respectively)
#define MAX_TRANSMISSIONS               7
#define MAX_TRANSMISSIONS_RTS_CTS       4

// maximum size of a PSDU (for b and g in compatibility mode)
#define MAX_PSDU_SIZE_B                 2347

// maximum size of a PSDU (for a and g not in compatibility mode)
#define MAX_PSDU_SIZE_AG                4095

// minimum and maximum value of a channel identifier
#define MIN_CHANNEL                     0
#define MAX_CHANNEL                     200

// duration of a slot in 802.11b [us]
#define SLOT_802_11B                    20

// duration of a slot in 802.11g [us]
// short slots are used in 802.11g-only environments
#define SLOT_802_11G_LONG               20
#define SLOT_802_11G_SHORT              9

// duration of a slot in 802.11a [us]
#define SLOT_802_11A                    9

// SIFS duration in us for 802.11b
#define SIFS_802_11B                    10

// SIFS duration in us for 802.11g
#define SIFS_802_11G                    10

// SIFS duration in us for 802.11a
#define SIFS_802_11A                    16

// this is not used in current version
//#define INTERFERENCE_RANGE            100

// use this to enable short preambles for rates 
// 2, 5.5 and 11 Mb/s of 802.11b and 802.11g
#define USE_SHORT_PREAMBLE

// long 802.11b/g PHY header length in us (long preamble)
#define PHY_OVERHEAD_802_11BG_LONG      192

// short 802.11 b/g PHY header length in us (short preamble)
#define PHY_OVERHEAD_802_11BG_SHORT     96

// thermal noise level associated to Pr-threshold 
// determining technique; for the 22 MHz frequency band 
// of 802.11 b/g it is equal to -100 dB (official band is 20 MHz, but
// the standard specifies attenuation at 11 MHz on left and right of center frequency!
// Formula: P_dBm = -174 + 10*log10(delta_f) [Wikipedia: Johnson-Nyquist noise]
// with delta_f expressed in Hertz
// Note: 802.11a uses the same bandwidth => same noise level
#define STANDARD_NOISE                  -100.0

////////////////////////////////////////////////
// 802.11b model parameters
////////////////////////////////////////////////

// constants associated with each operating rate
#define B_RATE_1MBPS                    0
#define B_RATE_2MBPS                    1
#define B_RATE_5MBPS                    2
#define B_RATE_11MBPS                   3

// number of operating rates of 802.11b
#define B_RATES_NUMBER                  4

// data structure to hold 802.11b parameters
// Note: later all structures may be unified
struct parameters_802_11b
{
  // name of the adapter
  char name[MAX_STRING];

  // constants of model 1 (Pr-threshold based model)
  int use_model1;		// set to TRUE if model 1 is used, FALSE otherwise
  float Pr_thresholds[B_RATES_NUMBER];	// receive sensitivity thresholds
  float Pr_threshold_fer;	// FER at the Pr-threshold (valid for DSSS 
  // encoding both in 802.11b and g)
  float model1_alpha;		// constant of exponential dependency

  // constants of model 2 (SNR-based model)
  int use_model2;		// set to TRUE if model 2 is used, FALSE otherwise
  float model2_a[B_RATES_NUMBER];	// parameter 'a' of the exponential fitting
  float model2_b[B_RATES_NUMBER];	// parameter 'b' of the exponential fitting

};


////////////////////////////////////////////////
// 802.11g model parameters
////////////////////////////////////////////////

// constants associated with each operating rate
#define G_RATE_1MBPS                    0
#define G_RATE_2MBPS                    1
#define G_RATE_5MBPS                    2
#define G_RATE_6MBPS                    3
#define G_RATE_9MBPS                    4
#define G_RATE_11MBPS                   5
#define G_RATE_12MBPS                   6
#define G_RATE_18MBPS                   7
#define G_RATE_24MBPS                   8
#define G_RATE_36MBPS                   9
#define G_RATE_48MBPS                   10
#define G_RATE_54MBPS                   11

// number of operating rates of 802.11g
#define G_RATES_NUMBER                  12

// data structure to hold 802.11g parameters
struct parameters_802_11g
{
  // name of the adapter
  char name[MAX_STRING];

  // constants of model 1 (Pr-threshold based model)
  int use_model1;		// set to TRUE if model 1 is used, FALSE otherwise
  float Pr_thresholds[G_RATES_NUMBER];	// receive sensitivity thresholds
  float Pr_threshold_fer;	// FER at the Pr-threshold (valid for DSSS 
  // encoding both in 802.11b and g)
  float Pr_threshold_per;	// PER at the Pr-threshold (valid for OFDM
  // encoding); note that PER is only a different 
  // notation for FER
  float model1_alpha;		// constant of exponential dependency

};


////////////////////////////////////////////////
// 802.11a model parameters
////////////////////////////////////////////////

// constants associated with each operating rate
#define A_RATE_6MBPS                    0
#define A_RATE_9MBPS                    1
#define A_RATE_12MBPS                   2
#define A_RATE_18MBPS                   3
#define A_RATE_24MBPS                   4
#define A_RATE_36MBPS                   5
#define A_RATE_48MBPS                   6
#define A_RATE_54MBPS                   7

// number of operating rates of 802.11a
#define A_RATES_NUMBER                  8

// data structure to hold 802.11a parameters
struct parameters_802_11a
{
  // name of the adapter
  char name[MAX_STRING];

  // constants of model 1 (Pr-threshold based model)
  int use_model1;		// set to TRUE if model 1 is used, FALSE otherwise
  float Pr_thresholds[A_RATES_NUMBER];	// receive sensitivity thresholds
  float Pr_threshold_per;	// PER at the Pr-threshold (valid for OFDM
  // encoding)
  float model1_alpha;		// constant of exponential dependency

};


////////////////////////////////////////////////
// ARF model parameters
////////////////////////////////////////////////

#define ARF_FER_DOWN_THRESHOLD          0.5
//#define ARF_FER_DOWN_THRESHOLD          0.3
#define ARF_FER_UP_THRESHOLD            0.5
#define ARF_FER_KEEP_THRESHOLD          0.5


/////////////////////////////////////////////
// Arrays of constants for convenient use
/////////////////////////////////////////////

extern double b_operating_rates[];
extern double g_operating_rates[];
extern double a_operating_rates[];


/////////////////////////////////////////////
// Default values
/////////////////////////////////////////////

// default values for some connection parameters
#define DEFAULT_PARAMETER               "UNKNOWN"
#define DEFAULT_PACKET_SIZE             1024
#define DEFAULT_STANDARD                WLAN_802_11B
#define DEFAULT_CHANNEL_A               36
#define DEFAULT_CHANNEL_BG              1
#define DEFAULT_ADAPTER_TYPE            CISCO_ABG
#define DEFAULT_RTS_CTS_THRESHOLD       MAX_PSDU_SIZE_B

// default values for some environment parameters
// (also used when no environment information can be found
// in environment_update)
#define DEFAULT_ALPHA                   2.0
#define DEFAULT_SIGMA                   0.0
#define DEFAULT_W                       0.0
#define DEFAULT_NOISE_POWER             -100.0

// defines for supported WLAN adapters
#define ORINOCO                         0
#define DEI80211MR                      1
#define CISCO_340                       2
#define CISCO_ABG                       3


////////////////////////////////////////////////
// Special constants
////////////////////////////////////////////////

// various constants
#define MINIMUM_NOISE_POWER             -100.0
//define POWER_EPSILON                   1e-6        // not used anymore
#define MAXIMUM_ERROR_RATE              0.999999999

// maximum number of iterations when trying to determine 
// the steady state of a connection during init phase
#define MAXIMUM_PRECOMPUTE              13


///////////////////////////////////////
// deltaQ parameter computation
///////////////////////////////////////

// print parameters of an 802.11b model
void print_802_11b_parameters (struct parameters_802_11b *params);

// print parameters of an 802.11g model
void print_802_11g_parameters (struct parameters_802_11g *params);

// print parameters of an 802.11a model
void print_802_11a_parameters (struct parameters_802_11a *params);

// do various post-init configurations;
// return SUCCESS on succes, ERROR on error
//int wlan_connection_post_init(struct connection_class *connection);

// update Pr0, which is the power radiated by _this_ interface
// at the distance of 1 m
void wlan_interface_update_Pr0 (struct interface_class *interface);

// update the changing properties of a connection (e.g., relative distance, Pr)
// after the state of the system changed;
// return SUCCESS on succes, ERROR on error
int wlan_connection_update (struct connection_class *connection,
			    struct scenario_class *scenario);

// get a pointer to a parameter structure corresponding to 
// the receiving interface adapter and the connection standard;
// return value is this pointer
void *wlan_get_interface_adapter (struct connection_class *connection,
				  struct interface_class *interface);

// do compute channel interference between the current connection
// and a potentially interfering connection 'connection_i';
// return SUCCESS on succes, ERROR on error
int compute_channel_interference (struct connection_class *connection,
				  struct connection_class *connection_i,
				  struct scenario_class *scenario);

// compute the interference caused by other stations
// through either concurrent transmission or through noise;
// return SUCCESS on succes, ERROR on error
int wlan_interference (struct connection_class *connection,
		       struct scenario_class *scenario);

// compute FER corresponding to the current conditions
// for a given operating rate;
// return SUCCESS on succes, ERROR on error
int wlan_fer (struct connection_class *connection,
	      struct scenario_class *scenario, int operating_rate,
	      double *fer);

// compute the average number of retransmissions corresponding 
// to the current conditions for ALL frames (including errored ones);
// return SUCCESS on succes, ERROR on error
int wlan_retransmissions (struct connection_class *connection,
			  struct scenario_class *scenario,
			  double *num_retransmissions);

// compute loss rate based on FER;
// return SUCCESS on succes, ERROR on error
int wlan_loss_rate (struct connection_class *connection,
		    struct scenario_class *scenario, double *loss_rate);

// compute loss rate based on FER (we assume FER was 
// already calculated);
// return SUCCESS on succes, ERROR on error
int
wlan_do_compute_loss_rate (struct connection_class *connection,
			   double *loss_rate);

// compute operating rate based on FER and a model of the ARF mechanism;
// return SUCCESS on succes, ERROR on error
int wlan_operating_rate (struct connection_class *connection,
			 struct scenario_class *scenario,
			 int *operating_rate);

// calculate PPDU duration in us for 802.11 WLAN specified by 'connection';
// the results will be rounded up using ceil function;
// return SUCCESS on succes, ERROR on error
int wlan_ppdu_duration (struct connection_class *connection,
			double *ppdu_duration, int *slot_time);

// compute delay & jitter and return values in arguments
// Note: To obtain correct results the call to this function 
// must follow the call to "wlan_loss_rate()" since it 
// uses the field "frame_error_rate" internally;
// return SUCCESS on succes, ERROR on error
int wlan_delay_jitter (struct connection_class *connection,
		       struct scenario_class *scenario,
		       double *variable_delay, double *delay, double *jitter);

// compute variable delay & jitter and return values in arguments
// Note: To obtain correct results, the call to this function 
// must follow the call to "wlan_loss_rate()" since it 
// uses the field "frame_error_rate" internally;
// return SUCCESS on succes, ERROR on error
int
wlan_do_compute_delay_jitter (struct connection_class *connection,
			      double *avg_delay, double *avg_jitter,
			      float total_channel_utilization_others);

// compute bandwidth based on operating rate, delay and packet size;
// return SUCCESS on succes, ERROR on error
int wlan_bandwidth (struct connection_class *connection,
		    struct scenario_class *scenario, double *bandwidth);

// do compute bandwidth based on operating rate, delay and packet size;
// return SUCCESS on succes, ERROR on error
int wlan_do_compute_bandwidth (struct connection_class *connection,
			       double *bandwidth);

// used in wireconf/statistics.c
int wlan_operating_rate_index (struct connection_class *connection,
			       float op_rate);

#endif
