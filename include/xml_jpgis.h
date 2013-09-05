
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
 * File name: xml_jpgis.h
 * Function:  Header file of xml_jpgis.c
 *
 * Authors: Khin Thida Latt, Razvan Beuran
 *
 * $Id: xml_jpgis.h 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/


#ifndef __XML_JPGIS_H
#define __XML_JPGIS_H


#include <expat.h>		// required by the XML parser library

#include "deltaQ.h"


//////////////////////////////////////////////////////////
// Parser specific defines & constants
/////////////////////////////////////////////////////////

#if defined(__amigaos__) && defined(__USE_INLINE__)
#include <proto/expat.h>
#endif

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
//  xml_jpgis constants
///////////////////////////////////////////////////////////////

#define BUILDING_EDGE_LABEL                "BldL"
#define ROAD_EDGE_LABEL                    "RdEdg"


/////////////////////////////////////////
// xml_jpgis structure definition
/////////////////////////////////////////

struct xml_jpgis_class
{
  int object_found;
  int coordinate_found;

  char coordinate_text[MAX_STRING];

  struct scenario_class *scenario;
  struct object_class *objects;
  int object_number;
  int object_j;
  int coordinate_i;
  int error;

  double latitude;
  double longitude;

  int load_all_from_region;

  struct object_class temp_object;
};

/////////////////////////////////////////////////
// xml_jpgis functions
/////////////////////////////////////////////////

// init the xml_jpgis structure
void xml_jpgis_init (struct xml_jpgis_class *xml_jpgis,
		     struct scenario_class *scenario,
		     struct object_class *objects, int object_number);

/////////////////////////////////////////////////
// Main XML parsing function 
/////////////////////////////////////////////////

// top-level function of this module used to load the objects;
// return SUCCESS on success, ERROR on error
int xml_jpgis_load_objects (struct scenario_class *scenario,
			    struct object_class *objects, int object_number,
			    char *jpgis_filename);

#endif
