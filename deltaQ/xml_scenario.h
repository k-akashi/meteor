
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
 * File name: xml_scenario.h
 * Function:  Header file of xml_scenario.c
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 142 $
 *   $LastChangedDate: 2009-03-31 09:09:45 +0900 (Tue, 31 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#ifndef __XML_SCENARIO_H
#define __XML_SCENARIO_H


#include <float.h>
#include <expat.h>   // required by the XML parser library

#include "deltaQ.h"


//////////////////////////////////////////////////////////
// Parser specific defines & constants
/////////////////////////////////////////////////////////

#ifdef XML_LARGE_SIZE
#if defined(XML_USE_MSC_EXTENSIONS) && _MSC_VER < 1400
#define XML_FMT_INT_MOD "I64"
#else
#define XML_FMT_INT_MOD "ll"
#endif
#else
#define XML_FMT_INT_MOD "l"
#endif

// read buffer size
#define BUFFER_SIZE        8192

// read buffer
char xml_buffer[BUFFER_SIZE];

// xml tree depth
int xml_depth;


///////////////////////////////////////////////////////////////
//  xml_scenario constants
///////////////////////////////////////////////////////////////

#define TRUE_STRING                           "true"
#define FALSE_STRING                          "false"

// xml_scenario element and attribute names
#define XML_SCENARIO_STRING                        "qomet_scenario"
#define XML_SCENARIO_START_TIME_STRING             "start_time"
#define XML_SCENARIO_DURATION_STRING               "duration"
#define XML_SCENARIO_STEP_STRING                   "step"
#define XML_SCENARIO_MOTION_STEP_DIVIDER_STRING    "motion_step_divider"
#define XML_SCENARIO_COORD_SYST_STRING             "coordinate_system"
#define XML_SCENARIO_COORD_SYST_CARTESIAN_STRING   "cartesian"
#define XML_SCENARIO_COORD_SYST_LAT_LON_ALT_STRING "lat_lon_alt"
#define XML_SCENARIO_JPGIS_FILENAME_STRING         "jpgis_file_name"

// When adding new parameters make sure to update 
// the corresponding 'copy' functions

// node element and attribute names
#define NODE_STRING                           "node"
#define NODE_NAME_STRING                      "name"
#define NODE_TYPE_STRING                      "type"
#define NODE_TYPE_REGULAR_STRING              "regular"
#define NODE_TYPE_ACCESS_POINT_STRING         "access_point"
#define NODE_ID_STRING                        "id"
#define NODE_SSID_STRING                      "ssid"
#define NODE_CONNECTION_STRING                "connection"
#define NODE_CONNECTION_AD_HOC_STRING         "ad_hoc"
#define NODE_CONNECTION_INFRASTRUCTURE_STRING "infrastructure"
#define NODE_CONNECTION_ANY_STRING            "any"
#define NODE_ADAPTER_STRING                   "adapter"
#define NODE_ADAPTER_ORINOCO_STRING           "orinoco"
#define NODE_ADAPTER_DEI80211MR_STRING        "dei80211mr"
#define NODE_ADAPTER_CISCO_340_STRING         "cisco_340"
#define NODE_ADAPTER_CISCO_ABG_STRING         "cisco_abg"
#define NODE_ADAPTER_JENNIC_STRING            "jennic"
#define NODE_ADAPTER_S_NODE_STRING            "s_node"
#define NODE_X_STRING                         "x"
#define NODE_Y_STRING                         "y"
#define NODE_Z_STRING                         "z"
#define NODE_PT_STRING                        "Pt"
#define NODE_ANTENNA_GAIN_STRING              "antenna_gain"
#define NODE_AZIMUTH_ORIENTATION_STRING       "azimuth_orientation"
#define NODE_AZIMUTH_BEAMWIDTH_STRING         "azimuth_beamwidth"
#define NODE_ELEVATION_ORIENTATION_STRING     "elevation_orientation"
#define NODE_ELEVATION_BEAMWIDTH_STRING       "elevation_beamwidth"
#define NODE_INTERNAL_DELAY_STRING            "internal_delay"

// When adding new parameters make sure to update 
// the corresponding 'copy' functions

// object element and attribute names
#define OBJECT_STRING                         "object"
#define OBJECT_NAME_STRING                    "name"
#define OBJECT_TYPE_STRING                    "type"
#define OBJECT_TYPE_BUILDING_STRING           "building"
#define OBJECT_TYPE_ROAD_STRING               "road"
#define OBJECT_ENVIRONMENT_STRING             "environment"
#define OBJECT_X1_STRING                      "x1"
#define OBJECT_Y1_STRING                      "y1"
#define OBJECT_X2_STRING                      "x2"
#define OBJECT_Y2_STRING                      "y2"
#define OBJECT_HEIGHT_STRING                  "height"
#define OBJECT_LOAD_FROM_JPGIS_FILE_STRING    "load_from_jpgis_file"
#define OBJECT_MAKE_POLYGON_STRING            "make_polygon"
//#define OBJECT_IS_METRIC_STRING               "is_metric"  //USED????????

// When adding new parameters make sure to update 
// the corresponding 'copy' functions

// coordinate element and attribute names
#define COORDINATE_STRING                     "coordinate"
#define COORDINATE_NAME_STRING                "name"

// When adding new parameters make sure to update 
// the corresponding 'copy' functions
 
// environment element and attribute names
#define ENVIRONMENT_STRING                    "environment"
#define ENVIRONMENT_NAME_STRING               "name"
#define ENVIRONMENT_TYPE_STRING               "type"
#define ENVIRONMENT_IS_DYNAMIC_STRING         "is_dynamic"
#define ENVIRONMENT_ALPHA_STRING              "alpha"
#define ENVIRONMENT_SIGMA_STRING              "sigma"
#define ENVIRONMENT_W_STRING                  "W"
#define ENVIRONMENT_NOISE_POWER_STRING        "noise_power"

// When adding new parameters make sure to update 
// the corresponding 'copy' functions

// motion element and attribute names
#define MOTION_STRING                         "motion"
#define MOTION_NODE_NAME_STRING               "node_name"
#define MOTION_TYPE_STRING                    "type"
#define MOTION_TYPE_LINEAR_STRING             "linear"
#define MOTION_TYPE_CIRCULAR_STRING           "circular"
#define MOTION_TYPE_ROTATION_STRING           "rotation"
#define MOTION_TYPE_RANDOM_WALK_STRING        "random_walk"
#define MOTION_TYPE_BEHAVIORAL_STRING         "behavioral"
#define MOTION_SPEED_X_STRING                 "speed_x"
#define MOTION_SPEED_Y_STRING                 "speed_y"
#define MOTION_SPEED_Z_STRING                 "speed_z"
#define MOTION_CENTER_X_STRING                "center_x"
#define MOTION_CENTER_Y_STRING                "center_y"
#define MOTION_VELOCITY_STRING                "velocity"
#define MOTION_MIN_SPEED_STRING               "min_speed"
#define MOTION_MAX_SPEED_STRING               "max_speed"
#define MOTION_WALK_TIME_STRING               "walk_time"
#define MOTION_DESTINATION_X_STRING           "destination_x"
#define MOTION_DESTINATION_Y_STRING           "destination_y"
#define MOTION_DESTINATION_Z_STRING           "destination_z"
#define MOTION_ROTATION_ANGLE_HORIZONTAL_STRING "rotation_angle_horizontal"
#define MOTION_ROTATION_ANGLE_VERTICAL_STRING "rotation_angle_vertical"
#define MOTION_START_TIME_STRING              "start_time"
#define MOTION_STOP_TIME_STRING               "stop_time"

// When adding new parameters make sure to update 
// the corresponding 'copy' functions

// connection element and attribute names
#define CONNECTION_STRING                     "connection"
#define CONNECTION_FROM_NODE_STRING           "from_node"
#define CONNECTION_TO_NODE_STRING             "to_node"
//#define CONNECTION_TO_NODES_STRING            "to_nodes"
#define CONNECTION_TO_NODE_AUTO_CONNECT_STRING "auto_connect"
#define CONNECTION_THROUGH_ENVIRONMENT_STRING "through_environment"
#define CONNECTION_STANDARD_STRING            "standard"
#define CONNECTION_STANDARD_802_11B_STRING    "802.11b"
#define CONNECTION_STANDARD_802_11G_STRING    "802.11g"
#define CONNECTION_STANDARD_802_11A_STRING    "802.11a"
#define CONNECTION_STANDARD_ETH_10_STRING     "eth_10"
#define CONNECTION_STANDARD_ETH_100_STRING    "eth_100"
#define CONNECTION_STANDARD_ETH_1000_STRING   "eth_1000"
#define CONNECTION_STANDARD_ACTIVE_TAG_STRING "active_tag"
#define CONNECTION_STANDARD_ZIGBEE_STRING     "zigbee"
#define CONNECTION_CHANNEL_STRING             "channel"
#define CONNECTION_RTS_CTS_THRESHOLD_STRING   "RTS_CTS_threshold"
#define CONNECTION_PACKET_SIZE_STRING         "packet_size"
#define CONNECTION_BANDWIDTH_STRING           "bandwidth"
#define CONNECTION_LOSS_RATE_STRING           "loss_rate"
#define CONNECTION_DELAY_STRING               "delay"
#define CONNECTION_JITTER_STRING              "jitter"
#define CONNECTION_CONSIDER_INTERFERENCE_STRING "consider_interference"

// When adding new parameters make sure to update 
// the corresponding 'copy' functions


///////////////////////////////////////////////////////////////
//  Default parameter values
///////////////////////////////////////////////////////////////

#define DEFAULT_STRING                        "N/A"

#define DEFAULT_NODE_NAME                     DEFAULT_STRING
#define DEFAULT_NODE_TYPE                     REGULAR_NODE
#define DEFAULT_NODE_ID                       0
#define DEFAULT_NODE_SSID                     DEFAULT_STRING
#define DEFAULT_NODE_CONNECTION               ANY_CONNECTION
//????????????? CHANGE: CISCO_ABG NOT GOOD FOR ALL STANDARDS
#define DEFAULT_NODE_ADAPTER_TYPE             CISCO_ABG
#define DEFAULT_NODE_ANTENNA_GAIN             0.0
#define DEFAULT_NODE_COORDINATE               0.0
#define DEFAULT_NODE_PT                       20.0
#define DEFAULT_NODE_INTERNAL_DELAY           0.0

#define DEFAULT_OBJECT_NAME                   DEFAULT_STRING
#define DEFAULT_OBJECT_COORD_1                -DBL_MAX
#define DEFAULT_OBJECT_COORD_2                DBL_MAX

#define DEFAULT_ENVIRONMENT_NAME              DEFAULT_STRING
#define DEFAULT_ENVIRONMENT_TYPE              DEFAULT_STRING
#define DEFAULT_ENVIRONMENT_ALPHA             2.0
#define DEFAULT_ENVIRONMENT_SIGMA             0.0
#define DEFAULT_ENVIRONMENT_WALL_ATTENUATION  0.0
#define DEFAULT_ENVIRONMENT_NOISE_POWER       -100.0
#define DEFAULT_ENVIRONMENT_COORD_1           -DBL_MAX
#define DEFAULT_ENVIRONMENT_COORD_2           DBL_MAX

#define DEFAULT_MOTION_NODE_NAME              DEFAULT_STRING
#define DEFAULT_MOTION_TYPE                   LINEAR_MOTION
#define DEFAULT_MOTION_SPEED                  0.0
#define DEFAULT_MOTION_CENTER                 0.0
#define DEFAULT_MOTION_MAX_SPEED              1.0
#define DEFAULT_MOTION_WALK_TIME              5.0
#define DEFAULT_MOTION_TIME                   0.0

#define DEFAULT_CONNECTION_PARAMETER          DEFAULT_STRING
#define DEFAULT_CONNECTION_PACKET_SIZE        1024
#define DEFAULT_CONNECTION_STANDARD           WLAN_802_11B
#define DEFAULT_CONNECTION_CHANNEL            1
#define DEFAULT_CONNECTION_RTS_CTS_THRESHOLD  MAX_PSDU_SIZE_B
#define DEFAULT_CONNECTION_CONSIDER_INTERFERENCE TRUE


///////////////////////////////////////////////////////////////
//  Other constants
///////////////////////////////////////////////////////////////

#define ELEMENT_XML_SCENARIO                  0
#define ELEMENT_NODE                          1
#define ELEMENT_OBJECT                        2
#define ELEMENT_COORDINATE                    3
#define ELEMENT_ENVIRONMENT                   4
#define ELEMENT_MOTION                        5
#define ELEMENT_CONNECTION                    6

#define COORDINATE_NUMBER                     3


/////////////////////////////////////////
// xml_scenario structure definition
/////////////////////////////////////////

struct xml_scenario_class_s
{
  // main scenario components
  scenario_class scenario;

  // scenario start time
  double start_time;

  // scenario duration and deltaQ calculation step
  double duration;
  double step;

  // the divider that should be applied to deltaQ calculation
  // step; used for motion computations so that time step
  // is sufficiently small independent on the deltaQ calculation step
  double motion_step_divider;

  // flag to indicate whether the coordinate system
  // is cartesian or not (that is, latitude, longitude, altitude)
  int cartesian_coord_syst;

  // XML parse error indicator
  int xml_parse_error;

  // stack used during XML scenario parsing
  stack_class stack;

  // buffer used internally during parsing of XML character data
  char buffer[MAX_STRING];

  // storage used internally for storing one coordinate
  // while parsing
  coordinate_class coordinate;

  // JPGIS filename
  char jpgis_filename[MAX_STRING];

  // flag showing whether a JPGIS filename was provided
  int jpgis_filename_provided;
};


////////////////////////////////////////
// XML elements functions
////////////////////////////////////////

// initialize a node object from XML attributes
// flag 'cartesian_coord_syst' specifies whether
// cartesian coordinates are used or not;
// return SUCCESS on succes, ERROR on error
int xml_node_init(node_class *node, const char **attributes, 
		  int cartesian_coord_syst);

// initialize a topology object from XML attributes
// flag 'cartesian_coord_syst' specifies whether
// cartesian coordinates are used or not;
// return SUCCESS on succes, ERROR on error
int xml_object_init(object_class *object, const char **attributes,
		    int cartesian_coord_syst);

// initialize an environment object from XML attributes
// return SUCCESS on succes, ERROR on error
int xml_environment_init(environment_class *environment, 
			 const char **attributes);

// initialize a motion object from XML attributes
// flag 'cartesian_coord_syst' specifies whether
// cartesian coordinates are used or not;
// return SUCCESS on succes, ERROR on error
int xml_motion_init(motion_class *motion, const char **attributes, 
		    int cartesian_coord_syst);

// initialize a connection object from XML attributes
// return SUCCESS on succes, ERROR on error
int xml_connection_init(connection_class *connection, 
			const char **attributes);

// initialize an xml_scenario object from XML attributes
// return SUCCESS on succes, ERROR on error
int xml_scenario_init(xml_scenario_class *xml_scenario, 
		      const char **attributes);

// print the main properties of an XML scenario
void xml_scenario_print(xml_scenario_class *xml_scenario);


/////////////////////////////////////////////////
// Specific XML parsing functions
/////////////////////////////////////////////////

// this function is used whenever a new element is encountered;
// here it is used to read the scenario file, therefore it needs
// to differentiate between the different elements
// Note: used as a static function in 'scenario.c', therefore
// doesn't need to be defined
//static void XMLCALL start_element(void *data, const char *element, 
//			            const char **attributes);

// this function is executed when an element ends
// Note: used as a static function in 'scenario.c', therefore
// doesn't need to be defined
//static void XMLCALL end_element(void *data, const char *element);

//static void XML character_data (void *userData, const XML_Char *string, 
//				int length)

/////////////////////////////////////////////////
// Main XML parsing function 

// parse scenario file and store results in scenario object
// return SUCCESS on succes, ERROR on error
int xml_scenario_parse(FILE *scenario_file, xml_scenario_class *xml_scenario);


#endif
