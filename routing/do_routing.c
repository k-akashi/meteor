
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
 * File name: do_routing.c
 * Function: Source file of the application that effectively configures 
 *           system routing tables by calling the qomet_routing library
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 128 $
 *   $LastChangedDate: 2009-02-06 10:21:50 +0900 (Fri, 06 Feb 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "qomet_routing.h"
#include "route_xml.h"
#include "global.h"


///////////////////////////////////
// Generic variables and functions
///////////////////////////////////

#define MESSAGE_WARNING
#define MESSAGE_DEBUG
#define MESSAGE_INFO

#ifdef MESSAGE_WARNING
#define WARNING(message...) do {                                            \
  fprintf(stdout, "do_routing WARNING: %s, line %d: ", __FILE__, __LINE__); \
  fprintf(stdout, message); fprintf(stdout,"\n");                           \
} while(0)
#else
#define WARNING(message...) /* message */
#endif

#ifdef MESSAGE_DEBUG
#define DEBUG(message...) do {                                            \
  fprintf(stdout, "do_routing DEBUG: %s, line %d: ", __FILE__, __LINE__); \
  fprintf(stdout, message); fprintf(stdout,"\n");                         \
} while(0)
#else
#define DEBUG(message...) /* message */
#endif

#ifdef MESSAGE_INFO
#define INFO(message...) do {                                       \
  fprintf(stdout, message); fprintf(stdout,"\n");                   \
} while(0)
#else
#define INFO(message...) /* message */
#endif

// NOTE: no rule is deleted using this method

// merge two routing tables and write result to system;
// return SUCCESS on success, ERROR on error
int merge_routing_tables_and_write(routing_table_class *routing_table, 
				   routing_table_class *routing_table_new,
				   int from_node)
{
  int i;
  int rule_index;

  routing_table_copy_configuration(routing_table, routing_table_new);

  // search new rules in system table
  for(i=0; i<routing_table_new->table_size; i++)
    {
      rule_index=routing_table_find_rule(routing_table, 
					 routing_table_new->from_nodes[i],
					 routing_table_new->to_nodes[i]);
      if(rule_index>=0)
	{
	  // rule exists, therefore check next hop to see if 
	  // any changes are required
	  if(routing_table->next_hops[rule_index] != 
	     routing_table_new->next_hops[i])
	    {
	      // rule has a different next_hop, therefore a
	      // change is required
	      INFO("Rule from %d to %d already defined (index=%d). \
Changing...", routing_table_new->from_nodes[i], 
	       routing_table_new->to_nodes[i], rule_index);

	      // only do kernel changes for the rules that correspond 
	      // to the current PC
	      if(routing_table->from_nodes[rule_index] == from_node)
		routing_table_unwrite_rule(routing_table, rule_index);
	      
	      routing_table->next_hops[rule_index] = 
		routing_table_new->next_hops[i];
	      
	      // only do kernel changes for the rules that correspond 
	      // to the current PC
	      if(routing_table->from_nodes[rule_index] == from_node)
		if(routing_table_write_rule(routing_table, rule_index)==ERROR)
		  {
		    WARNING("Cannot write routing rule to kernel");
		    return ERROR;
		  }
	    }
	  else
	    INFO("Rule from %d to %d already defined (index=%d) and \
is the same. Not changing...", routing_table_new->from_nodes[i], 
	       routing_table_new->to_nodes[i], rule_index);

	}
      else
	{
	  INFO("Rule from %d to %d not defined. Merging...",
	       routing_table_new->from_nodes[i], 
	       routing_table_new->to_nodes[i]);

	  // rule doesn't exist, therefore it must be added
	  routing_table_add_rule(routing_table, 
				 routing_table_new->from_nodes[i],
				 routing_table_new->to_nodes[i], 
				 routing_table_new->next_hops[i]);

	  // only do kernel changes for the rules that correspond 
	  // to the current PC
	  if(routing_table->from_nodes[routing_table->table_size-1] == 
	     from_node)
	    if(routing_table_write_rule(routing_table, 
					routing_table->table_size-1) == ERROR)
	      {
		WARNING("Cannot write routing rule to kernel");
		return ERROR;
	      }
	}      
    }
  
  return SUCCESS;
}

