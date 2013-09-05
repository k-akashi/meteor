
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
 * File name: fixed_deltaQ.h
 * Function:  Header file of fixed_deltaQ.c
 *
 * Author: Razvan Beuran
 *
 * $Id: interface.h 29 2011-10-11 06:41:50Z razvan $
 *
 ***********************************************************************/


#ifndef __FIXED_DELTAQ_H
#define __FIXED_DELTAQ_H


////////////////////////////////////////////////
// fixed_deltaQ constants
////////////////////////////////////////////////

#define MAX_FIXED_DELTAQ                  10


////////////////////////////////////////////////
// fixed_deltaQ structure definition
////////////////////////////////////////////////

struct fixed_deltaQ_class
{
  // start and end time between which the fixed deltaQ conditions apply
  double start_time;
  double end_time;

  // the deltaQ conditions within the above interval
  double bandwidth;
  double loss_rate;
  double delay;
  double jitter;
};


/////////////////////////////////////////
// fixed_deltaQ structure functions
/////////////////////////////////////////

// init a fixed_deltaQ structure
void fixed_deltaQ_init (struct fixed_deltaQ_class *fixed_deltaQ,
			double start_time, double end_time,
			double bandwidth, double loss_rate,
			double delay, double jitter);

// copy the information in fixed_deltaQ_src to fixed_deltaQ_dst
void fixed_deltaQ_copy (struct fixed_deltaQ_class *fixed_deltaQ_dst,
			struct fixed_deltaQ_class *fixed_deltaQ_src);

// print the fields of a fixed_deltaQ structure
void fixed_deltaQ_print (struct fixed_deltaQ_class *fixed_deltaQ);

#endif
