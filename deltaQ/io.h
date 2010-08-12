
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
 * File name: io.h
 * Function: Header file of io.c
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/

#ifndef __IO_H
#define __IO_H

#include "deltaQ.h"


//////////////////////////////////
// Constants
//////////////////////////////////

#define DEFAULT_NS2_SPEED       1e6


//////////////////////////////////
// Binary I/O file structures
//////////////////////////////////

// binary file header
typedef struct
{
  char signature[4];
  int major_version;
  int minor_version;
  int subminor_version;
  int svn_revision;
  char reserved[4];
  long int time_record_number;
} binary_header_class;

// binary file time record
typedef struct
{
  float time;
  int record_number;
} binary_time_record_class;

// binary file record
// (only loss rate for now)
typedef struct
{
  //float time;
  int from_node;
  int to_node;
  float loss_rate;
} binary_record_class;

typedef struct
{
  binary_time_record_class binary_time_record;
  binary_record_class binary_records[MAX_CONNECTIONS];
  int state_changed[MAX_CONNECTIONS];
} io_connection_state_class;


////////////////////////////////////////////////
// Text I/O functions
////////////////////////////////////////////////

// write the header of the file in which connection description will be stored
void io_write_header_to_file(FILE *file_global, char *qomet_name);

// write connection description to file
void io_write_to_file(connection_class *connection,
		      scenario_class *scenario, double time, 
		      int cartesian_coord_syst,
		      FILE *file_global);

// write header of motion file in NAM format;
// return SUCCESS on succes, ERROR on error
int io_write_nam_motion_header_to_file(scenario_class *scenario, 
				       FILE *motion_file);

// write motion file information in NAM format;
// return SUCCESS on succes, ERROR on error
int io_write_nam_motion_info_to_file(scenario_class *scenario, 
				     FILE *motion_file, float time);

// write header of motion file in NS-2 format;
// return SUCCESS on succes, ERROR on error
int io_write_ns2_motion_header_to_file(scenario_class *scenario, 
				       FILE *motion_file);

// write motion file information in NAM format;
// return SUCCESS on succes, ERROR on error
int io_write_ns2_motion_info_to_file(scenario_class *scenario, 
				     FILE *motion_file, float time);

////////////////////////////////////////////////
// Binary I/O functions
////////////////////////////////////////////////

// print binary header
void io_binary_print_header(binary_header_class *binary_header);

// print binary time record
void io_binary_print_time_record(binary_time_record_class *binary_time_record);

// print binary record
void io_binary_print_record(binary_record_class *binary_record);

// build binary record
void io_binary_build_record(binary_record_class *binary_record, 
			    connection_class *connection,
			    scenario_class *scenario);

// compare with binary record;
// return TRUE if data is same with the one in the record,
// FALSE otherwise
int io_binary_compare_record(binary_record_class *binary_record, 
			     connection_class *connection,
			     scenario_class *scenario);

// read header of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_read_binary_header_from_file(binary_header_class *binary_header,
				    FILE *binary_input_file);

// write header of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_write_binary_header_to_file(long int time_record_number, 
				   int major_version,
				   int minor_version,
				   int subminor_version,
				   int svn_revision,
				   FILE *binary_file);

// read a time record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_read_binary_time_record_from_file(binary_time_record_class *
					 binary_time_record,
					 FILE *binary_input_file);

// write a time record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_write_binary_time_record_to_file(float time, int record_number,
					FILE *binary_file);

// directly write a time record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_write_binary_time_record_to_file2(binary_time_record_class *
					 binary_time_record, FILE *binary_file);

// read a record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_read_binary_record_from_file(binary_record_class *binary_record,
				    FILE *binary_input_file);

// read 'number_records' records from a QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_read_binary_records_from_file(binary_record_class *binary_records,
				     int number_records,
				     FILE *binary_input_file);

// write a record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_write_binary_record_to_file(connection_class *connection,
				   scenario_class *scenario,
				   FILE *binary_file);

// directly write a record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_write_binary_record_to_file2(binary_record_class *binary_record,
				    FILE *binary_file);

#endif
