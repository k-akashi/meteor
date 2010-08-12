
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
 * File name: deltaQ.c
 * Function: Main source file of the deltaQ computation library
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "deltaQ.h"
#include "motion.h"

#include "message.h"
#include "scenario.h"
#include "wlan.h"
#include "ethernet.h"
#include "active_tag.h"
#include "zigbee.h"


///////////////////////////////////////////////////////////////////////
// XML scenario structure functions
// Note: actually defined in scenario.c, but included in deltaQ.h 
// so that they are available for users only including this header file
///////////////////////////////////////////////////////////////////////

// parse scenario file and store results in scenario object
// return 0 if no error occured, non-0 values otherwise
//int xml_scenario_parse(FILE *scenario_file, 
//		       xml_scenario_class *xml_scenario);

// print the main properties of an XML scenario
//void xml_scenario_print(xml_scenario_class *xml_scenario);


///////////////////////////////////////////////////////////////////////
// Geodesy functions
///////////////////////////////////////////////////////////////////////

/*
   The code below is based on the Matlab files blh2xyz.m and xyz2blh.m
   from Leibniz Supercomputing Center:
     http://www.lrz-muenchen.de/~t5712ap/webserver/webdata/de/lehre/uebungsmaterial/blh2xyz.m
     http://www.lrz-muenchen.de/~t5712ap/webserver/webdata/de/lehre/uebungsmaterial/xyz2blh.m

   Razvan did some evaluations by comparing the results with those of a 
   website-based conversion and found that difference are of the order of 
   centimeters, therefore can be ignored. The website is available at:
     http://vldb.gsi.go.jp/sokuchi/surveycalc/transf.html

   Razvan decided to use WGS84 data, since it seems to be widely accepted as
   the latest data for GIS systems.

   The formulas used are taken from "GPS in der Praxis" by 
   Hofmann-Wellenhof et al.(1994)

   x, y, z coordinates are given in meters.
   Latitude and longitude are given in decimal notation (e.g., 33.3319193487).

   No negative heights are allowed.

   Note that these functions can be used for a 3D model of earth, but not 
   for calculating map coordinates, our main interest. For this purpose the 
   functions ll2xy and xy2ll should be used (see below).

*/

// compute x, y, z coordinates from latitude, longitude and height;
// input data is in point_blh, output data in point_xyz;
// algorithm verified with "A guide to coordinate systems in Great Britain",
// Annex B, pp. 41:
//   http://www.ordnancesurvey.co.uk/oswebsite/gps/docs/A_Guide_to_Coordinate_Systems_in_Great_Britain.pdf
// return SUCCESS on success, ERROR on error
int blh2xyz(const coordinate_class *point_blh, coordinate_class *point_xyz)
{
  double latitude, longitude; // coordinates in radians
  double a, f, b;             // geoid parameters

  double e_square;
  double n;

  // check whether negative height was provided
  if(point_blh->c[2]<0)
    {
      WARNING("Cannot process negative heights in latitude longitude \
to xyz conversion");
      return ERROR;
    }

  // transfor latitude and longitude from decimal degrees to radians
  latitude = point_blh->c[0]/(180/M_PI);
  longitude = point_blh->c[1]/(180/M_PI);

  // initialize geoid parameters
  a = GEOID_A;
  f = GEOID_F;
  b = (1-f)*a;

  // compute intermediate values
  e_square = (pow(a,2)-pow(b,2))/pow(a,2);
  n = a/sqrt(1-e_square*pow(sin(latitude),2));

  // compute x,y,z coordinates
  point_xyz->c[0] = (n+point_blh->c[2]) * cos(latitude) * cos(longitude);
  point_xyz->c[1] = (n+point_blh->c[2]) * cos(latitude) * sin(longitude);
  point_xyz->c[2] = (n*(1-e_square) + point_blh->c[2]) * sin(latitude);

  return SUCCESS;
}

