
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
 * File name: deltaQ.c
 * Function: Main source file of the deltaQ program that uses the deltaQ
 *           library to compute the communication conditions between
 *           emulated nodes
 *
 * Author: Razvan Beuran
 *
 * $Id: deltaQ.c 146 2013-06-20 00:50:48Z razvan $
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

#include "deltaQ.h"		// include file of deltaQ library
#include "message.h"

//#define DISABLE_EMPTY_TIME_RECORDS


///////////////////////////////////////////////////////////
// Various constants
///////////////////////////////////////////////////////////

// type of motion output
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

#define NOISE_DEFAULT   -95	// default noise level
#define NOISE_START1     30	// start time for first noise source
#define NOISE_STOP1      90	// stop time for first noise source
#define NOISE_LEVEL1    -50	// level of first noise source

#define NOISE_START2    150	// start time for second noise source
#define NOISE_STOP2     210	// stop time for second noise source
#define NOISE_LEVEL2    -60	// level of first second source

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

// structure holding the names of long options and their 
// short version; when updating also change the 'short_options' 
// structure below
static struct option long_options[] = {
  {"help", 0, 0, 'h'},
  {"version", 0, 0, 'v'},
  {"license", 0, 0, 'l'},

  {"text-only", 0, 0, 't'},
  {"binary-only", 0, 0, 'b'},
  {"no-deltaQ", 0, 0, 'n'},
  {"motion-nam", 0, 0, 'm'},
  {"motion-ns", 0, 0, 's'},
  {"object", 0, 0, 'j'},
  {"output", 1, 0, 'o'},

  {"disable-deltaQ", 0, 0, 'd'},

  {0, 0, 0, 0}
};

// structure holding name of short options; 
// should match the 'long_options' structure above 
static char *short_options = "hvltbnmsjo:d";


// print license info
static void
license (FILE * f)
{
  fprintf
    (f,
     "Copyright (c) 2006-2013 The StarBED Project  All rights reserved.\n"
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
     "SUCH DAMAGE.\n\n");
}

// print usage info
static void
usage (FILE * f)
{
  fprintf (f, "\nUsage: deltaQ [options] <scenario_file.xml>\n");
  fprintf (f, "General options:\n");
  fprintf (f, " -h, --help             - print this help message and exit\n");
  fprintf (f,
	   " -v, --version          - print version information and exit\n");
  fprintf (f,
	   " -l, --license          - print license information and exit\n");
  fprintf (f, "Output control:\n");
  fprintf (f, " -t, --text-only        - enable ONLY text deltaQ output \
(no binary output)\n");
  fprintf (f, " -b, --binary-only      - enable ONLY binary deltaQ output \
(no text output)\n");
  fprintf (f, " -n, --no-deltaQ        - NO text NOR binary deltaQ output \
will be written\n");
  fprintf (f, " -m, --motion-nam       - enable output of motion data \
in NAM format\n");
  fprintf (f, " -s, --motion-ns        - enable output of motion data \
in NS-2 format\n");
  fprintf (f, " -j, --object           - enable output of object data\n");
  fprintf (f, " -o, --output <base>    - use <base> as base for generating \
output files,\n");
  fprintf (f, "                          instead of the input file name\n");
  fprintf (f, "Computation control:\n");
  fprintf (f, " -d, --disable-deltaQ   - disable deltaQ computation \
(output still generated)\n");
  fprintf (f, "\n");
  fprintf (f, "See the documentation for more usage details.\n");
  fprintf (f, "Please send any comments or bug reports to \
'info@starbed.org'.\n\n");
}

