
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
 * File name: qomet_routing.h
 * Function: Header file of the qomet_routing library
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 128 $
 *   $LastChangedDate: 2009-02-06 10:21:50 +0900 (Fri, 06 Feb 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#ifndef __QOMET_ROUTING_H
#define __QOMET_ROUTING_H


#define MAX_ROUTING_TABLE            100
#define MAX_IP_ADDRESS_LENGTH        16
#define SUCCESS                      0
#define ERROR                        -1

#define TRUE                 1
#define FALSE                  0

// structure defining the routing rule
typedef struct
{
  int from_node;
  int to_node;
  int next_hop;
} routing_rule_class;

// structure defining the routing table
typedef struct
{
  int number_nodes;
  char alias_ip_head[MAX_IP_ADDRESS_LENGTH];
  char tunnel_ip_head[MAX_IP_ADDRESS_LENGTH];

  int from_nodes[MAX_ROUTING_TABLE];
  int to_nodes[MAX_ROUTING_TABLE];
  int next_hops[MAX_ROUTING_TABLE];
  int table_size;
} routing_table_class;

// copy configuration of routing table
void routing_table_copy_configuration(routing_table_class *routing_table_dst, 
				      routing_table_class *routing_table_src);

// copy routing table
void routing_table_copy(routing_table_class *routing_table_dst, 
			routing_table_class *routing_table_src);

// check whether two routing tables are equal;
// return TRUE on true, FALSE on false
int routing_tables_are_equal(routing_table_class *routing_table1, 
			      routing_table_class *routing_table2);

// find a rule in a routing table;
// if route is found return its index, otherwise return -1
int routing_table_find_rule(routing_table_class *routing_table, 
			    int from_node, int to_node);

// clear a routing table
void routing_table_clear(routing_table_class *routing_table);

// init a routing table;
// return SUCCESS on success, ERROR on error
int routing_table_init(routing_table_class *routing_table, int number_nodes,
		       int *from_nodes, int *to_nodes, int *next_hops, 
		       int table_size);

// compute modulo 'modulo' difference of 'n1' and 'n2';
// return the result
int modulo_difference(int n1, int n2, int modulo);

// write routing table to system;
// return SUCCESS on success, ERROR on error
int routing_table_write(routing_table_class *routing_table);

// unwrite (remove) routing table from system;
// return SUCCESS on success, ERROR on error
int routing_table_unwrite(routing_table_class *routing_table);

// write routing table information corresponding to the 'from_node' to system;
// return SUCCESS on success, ERROR on error
int routing_table_write_from_node(routing_table_class *routing_table, 
				  int from_node);

// unwrite routing table information corresponding to the 'from_node' 
// from system;
// return SUCCESS on success, ERROR on error
int routing_table_unwrite_from_node(routing_table_class *routing_table, 
				    int from_node);

// write routing table rule to system;
// return SUCCESS on success, ERROR on error
int routing_table_write_rule(routing_table_class *routing_table, 
			     int rule_index);

// unwrite routing table rule from system;
// return SUCCESS on success, ERROR on error
int routing_table_unwrite_rule(routing_table_class *routing_table, 
			       int rule_index);

// add rule to routing table using detailed parameters;
// return SUCCESS on success, ERROR on error
int routing_table_add_rule(routing_table_class *routing_table, 
		      int from_node, int to_node, int next_hop);

// add rule to routing table using a routing_rule_class structure;
// return SUCCESS on success, ERROR on error
int routing_table_add_rulec(routing_table_class *routing_table, 
			    routing_rule_class *routing_rule);

// print routing table
void routing_table_print(routing_table_class *routing_table);

// print rule in routing table
void routing_rule_print(routing_rule_class *routing_rule);


#endif