// compute latitude, longitude and height from x, y, z coordinates;
// input data is in point_xyz, output data in point_blh;
// algorithm is different than  the one given in "A guide to coordinate 
// systems in Great Britain", Annex B, pp. 41:
//   http://www.ordnancesurvey.co.uk/oswebsite/gps/docs/A_Guide_to_Coordinate_Systems_in_Great_Britain.pdf
// return SUCCESS on success, ERROR on error
int xyz2blh(const coordinate_class *point_xyz, coordinate_class *point_blh)
{
  double latitude, longitude; // coordinates in radians
  double a, f, b;             // geoid parameters

  double e1q, e2q;
  double p, t, n;

  if(point_xyz->c[2]<0)
    {
      WARNING("Cannot process negative heights in xyz to latitude longitude \
height conversion");
      return ERROR;
    }

  // initialize geoid parameters
  a = GEOID_A;
  f = GEOID_F;
  b = (1-f)*a;

  // compute intermediate values
  e1q = (pow(a,2)-pow(b,2))/pow(a,2);
  e2q = (pow(a,2)-pow(b,2))/pow(b,2);

  p = sqrt(pow(point_xyz->c[0],2)+pow(point_xyz->c[1],2));
  t = atan(point_xyz->c[2]*a/(p*b));

  // compute latitude and longitude
  latitude = atan((point_xyz->c[2] + e2q*b*pow(sin(t),3))/
		  (p - e1q*a*pow(cos(t),3)));
  longitude = atan2(point_xyz->c[1], point_xyz->c[0]);

  // compute intermediate value needed for height calculation
  n = a/sqrt(1 - e1q*pow(sin(latitude),2));

  // compute results
  point_blh->c[0] = latitude * (180/M_PI);
  point_blh->c[1] = longitude * (180/M_PI);
  point_blh->c[2] = p/cos(latitude) - n;

  return SUCCESS;
}

/* The functions below are based on Visual Basic code provided with
   "A guide to coordinate systems in Great Britain", as well as on 
   the formulas given in Annex B, pp. 43-45:
     http://www.ordnancesurvey.co.uk/oswebsite/gps/docs/A_Guide_to_Coordinate_Systems_in_Great_Britain.pdf

   Razvan checked the results with the website:
     http://vldb.gsi.go.jp/sokuchi/surveycalc/bl2xyf.html
   and found they don't excees a few centimeters when using world model
*/

// compute the meridional arc given the ellipsoid semi major axis
// multiplied by central meridian scale factor 'bF0' in meters,
// intermediate value 'n' (computed from a, b and F0), latitude of
// false origin 'phi0_rad' and initial or final latitude of point 
// 'phi_rad' in radians; 
// returns the meridional arc
double meridional_arc(double bF0, double n, double phi0_rad, double phi_rad)
{
  return bF0 *
    (((1 + n + (5.0/4.0)*pow(n,2) + (5.0/4.0)*pow(n,3)) * 
      (phi_rad - phi0_rad)) - 
     ((3*n + 3*pow(n,2) + (21.0/8.0) * pow(n,3)) * 
      sin(phi_rad - phi0_rad) * cos(phi_rad + phi0_rad)) + 
     (((15.0/8.0)*pow(n,2) + (15.0/8.0)*pow(n,3)) * 
      sin(2*(phi_rad - phi0_rad)) * cos(2*(phi_rad + phi0_rad))) - 
     ((35.0/24.0)*pow(n,3) * sin(3*(phi_rad-phi0_rad)) * 
      cos(3*(phi_rad + phi0_rad))));
}


