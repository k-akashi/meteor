
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
 * File name: generic.h
 * Function: Header file of generic.c
 *
 * Author: Razvan Beuran
 *
 * $Id: generic.h 145 2013-06-07 01:11:09Z razvan $
 *
 ***********************************************************************/


#ifndef __GENERIC_H
#define __GENERIC_H

#include <stdint.h>

#include "deltaQ.h"

// QOMET constants
#define SPEED_LIGHT                     2.9979e8
#define ANTENNA_MAX_ATTENUATION         100.0


/////////////////////////////////////////////
// Generic functions
/////////////////////////////////////////////

// log2 doesn't seem to exist on gccsparc-sun-solaris2.9, gcc version 4.1.1;
// on other platforms it requires compilation with '-std=c99' flag (Linux)
double log2 (double x);

// convert a string to a double value;
// return the value on success, -HUGE_VAL on error
double double_value (const char *string);

// convert a string to a long int value;
// return the value on success, LONG_MIN on error
long int long_int_value (const char *string);

// generate a random number in the interval [0,1)
double rand_0_1 ();

// generate a random double in the interval [min,max);
// return the number
double rand_min_max (double min, double max);

// generate a random double in the interval [min,max] (max inclusive);
// return the number
double rand_min_max_inclusive (double min, double max);

// generate a random number from a normal distribution 
// with mean 'mean' and standard deviation 'std';
// Remark: the function can generate pairs of numbers
//         (y1 and y2), but we only used y1 now
double randn (double mean, double stdev);

// compute the sum of powers expressed in dBm, by 
// converting them first to mW and summating im mW domain;
// sources with noise power less or equal MINIMUM_NOISE_POWER
// are excluded from computation so that we don't get 
// an additive effect for that case (may change in the future)
double add_powers (double power1, double power2, double minimum_noise_power);

// compute attenuation due to directional antenna, and
// that must be substracted from antenna gain; both azimuth
// and elevation parameters are taken into account;
// return the directional attenuation
double antenna_directional_attenuation (struct node_class *node_tx,
					struct interface_class *interface_tx,
					struct node_class *node_rx);

// compute attenuation in azimuth (horizontal) plane;
// return the azimuth attenuation
double antenna_azimuth_attenuation (struct node_class *node_tx,
				    struct interface_class *interface_tx,
				    struct node_class *node_rx);

// compute attenuation in elevation (vertical) plane;
// return the elevation attenuation
double antenna_elevation_attenuation (struct node_class *node_tx,
				      struct interface_class *interface_tx,
				      struct node_class *node_rx);

// based on the Jenkins "one at a time hash"
uint32_t string_hash (char *key, size_t key_len);

#endif
