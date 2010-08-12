
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
 * File name: route_xml.c
 * Function:  Load an XML route description into memory in the 
 *            routing table structure defined in qomet_routing.h
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 128 $
 *   $LastChangedDate: 2009-02-06 10:21:50 +0900 (Fri, 06 Feb 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <expat.h>
#include <string.h>

#include "qomet_routing.h"
#include "message.h"
#include "route_xml.h"
#include "global.h"


/////////////////////////////////////////
// Generic functions
/////////////////////////////////////////

// convert a string to a int value
int int_value(const char *string)
{
  long value;
  char *end_pointer;
  
  // try to use 'errno' to catch errors
  // the value doesn't seem to ever change though
  errno=0; 

  // convert the value
  value=strtol(string, &end_pointer, 10);

  // check the input string was not empty, 
  // that the string after parsing is empty (i.e., all input was parsed),
  // and that there was no error
  if((string[0]==0) || (end_pointer[0]!=0) || (errno!=0))
    WARNING("Converting value '%s' to int failed!", string);

  return (int)value;
}


////////////////////////////////////////
// XML elements functions
////////////////////////////////////////

// initialize a routing table object from XML attributes
// return SUCCESS on succes, ERROR on error
int xml_routing_rule_init(routing_rule_class *routing_rule, 
			   const char **attributes)
{
  int i, id;

  int from_node_provided=FALSE, to_node_provided=FALSE, 
    next_hop_provided=FALSE;

  // parse all possible attributes of a routing table
  for (i=0; attributes[i]; i+=2)
    if(strcasecmp(attributes[i], RULE_FROM_NODE_STRING)==0)
      {
	id = int_value(attributes[i+1]);
	if(id<0)
	  {
	    WARNING("Rule attribute '%s' must be a positive integer. \
Ignored negative value ('%s')", RULE_FROM_NODE_STRING, attributes[i+1]);
	    return ERROR;
	  }
	else
	  {
	    from_node_provided = TRUE;
	    routing_rule->from_node = id;
	  }
      }
    else if(strcasecmp(attributes[i], RULE_TO_NODE_STRING)==0)
      {
	id = int_value(attributes[i+1]);
	if(id<0)
	  {
	    WARNING("Rule attribute '%s' must be a positive integer. \
Ignored negative value ('%s')", RULE_TO_NODE_STRING, attributes[i+1]);
	    return ERROR;
	  }
	else
	  {
	    to_node_provided = TRUE;
	    routing_rule->to_node = id;
	  }
      }
    else if(strcasecmp(attributes[i], RULE_NEXT_HOP_STRING)==0)
      {
	id = int_value(attributes[i+1]);
	if(id<0)
	  {
	    WARNING("Rule attribute '%s' must be a positive integer. \
Ignored negative value ('%s')", RULE_NEXT_HOP_STRING, attributes[i+1]);
	    return ERROR;
	  }
	else
	  {
	    next_hop_provided = TRUE;
	    routing_rule->next_hop = id;
	  }
      }
    else
      {
	WARNING("Unknown XML attribute for element '%s': '%s'", 
		RULE_STRING, attributes[i]);
	return ERROR;
      }

  if(from_node_provided == FALSE)
    {
      WARNING("Rule attribute '%s' is mandatory!", RULE_FROM_NODE_STRING);
      return ERROR;
    }

  if(to_node_provided == FALSE)
    {
      WARNING("Rule attribute '%s' is mandatory!", RULE_TO_NODE_STRING);
      return ERROR;
    }

  if(next_hop_provided == FALSE)
    {
      WARNING("Rule attribute '%s' is mandatory!", RULE_FROM_NODE_STRING);
      return ERROR;
    }

#ifdef MESSAGE_DEBUG
  DEBUG("Creating the following routing rule:");
  routing_rule_print(routing_rule);
#endif

  return SUCCESS;
}

// initialize a routing table object from XML attributes
// return SUCCESS on succes, ERROR on error
int xml_routing_table_init(xml_routing_table_class *xml_routing_table, 
			   const char **attributes)
{
  int i;
  int number_nodes;
  int number_nodes_provided = FALSE;
  int alias_ip_head_provided = FALSE;
  int tunnel_ip_head_provided = FALSE;


  // clear routing table  
  routing_table_clear(&(xml_routing_table->routing_table));
  
  // parse all possible attributes of a routing table
  for (i=0; attributes[i]; i+=2)
    if(strcasecmp(attributes[i], ROUTING_TABLE_NUMBER_NODES_STRING)==0)
      {
	number_nodes = int_value(attributes[i+1]);
	if(number_nodes<=0)
	  {
	    WARNING("Rule attribute '%s' must be a larger that 0. \
Ignored invalid value ('%s')", ROUTING_TABLE_NUMBER_NODES_STRING, 
		    attributes[i+1]);
	    return ERROR;
	  }
	else
	  {
	    number_nodes_provided = TRUE;
	    xml_routing_table->routing_table.number_nodes = number_nodes;
	  }
      }
    else if(strcasecmp(attributes[i], ROUTING_TABLE_ALIAS_IP_HEAD_STRING)==0)
      {
	alias_ip_head_provided = TRUE;
	strncpy(xml_routing_table->routing_table.alias_ip_head,
		attributes[i+1], MAX_IP_ADDRESS_LENGTH);
      }
    else if(strcasecmp(attributes[i], ROUTING_TABLE_TUNNEL_IP_HEAD_STRING)==0)
      {
	tunnel_ip_head_provided = TRUE;
	strncpy(xml_routing_table->routing_table.tunnel_ip_head,
		attributes[i+1], MAX_IP_ADDRESS_LENGTH);
      }
    else
      {
	WARNING("Unknown XML attribute for element '%s': '%s'", 
		ROUTING_TABLE_STRING, attributes[i]);
	return ERROR;
      }

  if(number_nodes_provided == FALSE)
    {
      WARNING("Routing table attribute '%s' is mandatory!", 
	      ROUTING_TABLE_NUMBER_NODES_STRING);
      return ERROR;
    }

  if(alias_ip_head_provided == FALSE)
    {
      WARNING("Routing table attribute '%s' is mandatory!", 
	      ROUTING_TABLE_ALIAS_IP_HEAD_STRING);
      return ERROR;
    }

  if(tunnel_ip_head_provided == FALSE)
    {
      WARNING("Routing table attribute '%s' is mandatory!", 
	      ROUTING_TABLE_TUNNEL_IP_HEAD_STRING);
      return ERROR;
    }

  return SUCCESS;
}

// print the main properties of an XML scenario
void xml_routing_table_print(xml_routing_table_class *xml_routing_table)
{
  routing_table_print(&(xml_routing_table->routing_table));
}


/////////////////////////////////////////////////
// Specific XML parsing functions
/////////////////////////////////////////////////

// this function is used whenever a new element is encountered.
// here it is used to read the scenario file, therefore it needs
// to differentiate between the different elements
static void XMLCALL start_element(void *data, const char *element, 
				  const char **attributes)
{
  routing_rule_class routing_rule;

  // get a pointer to user data in our XML object
  xml_routing_table_class *xml_routing_table =(xml_routing_table_class *)data;

#ifdef MESSAGE_DEBUG
  // print XML tree

  int i;

  DEBUG("Printing XML element...");
  // print depth
  for (i=0; i<xml_depth; i++)
    printf(">");

  printf(">ELEMENT: %s\n", element);

  if(attributes[0]!=NULL)// some attributes  
    {
      printf("\tATTRIBUTES:");
      for (i=0; attributes[i]; i+=2)
	printf(" %s='%s'", attributes[i], attributes[i+1]);
    }
  else // no attributes
    printf("\tNO ATTRIBUTES");
  printf("\n");
#endif

  // check if we are at top level
  if(xml_depth==0)
    //check we have the right type of document
    if(strcasecmp(element, ROUTING_TABLE_STRING)==0)
      {
	INFO("Parsing QOMET routing table definition XML file...");
	if(xml_routing_table_init(xml_routing_table, attributes)==ERROR)
	  {
	    WARNING("Cannot initialize routing table");
	    xml_routing_table->xml_parse_error = TRUE;
	  }
      }
    else
      {
	WARNING("Expected top-level entity '%s' not found. XML file \
is not of 'QOMET routing table definition' type!", ROUTING_TABLE_STRING);
	xml_routing_table->xml_parse_error = TRUE;
      }

  // not top level, so look for element definitions
  else if(strcasecmp(element, RULE_STRING)==0) // check if rule
    {
      INFO("Routing rule element found");

      if(xml_routing_rule_init(&routing_rule, attributes)==ERROR)
	{
	  WARNING("Cannot initialize routing rule"); 
	  xml_routing_table->xml_parse_error = TRUE;
	}
      else 
	if(routing_rule_check_valid(&routing_rule)==TRUE)
	  {
	    if(routing_table_add_rulec(&(xml_routing_table->routing_table), 
				      &routing_rule)==ERROR)
	      {
		WARNING("Cannot add a new rule"); 
		xml_routing_table->xml_parse_error = TRUE;
	      }
	  }
	else
	  {
	    WARNING("Rule is not valid");
	    xml_routing_table->xml_parse_error = TRUE;
	  }
    }
  else // unknown element
    {
      WARNING("Unknown XML element found ('%s')", element);
      xml_routing_table->xml_parse_error = TRUE;
    }

  // other actions go BEFORE this
  xml_depth++;
}

// this function is executed when an element ends
static void XMLCALL end_element(void *data, const char *element)
{
  // other actions go AFTER this
  xml_depth--;

  // other actions...
}

// parse routing table file and store results in routing_table object
// return SUCCESS on succes, ERROR on error
int xml_routing_table_parse(FILE *scenario_file, 
			    xml_routing_table_class *xml_routing_table)
{
  int parsing_done;
  int buffer_length;

  // create the parser
  XML_Parser xml_parser = XML_ParserCreate(NULL);
  if(!xml_parser) 
    {
      WARNING("Could not allocate memory for parser");
      return ERROR;
    }

  // set the element handlers 'start' and 'end'
  XML_SetElementHandler(xml_parser, start_element, end_element);

  // initialize error status
  //xml_scenario->xml_parse_error = FALSE;

  // set the user data pointer to the scenario object
  XML_SetUserData(xml_parser, xml_routing_table);

  // parsing loop
  do
    {
      // read one buffer
      buffer_length = (int)fread(xml_buffer, 1, BUFFER_SIZE, scenario_file);
      
      // check for read error
      if(ferror(scenario_file))
	{
	  WARNING("Read error while parsing");
	  return ERROR;
	}
      
      // check for end of file
      parsing_done = feof(scenario_file);

      // check for parse error
      if(XML_Parse(xml_parser, xml_buffer, buffer_length, parsing_done) 
	  == XML_STATUS_ERROR)
	{
	  WARNING("Syntax XML parse error in input file at line #%" 
		  XML_FMT_INT_MOD "u: \"%s\"", 
		  (long unsigned int) XML_GetCurrentLineNumber(xml_parser),
		  XML_ErrorString(XML_GetErrorCode(xml_parser)));
	  return ERROR;
	}
      else if(xml_routing_table->xml_parse_error == TRUE)
	{
	  WARNING("User XML parse error in input file");
	  return ERROR;
	}
    }
  while(parsing_done==0);

  return SUCCESS;
}


////////////////////////////////////////////
// Check validity of elements
////////////////////////////////////////////

// check whether a newly defined node conflicts with existing ones
// return TRUE if node is valid, FALSE otherwise
int routing_rule_check_valid(routing_rule_class *routing_rule)
{
  if(routing_rule->from_node == routing_rule->to_node)
    {
      WARNING("Routing rule fields 'from_node' and 'to_node' cannot have \
the same id");
      return ERROR;
    }
  else if(routing_rule->from_node == routing_rule->next_hop)
    {
      WARNING("Routing rule fields 'from_node' and 'next_hop' cannot have \
the same id");
      return ERROR;
    }

  return TRUE;
}
