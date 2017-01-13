
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
 * File name: io.h
 * Function: Header file of io.c
 *
 * Author: Razvan Beuran
 *
 * $Id: io.h 161 2014-02-04 00:32:16Z razvan $
 *
 ***********************************************************************/

#ifndef __IO_H
#define __IO_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#include "deltaQ.h"


//////////////////////////////////
// Constants
//////////////////////////////////

#define DEFAULT_NS2_SPEED       1e6


//////////////////////////////////
// Binary I/O file structures
//////////////////////////////////

// binary file header
typedef struct binary_header_class
{
  char signature[4];
  int major_version;
  int minor_version;
  int subminor_version;
  int svn_revision;
  //char reserved[4];
  int interface_number;
  int time_record_number;

} bin_hdr_cls;

// binary file time record
typedef struct binary_time_record_class
{
  float time;
  int record_number;
} bin_time_rec_cls;

// binary file record holding most important fields
// NOTE: update 'io_binary_print_record', 'io_binary_build_record', 
// 'io_copy_record' and 'io_binary_compare_record' when making changes
typedef struct binary_record_class
{
  int from_id;
  int to_id;
  float frame_error_rate;
  float num_retransmissions;
  int standard;
  float operating_rate;
  float bandwidth;
  float loss_rate;
  float delay;
  //float jitter; // not needed yet
} bin_rec_cls;


typedef struct io_connection_state_class
{
  struct binary_time_record_class binary_time_record;
  struct binary_record_class binary_records[MAX_CONNECTIONS];
  int state_changed[MAX_CONNECTIONS];
} io_cls;


////////////////////////////////////////////////
// Text I/O functions
////////////////////////////////////////////////

// write the header of the file in which connection description will be stored
void io_write_header_to_file (FILE * file_global, char *qomet_name);

// write connection description to file
void io_write_to_file (struct connection_class *connection,
		       struct scenario_class *scenario, double time,
		       int cartesian_coord_syst, FILE * file_global);

// write header of motion file in NAM format;
// return SUCCESS on succes, ERROR on error
int io_write_nam_motion_header_to_file (struct scenario_class *scenario,
					FILE * motion_file);

// write motion file information in NAM format;
// return SUCCESS on succes, ERROR on error
int io_write_nam_motion_info_to_file (struct scenario_class *scenario,
				      FILE * motion_file, float time);

// write header of motion file in NS-2 format;
// return SUCCESS on succes, ERROR on error
int io_write_ns2_motion_header_to_file (struct scenario_class *scenario,
					FILE * motion_file);

// write header of motion file in NS-2 format;
// return SUCCESS on succes, ERROR on error
int io_write_ns2_motion_header_to_file_new (struct scenario_class *scenario,
					    FILE *motion_file, float time);

// write motion file information in NAM format;
// return SUCCESS on succes, ERROR on error
int io_write_ns2_motion_info_to_file (struct scenario_class *scenario,
				      FILE * motion_file, float time);

// write motion file information in NAM format;
// return SUCCESS on succes, ERROR on error
int io_write_ns2_motion_info_to_file_new (struct scenario_class *scenario,
					  FILE * motion_file, float time);

// write objects to file;
// return SUCCESS on succes, ERROR on error
int io_write_objects (struct scenario_class *scenario,
		      int cartesian_coord_syst, FILE * object_file);

// generate the settings file based on scenario properties
int io_write_settings_file (struct scenario_class *scenario,
			    FILE * settings_file);

// read the scenario settings (ids and corresponding IP adresses)
// from a file, and store the adresses in the arrays p (binary) and 
// p_char (string) at the corresponding index;
// return the number of addresses (=interfaces) successfully read, 
// or ERROR on error
int io_read_settings_file (char *settings_filename,
			   in_addr_t * p, char *p_char, int p_size);

// read the MAC-level scenario settings (ids and corresponding MAC
// adresses) from a file, and store the addresses in the array
// 'mac_addresses' (binary form) at the corresponding index;
// 'array_size' represents the number of entries allocated in the
// 'mac_addresses' array;
// return the number of addresses (=interfaces) successfully read, 
// or ERROR on error
int io_read_settings_file_mac (char *settings_filename,
			       in_addr_t * p, char *p_char,
			       unsigned char mac_addresses[][ETH_SIZE],
			       char mac_char_addresses[][MAC_ADDR_SIZE],
			       int array_size);


////////////////////////////////////////////////
// Binary I/O functions
////////////////////////////////////////////////

// print binary header
void io_binary_print_header (struct binary_header_class *binary_header);

// print binary time record
void io_binary_print_time_record (struct binary_time_record_class
				  *binary_time_record, FILE *logfd);

// print binary record
void io_binary_print_record (struct binary_record_class *binary_record, FILE *logfd);

// copy binary record
void io_binary_copy_record (struct binary_record_class *binary_record_dst,
			    struct binary_record_class *binary_record_src);

// build binary record
void io_binary_build_record (struct binary_record_class *binary_record,
			     struct connection_class *connection,
			     struct scenario_class *scenario);

// compare with binary record;
// return TRUE if data is same with the one in the record,
// FALSE otherwise
int io_binary_compare_record (struct binary_record_class *binary_record,
			      struct connection_class *connection,
			      struct scenario_class *scenario);

// read header of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_binary_read_header_from_file (struct binary_header_class
				     *binary_header,
				     FILE * binary_input_file);

// write header of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_binary_write_header_to_file (int interface_number,
				    long int time_record_number,
				    int major_version,
				    int minor_version,
				    int subminor_version,
				    int svn_revision, FILE * binary_file);

// read a time record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_binary_read_time_record_from_file (struct binary_time_record_class
					  *binary_time_record,
					  FILE * binary_input_file);

// write a time record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_binary_write_time_record_to_file (float time, int record_number,
					 FILE * binary_file);

// directly write a time record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_binary_write_time_record_to_file2 (struct binary_time_record_class
					  *binary_time_record,
					  FILE * binary_file);

// read a record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_binary_read_record_from_file (struct binary_record_class
				     *binary_record,
				     FILE * binary_input_file);

// read 'number_records' records from a QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_binary_read_records_from_file (struct binary_record_class
				      *binary_records, int number_records,
				      FILE * binary_input_file);

// write a record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_binary_write_record_to_file (struct connection_class *connection,
				    struct scenario_class *scenario,
				    FILE * binary_file);

// directly write a record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_binary_write_record_to_file2 (struct binary_record_class
				     *binary_record, FILE * binary_file);

#endif
