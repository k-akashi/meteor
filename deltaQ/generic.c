
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
 * File name: generic.c
 * Function: Various generic functions used in different modules
 *            of the deltaQ library
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "message.h"
#include "deltaQ.h"
#include "generic.h"


/////////////////////////////////////////////
// Generic functions
/////////////////////////////////////////////

// log2 doesn't seem to exist on gccsparc-sun-solaris2.9, gcc version 4.1.1;
// on other platforms it requires compilation with '-std=c99' flag (Linux)
double log2(double x)
{
  return log10(x)/log10(2);
}

// convert a string to a double value;
// return the value on success, -HUGE_VAL on error
double double_value(const char *string)
{
  double value;
  char *end_pointer;

  if(string==NULL)
    {
      WARNING("Input string argument is NULL");
      return -HUGE_VAL;
    }

  // use 'errno' to catch "out of range" errors
  errno=0; 

  // convert the value
  value=strtod(string, &end_pointer);

  // check whether the input string was not empty,   
  // that the string after parsing is empty (i.e., all input was parsed),
  // and that there was no "out of range" error
  if((string[0]=='\0') || (end_pointer[0]!='\0') || (errno!=0))
    {
      perror("strtod");
      WARNING("Converting string '%s' to _one_ double value failed!", string);
      return -HUGE_VAL;
    }

  return value;
}

// convert a string to a long int value;
// return the value on success, LONG_MIN on error
long int long_int_value(const char *string)
{
  long int value;
  char *end_pointer;
  
  // use 'errno' to catch "out of range" errors
  errno=0; 

  // convert the value
  value=strtol(string, &end_pointer, 10);

  // check whether the input string was not empty,   
  // that the string after parsing is empty (i.e., all input was parsed),
  // and that there was no "out of range" error
  if((string[0]=='\0') || (end_pointer[0]!='\0') || (errno!=0))
    {
      perror("strtol");
      WARNING("Converting value '%s' to long int failed! %d", string, errno);
      return LONG_MIN;
    }

  return value;
}

// generate a random number in the interval [0,1)
double rand_0_1()
{
  return rand()/((double)RAND_MAX+1.0);
}

// generate a random double in the interval [min,max);
// return the number
double rand_min_max(double min, double max)
{
  return (min + (rand()/((double)RAND_MAX+1.0)) * (max-min));
}

// generate a random number from a normal distribution 
// with mean 'mean' and standard deviation 'std';
// Remark: the function can generate pairs of numbers
//         (y1 and y2), but we only used y1 now
// Code based on the polar form of the Box-Muller transformation,
// according to a webpage of Dr. Everett (Skip) F. Carter Jr.
double randn(double mean, double stdev)
{
  float x1, x2, w, y1;//, y2;

  do
    {
      // we could use drand48 instead of rand_0_1...
      // or R250 instead of rand()
      x1 = 2.0 * rand_0_1() - 1.0;
      x2 = 2.0 * rand_0_1() - 1.0;
      w = x1 * x1 + x2 * x2;
    }
  while(w >= 1.0);

  w = sqrt((-2.0 * log(w))/w);
  y1 = x1 * w;

  // this function computes pairs of values, but the 
  // second available value is currently not used
  //y2 = x2 * w;

  return (mean + stdev * y1);
}

/////////////////////  !!!!!!!!!!!!!!!!!!!!!!!!1
//////// CHANGE NAME OF FUNCTION BELOW


// compute the sum of powers expressed in dBm, by 
// converting them first to mW and summating im mW domain;
// sources with noise power less or equal MINIMUM_NOISE_POWER
// are excluded from computation so that we don't get 
// an additive effect for that case (may change in the future)
double add_powers(double power1, double power2, double minimum_noise_power)
{
  // a value in dBm is converted to mW as follows:
  // P_mW = 10^(P_dBm/10)
  // a value in mW is converted to dBm as follows:
  // P_dbM = 10*log10(P_mW)

  if(power1 <= minimum_noise_power)
    return power2;
  else if(power2 <= minimum_noise_power)
    return power1;
  else
    return 10*log10(pow(10, power1/10)+pow(10, power2/10));
}

