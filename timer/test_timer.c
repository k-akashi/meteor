
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
 * File name: test_timer.c
 * Function: Tests the timer library implementation
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 143 $
 *   $LastChangedDate: 2009-03-31 11:30:13 +0900 (Tue, 31 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#include <stdio.h>
#include <time.h>
#include <math.h>

#include "timer_global.h"
#include "timer_message.h"
#include "timer.h"

// main function of the program
int
main ()
{
  // timer handle
  struct timer_handle timer;

  // current loop counter
  int timer_i = 0;

  // total loop counter
  int timer_total_count = 101;

  // timer event step in seconds
  double timer_step = 0.5;

  // time of next timer event in seconds
  double next_time = -10.0;

  // start and end of execution
  struct timespec start_tp, end_tp;

  // expected duration in s
  double expected_duration;

  // actual duration in s
  double actual_duration;

  // duration error in ms
  double duration_error_ms;

  // threshold for duration error in ms;
  // on newer PCs/OSes (e.g., 3 GHz, kernel 2.6.32) 0.5 ms seems enough,
  // but on older PCs/OSes (e.g., 1.8 GHz, kernel 2.6.18) 5 ms is required 
  double DURATION_ERROR_MS_THRESH = 5.0;

  //////////////////////////////////////////////////////////////////
  // initial operations

  // compute the expected test duration
  expected_duration = timer_step * (timer_total_count - 1);

  // print settings
  INFO ("Testing timer library: expected_duration=%.4f s \
(%d x %.2f s steps); starting time=%.2f s...", timer_step * (timer_total_count - 1), timer_total_count - 1, timer_step, next_time);

  // get starting time
  clock_gettime (TIMER_TYPE, &start_tp);


  //////////////////////////////////////////////////////////////////
  // timer loop
  for (timer_i = 0; timer_i < timer_total_count; timer_i++)
    {
      // print current state
      DEBUG ("Current state: timer_i=%d next_time=%.4f", timer_i, next_time);

      // if this is the first operation, we only reset the timer,
      // and proceed without waiting; otherwise wait for the next timer event
      if (timer_i == 0)
	// reset timer
	timer_reset (&timer, next_time);
      else
	{
	  // wait for for the next timer event
	  DEBUG ("Waiting for next timer event at %.4f s...", next_time);

	  timer_wait (&timer, next_time);
	}

      // increment the time of next timer event
      next_time += timer_step;
    }


  //////////////////////////////////////////////////////////////////
  // final operations

  // get ending time
  clock_gettime (TIMER_TYPE, &end_tp);

  // compute actual test duration
  actual_duration = (end_tp.tv_sec + end_tp.tv_nsec / 1e9) -
    (start_tp.tv_sec + start_tp.tv_nsec / 1e9);

  duration_error_ms = fabs (actual_duration - expected_duration) * 1e3;

  // print actual duration time
  INFO ("Actual test duration=%.4f s (error=%.4f ms)",
	actual_duration, duration_error_ms);

  // test whether the error exceeds the threshold
  if (duration_error_ms > DURATION_ERROR_MS_THRESH)
    {
      WARNING ("Difference between actual and expected durations (%.4f ms) \
exceeds threshold (%.4f ms)", duration_error_ms, DURATION_ERROR_MS_THRESH);
      return ERROR;
    }
  else
    return SUCCESS;
}
