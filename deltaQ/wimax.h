
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
 * File name: wimax.h
 * Function: Header file of wimax.c
 *
 * Author: Razvan Beuran, Muhammad Imran Tariq
 *
 * $Id: wimax.h 66 2012-03-28 01:44:55Z razvan $
 *
 ***********************************************************************/


#ifndef __WIMAX_H
#define __WIMAX_H


/////////////////////////////////////////////
// Fundamental defines
/////////////////////////////////////////////

// WiMAX operating frequency (capacity paper, page 10)
#define FREQUENCY_WIMAX                  2.35e9
#define WIMAX_FRAME_DURATION             5
// defines for supported WiMAX adapters
#define NS3_WIMAX                        0

// number of "operating" rates of WiMAX ([1] Table 3) and their index
// QPSK:   1/8 1/4 1/2 3/4
// QAM-16: 1/2 2/3 3/4
// QAM-64: 1/2 2/3 3/4 5/6
#define WIMAX_RATES_NUMBER               11
#define QPSK_1_8                         0
#define QPSK_1_4                         1
#define QPSK_1_2                         2
#define QPSK_3_4                         3
#define QAM_16_1_2                       4
#define QAM_16_2_3                       5
#define QAM_16_3_4                       6
#define QAM_64_1_2                       7
#define QAM_64_2_3                       8
#define QAM_64_3_4                       9
#define QAM_64_5_6                       10

// number of system bandwidth values ([1] Table 1) and their values
#define WIMAX_SYS_BW_NUMBER              7
#define SYS_BW_1                         1.25
#define SYS_BW_3                         3.5
#define SYS_BW_5                         5.0
#define SYS_BW_7                         7.0
#define SYS_BW_8                         8.75
#define SYS_BW_10                        10.0
#define SYS_BW_20                        20.0

// thermal noise level associated to Pr-threshold 
// determining technique; for 10 MHz frequency band 
// of WiMAX it is equal to -104 dBm
// Formula: P_dBm = -174 + 10*log10(delta_f) [Wikipedia: Johnson-Nyquist noise]
// with delta_f expressed in Hertz
// FIXME: how to adjust for other system bandwidth values?
#define WIMAX_MINIMUM_NOISE_POWER        -104.0
#define WIMAX_STANDARD_NOISE             -104.0

#define WIMAX_FRAME_SIZE                 1024
#define MAXIMUM_ERROR_RATE               0.999999999	//FIXME: redefined in several headers

// data structure to hold 802.16 parameters
struct parameters_802_16
{
  // name of the adapter
  char name[MAX_STRING];

  float Pr_thresholds[WIMAX_RATES_NUMBER];	// receive sensitivity thresholds
  float Pr_threshold_ber;	// BER at the Pr-threshold
};

// array holding the operating rates associated
// to the different WiMAX operating speeds
extern double wimax_operating_rates[];


////////////////////////////////////////////
// Capacity calculation defines 
////////////////////////////////////////////

// 28/25 sampling is recommended for 1.25, 5, 10, 20 MHz bandwidth
// 8/7 sampling is recommended for 3.5, 7, 8.75 MHz bandwidth
// values from [1] Table 1
#define SAMPLING_FACTOR_28_25            (28.0/25.0)
#define SAMPLING_FACTOR_8_7              (8.0/7.0)

// frame duration in ns;
// 5 millisecond frame is recommended by WiMAX forum
// (together with 10 MHz system bandwidth: [1] Sec. 3, paragraph 5)
// other values (in ms) are: 2, 2.5, 4, 8, 10, 12.5, 20 ([4] Table 2)
#define FRAME_DURATION                   5000

// by default we use values in [1] for various parameters such as 
// the number of uplink and downlink symbols, or the DL overhead;
// comment line below in order to enable alternative values from 
// [2] use for result validation purposes
//#define USE_PAPER_VALUES

#ifdef USE_PAPER_VALUES
//#define DL_MAP_REPETITION_FACTOR         0
// in [1] no additional overhead is initially considered at PHY level;
// a detailed analysis (DL_MAP etc.) is then given in [1] 5.2.2 and 5.2.3
#define DL_ADDITIONAL_OVERHEAD_12        0
#define DL_ADDITIONAL_OVERHEAD_16        0
#else
//#define DL_MAP_REPETITION_FACTOR        6
// in [2] the DL overhead depends on system bandwidth; the values below
// do NOT include the DL preamble overhead which is dealt with separately
#define DL_ADDITIONAL_OVERHEAD_12       12
#define DL_ADDITIONAL_OVERHEAD_16       16
#endif

