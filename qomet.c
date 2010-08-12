
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
 * File name: qomet.c
 * Function: Main source file of QOMET
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 142 $
 *   $LastChangedDate: 2009-03-31 09:09:45 +0900 (Tue, 31 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <getopt.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <time.h>

#include "deltaQ.h" // include file of deltaQ library

//#define DISABLE_EMPTY_TIME_RECORDS


///////////////////////////////////////////////////////////
// Version management constants
///////////////////////////////////////////////////////////

#define MAJOR_VERSION              1
#define MINOR_VERSION              5
#define SUBMINOR_VERSION           0
#define IS_BETA                    TRUE


///////////////////////////////////////////////////////////
// Various constants
///////////////////////////////////////////////////////////

// a small value used when comparing doubles for equality
#define EPSILON         1e-6

#define MOTION_OUTPUT_NAM         0
#define MOTION_OUTPUT_NS2         1


///////////////////////////////////////////////////////////
// Constants used to add artificial noise;
// currently only used for specific experimental purposes;
// may become configurable at user level in future versions
///////////////////////////////////////////////////////////

// Uncomment the line to enable noise addition
//#define ADD_NOISE

#ifdef ADD_NOISE

#define NOISE_DEFAULT   -95  // default noise level
#define NOISE_START1     30   // start time for first noise source
#define NOISE_STOP1      90   // stop time for first noise source
#define NOISE_LEVEL1    -50   // level of first noise source

#define NOISE_START2    150   // start time for second noise source
#define NOISE_STOP2     210   // stop time for second noise source
#define NOISE_LEVEL2    -60   // level of first second source

#endif


///////////////////////////////////////////////////////////
// Constant used to activate the auto-connect feature
// (currently only used for active tags)
// PROBABLY SHOULD BE REMOVED??????????
///////////////////////////////////////////////////////////

//#define AUTO_CONNECT_ACTIVE_TAGS


///////////////////////////////////
// Generic variables and functions
///////////////////////////////////

#define MESSAGE_WARNING
//#define MESSAGE_DEBUG
#define MESSAGE_INFO

#ifdef MESSAGE_WARNING
#define WARNING(message...) do {                                       \
  fprintf(stderr, "qomet WARNING: %s, line %d: ", __FILE__, __LINE__); \
  fprintf(stderr, message); fprintf(stderr,"\n");                      \
} while(0)
#else
#define WARNING(message...) /* message */
#endif

#ifdef MESSAGE_DEBUG
#define DEBUG(message...) do {                                       \
  fprintf(stdout, "qomet DEBUG: %s, line %d: ", __FILE__, __LINE__); \
  fprintf(stdout, message); fprintf(stdout,"\n");                    \
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

// structure holding the names of long options and their 
// short version; when updating also change the 'short_options' 
// structure below
static struct option long_options[] = {
  {"help",            0, 0, 'h'},
  {"version",         0, 0, 'v'},
  {"license",         0, 0, 'l'},

  {"text-only",       0, 0, 't'},
  {"binary-only",     0, 0, 'b'},
  {"no-deltaQ",       0, 0, 'n'},
  {"motion-nam",      0, 0, 'm'},
  {"motion-ns",       0, 0, 's'},
  {"output",          1, 0, 'o'},

  {"disable-deltaQ",  0, 0, 'd'},

  {0, 0, 0, 0}
};

// structure holding name of short options; 
// should match the 'long_options' structure above 
static char *short_options = "hvltbnmso:d";


// print license info
static void license(FILE *f)
{
  fprintf
    (f, 
     "Copyright (c) 2006-2009 The StarBED Project  All rights reserved.\n"
     "\n"
     "Redistribution and use in source and binary forms, with or without\n"
     "modification, are permitted provided that the following conditions\n"
     "are met:\n"
     "1. Redistributions of source code must retain the above copyright\n"
     "   notice, this list of conditions and the following disclaimer.\n"
     "2. Redistributions in binary form must reproduce the above copyright\n"
     "   notice, this list of conditions and the following disclaimer in the\n"
     "   documentation and/or other materials provided with the distribution.\n"
     "3. Neither the name of the project nor the names of its contributors\n"
     "   may be used to endorse or promote products derived from this software\n"
     "   without specific prior written permission.\n"
     "\n"
     "THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS \"AS IS\" AND\n"
     "ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
     "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n"
     "ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE\n"
     "FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n"
     "DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS\n"
     "OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)\n"
     "HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT\n"
     "LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY\n"
     "OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF\n"
     "SUCH DAMAGE.\n\n"
     );
}

