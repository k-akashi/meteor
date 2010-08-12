
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
 * File name: route_xml.h
 * Function:  Header file of route_xml.c
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 128 $
 *   $LastChangedDate: 2009-02-06 10:21:50 +0900 (Fri, 06 Feb 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#ifndef __ROUTE_XML_H
#define __ROUTE_XML_H


#include <float.h>

// required by the XML parser library
#include <expat.h>

#include "qomet_routing.h"


//////////////////////////////////////////////////////////
//
// Parser specific defines
//
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
//
//  Routing table constants
//
///////////////////////////////////////////////////////////////

// When adding new parameters make sure to update 
// the corresponding 'copy' functions

// element and attribute names
#define ROUTING_TABLE_STRING                  "routing_table"
#define ROUTING_TABLE_NUMBER_NODES_STRING     "number_nodes"
#define ROUTING_TABLE_ALIAS_IP_HEAD_STRING    "alias_ip_head"
#define ROUTING_TABLE_TUNNEL_IP_HEAD_STRING   "tunnel_ip_head"

#define RULE_STRING                           "rule"
#define RULE_FROM_NODE_STRING                 "from_node"
#define RULE_TO_NODE_STRING                   "to_node"
#define RULE_NEXT_HOP_STRING                  "next_hop"

// When adding new parameters make sure to update 
// the corresponding 'copy' functions
 

///////////////////////////////////////////////////////////////
//
//  Default parameter values
//
///////////////////////////////////////////////////////////////

#define DEFAULT_NODE                          -1


//////////////////////////////
// xml_routing_table structure
typedef struct
{
  // main scenario components
  routing_table_class routing_table;

  // XML parse error indicator
  int xml_parse_error;

} xml_routing_table_class;



////////////////////////////////////////
// XML elements functions
////////////////////////////////////////

// initialize a routing table object from XML attributes
// return SUCCESS on succes, ERROR on error
int xml_routing_table_init(xml_routing_table_class *xml_routing_table, 
			   const char **attributes);

// print the main properties of an XML scenario
void xml_routing_table_print(xml_routing_table_class *xml_routing_table);


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

/////////////////////////////////////////////////
// Main XML parsing function 

// parse scenario file and store results in scenario object
// return SUCCESS on succes, ERROR on error
int xml_routing_table_parse(FILE *scenario_file, 
			    xml_routing_table_class *xml_routing_table);


////////////////////////////////////////////
// Check validity of elements
////////////////////////////////////////////

// check whether a newly defined node conflicts with existing ones
// return TRUE if node is valid, FALSE otherwise
int routing_rule_check_valid(routing_rule_class *routing_rule);

#endif
