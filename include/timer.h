
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
 * File name: timer.h
 * Function: Header file of the timer library
 *
 * Authors: Junya Nakata, Razvan Beuran
 *
 *   $Revision: 143 $
 *   $LastChangedDate: 2009-03-31 11:30:13 +0900 (Tue, 31 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#include <sys/types.h>
#include <sys/time.h>
#include <sys/sysctl.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __linux
#include <string.h>
#endif

#ifndef __TIMER_H
#define __TIMER_H

// define type of clock for timing operations
//#define TIMER_TYPE CLOCK_REALTIME
#define TIMER_TYPE CLOCK_MONOTONIC


///////////////////////////////////
// Structures of the timer library
///////////////////////////////////

// structure for the timer handle
struct timer_handle
{
    uint32_t cpu_frequency;
    // the relative "zero" of the timer
    struct timespec zero_tp;

    // the logical time equivalent to "zero"
    double zero_time;

    uint64_t zero;
    uint64_t next_event;
};


/////////////////////////////////////////////
// Functions implemented by the timer library
/////////////////////////////////////////////

// compute the time for the next event 
struct timespec compute_next_time (struct timer_handle *handle,
				   double seconds);

// print debug info for a timespec structure
void DEBUG_print_timespec (struct timespec *time);

// reset the timer (set its relative "zero" and the logical time
// associated to it
void timer_reset (struct timer_handle *handle, double zero_time);

// wait for a time to occur (specified in seconds)
void timer_wait (struct timer_handle *handle, float time_in_s);

// return the elapsed time since timer was last reset
// NOTE: the function used internally, clock_gettime, seems to be 
// very expensive, and may take a long time, hence this function 
// should not be called too often so as not to interfere with timing
double timer_elapsed_time (struct timer_handle *handle);

#endif
