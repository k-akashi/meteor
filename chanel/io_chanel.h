
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
 * File name: io_chanel.h (SPECIAL SIMPLIFIED VERSION FOR RUNE)
 * Function: Header file of io_chanel.c
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 93 $
 *   $LastChangedDate: 2008-09-12 11:16:33 +0900 (Fri, 12 Sep 2008) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/

#ifndef __IO_CHANEL_H
#define __IO_CHANEL_H


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

// print binary header
void io_binary_print_header(binary_header_class *binary_header);

// print binary time record
void io_binary_print_time_record(binary_time_record_class *binary_time_record);

// print binary record
void io_binary_print_record(binary_record_class *binary_record);

// read header of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_read_binary_header_from_file(binary_header_class *binary_header,
				    FILE *binary_input_file);

// read a time record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_read_binary_time_record_from_file(binary_time_record_class *
					 binary_time_record,
					 FILE *binary_input_file);

// return SUCCESS on succes, ERROR on error
int io_read_binary_record_from_file(binary_record_class *binary_record,
				    FILE *binary_input_file);

// read 'number_records' records from a QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_read_binary_records_from_file(binary_record_class *binary_records,
				     int number_records,
				     FILE *binary_input_file);

#endif