// print usage info
static void usage(FILE *f)
{
  fprintf(f, "Usage: qomet [options] <scenario_file.xml>\n");
  fprintf(f, "General options:\n");
  fprintf(f, " -h, --help             - print this help message and exit\n");
  fprintf(f, " -v, --version          - print version information and exit\n");
  fprintf(f, " -l, --license          - print license information and exit\n");
  fprintf(f, "Output control:\n");
  fprintf(f, " -t, --text-only        - enable ONLY text deltaQ output \
(no binary output)\n");
  fprintf(f, " -b, --binary-only      - enable ONLY binary deltaQ output \
(no text output)\n");
  fprintf(f, " -n, --no-deltaQ        - NO text NOR binary deltaQ output \
will be written\n");
  fprintf(f, " -m, --motion-nam       - enable output of motion data \
(NAM format)\n");
  fprintf(f, " -s, --motion-ns        - enable output of motion data \
(NS-2 format)\n");
  fprintf(f, " -o, --output <name>    - use <name> as base for generating \
output files,\n");
  fprintf(f, "                          instead of the input file name\n");
  fprintf(f, "Computation control:\n");
  fprintf(f, " -d, --disable-deltaQ   - disable deltaQ computation \
(output still generated)\n");
  fprintf(f, "\n");
  fprintf(f, "See documentation for more details.\n");
  fprintf(f, "Please send comments and bug reports to info@starbed.org.\n\n");
}

long int parse_svn_revision(char *svn_revision_str)
{
  /* According to svnversion version 1.4.2 (r22196) 
     this is the expected output:

  The version number will be a single number if the working
  copy is single revision, unmodified, not switched and with
  an URL that matches the TRAIL_URL argument.  If the working
  copy is unusual the version number will be more complex:

   4123:4168     mixed revision working copy
   4168M         modified working copy
   4123S         switched working copy
   4123:4168MS   mixed revision, modified, switched working copy

  */

  char *end_ptr, *next_ptr;
  long int return_value, backup_value;

  errno = 0;
  return_value = strtol(svn_revision_str, &end_ptr, 10);

  if((errno == ERANGE && 
      (return_value == LONG_MAX || return_value == LONG_MIN))
     || (errno != 0 && return_value == 0))
    {
      perror("strtol");
      return ERROR;
    }
  
  if(end_ptr == svn_revision_str)
    {
      WARNING("No digits were found");
      return ERROR;
    }

  // if we got here, strtol() successfully parsed a number
  
  DEBUG("Function strtol() returned %ld", return_value);
  
  if(*end_ptr != '\0')        // not necessarily an error...
    {
      DEBUG("Further characters after number: '%s'", end_ptr);

      // if ':' appeared, it means we must parse the next number, 
      // otherwise we ignore what follows
      if(*end_ptr == ':') 
	{
	  next_ptr = end_ptr+1;
	  backup_value = return_value;

	  errno = 0;
	  return_value = strtol(next_ptr, &end_ptr, 10);

	  if ((errno == ERANGE && 
	       (return_value == LONG_MAX || return_value == LONG_MIN))
	      || (errno != 0 && return_value == 0))
	    {
	      perror("strtol");
	      return ERROR;
	    }
  
	  if (end_ptr == next_ptr)
	    {
	      WARNING("No digits were found");
	      return_value = backup_value;	      
	      printf("restoring %ld\n", return_value);
	    }
	  else
	    DEBUG("Function strtol() returned %ld", return_value);

	  if(*end_ptr != '\0')        // not necessarily an error...
	    ;
	}
    }

  return return_value;
}


///////////////////////////////////////////////////
// main function
///////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  int error_status=SUCCESS;

  // xml scenario object
  xml_scenario_class *xml_scenario = NULL;

  // helper scenario object use temporarily
  scenario_class *scenario;

  // current time during scenario
  double current_time; 
  long int time_record_number = 0;

  // various indexes for scenario elements
