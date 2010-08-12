
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
 * File name: qomet_routing.c
 * Function: Source file of the QOMET routing application that 
 *           configures system routing tables
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 128 $
 *   $LastChangedDate: 2009-02-06 10:21:50 +0900 (Fri, 06 Feb 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#include <stdio.h>
#include "routing.h"
#include "qomet_routing.h"
#include "message.h"

// copy configuration of routing table
void routing_table_copy_configuration(routing_table_class *routing_table_dst, 
				      routing_table_class *routing_table_src)
{
  routing_table_dst->number_nodes = routing_table_src->number_nodes;
  strncpy(routing_table_dst->alias_ip_head, 
	  routing_table_src->alias_ip_head, 
	  MAX_IP_ADDRESS_LENGTH);
  strncpy(routing_table_dst->tunnel_ip_head, 
	  routing_table_src->tunnel_ip_head, 
	  MAX_IP_ADDRESS_LENGTH);
}

// copy routing table
void routing_table_copy(routing_table_class *routing_table_dst, 
			routing_table_class *routing_table_src)
{
  int i;
  
  routing_table_copy_configuration(routing_table_dst,
				   routing_table_src);

  for(i=0; i<routing_table_src->table_size; i++)
    {
      routing_table_dst->from_nodes[i] = routing_table_src->from_nodes[i];
      routing_table_dst->to_nodes[i] = routing_table_src->to_nodes[i];
      routing_table_dst->next_hops[i] = routing_table_src->next_hops[i];
    }

  routing_table_dst->table_size = routing_table_src->table_size;
}

// check whether two routing tables are equal;
// return TRUE on true, FALSE on false
int routing_tables_are_equal(routing_table_class *routing_table1, 
			      routing_table_class *routing_table2)
{
  int i;
  
  if(routing_table1->table_size != routing_table2->table_size)
    return FALSE;
  else
    for(i=0; i<routing_table1->table_size; i++)
      if(routing_table1->from_nodes[i] != routing_table2->from_nodes[i] ||
	 routing_table1->to_nodes[i] != routing_table2->to_nodes[i] ||
	 routing_table1->next_hops[i] != routing_table2->next_hops[i])
	return FALSE;

  return TRUE;
}

// find a rule in a routing table;
// if route is found return its index, otherwise return -1
int routing_table_find_rule(routing_table_class *routing_table, 
			    int from_node, int to_node)
{
  int i;

  for(i=0; i<routing_table->table_size; i++)
    if(routing_table->from_nodes[i] == from_node &&
       routing_table->to_nodes[i] == to_node)
      return i;

  return -1;
}

// clear a routing table
void routing_table_clear(routing_table_class *routing_table)
{
  routing_table->table_size = 0;
}

// init a routing table;
// return SUCCESS on success, ERROR on error
int routing_table_init(routing_table_class *routing_table, int number_nodes,
		       int *from_nodes, int *to_nodes, int *next_hops, 
		       int table_size)
{
  int i;

  if(table_size>MAX_ROUTING_TABLE)
    {
      WARNING("Table size (%d) exceeds maximum limit (%d)", table_size, MAX_ROUTING_TABLE);
      return ERROR;
    }

  routing_table->number_nodes = number_nodes;

  for(i=0; i<table_size; i++)
    {
      routing_table->from_nodes[i] = from_nodes[i];
      routing_table->to_nodes[i] = to_nodes[i];
      routing_table->next_hops[i] = next_hops[i];
    }

  routing_table->table_size = table_size;

  return SUCCESS;
}

// compute modulo 'modulo' difference of 'n1' and 'n2';
// return the result
int modulo_difference(int n1, int n2, int modulo)
{
  if(n1>=n2)
    return n1-n2;
  else
    return n1-n2+modulo;
}

// write routing table to system;
// return SUCCESS on success, ERROR on error
int routing_table_write(routing_table_class *routing_table)
{
  int i;

  for(i=0; i<routing_table->table_size; i++)
    if(routing_table_write_rule(routing_table, i)==ERROR)
      {
	WARNING("Cannot write rule");
	return ERROR;
      }

  return SUCCESS;
}

// unwrite (remove) routing table from system;
// return SUCCESS on success, ERROR on error
int routing_table_unwrite(routing_table_class *routing_table)
{
  int i;

  for(i=0; i<routing_table->table_size; i++)
    if(routing_table_unwrite_rule(routing_table, i)==ERROR)
      {
	WARNING("Cannot ""unwrite"" rule");
	return ERROR;
      }

  return SUCCESS;
}

// write routing table information corresponding to the 'from_node' to system;
// return SUCCESS on success, ERROR on error
int routing_table_write_from_node(routing_table_class *routing_table, 
				  int from_node)
{
  int i;
  
  for(i=0; i<routing_table->table_size; i++)
    if(routing_table->from_nodes[i] == from_node)
      if(routing_table_write_rule(routing_table, i)==ERROR)
	{
	  WARNING("Cannot write rule");
	  return ERROR;
	}

  return SUCCESS;
}

// unwrite routing table information corresponding to the 'from_node' 
// from system;
// return SUCCESS on success, ERROR on error
int routing_table_unwrite_from_node(routing_table_class *routing_table, 
				    int from_node)
{
  int i;
  
  for(i=0; i<routing_table->table_size; i++)
    if(routing_table->from_nodes[i] == from_node)
      if(routing_table_unwrite_rule(routing_table, i)==ERROR)
	{
	  WARNING("Cannot ""unwrite"" rule");
	  return ERROR;
	}

  return SUCCESS;
}

// write routing table rule to system;
// return SUCCESS on success, ERROR on error
int routing_table_write_rule(routing_table_class *routing_table, 
			     int rule_index)
{
  int j;
  
  char src_addr[MAX_IP_ADDRESS_LENGTH];
  char dest_addr[MAX_IP_ADDRESS_LENGTH];
  //char *gw_addr = NULL;
  //char *mask_addr = NULL;
  char if_name[MAX_IP_ADDRESS_LENGTH];
  int if_number, if_number_to;

  routing_table_print(routing_table);

  snprintf(src_addr, MAX_IP_ADDRESS_LENGTH-1, "%s.%d.%d", 
	   routing_table->tunnel_ip_head, 
	   routing_table->from_nodes[rule_index], 0); 
  snprintf(dest_addr, MAX_IP_ADDRESS_LENGTH-1, "%s.%d", 
	   routing_table->alias_ip_head, routing_table->to_nodes[rule_index]); 
  if_number=modulo_difference(routing_table->next_hops[rule_index], 
			      routing_table->from_nodes[rule_index], 
			      routing_table->number_nodes)-1;

  snprintf(if_name, MAX_IP_ADDRESS_LENGTH, "gif%d", if_number);

  printf("*** adding new route\n");
  printf("\t%s -> %s ----> %s\n", src_addr, if_name, dest_addr);

  if(rtctl_add(dest_addr, NULL, NULL, if_name) < 0)
    {
      WARNING("Error configuring routing table\n");
      return ERROR;
    }

  if(routing_table->next_hops[rule_index] != 
     routing_table->to_nodes[rule_index])// indirect connection
    {
      if_number_to=modulo_difference(routing_table->from_nodes[rule_index], 
				     routing_table->to_nodes[rule_index], 
				     routing_table->number_nodes)-1;
      for(j=0; j<routing_table->number_nodes-1; j++)
	if(j!=if_number_to)
	  {
	    //snprintf(src_addr, MAX_IP_ADDRESS_LENGTH-1, "%d.%d.%d.%d", 
	    //	 10, 3, routing_table->from_nodes[rule_index], 0); 
	    snprintf(dest_addr, MAX_IP_ADDRESS_LENGTH-1,  "%s.%d.%d", 
		     routing_table->tunnel_ip_head, 
		     routing_table->to_nodes[rule_index], j); 

	    printf("*** adding new route (additional)\n");
	    printf("\t%s -> %s ----> %s\n", src_addr, if_name, dest_addr);

	    if(rtctl_add(dest_addr, NULL, NULL, if_name) < 0)
	      {
		WARNING("Error configuring routing table\n");
		return ERROR;
	      }
	  }
    }

  return SUCCESS;
}

// unwrite routing table rule from system;
// return SUCCESS on success, ERROR on error
int routing_table_unwrite_rule(routing_table_class *routing_table, 
			       int rule_index)
{
  int j;
  
  char src_addr[MAX_IP_ADDRESS_LENGTH];
  char dest_addr[MAX_IP_ADDRESS_LENGTH];

  char if_name[MAX_IP_ADDRESS_LENGTH];
  int if_number, if_number_to;

  snprintf(src_addr, MAX_IP_ADDRESS_LENGTH-1,  "%s.%d.%d", 
	   routing_table->tunnel_ip_head, 
	   routing_table->from_nodes[rule_index], 0); 
  snprintf(dest_addr, MAX_IP_ADDRESS_LENGTH-1, "%s.%d", 
	   routing_table->alias_ip_head, routing_table->to_nodes[rule_index]); 
  if_number=modulo_difference(routing_table->next_hops[rule_index], 
			      routing_table->from_nodes[rule_index], 
			      routing_table->number_nodes)-1;

  snprintf(if_name, MAX_IP_ADDRESS_LENGTH, "gif%d", if_number);

  printf("*** deleting potential previous routes\n");
  printf("\t%s -> %s ----> %s\n", src_addr, if_name, dest_addr);

  if(rtctl_delete(dest_addr, NULL, NULL, NULL) < 0)
    {
      printf("Error setting routing table\n");
      return ERROR;
    }

  if(routing_table->next_hops[rule_index] != routing_table->to_nodes[rule_index])// indirect connection
    {
      if_number_to=modulo_difference(routing_table->from_nodes[rule_index], 
				     routing_table->to_nodes[rule_index], 
				     routing_table->number_nodes)-1;
      for(j=0; j<routing_table->number_nodes-1; j++)
	if(j!=if_number_to)
	  {
	    snprintf(dest_addr, MAX_IP_ADDRESS_LENGTH-1, "%s.%d.%d", 
		     routing_table->tunnel_ip_head, 
		     routing_table->to_nodes[rule_index], j); 

	    printf("*** deleting potential previous routes (additional)\n");
	    printf("\t%s -> %s ----> %s\n", src_addr, if_name, dest_addr);

	    if (rtctl_delete(dest_addr, NULL, NULL, NULL) < 0)
	      {
		printf("Error setting routing table\n");
		return ERROR;
	      }
	  }
    }

  return SUCCESS;
}

// add rule to routing table using detailed parameters;
// return SUCCESS on success, ERROR on error
int routing_table_add_rule(routing_table_class *routing_table, 
			   int from_node, int to_node, int next_hop)
{
  if(routing_table->table_size == MAX_ROUTING_TABLE)
    {
      WARNING("Routing table is full");
      return ERROR;
    }
  
  routing_table->from_nodes[routing_table->table_size] = from_node;
  routing_table->to_nodes[routing_table->table_size] = to_node;
  routing_table->next_hops[routing_table->table_size] = next_hop;
  
  routing_table->table_size++;

  return SUCCESS;
}

// add rule to routing table using a routing_rule_class structure;
// return SUCCESS on success, ERROR on error
int routing_table_add_rulec(routing_table_class *routing_table, 
			    routing_rule_class *routing_rule)
{
  if(routing_table->table_size == MAX_ROUTING_TABLE)
    {
      WARNING("Routing table is full"); 
      return ERROR;
    }
  
  routing_table->from_nodes[routing_table->table_size] = 
    routing_rule->from_node;
  routing_table->to_nodes[routing_table->table_size] = 
    routing_rule->to_node;
  routing_table->next_hops[routing_table->table_size] = 
    routing_rule->next_hop;
  
  routing_table->table_size++;

  return SUCCESS;
}

// print routing table
void routing_table_print(routing_table_class *routing_table)
{
  int i;

  printf("Routing table contents (number_nodes=%d table_size=%d \
alias_ip_head='%s' tunnel_ip_head='%s'):\n", routing_table->number_nodes, 
	 routing_table->table_size, routing_table->alias_ip_head, 
	 routing_table->tunnel_ip_head);
  if(routing_table->table_size > 0)
    for(i=0; i<routing_table->table_size; i++)
      printf("\troute #%d: %d -> %d(next hop) ----> %d\n", i, 
	     routing_table->from_nodes[i], routing_table->next_hops[i],
	     routing_table->to_nodes[i]);
  else
    printf("\t<EMPTY>\n");
}

// print rule in routing table
void routing_rule_print(routing_rule_class *routing_rule)
{
  printf("Routing rule: %d -> %d(next hop) ----> %d\n", 
	 routing_rule->from_node, routing_rule->next_hop, 
	 routing_rule->to_node);
}
