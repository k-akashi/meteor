
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
 * File name: show_bin.c
 * Function: Display binary QOMET output in human-readable form
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 143 $
 *   $LastChangedDate: 2009-03-31 11:30:13 +0900 (Tue, 31 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#include <stdio.h>
#include <string.h>

#include "deltaQ.h"


///////////////////////////////////
// Generic variables and functions
///////////////////////////////////

#define MESSAGE_WARNING
//#define MESSAGE_DEBUG
#define MESSAGE_INFO

#ifdef MESSAGE_WARNING
#define WARNING(message...) do {                                          \
  fprintf(stderr, "show_bin WARNING: %s, line %d: ", __FILE__, __LINE__); \
  fprintf(stderr, message); fprintf(stderr,"\n");                         \
} while(0)
#else
#define WARNING(message...)	/* message */
#endif

#ifdef MESSAGE_DEBUG
#define DEBUG(message...) do {                                          \
  fprintf(stdout, "show_bin DEBUG: %s, line %d: ", __FILE__, __LINE__); \
  fprintf(stdout, message); fprintf(stdout,"\n");                       \
} while(0)
#else
#define DEBUG(message...)	/* message */
#endif

#ifdef MESSAGE_INFO
#define INFO(message...) do {                                       \
  fprintf(stdout, message); fprintf(stdout,"\n");                   \
} while(0)
#else
#define INFO(message...)	/* message */
#endif

// print usage info
static void
usage (FILE * f)
{
  fprintf (f, "\nshow_bin. Display binary QOMET output as text.\n\n");
  fprintf (f, "Usage: show_bin <scenario_file.xml.bin>\n");
  fprintf (f, "\n");
  fprintf (f, "See documentation for more details.\n");
  fprintf (f,
	   "Please send comments and bug reports to 'info@starbed.org'.\n\n");
}

int
main (int argc, char *argv[])
{
  // binary file name and descriptor
  char bin_filename[MAX_STRING];
  FILE *bin_file;

  // binary file header data structure
  struct bin_hdr_cls bin_hdr;

  // binary file time record data structure
  struct binary_time_record_class binary_time_record;

  // binary file record data structure and max count
  struct bin_rec_cls *binary_records = NULL;
  int binary_records_max_count;

  // counters for time and binary records
  long int time_i;
  int rec_i;

  INFO ("\nshow_bin. Display binary QOMET output as text.\n");

  printf
    ("------------------------------------------------------------------------\n");
  printf ("System information:     ");
#ifdef __linux__
  printf ("Linux OS, ");
#elif __freebsd__
  printf ("FreeBSD OS, ");
#elif __macosx__
  printf ("Mac OS X, ");
#else
  printf ("Unknown OS, ");
#endif
#if __WORDSIZE == 32
  printf ("32 bit architecture\n");
#elif __WORDSIZE == 64
  printf ("64 bit architecture\n");
#else
  printf ("unknown architecture\n");
#endif
  printf ("Variable sizes [bytes]: ");

  //#if __WORDSIZE == 64
  //#define PRI_SIZEOF "lu"
  //#else
  //#define PRI_SIZEOF "u"
  //#endif

  printf ("char=%zu  short=%zu  int=%zu  long=%zu  float=%zu  double=%zu\n",
	  sizeof (char), sizeof (short), sizeof (int), sizeof (long),
	  sizeof (float), sizeof (double));

  printf ("Data structure [bytes]: ");
  printf ("bin_hdr_cls=%zu  binary_time_record_class=%zu  \
bin_rec_cls=%zu\n", sizeof (struct bin_hdr_cls), sizeof (struct binary_time_record_class), sizeof (struct bin_rec_cls));
  printf
    ("------------------------------------------------------------------------\n");

  // check argument count
  if (argc <= 1)
    {
      WARNING ("No binary QOMET output file was provided");
      usage (stdout);
      exit (1);
    }

  // save binary filename
  strncpy (bin_filename, argv[1], MAX_STRING - 1);

  INFO ("\nShowing file '%s'...\n", bin_filename);

  // open binary file
  bin_file = fopen (argv[1], "r");
  if (bin_file == NULL)
    {
      WARNING ("Cannot open binary file '%s'", bin_filename);
      exit (1);
    }

  // read and check binary header
  if (io_binary_read_header_from_file (&bin_hdr, bin_file) == ERROR)
    {
      WARNING ("Aborting on input error (binary header)");
      // close binary file
      fclose (bin_file);

      exit (1);
    }
  printf ("* HEADER INFORMATION:\n");
  io_binary_print_header (&bin_hdr);

  // allocate memory for binary records
  binary_records_max_count = bin_hdr.if_num
    * (bin_hdr.if_num - 1);
  binary_records =
    (struct bin_rec_cls *) calloc (binary_records_max_count,
					   sizeof (struct
						   bin_rec_cls));
  if (binary_records == NULL)
    {
      WARNING ("Cannot allocate memory for records");
      // close binary file
      fclose (bin_file);

      exit (1);
    }

  printf ("* RECORD CONTENT:\n");
  // read all records
  for (time_i = 0; time_i < bin_hdr.time_rec_num; time_i++)
    {
      // read time record
      if (io_binary_read_time_record_from_file (&binary_time_record,
						bin_file) == ERROR)
	{
	  WARNING ("Aborting on input error (time record)");
	  // close binary file
	  fclose (bin_file);

	  exit (1);
	}
      io_binary_print_time_record (&binary_time_record);

      if (binary_time_record.record_number > binary_records_max_count)
	{
	  WARNING ("Number of records exceeds maximum (%d)",
		   binary_records_max_count);
	  // close binary file
	  fclose (bin_file);

	  exit (1);
	}

      // read binary records
      if (io_binary_read_records_from_file (binary_records,
					    binary_time_record.record_number,
					    bin_file) == ERROR)
	{
	  WARNING ("Aborting on input error (records)");
	  // close binary file
	  fclose (bin_file);

	  exit (1);
	}

      // print binary records
      for (rec_i = 0; rec_i < binary_time_record.record_number; rec_i++)
	io_binary_print_record (&binary_records[rec_i]);
    }

  // free memory for binary records
  free (binary_records);

  // close binary file
  fclose (bin_file);

  exit (0);
}
