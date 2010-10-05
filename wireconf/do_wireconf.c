
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
 * File name: do_wireconf.c
 * Function: Main source file of the real-time wired-network emulator 
 *           configuration program. At the moment it can be used to 
 *           drive the netwrok emulator dummynet on FreeBSD 
 *
 * Authors: Junya Nakata, Lan Nguyen Tien, Razvan Beuran
 *
 *   $Revision: 128 $
 *   $LastChangedDate: 2009-02-06 10:21:50 +0900 (Fri, 06 Feb 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>

#include "global.h"
#include "wireconf.h"
#include "timer.h"
#include "message.h"
#include "routing_info.h"

#include "tc_util.h"

// use this define to enable support for OLSR routing
// in usage (1) type
//#define OLSR_ROUTING


/////////////////////////////////////////////
// Basic constants
/////////////////////////////////////////////

#define PARAMETERS_TOTAL        19
#define PARAMETERS_UNUSED       13

#define MIN_PIPE_ID_IN          10000
#define MIN_PIPE_ID_IN_BCAST	20000
#define MIN_PIPE_ID_OUT         30000

#define MAX_RULE_NUM            100

/////////////////////////////////////////////
// Structure to hold QOMET parameters
/////////////////////////////////////////////

typedef struct
{
  float time;
  int next_hop_id;
  float bandwidth;
  float delay;
  float lossrate;
} qomet_param;


////////////////////////////////////////////////
// Constants for making a predefined experiment
////////////////////////////////////////////////

//#define PREDEFINED_EXPERIMENT          // enable predefined experiment
#define BANDWIDTH               10e6   // bandwidth in bps
#define DELAY                   5      // delay in ms 
#define PACKET_LOSS_RATE        0      // loss rate in range [0,1]
#define LOOP_COUNT              4      // number of iterations


///////////////////////////////////
// Generic variables and functions
///////////////////////////////////

// print a brief usage of this program
void usage(char *argv0)
{
  fprintf(stderr, "\ndo_wireconf. Drive network emulation using QOMET \
data.\n\n");
  fprintf(stderr, "do_wireconf can use the QOMET data in <qomet_output_file> \
in two different ways:\n");
  fprintf(stderr, "(1) Configure a specified pair <from_node_id> (IP address \
<from_node_addr>)\n");
  fprintf(stderr, "    <to_node_id> (IP address <to_node_addr>) using the \
dummynet rule\n");
  fprintf(stderr, "    <rule_number> and pipe <pipe_number>, optionally in \
direction 'in' or 'out'.\n");
  fprintf(stderr, "    Usage: %s -q <qomet_output_file>\n"
	  "\t\t\t-f <from_node_id> \t-F <from_node_addr>\n"
	  "\t\t\t-t <to_node_id> \t-T <to_node_addr>\n"
	  "\t\t\t-r <rule_number> \t-p <pipe_number>\n"
	  "\t\t\t[-d {in|out}]\n", argv0);
  fprintf(stderr, "(2) Configure all connections from the current node \
<current_id> given the\n");
  fprintf(stderr, "    IP settings in <settings_file> and the OPTIONAL \
broadcast address\n");
  fprintf(stderr, "    <broadcast_address>; Interval between configurations \
is <time_period>.\n");
  fprintf(stderr, "    The settings file contains on each line pairs of ids \
and IP addresses.\n");
  fprintf(stderr, "    Usage: %s -q <qomet_output_file>\n"
	  "\t\t\t-i <current_id> \t-s <settings_file>\n"
	  "\t\t\t-m <time_period> \t[-b <broadcast_address>]\n", argv0);
  fprintf(stderr, "NOTE: If option '-s' is used, usage (2) is inferred, \
otherwise usage (1) is assumed.\n");
}

// read settings (node ids and corresponding IP adresses)
// from a file, and store the adresses in the array p at
// the corresponding index;
// return the number of addresses successfully read, 
// or -1 on ERROR
int read_settings(char *path, in_addr_t *p, int p_size)
{
  static char buf[BUFSIZ];
  int i=0;
  int line_nr=0;
  FILE *fd;

  int node_id;
  char node_ip[IP_ADDR_SIZE];

  if((fd = fopen(path, "r")) == NULL)
    {
      WARNING("Cannot open settings file '%s'", path);
      return -1;
    }

  while(fgets(buf, BUFSIZ, fd) != NULL)
    {
      line_nr++;

      if(i>=p_size)
	{
	  WARNING("Maximum number of IP addresses (%d) exceeded",
		  p_size);
	  fclose(fd);
	  return -1;
	}
      else
	{
	  int scaned_items;
	  scaned_items=sscanf(buf, "%d %16s", &node_id, node_ip);
	  if(scaned_items<2)
	    {
	      WARNING("Skipped invalid line #%d in settings file '%s'", 
		      line_nr, path);
	      continue;
	    }
	  if(node_id<0 || node_id<FIRST_NODE_ID || node_id >= MAX_NODES)
	    {
	      WARNING("Node id %d is not within the permitted range [%d, %d]",
		      node_id, FIRST_NODE_ID, MAX_NODES);
	      fclose(fd);
	      return -1;
	    }
	  if((p[node_id] = inet_addr(node_ip)) != INADDR_NONE)
	    i++;
	}
    }

  fclose(fd);
  return i;
}

// lookup parameters in QOMET parameter table
/*********************************************************
 * Input: next hop id, pointer to param_table(p),
 *        the number of rules in param table (rule_count)
 * Output: the id of rule in param_table or -1 if fail
 *********************************************************/
int lookup_param(int next_hop_id, qomet_param *p, int rule_count)
{
  int i;
  for (i = 0; i < rule_count; i++)
    {
      if (p[i].next_hop_id == next_hop_id)
	return i;
    }

  return -1;
}

// dump QOMET parameter table
void dump_param_table(qomet_param *p, int rule_count)
{
  int i;
  INFO("Dumping the param_table (rule count=%d)...", rule_count);
  for (i = 0; i < rule_count; i++)
    {
      INFO("%f \t %d \t %f \t %f \t %f", p[i].time, p[i].next_hop_id, 
	   p[i].bandwidth, p[i].delay, p[i].lossrate);
    }
}

// main function
int main(int argc, char *argv[])
{
  char ch, *p, *argv0, *faddr, *taddr, buf[BUFSIZ];
  int s, fid, tid, from, to, pipe_nr;
  uint16_t rulenum;

  int usage_type;

  float time, dummy[PARAMETERS_UNUSED], bandwidth, delay, lossrate;
  FILE *fd;
  timer_handle *timer;
  int loop_count=0;

  int direction=DIRECTION_BOTH;

  struct timeval tp_begin, tp_end;
  
  int i, my_id;
  in_addr_t IP_addresses[MAX_NODES];
  char IP_char_addresses[MAX_NODES*IP_ADDR_SIZE];
  int node_number;

  int j, offset_num, rule_count, next_hop_id, rule_num;
  float time_period, next_time;
  qomet_param param_table[MAX_RULE_NUM];
  qomet_param param_over_read;
  unsigned int over_read;
  char broadcast_address[IP_ADDR_SIZE];

  usage_type = 1;
  
  // initializing the param_table
  memset(param_table, 0, sizeof(qomet_param)*MAX_RULE_NUM);
  memset(&param_over_read, 0, sizeof(qomet_param));
  over_read = 0;
  rule_count = 0;
  next_time = 0.0;
  next_hop_id = 0;
  rule_num = -1;

  // init variables to invalid values
  argv0 = argv[0];
  fd = NULL;

  faddr = taddr = NULL;
  fid = tid = pipe_nr = -1;
  rulenum = 65535;

  my_id = -1;
  node_number = -1;
  time_period = -1;
  strncpy(broadcast_address, "255.255.255.255", IP_ADDR_SIZE);


  if(argc<2)
    {
      WARNING("No arguments provided");
      usage(argv0);
      exit(1);
    }

  // parse command-line options
  while((ch = getopt(argc, argv, "q:f:F:t:T:r:p:d:i:s:m:b:")) != -1)
    {
      switch(ch)
	{
	  //QOMET output file
	case 'q':
	  if((fd = fopen(optarg, "r")) == NULL) 
	    {
	      WARNING("Could not open QOMET output file '%s'", optarg);
	      exit(1);
	    }
	  break;

	  /////////////////////////////////////////////
	  // Usage (1) parameters
	  /////////////////////////////////////////////
	  // from_node_id
	case 'f':
	  fid = strtol(optarg, &p, 10);
	  if((*optarg == '\0') || (*p != '\0'))
	    {
	      WARNING("Invalid from_node_id '%s'", optarg);
	      exit(1);
	    }
		// add my_id Fix me
		my_id = fid;
		// add node_number tmp Fix me
		node_number = 2;
	  break;

	  // IP address of from_node
	case 'F':
	  faddr = optarg;
	  break;

	  // to_node_id
	case 't':
	  tid = strtol(optarg, &p, 10);
	  if((*optarg == '\0') || (*p != '\0'))
	    {
	      WARNING("Invalid to_node_id '%s'", optarg);
	      exit(1);
	    }
	  break;

	  // IP address of to_node
	case 'T':
	  taddr = optarg;
	  break;

	  // rule number for dummynet configuration
	case 'r':
	  rulenum = strtol(optarg, &p, 10);
	  if((*optarg == '\0') || (*p != '\0'))
	    {
	      WARNING("Invalid rule_number '%s'", optarg);
	      exit(1);
	    }
	  break;

	  // pipe number for dummynet configuration
	case 'p':
	  pipe_nr = strtol(optarg, &p, 10);
	  if((*optarg == '\0') || (*p != '\0'))
	    {
	      WARNING("Invalid pipe_number '%s'", optarg);
	      exit(1);
	    }
	  break;

	  // direction option for dummynet configuration
	case 'd':
	  if(strcmp(optarg, "in")==0)
	    {
	      direction = DIRECTION_IN;
	    }
	  else if(strcmp(optarg, "out")==0)
	    {
	      direction = DIRECTION_OUT;
	    }
	  else
	    {
	      WARNING("Invalid direction '%s'", optarg);
	      exit(1);
	    }
	  break;

	  /////////////////////////////////////////////
	  // Usage (2) parameters
	  /////////////////////////////////////////////
	  // current node ID
	case 'i':
	  my_id = strtol(optarg, NULL, 10);
	  break;

	  // settings file
	case 's':
	  usage_type = 2;
	  if((node_number = read_settings(optarg, IP_addresses, MAX_NODES)) < 1)
	    {
	      WARNING("Settings file '%s' is invalid", optarg);
	      exit(1);
	    }
	  for(i = 0; i < node_number; i++)
	    snprintf(IP_char_addresses + i * IP_ADDR_SIZE, IP_ADDR_SIZE, 
		     "%hu.%hu.%hu.%hu",
		     *(((uint8_t *)&IP_addresses[i]) + 0),
		     *(((uint8_t *)&IP_addresses[i]) + 1),
		     *(((uint8_t *)&IP_addresses[i]) + 2),
		     *(((uint8_t *)&IP_addresses[i]) + 3));
	  break;

	  // time interval between settings
	case 'm':
	  // check if conversion was performed (we assume a time
	  // period of 0 is also invalid)
	  if ((time_period = strtod(optarg,NULL)) == 0)
	    {
	      WARNING("Invalid time period");
	      exit(1);
	    }
	  break;

	case 'b':
	  strncpy(broadcast_address, optarg, IP_ADDR_SIZE);
	  break;

	  // help output
	case '?':
	default:
	  usage(argv0);
	  exit(1);
	}
    }

  if((my_id<FIRST_NODE_ID) || (my_id>=node_number+FIRST_NODE_ID))
  {
    WARNING("Invalid ID '%d'. Valid range is [%d, %d]", my_id,
	    FIRST_NODE_ID, node_number+FIRST_NODE_ID-1);
    exit(1);
  }

  // update argument-related counter and pointer
  argc -= optind;
  argv += optind;

  // check that all the required arguments were provided
  if(fd == NULL)
    {
      WARNING("No QOMET data file was provided");
      usage(argv0);
      exit(1);
    }

  if((usage_type == 1) &&
     ((fid == -1) || (faddr == NULL) ||
      (tid == -1) || (taddr == NULL) || (pipe_nr == -1) || (rulenum == 65535)))
    {
      WARNING("Insufficient arguments were provided for usage (1)");
      usage(argv0);
      fclose(fd);
      exit(1);
    }
//  else if ((my_id == -1) || (node_number == -1) || (time_period == -1))
//    {
//      WARNING("Insufficient arguments were provided for usage (2)");
//      usage(argv0);
//      fclose(fd);
//      exit(1);
//    }


  // initialize timer
  DEBUG("Initialize timer...");
  if((timer = timer_init()) == NULL)
    {
      WARNING("Could not initialize timer");
      exit(1);
    }

  // open control socket
  DEBUG("Open control socket...");
  if((s = get_socket()) < 0)
    {
      WARNING("Could not open control socket (requires root priviledges)\n");
      exit(1);
    }

  // add pipe to dummynet in normal manner
  if(usage_type==1)
    {
      // get rule
      //get_rule(s, 123);

      INFO("Add rule #%d with pipe #%d from %s to %s", 
	   rulenum, pipe_nr, faddr, taddr);
      if(add_rule(s, rulenum, pipe_nr, faddr, taddr, direction) < 0)
	{
	  WARNING("Could not add rule #%d with pipe #%d from %s to %s", 
		  rulenum, pipe_nr, faddr, taddr);
	  exit(1);
	}

      // get rule
      //get_rule(s, 123);

      //exit(2);
    }
  else // usage (2) => sets of rules must be added
    {
      // add rule & pipe for unicast traffic _to_ j
      for(j = FIRST_NODE_ID; j < node_number + FIRST_NODE_ID; j++)
	{
	  if(j == my_id)
	    continue;

	  offset_num = j; 

	  INFO("Node %d: Add rule #%d with pipe #%d to destination %s", 
	       my_id, MIN_PIPE_ID_OUT+offset_num, MIN_PIPE_ID_OUT+offset_num, 
	       IP_char_addresses+(j-FIRST_NODE_ID)*IP_ADDR_SIZE);
	  if(add_rule(s, MIN_PIPE_ID_OUT+offset_num, MIN_PIPE_ID_OUT+offset_num,
		      "any", IP_char_addresses+(j-FIRST_NODE_ID)*IP_ADDR_SIZE, 
		      DIRECTION_OUT) < 0)
	    {
	      WARNING("Node %d: Could not add rule #%d with pipe #%d to \
destination %s", my_id, MIN_PIPE_ID_OUT + offset_num, 
		      MIN_PIPE_ID_OUT + offset_num, 
		      IP_char_addresses + (j-FIRST_NODE_ID) * IP_ADDR_SIZE);
	      exit(1);
	    }
	}// end for loop


      // add rule & pipe for broadcast traffic _from_ j
      for(j = FIRST_NODE_ID; j < node_number + FIRST_NODE_ID; j++)
	{
	  if(j == my_id)
	    continue;

	  offset_num = j; 

	  INFO("Node %d: Add rule #%d with pipe #%d to destination %s",
	       my_id, MIN_PIPE_ID_IN_BCAST + offset_num, 
	       MIN_PIPE_ID_IN_BCAST + offset_num, broadcast_address);
	  if(add_rule(s, MIN_PIPE_ID_IN_BCAST + offset_num, 
		      MIN_PIPE_ID_IN_BCAST + offset_num, 
		      IP_char_addresses+(j-FIRST_NODE_ID)*IP_ADDR_SIZE,
		      broadcast_address, DIRECTION_IN) < 0)
	    {
	      WARNING("Node %d: Could not add rule #%d with pipe #%d from %s to \
destination %s", my_id, MIN_PIPE_ID_IN_BCAST + offset_num, 
		      MIN_PIPE_ID_IN_BCAST + offset_num, 
		      IP_char_addresses+(j-FIRST_NODE_ID)*IP_ADDR_SIZE, broadcast_address);
	      exit(1);
	    }
	}
    }

  // get the time at the beginning of the experiment
  gettimeofday(&tp_begin, NULL);

  // do this only for usage (1)
  if(usage_type==1)
    {
#ifdef OLSR_ROUTING
      // get the next hop ID to find correct configuration lines
      if( (next_hop_id = get_next_hop_id(tid, direction)) == ERROR )
	{
	  WARNING("Time=%.2f: Couldn't locate the next hop for destination node %i, direction=%i", time, tid, direction);
	  exit(1);
	} 
      else
	INFO("Time=%.2f: Next_hop=%i for destination=%i", time, 
	     next_hop_id, tid);
#else
      next_hop_id = tid;
#endif
    }

  // read input data from QOMET output file
  INFO("Reading QOMET data from file...");
  next_time = 0;
  while(fgets(buf, BUFSIZ, fd) != NULL)
    {
      if(sscanf(buf, "%f %d %f %f %f %d %f %f %f %f %f %f "
		"%f %f %f %f %f %f %f", &time, &from, &dummy[0],
		&dummy[1], &dummy[2], &to, &dummy[3], &dummy[4],
		&dummy[5],  &dummy[6], &dummy[7], &dummy[8],
		&dummy[9], &dummy[10], &dummy[11], &bandwidth, 
		&lossrate, &delay, &dummy[12]) != PARAMETERS_TOTAL)
	{
	  INFO("Skipped non-parametric line");
	  continue;
	}

      if(usage_type==1)
	{
	  // check whether the from_node and to_node from the file
	  // match the user selected ones
	  if((from == fid) && (to == next_hop_id))
	    {
	      
#ifdef PREDEFINED_EXPERIMENT
	      bandwidth=BANDWIDTH;
	      delay=DELAY;
	      lossrate=PACKET_LOSS_RATE;
#endif
	      // print current configuration info
	      INFO("* Wireconf configuration (time=%.2f s): bandwidth=%.2fbit/s \
loss_rate=%.4f delay=%.4f ms", time, bandwidth, lossrate, delay);
	      
	      // if this is the first operation we reset the timer
	      // Note: it is assumed time always equals 0.0 for the first line
	      if(time == 0.0)
		timer_reset(timer);
	      else 
		{
		  // wait for for the next timer event
// debug messsage by k-akashi
//			uint64_t t;
//			rdtsc(t);
//			printf("time_now  = %llu\n", t);
//			printf("timer_now = %llu\n", timer->next_event);
//			printf("CPU Frequency = %u\n", timer->cpu_frequency);
			
		  if(timer_wait(timer, time * 1000000) < 0)
		    {
		      WARNING("Timer deadline missed at time=%.2f s", time);
		      //exit(1); // NOT NEEDED ANYMORE!!!!
		    }
		}
	      
#ifdef OLSR_ROUTING
	      // get the next hop ID to find correct configuration lines
	      if((next_hop_id = get_next_hop_id(tid, direction)) == ERROR)
		{
		  WARNING("Time=%.2f: Couldn't locate the next hop for destination node %i,direction=%i", time, tid, direction);
		  exit(1);
		}
	      else
		INFO("Time=%.2f: Next_hop=%i for destination=%i", time, next_hop_id, tid);
#endif

	      // prepare adjusted values:
	      // 1. if packet size is known bandwidth could be adjusted:
	      //   multiplication factor 1.0778 was computed for 400 byte datagrams
	      //   because of 28 bytes header (428/400=1.07)
	      //   however we consider bandwidth at Ethernet level, therefore we
	      //   don't multiply here but when plotting results
	      //   Note: ip_dummynet.h says that bandwidth is in bytes/tick
	      //         but we seem to obtain correct values using bits/second
#ifdef __FreeBSD__
	      bandwidth = (int)round(bandwidth);// * 2.56);
	      
	      // 2. no adjustment necessary for delay expressed in ms
	      delay = (int) round(delay);//(delay / 2);
	      
	      // 3. loss rate probability must be extended to 
	      //    2^31-1 (=0x7fffffff) range, equivalent to 100% loss
	      lossrate = (int)round(lossrate * 0x7fffffff);

	      // do configure pipe
	      configure_pipe(s, pipe_nr, bandwidth, delay, lossrate);
#elif __linux
		  bandwidth = (int)rint(bandwidth);// * 2.56);
		  delay = (int)rint(delay);//(delay / 2);
		  //lossrate = (int)rint(lossrate * 0x7fffffff);

		  // do configure Qdisc
		  configure_qdisc(s, taddr, pipe_nr, bandwidth, delay, lossrate);
#endif

	      // increase loop counter
	      loop_count++;
	      
#ifdef PREDEFINED_EXPERIMENT
	      if(loop_count>LOOP_COUNT)
		break;
#endif
	    }
	}
      else // usage (2) => manage multiple rules
	{
	  // add rules regarding next deadline to parameter table
	  if(time == next_time)
	    {
	      if(from == my_id)
		{
		  param_table[rule_count].time = time;
		  param_table[rule_count].next_hop_id = to;
		  param_table[rule_count].bandwidth = bandwidth;
		  param_table[rule_count].delay = delay;
		  param_table[rule_count].lossrate = lossrate;
		  rule_count++;
		}
	      continue;
	    }
	  else 
	    {
	      // check the reading link, if it is involved, 
	      // store on the param_over_read
	      if(from == my_id)
		{
		  over_read = 1;
		  param_over_read.time = time;
		  param_over_read.next_hop_id = to;
		  param_over_read.bandwidth = bandwidth;
		  param_over_read.delay = delay;
		  param_over_read.lossrate = lossrate;
		}
	      else
		over_read = 0;

	      // waiting for the next timer deadline event;
	      // if this is the first operation we reset the timer
	      // Note: it is assumed time always equals 0.0 for the first line
	      if(time == 0.0)
		timer_reset(timer);
	      else
		{
		  // wait for for the next timer event
		  if(timer_wait(timer, time * 1000000) < 0)
		    {
		      WARNING("Timer deadline missed at time=%.2f s", time);
		    }
		}

	      // when timer event comes, apply configuration for all 
	      // the unicast links with destination 'i'
	      for(i=FIRST_NODE_ID; i<(node_number+FIRST_NODE_ID); i++)
		{
		  if(i == my_id)
		    //if ( (j != 1) || (i != 2) )		// Lan optimize for the routing experiment
		    continue;

		  // get the next hop ID for destination i 
		  // to find correct configuration lines
		  if((next_hop_id = get_next_hop_id(IP_addresses, IP_char_addresses, 
						    i, DIRECTION_OUT)) == ERROR)
		    {
		      WARNING("Could not locate the next hop for destination node %i", i);
		      exit(1);
		    }
		  //else
		  //INFO("Next_hop=%i for destination=%i", next_hop_id, i);
	  
		  offset_num = i;
		  if((rule_num = lookup_param(next_hop_id, param_table, 
					      rule_count)) == -1) 
		    {
		      WARNING("Could not locate the rule number for next hop = %i", 
			      next_hop_id);
		      dump_param_table(param_table, rule_count);
		      exit(1);
		    }

		  // prepare the configuration parameters
		  time = param_table[rule_num].time;
		  bandwidth = param_table[rule_num].bandwidth;
		  delay = param_table[rule_num].delay;
		  lossrate = param_table[rule_num].lossrate;

#ifdef PREDEFINED_EXPERIMENT
		  bandwidth=BANDWIDTH;
		  delay=DELAY;
		  lossrate=PACKET_LOSS_RATE;
#endif
		  // print current configuration info
		  INFO("* Wireconf: #%d UCAST to #%d [%s] (time=%.2f s): bandwidth=%.2fbit/s \
loss_rate=%.4f delay=%.4f ms, offset=%d", my_id, i, IP_char_addresses+(i-FIRST_NODE_ID)*IP_ADDR_SIZE, 
		       time, bandwidth, lossrate, delay, offset_num);

		  // prepare adjusted values:
		  // 1. if packet size is known bandwidth could be adjusted:
		  //   multiplication factor 1.0778 was computed for 400 byte datagrams
		  //   because of 28 bytes header (428/400=1.07)
		  //   however we consider bandwidth at Ethernet level, therefore we
		  //   don't multiply here but when plotting results
		  //   Note: ip_dummynet.h says that bandwidth is in bytes/tick
		  //   but we seem to obtain correct values using bits/second
#ifdef __FreeBSD__
		  bandwidth = (int)round(bandwidth);// * 2.56);
		  // 2. no adjustment necessary for delay expressed in ms
		  delay = (int) round(delay);//(delay / 2);
		  // 3. loss rate probability must be extended to
		  //    2^31-1 (=0x7fffffff) range, equivalent to 100% loss
		  lossrate = (int)round(lossrate * 0x7fffffff);
		  // just configure pipe for the output traffic as in proposed solution for OLSR
		  //configure_pipe(s, MIN_PIPE_ID_IN + offset_num, bandwidth, delay, lossrate);
	      
		  //dump_param_table(param_table, rule_count);
		  configure_pipe(s, MIN_PIPE_ID_OUT + offset_num, bandwidth, delay, lossrate);
#elif __linux
		  bandwidth = (int)rint(bandwidth);// * 2.56);
		  delay = (int) rint(delay);//(delay / 2);
		  lossrate = (int)rint(lossrate * 0x7fffffff);
#endif

		  // increase loop counter
		  loop_count++;

#ifdef PREDEFINED_EXPERIMENT
		  if(loop_count>LOOP_COUNT)
		    break;
#endif
		}// end for loop

	      // config the broadcast links have source node id = i
	      for(i=FIRST_NODE_ID; i<(node_number+FIRST_NODE_ID); i++)
		{
		  if(i == my_id)
		    //if ( (j != 1) || (i != 2) )		// Lan optimize for the routing experiment
		    continue;

		  offset_num = i;
		  if((rule_num = lookup_param(i, param_table, rule_count)) == -1)
		    {
		      WARNING("Could not locate the rule number for next hop = %d with rule count = %d", 
			      i, rule_count);
		      dump_param_table(param_table, rule_count);
		      exit(1);
		    }

		  // prepare the configuration parameters
		  time = param_table[rule_num].time;
		  bandwidth = param_table[rule_num].bandwidth;
		  delay = param_table[rule_num].delay;
		  lossrate = param_table[rule_num].lossrate;

#ifdef PREDEFINED_EXPERIMENT
		  bandwidth=BANDWIDTH;
		  delay=DELAY;
		  lossrate=PACKET_LOSS_RATE;
#endif

		  // print current configuration info
		  INFO("* Wireconf: #%d BCAST from #%d [%s] (time=%.2f s): bandwidth=%.2fbit/s \
loss_rate=%.4f delay=%.4f ms, offset=%d", my_id, i, IP_char_addresses+(i-FIRST_NODE_ID)*IP_ADDR_SIZE, 
		       time, bandwidth, lossrate, delay, offset_num);
	  
		  // prepare adjusted values:
		  // 1. if packet size is known bandwidth could be adjusted:
		  //   multiplication factor 1.0778 was computed for 400 byte datagrams
		  //   because of 28 bytes header (428/400=1.07)
		  //   however we consider bandwidth at Ethernet level, therefore we
		  //   don't multiply here but when plotting results
		  //   Note: ip_dummynet.h says that bandwidth is in bytes/tick
		  //   but we seem to obtain correct values using bits/second
#ifdef __FreeBSD__
		  bandwidth = (int)round(bandwidth);// * 2.56);
		  // 2. no adjustment necessary for delay expressed in ms
		  delay = (int) round(delay);//(delay / 2);
		  // 3. loss rate probability must be extended to
		  //    2^31-1 (=0x7fffffff) range, equivalent to 100% loss
		  lossrate = (int)round(lossrate * 0x7fffffff);

		  //dump_param_table(param_table, rule_count);
		  configure_pipe(s, MIN_PIPE_ID_IN_BCAST + offset_num, bandwidth, delay, lossrate);
#elif __linux
		  bandwidth = (int)rint(bandwidth);// * 2.56);
		  delay = (int) rint(delay);//(delay / 2);
		  lossrate = (int)rint(lossrate * 0x7fffffff);
#endif

		  // increase loop counter
		  loop_count++;

#ifdef PREDEFINED_EXPERIMENT
		  if(loop_count>LOOP_COUNT)
		    break;
#endif
		}// end for loop for broadcast
	      // End config for broadcast traffic

	      // Reset the parameter and param_table for the next time
	      memset(param_table, 0, sizeof(qomet_param)*MAX_RULE_NUM);
	      rule_count = 0;
	      next_time += time_period;
	      INFO("New timer deadline=%f", next_time);
	      // if there is over read then assign to param_table
	      if(over_read == 1)
		{
		  param_table[0].time = param_over_read.time;
		  param_table[0].next_hop_id = param_over_read.next_hop_id;
		  param_table[0].bandwidth = param_over_read.bandwidth;
		  param_table[0].delay = param_over_read.delay;
		  param_table[0].lossrate = param_over_read.lossrate;
		  rule_count++;
		}
	      memset(&param_over_read, 0, sizeof(qomet_param));
	      over_read = 0;
	    }
#if 0
	  if (loop_count > 10 )
	    break;
#endif
	} // end else, usage (2)
    } // end while loop

  // if the loop was not entered not even once
  // there is an error
  if(loop_count == 0)
    {
      if(usage_type==1)
	WARNING("The specified pair from_node_id=%d & to_node_id=%d "
	      "could not be found", fid, tid);
      else
	WARNING("No valid line was found for the node %d", my_id); 
    }

  // release the timer
  timer_free(timer);

  // delete the dummynet rule
  if(usage_type==1)
    {
#ifdef __FreeBSD__
      if(delete_rule(s, rulenum)==ERROR)
	{
	  WARNING("Could not delete rule #%d", rulenum);
	  exit(1);
	}
#elif __linux
      if(delete_netem(s, taddr, rulenum)==ERROR)
	{
	  WARNING("Could not delete rule #%d", rulenum);
	  exit(1);
	}
#endif
    }
  else // usage (2) => delete multiple rules
    {
      // delete the unicast dummynet rules
      for (j = FIRST_NODE_ID; j < node_number + FIRST_NODE_ID; j++)
	{
	  if(j == my_id)
	    continue;

	  offset_num = j;
	  INFO("Deleting rule %d...", MIN_PIPE_ID_OUT+offset_num);
#ifdef __FreeBSD__
	  if(delete_rule(s, MIN_PIPE_ID_OUT+offset_num)==ERROR)
	    {
	      WARNING("Could not delete rule #%d", MIN_PIPE_ID_OUT+offset_num);
	    }
#elif __linux
	  if(delete_netem(s, taddr, MIN_PIPE_ID_OUT+offset_num)==ERROR)
	    {
	      WARNING("Could not delete rule #%d", MIN_PIPE_ID_OUT+offset_num);
	    }
#endif
	}

      //delete broadcast dummynet rules
      for (j = FIRST_NODE_ID; j < node_number + FIRST_NODE_ID; j++)
	{
	  if(j == my_id)
	    continue;
        
	  offset_num = j;
	  INFO("Deleting rule %d...", MIN_PIPE_ID_IN_BCAST+offset_num);
#ifdef __FreeBSD__
	  if(delete_rule(s, MIN_PIPE_ID_IN_BCAST+offset_num)==ERROR)
	    {
	      WARNING("Could not delete rule #%d", 
		      MIN_PIPE_ID_IN_BCAST+offset_num);
	    }
#elif __linux
	  if(delete_netem(s, taddr, MIN_PIPE_ID_IN_BCAST+offset_num)==ERROR)
		{
	      WARNING("Could not delete rule #%d", 
		      MIN_PIPE_ID_IN_BCAST+offset_num);
		}
#endif
	}
    }

  // get the time at the end of the experiment
  gettimeofday(&tp_end, NULL);

  // print execution time
  INFO("Experiment execution time=%.4f s", 
       (tp_end.tv_sec+tp_end.tv_usec/1.0e6) - 
       (tp_begin.tv_sec+tp_begin.tv_usec/1.0e6));

  // close socket
  DEBUG("Closing socket...");
  rtnl_close(&rth);
  close_socket(s);

  fclose(fd);

  return 0;
}