#ifdef MESSAGE_DEBUG
  int node_i, environment_i, object_i;
#endif
  int motion_i, connection_i;

  // keep track whether any valid motion was found
  int motion_found;

  // file pointers
  FILE *scenario_file = NULL;       // scenario file pointer
  FILE *text_output_file = NULL;    // text output file pointer
  FILE *binary_output_file = NULL;  // binary output file pointer
  FILE *motion_file = NULL;         // motion file pointer

  // file name related strings
  char scenario_filename[MAX_STRING];
  char text_output_filename[MAX_STRING];
  char binary_output_filename[MAX_STRING];
  char motion_filename[MAX_STRING];

  // revision related variables
  long int svn_revision;
  char svn_revision_str[MAX_STRING];
  char qomet_name[MAX_STRING];
  char c;

  // output control variables
  int motion_output_enabled;
  int motion_output_type;
  int text_output_enabled;
  int binary_output_enabled;
  int text_only_enabled;
  int binary_only_enabled;
  int no_deltaQ_enabled;

  char output_filename_base[MAX_STRING];
  int output_filename_provided;

  // computation control variables
  int deltaQ_disabled;

  io_connection_state_class io_connection_state;

  int divider_i;
  double motion_step, motion_current_time;
	  
  ////////////////////////////////////////////////////////////
  // initialization

#ifndef SVN_REVISION
  svn_revision = ERROR;
#else
  if((strcmp(SVN_REVISION, "exported")==0) || (strlen(SVN_REVISION)==0))
    svn_revision = ERROR;
  else
    svn_revision = parse_svn_revision(SVN_REVISION);
#endif

  if(svn_revision == ERROR)
    sprintf(svn_revision_str, "N/A");
  else
    sprintf(svn_revision_str, "%ld", svn_revision);

  snprintf(qomet_name, MAX_STRING, "QOMET v%d.%d.%d %s(revision %s)", 
	   MAJOR_VERSION, MINOR_VERSION, SUBMINOR_VERSION, 
	   (IS_BETA==TRUE)?"beta ":"", svn_revision_str);


  ////////////////////////////////////////////////////////////
  // option parsing

  // uncomment the following line to disable 
  // option parsing error printing
  //opterr = 0;

  // default values for output
  text_output_enabled = TRUE;
  binary_output_enabled = TRUE;
  motion_output_enabled = FALSE;
  motion_output_type = MOTION_OUTPUT_NAM;
  output_filename_provided = FALSE;

  // option values initialization
  text_only_enabled = FALSE;
  binary_only_enabled = FALSE;
  no_deltaQ_enabled = FALSE;
  deltaQ_disabled = FALSE;

  // parse options
  while((c = getopt_long(argc, argv, short_options, 
			 long_options, NULL)) != -1) 
    {
      switch (c)
	{
	  // generic options
	case 'h':
	  printf("\n%s, a versatile wireless network emulator.\n\n", 
		 qomet_name);
	  usage(stdout);
	  exit(0);
	case 'v':
	  printf("\n%s. Copyright (c) 2006-2009 The StarBED Project.\n\n", 
		 qomet_name);
	  exit(0);
	case 'l':
	  printf("\n%s.\n", qomet_name);
	  license(stdout);
	  exit(0);

	  // output control
	case 't':
	  text_only_enabled = TRUE;
	  break;
	case 'b':
	  binary_only_enabled = TRUE;
	  break;
	case 'n':
	  no_deltaQ_enabled = TRUE;
	  break;
	case 'm':
	  motion_output_enabled = TRUE;
	  motion_output_type = MOTION_OUTPUT_NAM;
	  break;
	case 's':
	  motion_output_enabled = TRUE;
	  motion_output_type = MOTION_OUTPUT_NS2;
	  break;
	case 'o':
	  output_filename_provided = TRUE;
	  strncpy(output_filename_base, optarg, MAX_STRING-1);
	  break;

	  // computation control
	case 'd':
	  deltaQ_disabled = TRUE;
	  break;

	  // unknown options
	case '?':
	  printf("Try --help for more info\n");
	  exit(1);
	default:
	  WARNING("Unimplemented option -- %c", c);
	  printf("Try --help for more info\n");
	  exit(1);
	}
    }

  if((text_only_enabled == TRUE && binary_only_enabled == TRUE) || 
     (text_only_enabled == TRUE && no_deltaQ_enabled == TRUE) ||
     (binary_only_enabled == TRUE && no_deltaQ_enabled == TRUE))
    {
      WARNING("The output control options 'text-only', 'binary-only', and \
'no-deltaQ' are mutually exclusive and cannot be used together!\n");
      usage(stdout);
      exit(1);
    }

  // check if text-only output is enabled
  if(text_only_enabled == TRUE)
    {
      text_output_enabled = TRUE;
      binary_output_enabled = FALSE;
    }
  // check if binary-only output is enabled
  else if(binary_only_enabled == TRUE)
    {
      text_output_enabled = FALSE;
      binary_output_enabled = TRUE;
    }
  // check if no-deltaQ output is enabled
  else if(no_deltaQ_enabled == TRUE)
    {
      text_output_enabled = FALSE;
      binary_output_enabled = FALSE;
    }
  
  // optind represents the index where option parsing stopped
  // and where non-option arguments parsing can start;
  // check whether non-option arguments are present
  if(argc==optind)
    {
      WARNING("No scenario configuration file was provided!\n");
      usage(stdout);
      exit(1);
    }


  ////////////////////////////////////////////////////////////
  // more initialization

  INFO("\n-- Wireless network emulator %s --", qomet_name); 
  INFO("Starting...");