long int
parse_svn_revision (char *svn_revision_str)
{
  /* According to svnversion version 1.4.2 (r22196) 
     this is the expected output:

     The version number will be a single number if the working
     copy is single revision, unmodified, not switched and with
     an URL that matches the TRAIL_URL argument.  If the working
     copy is unusual the version number will be more complex:

     4123:4168     mixed revision working copy  [=> use most recent (highest) number]
     4168M         modified working copy        [=> use number part]
     4123S         switched working copy        [=> use number part]
     4123:4168MS   mixed revision, modified, switched working copy [=> use most recent (highest) number part]

   */

  char *end_ptr, *next_ptr;
  long int return_value, backup_value;

  errno = 0;
  return_value = strtol (svn_revision_str, &end_ptr, 10);

  if ((errno == ERANGE &&
       (return_value == LONG_MAX || return_value == LONG_MIN))
      || (errno != 0 && return_value == 0))
    {
      WARNING ("Could not parse integer.");
      perror ("strtol");
      return ERROR;
    }

  if (end_ptr == svn_revision_str)
    {
      WARNING ("No digits were found");
      return ERROR;
    }

  // if we got here, strtol() successfully parsed a number

  DEBUG ("Function strtol() returned %ld.", return_value);

  if (*end_ptr != '\0')		// not necessarily an error...
    {
      DEBUG ("Further characters after number: '%s'", end_ptr);

      // if ':' appeared, it means we must parse the next number, 
      // otherwise we ignore what follows
      if (*end_ptr == ':')
	{
	  next_ptr = end_ptr + 1;
	  backup_value = return_value;

	  errno = 0;
	  return_value = strtol (next_ptr, &end_ptr, 10);

	  if ((errno == ERANGE &&
	       (return_value == LONG_MAX || return_value == LONG_MIN))
	      || (errno != 0 && return_value == 0))
	    {
	      perror ("strtol");
	      return ERROR;
	    }

	  if (end_ptr == next_ptr)
	    {
	      WARNING ("No digits were found");
	      return_value = backup_value;
	      printf ("restoring %ld\n", return_value);
	    }
	  else
	    DEBUG ("Function strtol() returned %ld", return_value);

//	  if (*end_ptr != '\0')	// not necessarily an error...
//	    ;
	}
    }

  return return_value;
}


///////////////////////////////////////////////////
// main function
///////////////////////////////////////////////////

