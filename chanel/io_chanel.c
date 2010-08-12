
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
 * File name: io_chanel.c (SPECIAL SIMPLIFIED VERSION FOR RUNE)
 * Function: I/O functionality for deltaQ library
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 93 $
 *   $LastChangedDate: 2008-09-12 11:16:33 +0900 (Fri, 12 Sep 2008) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/

#include <stdio.h>
#include <math.h>

#include "io_chanel.h"
#include "global.h"
#include "message.h"


#define GRID_SIZE_X            300.0
#define GRID_SIZE_Y            300.0
#define NODE_RADIUS            5.0

// print binary header
void io_binary_print_header(binary_header_class *binary_header)
{
  // print signature (only first 3 characters)
  printf("Header signature: %c%c%c\n", binary_header->signature[0],
         binary_header->signature[1], binary_header->signature[2]);

  printf("Version information: %d.%d.%d (revision %d)\n",
         binary_header->major_version, binary_header->minor_version,
	 binary_header->subminor_version, binary_header->svn_revision);

  printf("Number of time records: %ld\n", binary_header->time_record_number);
}

// print binary time record
void io_binary_print_time_record(binary_time_record_class *binary_time_record)
{
  printf("> Time: %f s (%d records)\n", binary_time_record->time, 
	 binary_time_record->record_number);
}

// print binary record
void io_binary_print_record(binary_record_class *binary_record)
{
  printf("> > Record: from_node=%d to_node=%d loss_rate=%f\n", 
	 binary_record->from_node, binary_record->to_node,
	 binary_record->loss_rate);
}

// read header of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_read_binary_header_from_file(binary_header_class *binary_header,
				    FILE *binary_input_file)
{
  // read header from file
  if(fread(binary_header, sizeof(binary_header_class),
	   1, binary_input_file)!=1)
    {
      INFO_("Error reading binary header from file");
      return ERROR;
    }

  // check signature (DEFINE SOMEWHERE IN HEADER FILE...)
  if(!(binary_header->signature[0]=='Q' &&
       binary_header->signature[1]=='M' &&
       binary_header->signature[2]=='T' &&
       binary_header->signature[3]=='\0'))
    {
      INFO_("Incorrect signature in binary file");
      return ERROR;
    }

  return SUCCESS;
}

// read a time record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_read_binary_time_record_from_file(binary_time_record_class *
					 binary_time_record,
					 FILE *binary_input_file)
{
  // read time record from file
  if(fread(binary_time_record, sizeof(binary_time_record_class),
	   1, binary_input_file)!=1)
    {
      INFO_("Error reading binary time record from file");
      return ERROR;
    }

  return SUCCESS;
}

// read a record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_read_binary_record_from_file(binary_record_class *binary_record,
				    FILE *binary_input_file)
{
  // record from file
  if(fread(binary_record, sizeof(binary_record_class),
	   1, binary_input_file)!=1)
    {
      INFO_("Error reading binary record from file");
      return ERROR;
    }

  return SUCCESS;
}

// read 'number_records' records from a QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int io_read_binary_records_from_file(binary_record_class *binary_records,
				     int number_records,
				     FILE *binary_input_file)
{
  // records from file
  //printf("Reading %d records from file\n", number_records);
  //fflush(stdout);
  if(fread(binary_record, sizeof(binary_record_class),
	   number_records, binary_input_file)!=number_records)
    {
      INFO_("Error reading binary records from file");
      return ERROR;
    }

  return SUCCESS;
}
