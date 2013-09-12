
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
 * File name: timer.c
 * Function: Main file of the timer library
 *
 * Authors: Junya Nakata, Razvan Beuran
 *
 *   $Revision: 128 $
 *   $LastChangedDate: 2009-02-06 10:21:50 +0900 (Fri, 06 Feb 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#include <time.h>
#include <math.h>

#include "timer_global.h"
#include "timer_message.h"
#include "timer.h"


/////////////////////////////////////////////
// Functions implemented by the timer library
/////////////////////////////////////////////

// compute the time for the next event 
struct timespec
compute_next_time (struct timer_handle *handle, double seconds)
{
  struct timespec result;
  double tv_sec, tv_nsec;

  // compensate 'seconds' based on the zero time reference;
  // store integral part of the result in tv_sec,
  // and the fractional part in tv_nsec
  tv_nsec = modf (seconds - handle->zero_time, &tv_sec) * 1e9;

  // add the above parts to the sero reference
  result.tv_sec = handle->zero_tp.tv_sec + (time_t) tv_sec;
  result.tv_nsec = handle->zero_tp.tv_nsec + (long) tv_nsec;

  // check for overflow in the nanosecond component
  if (result.tv_nsec >= 1e9) {
      result.tv_sec++;
      result.tv_nsec -= 1e9;
  }

  return result;
}

// print debug info for a timespec structure
void
DEBUG_print_timespec (struct timespec *time)
{
  DEBUG ("timespec: %ld s + %ld ns", time->tv_sec, time->tv_nsec);
}

// reset the timer (set its relative "zero" and the logical time
// associated to it
void
timer_reset(struct timer_handle *handle, double zero_time)
{
  DEBUG ("Resetting timer...");

  // use current time as the "zero" reference
  clock_gettime(TIMER_TYPE, &(handle->zero_tp));
  DEBUG_print_timespec(&(handle->zero_tp));

  // set the logical "zero" time
  handle->zero_time = zero_time;
}

// wait for a time to occur (specified in seconds)
void
timer_wait(struct timer_handle *handle, float time_in_s)
{
  struct timespec next_tp;

  printf("[time_wait] rec_time : %6f\n", time_in_s);
  next_tp = compute_next_time(handle, time_in_s);
  DEBUG_print_timespec(&next_tp);

  DEBUG ("Waiting for timer...");

  // wait for next event
  clock_nanosleep(TIMER_TYPE, TIMER_ABSTIME, &next_tp, NULL);
}

// convert a "struct timespec" time value to seconds
static __inline double
timespec2sec (struct timespec *time_spec)
{
  return ((double) time_spec->tv_sec + (double) time_spec->tv_nsec / 1e9);
}

// compute the difference in seconds between two "struct timespec" time values
static __inline double
timespec_diff2sec (struct timespec *time_spec1, struct timespec *time_spec2)
{
  /*
     return ((float)(time_spec1->tv_sec - time_spec2->tv_sec) + 
     (float)(time_spec1->tv_nsec - time_spec2->tv_nsec) / 1e9);
   */

  // the algorithm below is inspired from: 
  // http://www.delorie.com/gnu/docs/glibc/libc_428.html
  // but it is not yet clear if it is better than the above simple calculation
  int sec_no;
  struct timespec result;

  // carry for the later subtraction by updating time_spec2
  if (time_spec1->tv_nsec < time_spec2->tv_nsec)
    {
      sec_no = (time_spec2->tv_nsec - time_spec1->tv_nsec) / 1e9 + 1;
      time_spec2->tv_nsec -= (1e9 * sec_no);
      time_spec2->tv_sec += sec_no;
    }

  if (time_spec1->tv_nsec - time_spec2->tv_nsec > 1e9)
    {
      sec_no = (time_spec1->tv_nsec - time_spec2->tv_nsec) / 1e9;
      time_spec2->tv_nsec += (1e9 * sec_no);
      time_spec2->tv_sec -= sec_no;
    }

  // compute the time difference; tv_nsec is certainly positive
  result.tv_sec = time_spec1->tv_sec - time_spec2->tv_sec;
  result.tv_nsec = time_spec1->tv_nsec - time_spec2->tv_nsec;

  return timespec2sec (&result);
}

// return the elapsed time since timer was last reset
// NOTE: the function used internally, clock_gettime, seems to be 
// very expensive, and may take a long time, hence this function 
// should not be called too often so as not to interfere with timing
double
timer_elapsed_time (struct timer_handle *handle)
{
  struct timespec crt_tp;

  // get current time
  clock_gettime (TIMER_TYPE, &crt_tp);
  DEBUG_print_timespec (&crt_tp);

  return timespec_diff2sec (&crt_tp, &(handle->zero_tp));
}
