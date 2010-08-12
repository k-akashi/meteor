
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
 * File name: xml_jpgis.h
 * Function:  Header file of xml_jpgis.c
 *
 * Authors: Khin Thida Latt, Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#ifndef __XML_JPGIS_H
#define __XML_JPGIS_H


#include <expat.h>   // required by the XML parser library

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

struct xml_jpgis_class_s
{
  int object_found;
  int coordinate_found;

  char coordinate_text[MAX_STRING];

  object_class *objects;
  int object_number;
  int object_j;
  int coordinate_i;
  int error;

  double latitude;
  double longitude;
};

/////////////////////////////////////////////////
// xml_jpgis functions
/////////////////////////////////////////////////

// init the xml_jpgis structure
void xml_jpgis_init(xml_jpgis_class *xml_jpgis, object_class *objects,
		    int object_number);

/////////////////////////////////////////////////
// Main XML parsing function 
/////////////////////////////////////////////////

// top-level function of this module used to load the objects;
// return SUCCESS on success, ERROR on error
int xml_jpgis_load_objects(object_class *objects, int object_number,
			   char *jpgis_filename);


#endif