// convert latitude & longitude coordinates to easting & northing 
// (xy map coordinates);
// height/z parameter is copied;
// input data is in point_blh, output data in point_xyz
void ll2en(coordinate_class  *point_blh, coordinate_class *point_xyz)
{
  double E0 = GEOID_COORD_E0;          // easting of true origin
  double N0 = GEOID_COORD_N0;          // northing of true origin
  double F0 = GEOID_COORD_F0;          // scale factor of central meridian (m0)
  double phi0 = GEOID_COORD_PHI0;      // latitude of true origin
  double phi0_rad = phi0*(M_PI/180);
  double lambda0 = GEOID_COORD_LAMBDA0;// longitude of true origin and 
                                       // central meridian
  double lambda0_rad = lambda0*(M_PI/180);

  double a = GEOID_A;          // semi-major axis length
  double f = GEOID_F;          // reciprocal of flattening
  double b = (1-f)*a;          // semi-minor axis length
  double aF0 = a*F0;
  double bF0 = b*F0;

  double phi = point_blh->c[0];          // latitude
  double lambda = point_blh->c[1];       // longitude
  double phi_rad = phi*(M_PI/180);       // latitude in radians
  double lambda_rad = lambda*(M_PI/180); // longitude in radians

  double e_squared = (pow(aF0,2)-pow(bF0,2))/pow(aF0,2);

  double n = (aF0-bF0)/(aF0+bF0);// why not (a-b)/(a+b)?????
  double nu = aF0/sqrt(1 - (e_squared*pow(sin(phi_rad),2)));
  double rho = (nu*(1 - e_squared))/(1 - (e_squared*pow(sin(phi_rad),2)));
  double eta_squared = (nu/rho)-1;
  double delta_lambda_rad = lambda_rad-lambda0_rad;
  double M = meridional_arc(bF0, n, phi0_rad, phi_rad);

  double I = M + N0;
  double II = (nu / 2) * sin(phi_rad) * cos(phi_rad);
  double III = (nu / 24) * sin(phi_rad) * pow(cos(phi_rad),3) * 
    (5 - pow(tan(phi_rad),2) + (9 * eta_squared));//???
  double IIIA = (nu / 720) * sin(phi_rad) * pow(cos(phi_rad),5) * 
    (61 - (58 * pow(tan(phi_rad),2)) + pow(tan(phi_rad),4));
  double IV = nu * cos(phi_rad);
  double V = (nu / 6) * pow(cos(phi_rad),3)*((nu / rho)-pow(tan(phi_rad),2));
  double VI = (nu / 120) * pow(cos(phi_rad),5) * 
    (5 - (18*pow(tan(phi_rad),2)) + pow(tan(phi_rad),4) + (14*eta_squared) -
     (58 * pow(tan(phi_rad),2) * eta_squared));

  DEBUG("phi=%.8e lambda=%.8e phi_rad=%.8e lambda_rad=%.8e phi0_rad=%.8e \
lambda0_rad=%.8e aF0=%.8e bF0=%.8e", phi, lambda, phi_rad, lambda_rad, 
	phi0_rad, lambda0_rad, aF0, bF0);

  DEBUG("e2=%.8e n=%.8e nu=%.8e rho=%.8e eta2=%.8e dl=%.8e M=%.8e I=%.8e \
II=%.8e III=%.8e IIIA=%.8e IV=%.8e V=%.8e VI=%.8e", e_squared, n, nu, rho, 
	eta_squared, delta_lambda_rad, M, I, II, III, IIIA, IV, V, VI);

  // compute x,y
  point_xyz->c[0] = E0 + (IV * delta_lambda_rad) + 
    (V * pow(delta_lambda_rad,3)) +
    (VI * pow(delta_lambda_rad,5));
  point_xyz->c[1] = I + (II * pow(delta_lambda_rad,2)) + 
    (III * pow(delta_lambda_rad,4)) + (IIIA * pow(delta_lambda_rad,6));

  // copy height
  point_xyz->c[2] = point_blh->c[2];
}

// compute initial value of latitude given the northing of point 'North' 
// and northing of false origin 'n0' in meters, semi major axis multiplied 
// by central meridian scale factor 'aF0' in meters, latitude of false origin 
// 'phi0' in radians, intermediate value 'n' (computed from a, b and f0) and
// ellipsoid semi major axis multiplied by central meridian scale factor 'bf0'
// in meters;
// returns the initial value of latitude
double initial_latitude(double North, double n0, double aF0, double phi0,
			double n, double bF0)
{
  double phi1 = ((North - n0) / aF0) + phi0;
  double M = meridional_arc(bF0, n, phi0, phi1);
  double phi2 = ((North - n0 - M) / aF0) + phi1;
  
  int count=0; // count number of iterations

  while(fabs(North - n0 - M) > 0.00001)
    {
      phi2 = ((North - n0 - M) / aF0) + phi1;
      M = meridional_arc(bF0, n, phi0, phi2);
      phi1 = phi2;
      count++;

      // if too many iterations are performed, abort
      // (the value of phi2 at that moment will be returned)
      if(count>10000)
	{
	  WARNING("Too many iterations. Aborting");
	  break;
	}
    }
    
  return phi2;
}