#ifdef ADD_NOISE
  fprintf(stderr, "Noise will be added to your experiment!\n");
  fprintf(stderr, "Press <Return> to continue...\n");
  getchar();
#endif


  // try to allocate the xml_scenario object
  xml_scenario = (xml_scenario_class *)malloc(sizeof(xml_scenario_class));

  if(xml_scenario==NULL)
    {
      WARNING("Cannot allocate memory (tried %d bytes)", 
	      sizeof(xml_scenario_class));
      goto ERROR_HANDLE;
    }

  // initialize the scenario object
  scenario_init(&(xml_scenario->scenario));


  ////////////////////////////////////////////////////////////
  // scenario parsing phase

  INFO("\n-- QOMET Emulator: Scenario Parsing --\n");

  if(strlen(argv[optind])>MAX_STRING)
    {
      WARNING("Input file name '%s' longer than %d characters!",
	      argv[optind], MAX_STRING);
      goto ERROR_HANDLE;
    }
  else
    strncpy(scenario_filename, argv[optind], MAX_STRING-1);

  // set the output filename base to the scenario filename
  // if no output filename base was provided
  if(output_filename_provided == FALSE)
    strncpy(output_filename_base, scenario_filename, MAX_STRING-1);

  // open scenario file
  scenario_file = fopen(scenario_filename, "r");
  if(scenario_file==NULL)
    {
      WARNING("Cannot open scenario file '%s'!", scenario_filename);
      goto ERROR_HANDLE;
    }
    
  // parse scenario file
  if(xml_scenario_parse(scenario_file, xml_scenario)==ERROR)
    {
      WARNING("Cannot parse scenario file '%s'!", scenario_filename);
      goto ERROR_HANDLE;
    }

  // print parse summary
  INFO("Scenario file '%s' parsed.", scenario_filename);
#ifdef MESSAGE_DEBUG
  DEBUG("Loaded scenario file summary:");
  xml_scenario_print(xml_scenario);
