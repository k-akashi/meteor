
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
 * File name: fixed_deltaQ.c
 * Function: Source file related to the fixed_deltaQ scenario element
 *
 * Author: Razvan Beuran
 *
 * $Id: fixed_deltaQ.c 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/


#include <stdio.h>

#include "message.h"
#include "fixed_deltaQ.h"


/////////////////////////////////////////
// fixed_deltaQ structure functions
/////////////////////////////////////////

// init a fixed_deltaQ structure
void
fixed_deltaQ_init (struct fixed_deltaQ_class *fixed_deltaQ,
		   double start_time, double end_time,
		   double bandwidth, double loss_rate,
		   double delay, double jitter)
{
  if (fixed_deltaQ == NULL)
    return;

  fixed_deltaQ->start_time = start_time;
  fixed_deltaQ->end_time = end_time;
  fixed_deltaQ->bandwidth = bandwidth;
  fixed_deltaQ->loss_rate = loss_rate;
  fixed_deltaQ->delay = delay;
  fixed_deltaQ->jitter = jitter;
}

// copy the information in fixed_deltaQ_src to fixed_deltaQ_dst
void
fixed_deltaQ_copy (struct fixed_deltaQ_class *fixed_deltaQ_dst,
		   struct fixed_deltaQ_class *fixed_deltaQ_src)
{
  if (fixed_deltaQ_dst == NULL || fixed_deltaQ_src == NULL)
    return;

  fixed_deltaQ_dst->start_time = fixed_deltaQ_src->start_time;
  fixed_deltaQ_dst->end_time = fixed_deltaQ_src->end_time;
  fixed_deltaQ_dst->bandwidth = fixed_deltaQ_src->bandwidth;
  fixed_deltaQ_dst->loss_rate = fixed_deltaQ_src->loss_rate;
  fixed_deltaQ_dst->delay = fixed_deltaQ_src->delay;
  fixed_deltaQ_dst->jitter = fixed_deltaQ_src->jitter;
}

// print the fields of a fixed_deltaQ structure
void
fixed_deltaQ_print (struct fixed_deltaQ_class *fixed_deltaQ)
{
  if (fixed_deltaQ == NULL)
    printf ("fixed_deltaQ:  NULL\n");
  else
    printf ("fixed_deltaQ: start_time=%.2f end_time=%.2f bandwidth=%.2f \
loss_rate=%.4f delay=%.4f jitter=%.4f\n", fixed_deltaQ->start_time, fixed_deltaQ->end_time, fixed_deltaQ->bandwidth, fixed_deltaQ->loss_rate, fixed_deltaQ->delay, fixed_deltaQ->jitter);
}