// convert easting & northing (xy map coordinates) to 
// latitude & longitude;
// height/z parameter is copied;
// input data is in point_blh, output data in point_xyz
void en2ll(coordinate_class  *point_xyz, coordinate_class *point_blh)
{
  double E0 = GEOID_COORD_E0;          // easting of true origin
  double N0 = GEOID_COORD_N0;          // northing of true origin
  double F0 = GEOID_COORD_F0;          // scale factor of central meridian (m0)
  double phi0 = GEOID_COORD_PHI0;      // latitude of true origin
  double phi0_rad = phi0*(M_PI/180);
  double lambda0 = GEOID_COORD_LAMBDA0;// longitude of true origin and 
                                       // central meridian
  double lambda0_rad = lambda0*(M_PI/180);

  double a = GEOID_A;          // semi-major axis length
  double f = GEOID_F;          // reciprocal of flattening
  double b = (1-f)*a;          // semi-minor axis length
  double aF0 = a*F0;
  double bF0 = b*F0;

  double e_squared = (pow(aF0,2)-pow(bF0,2))/pow(aF0,2);

  double n = (aF0-bF0)/(aF0+bF0);   // why not (a-b)/(a+b)?????

  double East  = point_xyz->c[0];   // easting
  double North = point_xyz->c[1];   // northing
  double Et = East - E0;

  double phi_d = initial_latitude(North, N0, aF0, phi0_rad, n, bF0);

  double nu = aF0/sqrt(1 - (e_squared*pow(sin(phi_d),2)));
  double rho = (nu * (1-e_squared))/(1 - (e_squared*pow(sin(phi_d),2)));
  double eta_squared = (nu/rho)-1;

  //compute latitude params
  double VII = tan(phi_d) / (2*rho*nu);
  double VIII = (tan(phi_d) / (24*rho*pow(nu,3))) * 
    (5 + (3 * (pow(tan(phi_d),2))) + eta_squared - 
     (9 * eta_squared * (pow(tan(phi_d),2))));
  double IX = (tan(phi_d) / (720*rho*pow(nu,5))) * 
    (61 + (90 * (pow(tan(phi_d),2))) + (45 * (pow(tan(phi_d),4))));
    
  //compute longitude params
  double X = pow(cos(phi_d),-1) / nu;
  double XI = (pow(cos(phi_d),-1) / (6 * pow(nu,3))) * 
    ((nu / rho) + 2*pow(tan(phi_d),2));
  double XII = ((pow(cos(phi_d),-1) / (120 * pow(nu,5)))) * 
    (5 + (28 * pow(tan(phi_d),2)) + (24*pow(tan(phi_d),4)));
  double XIIA = (pow(cos(phi_d),-1) / (5040 * pow(nu,7))) * 
    (61 + (662 * (pow(tan(phi_d),2))) + (1320 * (pow(tan(phi_d),4))) + 
     (720 * (pow(tan(phi_d),6))));

  // latitude
  point_blh->c[0] = (180 / M_PI) * 
    (phi_d - (pow(Et,2)*VII) + (pow(Et,4)*VIII) - (pow(Et,6)*IX));

  // longitude
  point_blh->c[1] = (180 / M_PI) * 
    (lambda0_rad + (Et*X) - (pow(Et,3)*XI) + (pow(Et,5)*XII) - 
     (pow(Et,7)*XIIA));

  // altitude
  point_blh->c[2] = point_xyz->c[2];
}