#endif

  // even more initialization
  motion_step=xml_scenario->step/xml_scenario->motion_step_divider;

  // save a pointer to the scenario data structure
  scenario = &(xml_scenario->scenario);


  ////////////////////////////////////////////////////////////
  // computation phase

  INFO("\n-- QOMET Emulator: Scenario Computation --");

  // check if binary output is enabled
  if(binary_output_enabled == TRUE)
    {
      // prepare binary output filename
      strncpy(binary_output_filename, output_filename_base, MAX_STRING-1);

      if(strlen(binary_output_filename)>MAX_STRING-5)
	{
	  WARNING("Cannot create binary output file name because input \
filename '%s' exceeds %d characters!", output_filename_base, MAX_STRING-5);
	  goto ERROR_HANDLE;
	}

      // append extension ".bin"
      strncat(binary_output_filename, ".bin", 
	      MAX_STRING-strlen(binary_output_filename)-5);
      binary_output_file = fopen(binary_output_filename, "w");
      if(binary_output_file==NULL)
	{
	  WARNING("Cannot open binary output file '%s'!", 
		  binary_output_filename);
	  goto ERROR_HANDLE;
	}

      // start writing binary output
      io_write_binary_header_to_file(0, MAJOR_VERSION, MINOR_VERSION,
				     SUBMINOR_VERSION, svn_revision,
				     binary_output_file);
    }

  // check if text output is enabled
  if(text_output_enabled == TRUE)
    {
      // prepare text output filename
      strncpy(text_output_filename, output_filename_base, MAX_STRING-1);

      if(strlen(text_output_filename)>MAX_STRING-5)
	{
	  WARNING("Cannot create output file name because input filename \
'%s' exceeds %d characters!", output_filename_base, MAX_STRING-5);
	  goto ERROR_HANDLE;
	}

      // append extension ".out"
      strncat(text_output_filename, ".out", 
	      MAX_STRING-strlen(text_output_filename)-5);

      // open file
      text_output_file = fopen(text_output_filename, "w");
      if(text_output_file==NULL)
	{
	  WARNING("Cannot open output file '%s'!", text_output_filename);
	  goto ERROR_HANDLE;
	}

      // write global file header for matlab
      io_write_header_to_file(text_output_file, qomet_name);
    }

  // check if motion output is enabled
  if(motion_output_enabled == TRUE)
    {
      // prepare motion filename
      strncpy(motion_filename, output_filename_base, MAX_STRING-1);

      if(strlen(motion_filename)>MAX_STRING-5)
	{
	  WARNING("Cannot create motion file name because input filename \
'%s' exceeds %d characters!", output_filename_base, MAX_STRING-5);
	  goto ERROR_HANDLE;
	}

      if(motion_output_type==MOTION_OUTPUT_NAM)
	{
	  // append extension ".nam"
	  strncat(motion_filename, ".nam", 
		  MAX_STRING-strlen(motion_filename)-5);
	}
      else if(motion_output_type==MOTION_OUTPUT_NS2)
	{
	  // append extension ".ns2"
	  strncat(motion_filename, ".ns2", 
		  MAX_STRING-strlen(motion_filename)-5);
	}
      else
	{
	  WARNING("Unknown motion output type (%d)!", motion_output_type);
	  goto ERROR_HANDLE;
	}

      motion_file = fopen(motion_filename, "w");
      if(motion_file==NULL)
	{
	  WARNING("Cannot open motion file '%s'!", motion_filename);
	  goto ERROR_HANDLE;
	}
    }

#ifdef MESSAGE_DEBUG

  // print the initial condition of the scenario
  printf("\n-- Initial condition:\n");
  fprintf(stderr, "\n-- Initial condition:\n");
  for(node_i=0; node_i<scenario->node_number; node_i++)
    node_print(&(scenario->nodes[node_i]));
  for(object_i=0; object_i<scenario->object_number; object_i++)
    object_print(&(scenario->objects[object_i]));
  for(environment_i=0; environment_i<scenario->environment_number; 
      environment_i++)
      environment_print(&(scenario->environments[environment_i]));
  for(motion_i=0; motion_i<scenario->motion_number; motion_i++)
    motion_print(&(scenario->motions[motion_i]));
  for(connection_i=0; connection_i<scenario->connection_number; connection_i++)
    connection_print(&(scenario->connections[connection_i]));