// compute attenuation due to directional antenna, and
// that must be substracted from antenna gain; both azimuth
// and elevation parameters are taken into account;
// return the directional attenuation
double antenna_directional_attenuation(node_class *node_tx, 
				       node_class *node_rx)
{
  return (antenna_azimuth_attenuation(node_tx, node_rx)+
	  antenna_elevation_attenuation(node_tx, node_rx));
}

// compute attenuation in azimuth (horizontal) plane;
// return the azimuth attenuation
double antenna_azimuth_attenuation(node_class *node_tx, node_class *node_rx)
{
  double angle_tx_rx=0;
  double angle_diff=0;
  double directional_attenuation=0;

  // omni-directional antenna
  if(node_tx->azimuth_beamwidth==360)
    directional_attenuation = 0.0;
  else
    {
      // compute angle in x0y plane between transmitter and receiver
      angle_tx_rx = 
	atan2(node_rx->position.c[1]-node_tx->position.c[1],
	      node_rx->position.c[0]-node_tx->position.c[0])*180/M_PI;

      // compute difference with respect to azimuth orientation
      angle_diff=fabs(node_tx->azimuth_orientation-angle_tx_rx);

      // make the difference fit in the range [0,180]
      angle_diff=(angle_diff>360.0-angle_diff)?360.0-angle_diff:angle_diff;

      // if receiver is outiside beamwidth, assume no signal
      if(angle_diff>node_tx->azimuth_beamwidth/2)
	directional_attenuation = ANTENNA_MAX_ATTENUATION;
      else
	// 3 dB is attenuation at beamwidth/2 angle
	directional_attenuation = 3 * 2 * angle_diff / 
	  node_tx->azimuth_beamwidth;
    }

  /*
  DEBUG("*_*_*_*_*_*_*_  rx_y=%f tx_y=%f rx_x=%f tx_x=%f a_t_r=%f a_o=%f a_d=%f a_b=%f d_g=%f",
	node_rx->position_y, node_tx->position_y, node_rx->position_x, 
	node_tx->position_x,
	angle_tx_rx,  node_tx->azimuth_orientation, angle_diff, 
	node_tx->azimuth_beamwidth, directional_attenuation);
  */

  return directional_attenuation;
}

// compute attenuation in elevation (vertical) plane;
// return the elevation attenuation
double antenna_elevation_attenuation(node_class *node_tx, node_class *node_rx)
{
  double angle_tx_rx=0;
  double angle_diff=0;
  double directional_attenuation=0;

  // omni-directional antenna
  if(node_tx->elevation_beamwidth==360)
    directional_attenuation = 0.0;
  else
    {
      // compute angle in x0z plane between transmitter and receiver
      angle_tx_rx = 
	atan2(node_rx->position.c[2]-node_tx->position.c[2],
	      node_rx->position.c[0]-node_tx->position.c[0])*180/M_PI;

      // compute difference with respect to azimuth orientation
      angle_diff=fabs(node_tx->elevation_orientation-angle_tx_rx);

      // make the difference fit in the range [0,180]
      angle_diff=(angle_diff>360.0-angle_diff)?360.0-angle_diff:angle_diff;

      // if receiver is outiside beamwidth, assume no signal
      if(angle_diff>node_tx->elevation_beamwidth/2)
	directional_attenuation = ANTENNA_MAX_ATTENUATION;
      else
	// 3 dB is attenuation at beamwidth/2 angle
	directional_attenuation = 3 * 2 * angle_diff / 
	  node_tx->elevation_beamwidth;
    }

  /*
  DEBUG("*_*_*_*_*_*_*_  rx_y=%f tx_y=%f rx_x=%f tx_x=%f a_t_r=%f a_o=%f a_d=%f a_b=%f d_g=%f",
	node_rx->position_y, node_tx->position_y, node_rx->position_x, 
	node_tx->position_x,
	angle_tx_rx,  node_tx->azimuth_orientation, angle_diff, 
	node_tx->azimuth_beamwidth, directional_attenuation);
  */

  return directional_attenuation;
}

// based on the Jenkins "one at a time hash"
uint32_t string_hash(char *key, size_t key_len)
{
    uint32_t hash = 0;
    size_t i;
 
    for (i = 0; i < key_len; i++) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}