// DL preamble overhead that needs to be considered in all cases;
// (e.g., DL slots = 29 in total, and = 28 after subtracting preamble:
// [1] Sec. 3, paragraph 2 and Table 4)
#define DL_PREAMBLE_OVERHEAD            1

// number of uplink control symbols (columns) reserved for 
// CQI, ACK, Ranging etc. ([1] Table 4)
#define UL_CONTROL_SYMBOLS              3

// ratio of cyclic prefix (CP) time to useful time ([2] Fig. 2)
// other values are 1/4, 1/16, 1/32 ([4], page 203, paragraph 3)
#define CYCLIC_PREFIX_RATIO_G           (1.0/8.0)

// number of DC carriers sent without power
// FIXME: reference?
#define DC_SUBCARRIER                   1

// values from [1] Fig. 3 and 2
#define DL_SUBCARRIERS_PER_SLOT         28
#define UL_SUBCARRIERS_PER_SLOT         24
#define DL_SYMBOLS_PER_SLOT             2
#define UL_SYMBOLS_PER_SLOT             3
#define DL_PILOTS_PER_SLOT              8
#define UL_PILOTS_PER_SLOT              24

// repetition factor for data (different from that for DL_MAP?!);
// NOTE: the value 1 means no repetition, not 0 which is only its "code"
// FIXME: assume 1 for now, but values 2, 4, 6 are also possible
#define REPETITION_FACTOR               1

// type of MIMO
#define MIMO_TYPE_SISO                  0	// not real MIMO but SISO
#define MIMO_TYPE_MATRIX_A              1	// matrix A MIMO (robustness gain)
#define MIMO_TYPE_MATRIX_B              2	// matrix B MIMO (throughput gain)

//#define TX_ANTENNA_COUNT                2
//#define RX_ANTENNA_COUNT                2


// capacity parameters structure
struct capacity_class
{
  // parameters in Table 1 from capacity paper
  double system_bandwidth;
  double sampling_factor;
  double sampling_frequency;
  double sample_time;
  int fft_size;			// size of FFT
  double subcarrier_spacing;
  double data_symbol_time;
  double guard_time;
  double OFDMA_symbol_time;	// = data_symbol_time + guard_time

  // parameters in Table 2 from capacity paper
  int dl_guard_subcarriers;
  int dl_used_subcarriers;
  int dl_pilot_subcarriers;
  int dl_data_subcarriers;
  int ul_guard_subcarriers;
  int ul_used_subcarriers;
  int ul_pilot_subcarriers;
  int ul_data_subcarriers;

  // other parameters
  int mcs;			// modulation coding scheme

  // total number of symbols in one frame 
  // = number of DL symbols + UL symbols + (TTG + RTG) symbols 
  // = number of OFDMA symbols x frame duration
  int total_symbols;

  // duration of TTG and RTG in symbols (depends on frame duration?!)

  // See below for indications for computation based on Agilent
  // application note "Mobile WiMAX PHY Layer (RF) Operation and
  // Measurement" together with Table 9 in "WiMAX Forum Mobile System
  // Profile Release 1.0 Approved Specification"

  // System bandwidth [Mhz]  TTG [us]  RTG [us]  Symbol [us] ttg_rtg [symbols]
  // 1.25, 5, 10, 20         105.71    60        102.9       1.61
  // 3.5, 7                  188       60        144         1.72
  // 8                       87.2      74.4      115.2       1.40
  // ttg_rtg_duration = (TTG+RTG)/symbol_duration
  // 
  double ttg_rtg_duration;

  int dl_symbols;		// number of DL symbols
  int dl_slots;
  double dl_bytes_per_slot;	// bytes per slot
  double dl_data_rate;

  int ul_symbols;		// number of UL symbols
  int ul_slots;
  double ul_bytes_per_slot;	// bytes per slot
  double ul_data_rate;

  // signaling overhead
  int dl_signaling_overhead;
  int ul_signaling_overhead;

  // type of MIMO
  int mimo_type;
  // number of Tx and Rx antennas
  int Nt;
  int Nr;

  double thermal_noise_power;
};

extern struct parameters_802_16 ns3_wimax;


/////////////////////////////////////////////
// Default values
/////////////////////////////////////////////

