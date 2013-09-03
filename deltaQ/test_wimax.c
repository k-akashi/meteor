
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
 * File name: test_wimax.c
 * Function: Test file for wimax.c
 *
 * Author: Razvan Beuran, Muhammad Imran Tariq
 *
 * $Id: test_wimax.c 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/

#include "deltaQ.h"
#include "message.h"
#include "wimax.h"

// number of bandwidth values for which we compute the results
#define MAX_CALCULATION_BW   7
// number of MCS values for which we compute the results
#define MAX_CALCULATION_MCS  8

int
main (void)
{
  // capacity variable
  struct capacity_class capacity;

  // system bandwidth values and selection array
  double system_bandwidth_array[WIMAX_SYS_BW_NUMBER]
    =
    { SYS_BW_1, SYS_BW_3, SYS_BW_5, SYS_BW_7, SYS_BW_8, SYS_BW_10,
SYS_BW_20 };
  // indexes in above array
  int system_bandwidth_select_array[MAX_CALCULATION_BW] =
    { 6, 5, 4, 3, 2, 1, 0 };
  double system_bandwidth;

  // MCS values and selection array
  int mcs_array[WIMAX_RATES_NUMBER] =
    { QPSK_1_8, QPSK_1_4, QPSK_1_2, QPSK_3_4,
    QAM_16_1_2, QAM_16_2_3, QAM_16_3_4,
    QAM_64_1_2, QAM_64_2_3, QAM_64_3_4,
    QAM_64_5_6
  };
  // indexes in above array
  int mcs_select_array[MAX_CALCULATION_MCS] = { 2, 3, 4, 6, 7, 8, 9, 10 };
  int mcs;

  int i, j;

  // loop over system bandwidth values
  for (i = 0; i < MAX_CALCULATION_BW; i++)
    {
      // select a system bandwidth
      system_bandwidth
	= system_bandwidth_array[system_bandwidth_select_array[i]];

      // update the entire capacity calculation structure;
      // MCS is not yet know, so we use the value "1"
      if (capacity_update_all (&capacity, system_bandwidth, QPSK_1_8,
			       MIMO_TYPE_MATRIX_A, 1, 1) == ERROR)
	{
	  printf ("Error updating system bandwidth and MCS for the \
capacity calculation structure.\n");
	  return ERROR;
	}

      // loop over MCS values 
      for (j = 0; j < MAX_CALCULATION_MCS; j++)
	{
	  // select an MCS
	  mcs = mcs_array[mcs_select_array[j]];

	  // update the MCS-dependent parameters, including data rates
	  if (capacity_update_mcs (&capacity, mcs) == ERROR)
	    {
	      printf ("Error updating MCS for the capacity calculation \
structure.\n");
	      return ERROR;
	    }

	  // print header only once for each system bandwidth
	  if (j == 0)
	    INFO ("* Calculation for system bandwidth = %.2f MHz \
(dl_symbols=%d ul_symbols=%d)", system_bandwidth, capacity.dl_symbols, capacity.ul_symbols);

#define LONG_FORMAT
#ifdef LONG_FORMAT
	  // print full content of capacity structure
	  INFO ("** MCS = %d", capacity.mcs);
	  capacity_print (&capacity);
#else
	  // print data rate summary
	  //INFO ("MCS = %2d: PHY data rate [Mbps]: DL = %5.2f  UL = %5.2f", 
	  //    mcs, capacity.dl_data_rate, capacity.ul_data_rate);

	  INFO ("** MCS = %2d: min R_SS = %f", capacity.mcs,
		wimax_min_threshold (&capacity));
#endif

	}
    }

  if (wimax_init_ns3_adapter (&ns3_wimax) == ERROR)
    {
      WARNING ("Could not initialize the Ns-3 WiMAX adapter.");
      return ERROR;
    }
  wimax_print_adapter (&ns3_wimax);

  return SUCCESS;
}
