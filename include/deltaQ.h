
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
 * File name: deltaQ.h
 * Function:  Main header file of the deltaQ computation library 
 *            implemented in deltaQ.c
 *
 * Author: Razvan Beuran
 *
 * $Id: deltaQ.h 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/


#ifndef __DELTAQ_H
#define __DELTAQ_H


#include <stdio.h>		// needed for declaration of FILE

#include "global.h"
#include "stack.h"

#include "node.h"
#include "object.h"
#include "coordinate.h"
#include "environment.h"
#include "motion.h"
#include "connection.h"
#include "scenario.h"
#include "xml_scenario.h"
#include "io.h"


///////////////////////////////////////////////////////////
// Version management constants
///////////////////////////////////////////////////////////

#define MAJOR_VERSION              2
#define MINOR_VERSION              1
#define SUBMINOR_VERSION           0
#define IS_BETA                    FALSE	//TRUE


/////////////////////////////////////////////
// Windows Visual C specific defines
/////////////////////////////////////////////

/* NOT USED ANYMORE SINCE strcasecmp HAS BEEN REPLACED BY strcmp
#ifdef __WINDOWS__
#define strcasecmp stricmp
#endif
*/

//#define AUTO_CONNECT_BASED_ON_FER


/////////////////////////////////////////////
// Basic constants
/////////////////////////////////////////////

#define UNDEFINED_VALUE                 -1

// a small value used when comparing doubles for equality
#define EPSILON                        1e-5	//1e-6

// defines for supported network technologies
// currently: 802.11 and Ethernet standards
#define WLAN_802_11B                    0
#define WLAN_802_11G                    1
#define WLAN_802_11A                    2
#define ETHERNET_10                     3
#define ETHERNET_100                    4
#define ETHERNET_1000                   5
#define ACTIVE_TAG                      6
#define ZIGBEE                          7
#define WIMAX_802_16                    8


////////////////////////////////////////////////
// Geodesy constants
////////////////////////////////////////////////

// (from blh2xyz.par in software tool trns96)

#define GEOID_WGS84
//#define GEOID_GRS83
//#define GEOID_GRS80
//#define GEOID_BESSEL

#ifdef GEOID_WGS84
// equatorial radius
#define GEOID_A                         6378137.0
// flattening
#define GEOID_F                         1/298.257223563
#elif  GEOID_GRS83
#define GEOID_A                         6378136.0
#define GEOID_F                         1/298.257222101
#elif  GEOID_GRS80
#define GEOID_A                         6378137.0
#define GEOID_F                         1/298.257222101
#elif  GEOID_BESSEL
#define GEOID_A                         6377397.155
#define GEOID_F                         1/299.1528128
#endif

// from "www.gsi.go.jp/LAW/heimencho.html" and "A guide to 
// coordinate systems in Great Britain

#define JAPAN_IX_COORDINATES
//#define UK_COORDINATES

#ifdef JAPAN_IX_COORDINATES	// Japan values (zone IX - Tokyo etc.)
#define GEOID_COORD_E0      0.0	//easting of true origin
#define GEOID_COORD_N0      0.0	// northing of true origin
#define GEOID_COORD_F0      0.9999	// scale factor of central
					 // meridian (m0)
#define GEOID_COORD_PHI0    36.0	// latitude of true origin
#define GEOID_COORD_LAMBDA0 139.8333333;	// longitude of true origin and
					 // central meridian
#elif UK_COORDINATES		// UK values
#define GEOID_COORD_E0      400000.0	//easting of true origin
#define GEOID_COORD_N0      -100000.0	// northing of true origin
#define GEOID_COORD_F0      0.9996012717	// scale factor of central
					 // meridian (m0)
#define GEOID_COORD_PHI0    49.0	// latitude of true origin
#define GEOID_COORD_LAMBDA0 -2.0	// longitude of true origin and
					 // central meridian
#endif


//////////////////////////////////////////////////////////////////
// XML scenario structure functions
// Note: actually defined in xml_scenario.c, but included here so that
// they are available for users only including this header file
//
//  CHANGE NAMES SO THAT THEY CAN BE DEFINED HERE!!!!!!!!!!!!!
//
//////////////////////////////////////////////////////////////////

// parse scenario file and store results in scenario object;
// return 0 if no error occured, non-0 values otherwise
int xml_scenario_parse(FILE *scenario_file, struct xml_scenario_class *xml_scenario);

// print the main properties of an XML scenario
void xml_scenario_print(struct xml_scenario_class *xml_scenario);


///////////////////////////////////////////////////////////////////////
// Geodesy functions
// (TO BE MOVED LATER)
///////////////////////////////////////////////////////////////////////

// compute x, y, z coordinates from latitude, longitude and height;
// input data is in point_blh, output data in point_xyz;
// algorithm verified with "A guide to coordinate systems in Great Britain",
// Annex B, pp. 41:
//   http://www.ordnancesurvey.co.uk/oswebsite/gps/docs/A_Guide_to_Coordinate_Systems_in_Great_Britain.pdf
// return SUCCESS on success, ERROR on error
int blh2xyz(const struct coordinate_class *point_blh, struct coordinate_class *point_xyz);

// compute latitude, longitude and height from x, y, z coordinates;
// input data is in point_xyz, output data in point_blh;
// algorithm is different than  the one given in "A guide to coordinate 
// systems in Great Britain", Annex B, pp. 41:
//   http://www.ordnancesurvey.co.uk/oswebsite/gps/docs/A_Guide_to_Coordinate_Systems_in_Great_Britain.pdf
// return SUCCESS on success, ERROR on error
int xyz2blh(const struct coordinate_class *point_xyz, struct coordinate_class *point_blh);

// compute the meridional arc given the ellipsoid semi major axis
// multiplied by central meridian scale factor 'bF0' in meters,
// intermediate value 'n' (computed from a, b and F0), latitude of
// false origin 'phi0_rad' and initial or final latitude of point 
// 'phi_rad' in radians; 
// returns the meridional arc
double meridional_arc(double bF0, double n, double phi0_rad, double phi_rad);

// convert latitude & longitude coordinates to easting & northing 
// (xy map coordinates);
// height/z parameter is copied;
// input data is in point_blh, output data in point_xyz
void ll2en(struct coordinate_class *point_blh, struct coordinate_class *point_xyz);

// compute initial value of latitude given the northing of point 'North' 
// and northing of false origin 'n0' in meters, semi major axis multiplied 
// by central meridian scale factor 'aF0' in meters, latitude of false origin 
// 'phi0' in radians, intermediate value 'n' (computed from a, b and f0) and
// ellipsoid semi major axis multiplied by central meridian scale factor 'bf0'
// in meters;
// returns the initial value of latitude
double initial_latitude(double North, double n0, double aF0, double phi0, double n, double bF0);

// convert easting & northing (xy map coordinates) to 
// latitude & longitude;
// height/z parameter is copied;
// input data is in point_blh, output data in point_xyz
void en2ll(struct coordinate_class *point_xyz, struct coordinate_class *point_blh);


#endif