// default values for some connection parameters
//#define DEFAULT_PARAMETER               "UNKNOWN"
//#define DEFAULT_PACKET_SIZE             1024
//#define DEFAULT_STANDARD                WLAN_802_11B
#define DEFAULT_CHANNEL_WIMAX             1
//#define DEFAULT_ADAPTER_TYPE            CISCO_ABG
//#define DEFAULT_RTS_CTS_THRESHOLD       MAX_PSDU_SIZE_B

// default values for some environment parameters
// (also used when no environment information can be found
// in environment_update)
#define DEFAULT_ALPHA                   2.0
#define DEFAULT_SIGMA                   0.0
#define DEFAULT_W                       0.0
//#define DEFAULT_NOISE_POWER             -100.0

// assigned same value with PSDU_OFDM in Wi-Fi
#define PSDU_OFDMA                      1000

////////////////////////////////////////////////
// WiMAX related functions
////////////////////////////////////////////////

// update Pr0, which is the power radiated by _this_ interface
// at the distance of 1 m
void wimax_interface_update_Pr0 (struct interface_class *interface);

// init the Ns-3 WiMAX adapter structure;
// return SUCCESS on succes, ERROR on error
int wimax_init_ns3_adapter (struct parameters_802_16 *ns3_wimax);

// print the content of a WiMAX adapter structure
void wimax_print_adapter (struct parameters_802_16 *adapter);

// compute the minimum receiver sensitivity threshold according to Eq. 149b
// in IEEE 802.16e-2005 standard (page 646) (denoted as reference [s]);
// return the threshold
double wimax_min_threshold (struct capacity_class *capacity);

// compute FER corresponding to the current conditions
// for a given operating rate
// return SUCCESS on succes, ERROR on error
int wimax_fer (struct connection_class *connection,
	       struct scenario_class *scenario, int operating_rate,
	       double *fer);

// compute loss rate based on FER
// return SUCCESS on succes, ERROR on error
int wimax_loss_rate (struct connection_class *connection,
		     struct scenario_class *scenario, double *loss_rate);

// compute operating rate based on FER and a model of rate
// auto-selection mechanism
// return SUCCESS on succes, ERROR on error
int wimax_operating_rate (struct connection_class *connection,
			  struct scenario_class *scenario,
			  int *operating_rate);

// compute delay & jitter and return values in arguments
// return SUCCESS on succes, ERROR on error
int wimax_delay_jitter (struct connection_class *connection,
			struct scenario_class *scenario,
			double *variable_delay, double *delay,
			double *jitter);

// compute bandwidth based on operating rate, delay and packet size
// return SUCCESS on succes, ERROR on error
int wimax_bandwidth (struct connection_class *connection,
		     struct scenario_class *scenario, double *bandwidth);

// update the changing properties of a connection (e.g., relative distance, Pr)
// after the state of the system changed;
// return SUCCESS on succes, ERROR on error
int wimax_connection_update (struct connection_class *connection,
			     struct scenario_class *scenario);


////////////////////////////////////////////////
// Capacity calculation functions
////////////////////////////////////////////////

// update the entire capacity calculation structure, 
// including parameters that depend on system bandwidth;
// return SUCCESS on success, ERROR on error
int capacity_update_all (struct capacity_class *capacity,
			 double system_bandwidth, int mcs, int mimo_type,
			 int Nt, int Nr);

// update the MIMO-dependent parameters capacity calculation structure,
// which effectively recomputes the DL and UL data rates;
// return SUCCESS on success, ERROR on error
int capacity_update_mimo (struct capacity_class *capacity, int mimo_type);

// update the MCS-dependent parameters capacity calculation structure,
// which effectively recomputes the DL and UL data rates;
// return SUCCESS on success, ERROR on error
int capacity_update_mcs (struct capacity_class *capacity, int mcs);

// print capacity calculation structure
void capacity_print (struct capacity_class *capacity);

// initialize subcarrier-based information;
// return SUCCESS on success, ERROR on error
int capacity_subcarriers (struct capacity_class *capacity);

// compute number of bytes per slot depending on modulation coding schemes
// according to the following formula:
//   bits per symbol x coding rate x 48 data carriers and symbols per slot / 8;
// specify whether it's for downlink or not by setting the value of 'downlink';
// returns number of bytes per slot
double capacity_bytes_per_slot (struct capacity_class *capacity,
				int downlink);

#endif