// main function
int main(int argc, char *argv[])
{
  char cmd_option;
  int pc_id=-2;
  FILE *routing_file=NULL;
  char routing_file_name[255];
  int routing_file_name_provided=FALSE;
  FILE *routing_table_file=NULL;
  char routing_table_file_name[255];
  float crt_time=0, next_time=0, sleep_time=0;
  int line_number = 0;
  int read_items;

  xml_routing_table_class xml_routing_table;  
  // new routing table that needs to be merged 
  // with the system routing table
  routing_table_class *routing_table_new = &(xml_routing_table.routing_table);

  // system routing table
  routing_table_class routing_table;


  // init system routing table
  routing_table_clear(&routing_table);

  while((cmd_option = getopt(argc, argv, "12345f:")) != -1) 
    {
      switch(cmd_option)
	{
	  case '1':
	    pc_id = 1;
	    break;
	  case '2':
	    pc_id = 2;
	    break;
	  case '3':
	    pc_id = 3;
	    break;
	  case '4':
	    pc_id = 4;
	    break;
	  case '5':
	    pc_id = 5;
	    break;
	  case 'f':
	    strncpy(routing_file_name, optarg, 254);
	    routing_file_name_provided = TRUE;
	    break;
	  default:
	    pc_id=-1;
	}
    }
  
  if(pc_id==-2)
    {
      printf("PC id not specified\n");
      return -2;
    }
  else if(pc_id==-1)
    {
      printf("Unknown PC id\n");
      return -1;
    }

  if(routing_file_name_provided == FALSE)
    {
      printf("Routing file name not specified\n");
      return -3;
    }

  // open routing file
  INFO("Opening routing information file '%s'...", routing_file_name);
  routing_file = fopen(routing_file_name, "r");
  if(routing_file==NULL)
    {
      WARNING("Cannot open routing file '%s'!", routing_file_name);
      goto ERROR_HANDLE;
    }

  while(!feof(routing_file))
    {
      line_number++;
      read_items=fscanf(routing_file, "%f %254s", &next_time, 
			routing_table_file_name);

      if(read_items==1 || read_items>3)//read too few or too many items
	{
	  WARNING("Error reading line #%d of routing information file '%s'", 
		  line_number, routing_file_name);
	  break;
	}
      else if(read_items!=2)// probably EOF or empty line => ignore
	continue;

      printf("Read data (line #%d): next_time=%.2f file name='%s'\n", 
	     line_number, next_time, routing_table_file_name);

      sleep_time=(next_time-crt_time);
      if(sleep_time<0)
	{
	  WARNING("Error computing sleeping time");
	  break;
	}
      INFO("Current time=%.2f s; sleeping %.2f s...", crt_time, sleep_time);
      usleep(sleep_time*1e6);
      crt_time+=sleep_time;


      // open routing table file
      routing_table_file = fopen(routing_table_file_name, "r");
      if(routing_table_file==NULL)
	{
	  WARNING("Cannot open routing table file '%s'!", 
		  routing_table_file_name);
	  goto ERROR_HANDLE;
	}
      
      // parse routing table file
      if(xml_routing_table_parse(routing_table_file, &xml_routing_table))
	{
	  WARNING("Cannot parse routing table file '%s'!", 
		  routing_table_file_name);
	  goto ERROR_HANDLE;
	}

      INFO("System routing table");
      routing_table_print(&routing_table);
      INFO("New routing table");
      routing_table_print(routing_table_new);

      printf("Writing QOMET routing table to kernel...\n");
      
      if(merge_routing_tables_and_write(&routing_table, 
					routing_table_new, pc_id)==ERROR)
	{
	  WARNING("Cannot merge routing table information");
 	  goto ERROR_HANDLE;
 	}

    }

  INFO("No more routing configurations. Exiting...");
  fclose(routing_file);
  return 0;

    
ERROR_HANDLE:

  if(routing_file != NULL)
    fclose(routing_file);

  return -1;
}
