
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
 * File name: generate_aodv.c
 * Function: Source file of the AODV route generation application
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

#include "message.h"
#include "qomet_routing.h"
//#include "../qomet.h"
#include "../chanel/io_chanel.h"

#define TRUE  1
#define FALSE 0
#define MAX_STRING  256

#define MAX_NODES     1000
#define MAX_METRIC  1000000


typedef struct
{
  int node_matrix[MAX_NODES][MAX_NODES];
  int node_is_active[MAX_NODES];
}graph_class;
/*
int graph[NMAX][NMAX]={{FALSE, TRUE, TRUE, TRUE},
		       {FALSE, FALSE, TRUE, TRUE},
		       {FALSE, FALSE, FALSE, TRUE},
		       {FALSE, FALSE, FALSE, FALSE}};
*/		       

void graph_clear(graph_class *graph)
{
  int i,j;

  for(i=0; i<MAX_NODES; i++)
    {
      graph->node_is_active[i]=FALSE;
      for(j=0; j<MAX_NODES; j++)
	graph->node_matrix[i][j]=FALSE;
    }
}

void graph_print(graph_class *graph)
{
  int i,j;
  int first_line=TRUE;

  for(i=0; i<MAX_NODES; i++)
    {
      if(graph->node_is_active[i]==FALSE)
	continue;

      // print first line if necessary
      if(first_line==TRUE)
    	{
	  printf(" ");

	  for(j=0; j<MAX_NODES; j++)
	    {
	      if(graph->node_is_active[j]==FALSE)
		continue;
	      
	      printf(" %d", j);
	    }
	  printf("\n");
	  first_line=FALSE;
	}

      // print current line
      printf("%d",i);

      for(j=0; j<MAX_NODES; j++)
	{
	  if(graph->node_is_active[j]==FALSE)
	    continue;

	  printf(" %c", (graph->node_matrix[i][j]==TRUE)?'O':'.');
	}

      printf("\n");
    }
}

int compute_route(graph_class *graph, int traversed[MAX_NODES],
		  int n_from, int n_to, int* n_hop, int *metric)
{
  int n;
  int call_n_hop, call_metric;
  int save_n_hop, save_metric;

  // HOW TO PREVENT LOOPS??? => pass array of n nodes in which 
  // each traversed node is marked
  printf("\tsub-route from node '%d' to node '%d'...\n", n_from, n_to);
  if(graph->node_matrix[n_from][n_to]==TRUE)
    {
      (*n_hop) = n_to;
      (*metric) = 1;
      printf("\t\tdirect route found\n");
      return TRUE;
    }
  else
    {
      traversed[n_from]=TRUE;
      save_metric=MAX_METRIC;
      for(n=0;n<MAX_NODES;n++)
	{
	  if(graph->node_is_active[n]==FALSE)
	    continue;

	  if(graph->node_matrix[n_from][n]==TRUE && traversed[n]==FALSE)
	    {
	      if(compute_route(graph, traversed, n, n_to, 
			       &call_n_hop, &call_metric)==TRUE)
		{
		  // risk to overload the same node; perhaps use random 
		  // selection if metric is equal???
		  if(call_metric<save_metric)
		    {
		      if(save_metric!=MAX_METRIC)
			printf("\t\tdiscarding node '%d' since metric its \
'%d' is larger than for '%d': new '%d'\n", save_n_hop, save_metric, n, 
			       call_metric);
		      save_n_hop = n;
		      save_metric = call_metric;
		    }
		  else
		    printf("\t\tignoring node '%d' since metric its '%d' \
is not larger than for '%d': old '%d'\n", n, call_metric, save_n_hop, 
			   save_metric);
		}
	    }
	}
      if(save_metric!=MAX_METRIC)
	{
	  (*n_hop) = save_n_hop;
	  (*metric) = save_metric+1;
	  printf("\t\tindirect route found\n");
	  return TRUE;
	}
      else
	{
	  printf("\t\tno route found\n");
	  return FALSE;
	}
    }
}

int routing_table_write2file(routing_table_class *routing_table, double time, char *base_name, char *routing_table_file_name)
{
  FILE *file;
  char filename[MAX_STRING];
  int i;

  snprintf(filename, MAX_STRING-1, "%s.rt_%08.3f.xml", base_name, time);
  file = fopen(filename, "w");

  if(file == NULL)
    {
      WARNING("ERROR: Cannot open file '%s'", filename);
      return ERROR;
    }

  INFO("Outputing %d rules...", routing_table->table_size);
  fprintf(file, "<routing_table>\n\n");

  for(i=0; i<routing_table->table_size; i++)
    fprintf(file, 
	    "  <route from_node=\"%d\" to_node=\"%d\" next_hop=\"%d\"/>\n",
	    routing_table->from_nodes[i], routing_table->to_nodes[i], 
	    routing_table->next_hops[i]);

  fprintf(file, "\n</routing_table>\n");

  strncpy(routing_table_file_name, filename, MAX_STRING-1);

  return SUCCESS;
}

int aodv_is_connected(float loss_rate)
{
  return (loss_rate<0.5)?TRUE:FALSE;
}