int
main (int argc, char *argv[])
{
  int error_status = SUCCESS;

  // xml scenario object
  struct xml_scenario_class *xml_scenario = NULL;

  // helper scenario object used temporarily
  struct scenario_class *scenario;

  // current time during scenario
  double current_time;
  long int time_rec_num = 0;

  // various indexes for scenario elements
#ifdef MESSAGE_DEBUG
  int node_i, environment_i, object_i;
#endif
  int motion_i, connection_i;

  // keep track whether any valid motion was found
  int motion_found;

  // file pointers
  FILE *scenario_file = NULL;	// scenario file pointer
  FILE *text_output_file = NULL;	// text output file pointer
  FILE *binary_output_file = NULL;	// binary output file pointer
  FILE *motion_file = NULL;	// motion file pointer
  FILE *object_output_file = NULL;	// object output file pointer

  FILE *settings_file = NULL;	// settings file pointer

  // file name related strings
  char scenario_filename[MAX_STRING];
  char text_output_filename[MAX_STRING];
  char binary_output_filename[MAX_STRING];
  char motion_filename[MAX_STRING];
  char settings_filename[MAX_STRING];
  char object_output_filename[MAX_STRING];

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
  int object_output_enabled;

  char output_filename_base[MAX_STRING];
  int output_filename_provided;

  // computation control variables
  int deltaQ_disabled;

  struct io_connection_state_class io_connection_state;

  int divider_i;
  double motion_step, motion_current_time;


  ////////////////////////////////////////////////////////////
  // initialization

#ifndef SVN_REVISION
  DEBUG ("SVN_REVISION not defined.");
  svn_revision = ERROR;
#else
  if ((strcmp (SVN_REVISION, "exported") == 0)
      || (strlen (SVN_REVISION) == 0))
    {
      DEBUG ("SVN_REVISION defined but equal to 'exported' or ''.");
      svn_revision = ERROR;
    }
  else
    {
      DEBUG ("SVN_REVISION defined.");
      svn_revision = parse_svn_revision (SVN_REVISION);
    }
#endif

  if (svn_revision == ERROR)
    {
      WARNING ("Could not identify revision number.");
      sprintf (svn_revision_str, "N/A");
    }
  else
    sprintf (svn_revision_str, "%ld", svn_revision);

  snprintf (qomet_name, MAX_STRING,
	    "QOMET v%d.%d.%d %s- deltaQ (revision %s)", MAJOR_VERSION,
	    MINOR_VERSION, SUBMINOR_VERSION, (IS_BETA == TRUE) ? "beta " : "",
	    svn_revision_str);


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
  object_output_enabled = FALSE;

  // parse options
  while ((c = getopt_long (argc, argv, short_options,
			   long_options, NULL)) != -1)
    {
      switch (c)
	{
	  // generic options
	case 'h':
	  printf ("\n%s: A versatile wireless network emulator.\n",
		  qomet_name);
	  usage (stdout);
	  exit (0);
	case 'v':
	  printf ("\n%s.  Copyright (c) 2006-2013 The StarBED Project.\n\n",
		  qomet_name);
	  exit (0);
	case 'l':
	  printf ("\n%s.\n", qomet_name);
	  license (stdout);
	  exit (0);

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
	  // check whether motion output has already been enabled
	  if (motion_output_enabled == TRUE)
	    {
	      WARNING
		("Multiple arguments related to motion output detected.");
	      WARNING ("Note that arguments '-m' and '-s' are mutually \
exclusive.");
	      usage (stdout);
	      exit (1);
	    }
	  else
	    {
	      motion_output_enabled = TRUE;
	      motion_output_type = MOTION_OUTPUT_NAM;
	    }
	  break;
	case 's':
	  // check whether motion output has already been enabled
	  if (motion_output_enabled == TRUE)
	    {
	      WARNING
		("Multiple arguments related to motion output detected.");
	      WARNING ("Note that arguments '-m' and '-s' are mutually \
exclusive.");
	      usage (stdout);
	      exit (1);
	    }
	  else
	    {
	      motion_output_enabled = TRUE;
	      motion_output_type = MOTION_OUTPUT_NS2;
	    }
	  break;
	case 'j':
	  object_output_enabled = TRUE;
	  break;
	case 'o':
	  output_filename_provided = TRUE;
	  strncpy (output_filename_base, optarg, MAX_STRING - 1);
	  break;

	  // computation control
	case 'd':
	  deltaQ_disabled = TRUE;
	  break;

	  // unknown options
	case '?':
	  printf ("Try --help for more info\n");
	  exit (1);
	default:
	  WARNING ("Unimplemented option -- %c", c);
	  WARNING ("Try --help for more info");
	  exit (1);
	}
    }

  if ((text_only_enabled == TRUE && binary_only_enabled == TRUE) ||
      (text_only_enabled == TRUE && no_deltaQ_enabled == TRUE) ||
      (binary_only_enabled == TRUE && no_deltaQ_enabled == TRUE))
    {
      WARNING ("The output control options 'text-only', 'binary-only', and \
'no-deltaQ' are mutually exclusive and cannot be used together!");
      usage (stdout);
      exit (1);
    }

  // check if text-only output is enabled
  if (text_only_enabled == TRUE)
    {
      text_output_enabled = TRUE;
      binary_output_enabled = FALSE;
    }
  // check if binary-only output is enabled
  else if (binary_only_enabled == TRUE)
    {
      text_output_enabled = FALSE;
      binary_output_enabled = TRUE;
    }
  // check if no-deltaQ output is enabled
  else if (no_deltaQ_enabled == TRUE)
    {
      text_output_enabled = FALSE;
      binary_output_enabled = FALSE;
    }

  // optind represents the index where option parsing stopped
  // and where non-option arguments parsing can start;
  // check whether non-option arguments are present
  if (argc == optind)
    {
      WARNING ("No scenario configuration file was provided.");
      printf ("\n%s: A versatile wireless network emulator.\n", qomet_name);
      usage (stdout);
      exit (1);
    }


  ////////////////////////////////////////////////////////////
  // more initialization

  INFO ("\n-- Wireless network emulator %s --", qomet_name);
  INFO ("Starting...");

#ifdef ADD_NOISE
  fprintf (stderr, "Noise will be added to your experiment!\n");
  fprintf (stderr, "Press <Return> to continue...\n");
  getchar ();
#endif


  // try to allocate the xml_scenario object
  xml_scenario =
    (struct xml_scenario_class *) malloc (sizeof (struct xml_scenario_class));

  if (xml_scenario == NULL)
    {
      WARNING
	("Cannot allocate memory (tried %zd bytes); you may need to reduce \
maximum scenario size by editing the maximum data sizes constants in the \
file 'deltaQ/scenario.h' (MAX_NODES, MAX_OBJECTS, MAX_ENVIRONMENTS, \
MAX_MOTIONS and MAX_CONNECTIONS)", sizeof (struct xml_scenario_class));
      goto ERROR_HANDLE;
    }
  else
    DEBUG ("Allocated %.2f MB (%zd bytes) for internal scenario storage.",
	   sizeof (struct xml_scenario_class) / 1048576.0,
	   sizeof (struct xml_scenario_class));

  // initialize the scenario object
  scenario_init (&(xml_scenario->scenario));


  ////////////////////////////////////////////////////////////
  // scenario parsing phase

  INFO ("\n-- QOMET Emulator: Scenario Parsing --\n");

  if (strlen (argv[optind]) > MAX_STRING)
    {
      WARNING ("Input file name '%s' longer than %d characters!",
	       argv[optind], MAX_STRING);
      goto ERROR_HANDLE;
    }
  else
    strncpy (scenario_filename, argv[optind], MAX_STRING - 1);

  // set the output filename base to the scenario filename
  // if no output filename base was provided
  if (output_filename_provided == FALSE)
    strncpy (output_filename_base, scenario_filename, MAX_STRING - 1);

  // open scenario file
  scenario_file = fopen (scenario_filename, "r");
  if (scenario_file == NULL)
    {
      WARNING ("Cannot open scenario file '%s'!", scenario_filename);
      goto ERROR_HANDLE;
    }

  // parse scenario file
  if (xml_scenario_parse (scenario_file, xml_scenario) == ERROR)
    {
      WARNING ("Cannot parse scenario file '%s'!", scenario_filename);
      goto ERROR_HANDLE;
    }

  // print parse summary
  INFO ("Scenario file '%s' parsed.", scenario_filename);
#ifdef MESSAGE_DEBUG
  DEBUG ("Loaded scenario file summary:");
  xml_scenario_print (xml_scenario);
#endif

  // even more initialization
  motion_step = xml_scenario->step / xml_scenario->motion_step_divider;

  // save a pointer to the scenario data structure
  scenario = &(xml_scenario->scenario);


  ////////////////////////////////////////////////////////////
  // computation phase

  // Latest version of expat (expat-2.0.1-11.el6_2.i686) fixed a bug
  // related to hash functions by initializing internally the random
  // number generator seed to values obtained from the clock. This causes
  // random values computed by us to change at each run. To fix this
  // we initialize the random number generator seed to its default
  // value (1).
  // References:
  // [1] http://cmeerw.org/blog/759.html
  // [2] https://bugzilla.redhat.com/show_bug.cgi?id=786617
  // [3] https://rhn.redhat.com/errata/RHSA-2012-0731.html
  DEBUG ("Initialize random seed because it was changed by expat during \
parsing.\n");
  srand (1);

  INFO ("\n-- QOMET Emulator: Scenario Computation --");

  // check if settings output is enabled
  if (!(text_only_enabled || binary_only_enabled))
    {
      // prepare settings filename
      strncpy (settings_filename, output_filename_base, MAX_STRING - 1);

      if (strlen (settings_filename) > MAX_STRING - 10)
	{
	  WARNING ("Cannot create settings file name because input \
filename '%s' exceeds %d characters!", settings_filename, MAX_STRING - 10);
	  goto ERROR_HANDLE;
	}

      // append extension ".settings"
      strncat (settings_filename, ".settings",
	       MAX_STRING - strlen (settings_filename) - 10);
      settings_file = fopen (settings_filename, "w");
      if (settings_file == NULL)
	{
	  WARNING ("Cannot open settings file '%s' for writing!",
		   settings_filename);
	  goto ERROR_HANDLE;
	}
    }

  // check if binary output is enabled
  if (binary_output_enabled == TRUE)
    {
      // prepare binary output filename
      strncpy (binary_output_filename, output_filename_base, MAX_STRING - 1);

      if (strlen (binary_output_filename) > MAX_STRING - 5)
	{
	  WARNING ("Cannot create binary output file name because input \
filename '%s' exceeds %d characters!", output_filename_base, MAX_STRING - 5);
	  goto ERROR_HANDLE;
	}

      // append extension ".bin"
      strncat (binary_output_filename, ".bin",
	       MAX_STRING - strlen (binary_output_filename) - 5);
      binary_output_file = fopen (binary_output_filename, "w");
      if (binary_output_file == NULL)
	{
	  WARNING ("Cannot open binary output file '%s' for writing!",
		   binary_output_filename);
	  goto ERROR_HANDLE;
	}

      // start writing binary output
      io_binary_write_header_to_file (scenario->if_num, 0,
				      MAJOR_VERSION, MINOR_VERSION,
				      SUBMINOR_VERSION, svn_revision,
				      binary_output_file);
    }

  // check if text output is enabled
  if (text_output_enabled == TRUE)
    {
      // prepare text output filename
      strncpy (text_output_filename, output_filename_base, MAX_STRING - 1);

      if (strlen (text_output_filename) > MAX_STRING - 5)
	{
	  WARNING ("Cannot create output file name because input filename \
'%s' exceeds %d characters!", output_filename_base, MAX_STRING - 5);
	  goto ERROR_HANDLE;
	}

      // append extension ".out"
      strncat (text_output_filename, ".out",
	       MAX_STRING - strlen (text_output_filename) - 5);

      // open file
      text_output_file = fopen (text_output_filename, "w");
      if (text_output_file == NULL)
	{
	  WARNING ("Cannot open text output file '%s'! for writing",
		   text_output_filename);
	  goto ERROR_HANDLE;
	}

      // write global file header for matlab
      io_write_header_to_file (text_output_file, qomet_name);
    }

  // check if motion output is enabled
  if (motion_output_enabled == TRUE)
    {
      // prepare motion filename
      strncpy (motion_filename, output_filename_base, MAX_STRING - 1);

      if (strlen (motion_filename) > MAX_STRING - 5)
	{
	  WARNING ("Cannot create motion file name because input filename \
'%s' exceeds %d characters!", output_filename_base, MAX_STRING - 5);
	  goto ERROR_HANDLE;
	}

      if (motion_output_type == MOTION_OUTPUT_NAM)
	{
	  // append extension ".nam"
	  strncat (motion_filename, ".nam",
		   MAX_STRING - strlen (motion_filename) - 5);
	}
      else if (motion_output_type == MOTION_OUTPUT_NS2)
	{
	  // append extension ".ns2"
	  strncat (motion_filename, ".ns2",
		   MAX_STRING - strlen (motion_filename) - 5);
	}
      else
	{
	  WARNING ("Unknown motion output type (%d)!", motion_output_type);
	  goto ERROR_HANDLE;
	}

      motion_file = fopen (motion_filename, "w");
      if (motion_file == NULL)
	{
	  WARNING ("Cannot open motion file '%s' for writing!",
		   motion_filename);
	  goto ERROR_HANDLE;
	}
    }

#ifdef MESSAGE_DEBUG

  // print the initial condition of the scenario
  printf ("\n-- Initial condition:\n");
  fprintf (stderr, "\n-- Initial condition:\n");
  for (node_i = 0; node_i < scenario->node_number; node_i++)
    node_print (&(scenario->nodes[node_i]));
  for (object_i = 0; object_i < scenario->object_number; object_i++)
    object_print (&(scenario->objects[object_i]));
  for (environment_i = 0; environment_i < scenario->environment_number;
       environment_i++)
    environment_print (&(scenario->environments[environment_i]));
  for (motion_i = 0; motion_i < scenario->motion_number; motion_i++)
    motion_print (&(scenario->motions[motion_i]));
  for (connection_i = 0; connection_i < scenario->connection_number;
       connection_i++)
    connection_print (&(scenario->connections[connection_i]));

#endif

  // initialize scenario state
  INFO ("\n-- Scenario initialization:");
  fprintf (stderr, "\n-- Scenario initialization:\n");

  if (scenario_init_state (scenario, xml_scenario->jpgis_filename_provided,
			   xml_scenario->jpgis_filename,
			   xml_scenario->cartesian_coord_syst,
			   deltaQ_disabled) == ERROR)
    {
      fflush (stdout);
      WARNING ("Error during scenario initialization. Aborting...");
      goto ERROR_HANDLE;
    }



  // it is now late enough to output objects if enabled
  if (object_output_enabled == TRUE)
    {
      // prepare object output filename
      strncpy (object_output_filename, output_filename_base, MAX_STRING - 1);

      if (strlen (object_output_filename) > MAX_STRING - 5)
	{
	  WARNING ("Cannot create object output file name because input \
filename '%s' exceeds %d characters!", output_filename_base, MAX_STRING - 5);
	  goto ERROR_HANDLE;
	}

      // append extension ".obj"
      strncat (object_output_filename, ".obj",
	       MAX_STRING - strlen (object_output_filename) - 5);
      object_output_file = fopen (object_output_filename, "w");
      if (object_output_file == NULL)
	{
	  WARNING ("Cannot open object output file '%s' for writing!",
		   object_output_filename);
	  goto ERROR_HANDLE;
	}

      // write object output
      io_write_objects (scenario, xml_scenario->cartesian_coord_syst,
			object_output_file);

      fclose (object_output_file);
    }



  ////////////////////////////////////////////////////////////
  // processing phase

  // start processing the scenario
  INFO ("\n-- Scenario processing:");
  fprintf (stderr, "\n-- Scenario processing:\n");

  for (current_time = xml_scenario->start_time;
       current_time <=
       (xml_scenario->duration + xml_scenario->start_time + EPSILON);
       current_time += xml_scenario->step)
    {
      // print the current state
      INFO ("* Current time=%.3f", current_time);
      fprintf (stderr,
	       "* Time=%.3f s: DONE                                 \r",
	       current_time);

      // save current time for internal use
      scenario->current_time = current_time;

      // check if motion output is enabled
      if (motion_output_enabled == TRUE)
	{
	  if (current_time == xml_scenario->start_time)
	    {
	      // write motion file header
	      if (motion_output_type == MOTION_OUTPUT_NAM)
		io_write_nam_motion_header_to_file (scenario, motion_file);
	      else
		io_write_ns2_motion_header_to_file (scenario, motion_file);
	    }
	  else
	    {
	      // write motion info 
	      if (motion_output_type == MOTION_OUTPUT_NAM)
		io_write_nam_motion_info_to_file (scenario, motion_file,
						  current_time);
	      else
		io_write_ns2_motion_info_to_file (scenario, motion_file,
						  current_time);
	    }
	}

#ifdef ADD_NOISE
      // special noise addition
      if (current_time >= NOISE_START1 && current_time < NOISE_STOP1)
	scenario->environments[0].noise_power[0] = NOISE_LEVEL1;
      else if (current_time >= NOISE_START2 && current_time < NOISE_STOP2)
	scenario->environments[0].noise_power[0] = NOISE_LEVEL2;
      else
	scenario->environments[0].noise_power[0] = NOISE_DEFAULT;
#endif

#ifdef AUTO_CONNECT_ACTIVE_TAGS

      INFO ("Auto-connecting active tag nodes...");
      if (scenario_auto_connect_nodes_at (scenario) == ERROR)
	{
	  WARNING ("Error auto-connecting active tag nodes");
	  return ERROR;
	}
#endif

      if (deltaQ_disabled == FALSE)
	{
	  // compute deltaQ parameters
	  INFO ("  DELTA_Q CALCULATION");
	  if (scenario_deltaQ (scenario, current_time) == ERROR)
	    {
	      WARNING ("Error while calculating deltaQ. Aborting...");
	      goto ERROR_HANDLE;
	    }
	}

      // check if binary output is enabled
      if (binary_output_enabled == TRUE)
	{
	  io_connection_state.binary_time_record.time = current_time;
	  io_connection_state.binary_time_record.record_number = 0;
	}

      // write all node status to files
      for (connection_i = 0; connection_i < scenario->connection_number;
	   connection_i++)
	{
	  struct connection_class *connection
	    = &(scenario->connections[connection_i]);

	  // do not output connections which start from a noise
	  // source, since they are only meant to be used internally
	  // for interference computation purposes
	  if (scenario->nodes
	      [connection->from_node_index].interfaces[connection->
						       from_interface_index].
	      noise_source == TRUE)
	    continue;

	  // check if text output is enabled
	  if (text_output_enabled == TRUE)
	    io_write_to_file (&(scenario->connections[connection_i]),
			      scenario, current_time,
			      xml_scenario->cartesian_coord_syst,
			      text_output_file);

	  // check if binary output is enabled
	  if (binary_output_enabled == TRUE)
	    {
	      // check if we are processing first time
	      if (current_time == xml_scenario->start_time)
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
		  if (io_binary_compare_record
		      (&(io_connection_state.binary_records[connection_i]),
		       &(scenario->connections[connection_i]),
		       scenario) == FALSE)
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
      if (binary_output_enabled == TRUE)
	{
	  int record_i, connection_i;

#ifdef DISABLE_EMPTY_TIME_RECORDS
	  if (io_connection_state.binary_time_record.record_number > 0)
	    {
#endif
	      time_rec_num++;

	      io_binary_write_time_record_to_file2
		(&(io_connection_state.binary_time_record),
		 binary_output_file);

	      record_i = 0;
	      for (connection_i = 0;
		   connection_i < scenario->connection_number; connection_i++)
		{

		  struct connection_class *connection
		    = &(scenario->connections[connection_i]);

		  // do not output connections which start from a noise
		  // source, since they are only meant to be used internally
		  // for interference computation purposes
		  if (scenario->nodes
		      [connection->from_node_index].interfaces[connection->
							       from_interface_index].
		      noise_source == TRUE)
		    continue;

		  if (io_connection_state.state_changed[connection_i] == TRUE)
		    {
		      io_binary_write_record_to_file2
			(&(io_connection_state.binary_records[connection_i]),
			 binary_output_file);
#ifdef MESSAGE_DEBUG
		      io_binary_print_record
			(&(io_connection_state.binary_records[connection_i]));
#endif

		      // check if there are any more records 
		      // to write for this time interval
		      if (record_i <
			  io_connection_state.
			  binary_time_record.record_number)
			record_i++;
		      else
			break;
		    }
		}

	      if (record_i <
		  io_connection_state.binary_time_record.record_number)
		WARNING ("At time %f wrote ONLY %d binary records out of %d",
			 current_time, record_i,
			 io_connection_state.
			 binary_time_record.record_number);
	      else
		INFO ("At time %f wrote %d binary records", current_time,
		      record_i);
#ifdef DISABLE_EMPTY_TIME_RECORDS
	    }
#endif
	}

      for (divider_i = 0; divider_i < xml_scenario->motion_step_divider;
	   divider_i++)
	{
	  motion_current_time = current_time + divider_i * motion_step;

	  if (motion_current_time >
	      (xml_scenario->duration + xml_scenario->start_time + EPSILON))
	    break;

	  // move nodes according to the 'motions' object in 'scenario'
	  // for the next step of evaluation
	  INFO ("  NODE MOVEMENT (sub-step %d)", divider_i);
	  motion_found = FALSE;
	  for (motion_i = 0; motion_i < scenario->motion_number; motion_i++)
	    if ((scenario->motions[motion_i].start_time
		 <= motion_current_time) &&
		(scenario->motions[motion_i].stop_time > motion_current_time))
	      {
		fprintf (stderr, "\t\t\tcalculation => \
motion %d             \r", motion_i);
		// TEMPORARY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		//if(motion_i!=45) continue;     // 0 corresponds to id 1

		if (motion_apply (&(scenario->motions[motion_i]), scenario,
				  motion_current_time, motion_step) == ERROR)
		  goto ERROR_HANDLE;

		motion_found = TRUE;
	      }
#ifdef MESSAGE_INFO
	  if (motion_found == FALSE)
	    INFO ("  No valid motion found");
#endif
	}
    }

  // check if binary output is enabled
  if (binary_output_enabled == TRUE)
    {
      // rewrite binary header now that all information is available
      rewind (binary_output_file);
      io_binary_write_header_to_file
	(scenario->if_num, time_rec_num,
	 MAJOR_VERSION, MINOR_VERSION, SUBMINOR_VERSION,
	 svn_revision, binary_output_file);
    }

  // write settings file
  if (!(text_only_enabled || binary_only_enabled))
    io_write_settings_file (scenario, settings_file);

  // if no errors occurred, skip next instruction
  goto FINAL_HANDLE;

ERROR_HANDLE:
  error_status = ERROR;


  ////////////////////////////////////////////////////////////
  // finalizing phase

FINAL_HANDLE:

  if (error_status == SUCCESS)
    {
      // print this in case of successful processing
      INFO ("\n-- Scenario processing completed successfully\n\n");
      fprintf (stderr,
	       "\n\n-- Scenario processing completed successfully\n\n");
    }
  else
    {
      // print this in case of unsuccessful processing
      INFO ("\n-- Scenario processing completed with errors \
(see the WARNING messages above)\n\n");
      fprintf (stderr, "\n\n-- Scenario processing completed with errors \
(see the WARNING messages above)\n\n");
    }

  // close output files
  if (scenario_file != NULL)
    fclose (scenario_file);

  if (settings_file != NULL)
    fclose (settings_file);

  // check if text output is enabled
  if (text_output_file != NULL)
    fclose (text_output_file);

  // check if binary output is enabled
  if (binary_output_file != NULL)
    fclose (binary_output_file);

  // check if motion output is enabled
  if (motion_file != NULL)
    fclose (motion_file);

  if (xml_scenario != NULL)
    free (xml_scenario);

  return error_status;
}
