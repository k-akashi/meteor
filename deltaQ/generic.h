
/*
 * Copyright (c) 2006-2009 The StarBED Project  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
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
double log2(double x);

// convert a string to a double value;
// return the value on success, -HUGE_VAL on error
double double_value(const char *string);

// convert a string to a long int value;
// return the value on success, LONG_MIN on error
long int long_int_value(const char *string);

// generate a random number in the interval [0,1)
double rand_0_1();

// generate a random double in the interval [min,max);
// return the number
double rand_min_max(double min, double max);

// generate a random number from a normal distribution 
// with mean 'mean' and standard deviation 'std';
// Remark: the function can generate pairs of numbers
//         (y1 and y2), but we only used y1 now
double randn(double mean, double stdev);

// compute the sum of powers expressed in dBm, by 
// converting them first to mW and summating im mW domain;
// sources with noise power less or equal MINIMUM_NOISE_POWER
// are excluded from computation so that we don't get 
// an additive effect for that case (may change in the future)
double add_powers(double power1, double power2, double minimum_noise_power);

// compute attenuation due to directional antenna, and
// that must be substracted from antenna gain; both azimuth
// and elevation parameters are taken into account;
// return the directional attenuation
double antenna_directional_attenuation(node_class *node_tx, 
				       node_class *node_rx);

// compute attenuation in azimuth (horizontal) plane;
// return the azimuth attenuation
double antenna_azimuth_attenuation(node_class *node_tx, node_class *node_rx);

// compute attenuation in elevation (vertical) plane;
// return the elevation attenuation
double antenna_elevation_attenuation(node_class *node_tx, node_class *node_rx);

// based on the Jenkins "one at a time hash"
uint32_t string_hash(char *key, size_t key_len);

#endif