int main(int argc, char *argv[])
{
  //char cmd_option;

  int nf,nt;

  graph_class graph;

  int traversed[MAX_NODES];
  int i;
  int n_hop, metric;
  routing_table_class routing_table;
  routing_table_class old_routing_table;

  FILE *qomet_binary_output_file=NULL;
  char qomet_binary_output_file_name[MAX_STRING];
  int qomet_binary_output_file_name_provided=FALSE;
  binary_header_class binary_header;
  binary_time_record_class binary_time_record;
  binary_record_class binary_record;
  long int time_i, rec_i;

  FILE *routing_file=NULL;
  char routing_file_name[MAX_STRING];
  char routing_table_file_name[MAX_STRING];
  /*
  while((cmd_option = getopt(argc, argv, "f:")) != -1) 
    {
      switch(cmd_option)
	{
  	  case 'f':
	    strncpy(qomet_binary_output_file_name, optarg, MAX_STRING-1);
	    qomet_binary_output_file_name_provided = TRUE;
	    break;
	}
    }
  */

  if(argc<=1)
    {
      WARNING("No QOMET binary output file was provided");
      WARNING("Usage: %s <qomet_binary_output_file>", argv[0]);
      return ERROR;
    }
  else
    {
      strncpy(qomet_binary_output_file_name, argv[1], MAX_STRING-1);
      qomet_binary_output_file_name_provided = TRUE;
    }

  qomet_binary_output_file = fopen(qomet_binary_output_file_name, "r");

  // read header from file
  if(qomet_binary_output_file==NULL)
    {
      WARNING("Cannot open binary output file '%s'", 
	      qomet_binary_output_file_name);
      return ERROR;
    }

  snprintf(routing_file_name, MAX_STRING-1, "%s.rt_file", 
	   qomet_binary_output_file_name);  

  routing_file = fopen(routing_file_name, "w");

  // read header from file
  if(routing_file==NULL)
    {
      WARNING("Cannot open output routing file '%s'", 
	      routing_file_name);
      return ERROR;
    }


  // read header from file
  if(fread(&binary_header, sizeof(binary_header_class),
	   1, qomet_binary_output_file)!=1)
    {
      WARNING("Error reading binary output header from file");
      return ERROR;
    }

  /*
  INFO("Input file '%s':", qomet_binary_output_file_name); 
  io_binary_print_header(&binary_header);
  */
  INFO("Input file '%s': QOMET v%d.%d  time_record_number=%ld", 
       qomet_binary_output_file_name, binary_header.major_version, 
       binary_header.minor_version, 
       binary_header.time_record_number);


  for(time_i=0; time_i<binary_header.time_record_number; time_i++)
    {
      // read time record from file
      if(fread(&binary_time_record, 
	       sizeof(binary_time_record_class),
	       1, qomet_binary_output_file)!=1)
	{
	  WARNING("Error reading binary output time record from file");
	  return ERROR;
	}
      INFO("\ttime=%f record_number=%d", binary_time_record.time,
	   binary_time_record.record_number);
  
      routing_table_clear(&routing_table);
      graph_clear(&graph);

      for(rec_i=0; rec_i<binary_time_record.record_number; rec_i++)
	{
	  // read record from file
	  if(fread(&binary_record, sizeof(binary_record_class),
		   1, qomet_binary_output_file)!=1)
	    {
	      WARNING("Error reading binary output record from file");
	      return ERROR;
	    }
	  INFO("\t\tfrom_node=%d to_node=%d loss_rate=%f", 
	       binary_record.from_node, binary_record.to_node,
	       binary_record.loss_rate);
	  if(binary_record.from_node<MAX_NODES && 
	     binary_record.to_node<MAX_NODES)
	    {
	      int aodv = aodv_is_connected(binary_record.loss_rate);

	      INFO("AODV is connected=%s", (aodv==TRUE)?"TRUE":"FALSE");
	      graph.node_matrix[binary_record.from_node]
		[binary_record.to_node]=aodv;
	      graph.node_is_active[binary_record.from_node]=TRUE;
	      graph.node_is_active[binary_record.to_node]=TRUE;
	    }
	  else
	    WARNING("Index of from_node (%d) or to_node (%d) exceeded upper \
bound %d", binary_record.from_node, binary_record.to_node, 
		    MAX_NODES);
	}

      graph_print(&graph);

      for(nf=0;nf<MAX_NODES;nf++)
	for(nt=0;nt<MAX_NODES;nt++)
	  {
	    if(graph.node_is_active[nf]==FALSE || 
	       graph.node_is_active[nt]==FALSE)
	      continue;

	    if(nf!=nt)
	      {
		for(i=0;i<MAX_NODES; i++)
		  traversed[i]=FALSE;
	      
		printf("\nComputing route from node '%d' to node '%d'...\n", 
		       nf, nt);
		if(compute_route(&graph, traversed, nf, nt, 
				 &n_hop, &metric)==TRUE)
		  {
		    printf("Route settings: next_hop=%d metric=%d \
(number hops)\n", n_hop, metric);
		    printf("nn=%d\n", routing_table.table_size);
		    routing_table_add_rule(&routing_table, nf, nt, n_hop);
		    printf("nn=%d\n", routing_table.table_size);
		  }
		else
		  printf("Route settings: NONE\n");
	      }
	  }

      if(routing_tables_are_equal(&routing_table, &old_routing_table)==FALSE)
	{
	  if(routing_table_write2file(&routing_table, 
				      binary_time_record.time,
				      qomet_binary_output_file_name, 
				      routing_table_file_name)==SUCCESS)
	    {
	      fprintf(routing_file, "% 8.3f   %s\n", 
		      binary_time_record.time, routing_table_file_name);
	    }
	  routing_table_copy(&old_routing_table, &routing_table);
	}
	  
    }

  if(qomet_binary_output_file!=NULL)
    fclose(qomet_binary_output_file);

  if(routing_file!=NULL)
    fclose(routing_file);

  return SUCCESS;
}