#endif

  // initialize scenario state
  INFO("\n-- Scenario initialization:");
  fprintf(stderr, "\n-- Scenario initialization:\n");

  if(scenario_init_state(scenario, xml_scenario->jpgis_filename_provided,
			 xml_scenario->jpgis_filename, deltaQ_disabled)==ERROR)
    {
      fflush(stdout);
      WARNING("Error during scenario initialization. Aborting...");
      goto ERROR_HANDLE;
    }


  ////////////////////////////////////////////////////////////
  // processing phase

  // start processing the scenario
  INFO("\n-- Scenario processing:");
  fprintf(stderr, "\n-- Scenario processing:\n");

  for(current_time=xml_scenario->start_time; 
      current_time<=(xml_scenario->duration+xml_scenario->start_time+EPSILON); 
      current_time+=xml_scenario->step)
    {
      // print the current state
      INFO("* Current time=%.3f", current_time);
      fprintf(stderr, "* Time=%.3f s:                                      \r",
	      current_time);

      // check if motion output is enabled
      if(motion_output_enabled == TRUE)
	{
	  if(current_time == xml_scenario->start_time)
	    {
	      // write motion file header
	      if(motion_output_type==MOTION_OUTPUT_NAM)
		io_write_nam_motion_header_to_file(scenario, motion_file);
	      else
		io_write_ns2_motion_header_to_file(scenario, motion_file);
	    }
	  else
	    {
	      // write motion info 
	      if(motion_output_type==MOTION_OUTPUT_NAM)
		io_write_nam_motion_info_to_file(scenario, motion_file, 
						 current_time);
	      else
		io_write_ns2_motion_info_to_file(scenario, motion_file, 
						 current_time);
	    }
	}

#ifdef ADD_NOISE
      // special noise addition
      if(current_time>=NOISE_START1 && current_time<NOISE_STOP1)
	scenario->environments[0].noise_power[0] = NOISE_LEVEL1;
      else if(current_time>=NOISE_START2 && current_time<NOISE_STOP2)
	scenario->environments[0].noise_power[0] = NOISE_LEVEL2;
      else
	scenario->environments[0].noise_power[0] = NOISE_DEFAULT;
#endif

#ifdef AUTO_CONNECT_ACTIVE_TAGS

      INFO("Auto-connecting active tag nodes...");
      if(scenario_auto_connect_nodes_at(scenario)==ERROR)
	{
	  WARNING("Error auto-connecting active tag nodes");
	  return ERROR;
	}
#endif

      if(deltaQ_disabled == FALSE)
	{
	  // compute deltaQ parameters
	  INFO("  DELTA_Q CALCULATION");
	  if(scenario_deltaQ(scenario)==ERROR)
	    {
	      WARNING("Error while calculating deltaQ. Aborting...");
	      goto ERROR_HANDLE;
	    }
	}

      // check if binary output is enabled
      if(binary_output_enabled == TRUE)
	{
	  io_connection_state.binary_time_record.time = current_time;
	  io_connection_state.binary_time_record.record_number = 0;
	}

      // write all node status to files
      for(connection_i=0; connection_i<scenario->connection_number; 
	  connection_i++)
	{
	  // check if text output is enabled
	  if(text_output_enabled == TRUE)
	    io_write_to_file(&(scenario->connections[connection_i]), 
			     scenario, current_time, 
			     xml_scenario->cartesian_coord_syst,
			     text_output_file);

	  // check if binary output is enabled
	  if(binary_output_enabled == TRUE)
	    {
	      // check if we are processing first time
	      if(current_time==xml_scenario->start_time)
		{
		  //save state without any checking
		  io_binary_build_record
		    (&(io_connection_state.binary_records[connection_i]),
		     &(scenario->connections[connection_i]), scenario);
		  io_connection_state.state_changed[connection_i] = TRUE;
		  io_connection_state.binary_time_record.record_number++;
		}
	      else
		{
		  //check if state changed
		  if(io_binary_compare_record
		     (&(io_connection_state.binary_records[connection_i]),
		      &(scenario->connections[connection_i]), scenario)==FALSE)
		    {
		      //save state
		      io_binary_build_record
			(&(io_connection_state.binary_records[connection_i]),
			 &(scenario->connections[connection_i]), scenario);
		      io_connection_state.state_changed[connection_i] = TRUE;
		      io_connection_state.binary_time_record.record_number++;
		    }
		  else
		    // state didn't change
		    io_connection_state.state_changed[connection_i] = FALSE;
		}
	    }
	}

      // check if binary output is enabled
      if(binary_output_enabled == TRUE)
	{
	  int record_i, connection_i;

#ifdef DISABLE_EMPTY_TIME_RECORDS
	  if(io_connection_state.binary_time_record.record_number>0)
	    {
#endif
	      time_record_number++;

	      io_write_binary_time_record_to_file2
		(&(io_connection_state.binary_time_record), 
		 binary_output_file);

	      record_i = 0;
	      for(connection_i=0; connection_i<scenario->connection_number;
		  connection_i++)
		{
		  if(io_connection_state.state_changed[connection_i] == TRUE)
		    {
		      io_write_binary_record_to_file2
			(&(io_connection_state.binary_records[connection_i]), 
			 binary_output_file);
		      io_binary_print_record
			(&(io_connection_state.binary_records[connection_i]));

		      // check if there are any more records 
		      // to write for this time interval
		      if(record_i<
			 io_connection_state.binary_time_record.record_number)
			record_i++;
		      else
			break;
		    }		 
		}

	      if(record_i<io_connection_state.binary_time_record.record_number)
		WARNING("At time %f wrote ONLY %d binary records out of %d", 
			current_time, record_i,
			io_connection_state.binary_time_record.record_number);
	      else
		INFO("At time %f wrote %d binary records", current_time, 
		     record_i);
#ifdef DISABLE_EMPTY_TIME_RECORDS
	    }
#endif
	}

      for(divider_i=0; divider_i<xml_scenario->motion_step_divider; 
	  divider_i++)
	{
	  motion_current_time=current_time + divider_i*motion_step;

	  if(motion_current_time>
	     (xml_scenario->duration+xml_scenario->start_time+EPSILON))
	    break;

	  // move nodes according to the 'motions' object in 'scenario'
	  // for the next step of evaluation
	  INFO("  NODE MOVEMENT (sub-step %d)", divider_i);
	  motion_found = FALSE;
	  for(motion_i=0; motion_i<scenario->motion_number; motion_i++)
	    if((scenario->motions[motion_i].start_time 
		<= motion_current_time) && 
	       (scenario->motions[motion_i].stop_time > motion_current_time))
	      {
		fprintf(stderr, "\t\t\tcalculation => \
motion %d             \r", motion_i);
		// TEMPORARY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		//if(motion_i!=45) continue;	 // 0 corresponds to id 1
      
		if(motion_apply(&(scenario->motions[motion_i]), scenario,
				motion_step)==ERROR)
		  goto ERROR_HANDLE;
		
		motion_found = TRUE;
	      }
#ifdef MESSAGE_INFO
	  if(motion_found == FALSE)
	    INFO("  No valid motion found");
#endif
	}
    }

  // check if binary output is enabled
  if(binary_output_enabled == TRUE)
    {
      // rewrite binary header now that all information is available
      rewind(binary_output_file);
      io_write_binary_header_to_file
	(time_record_number, MAJOR_VERSION, MINOR_VERSION, SUBMINOR_VERSION, 
	 svn_revision, binary_output_file);
    }

  // if no errors occurred, skip next instruction
  goto FINAL_HANDLE;

 ERROR_HANDLE:
  error_status=ERROR;


  ////////////////////////////////////////////////////////////
  // finalizing phase

 FINAL_HANDLE:

  if(error_status==SUCCESS)
    {
      // print this in case of successful processing
      INFO("\n-- Scenario processing completed successfully\n");
      fprintf(stderr, "\n\n-- Scenario processing completed successfully\n");
    }
  else
    {
      // print this in case of unsuccessful processing
      INFO("\n-- Scenario processing completed with errors \
(see the WARNING messages above)\n");
      fprintf(stderr, "\n\n-- Scenario processing completed with errors \
(see the WARNING messages above)\n");
    }

  // close output files
  if(scenario_file!=NULL)
    fclose(scenario_file);

  // check if text output is enabled
  if(text_output_enabled == TRUE && text_output_file !=NULL)
    fclose(text_output_file);

  // check if binary output is enabled
  if(binary_output_enabled == TRUE && binary_output_file !=NULL)
    fclose(binary_output_file);

  // check if motion output is enabled
  if(motion_output_enabled == TRUE && motion_file !=NULL)
    fclose(motion_file);

  if(xml_scenario!=NULL)
    free(xml_scenario);

  return error_status;
}
