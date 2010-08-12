
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
 * File name: chanel_config.c
 * Function: Main source file of the chanel configuration test program
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 93 $
 *   $LastChangedDate: 2008-09-12 11:16:33 +0900 (Fri, 12 Sep 2008) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>

#include "global.h"
#include "message.h"
#include "chanel.h"
#include "io_chanel.h"

#define READ_FROM_FILE
#define QOMET_FILENAME   "../scenario_plt-full5.xml.opt.bin"   // MUST CHECK

// used only for testing the pipe based configuration mechanism
int main(void)
{  
#ifndef READ_FROM_FILE
  int chanel_pipe_id;
#endif
  pipe_data_class pipe_data;

#ifdef READ_FROM_FILE

  int time_i, rec_i;
  binary_header_class binary_header;
  binary_time_record_class binary_time_record;
  binary_record_class binary_records[MAX_RECORDS];
  FILE *fd;

#endif


#ifdef READ_FROM_FILE

  // set destination
  pipe_data.source = 2;

  // set destination
  pipe_data.destination = 0;

  //////////////////////////////////
  // Initialize configuration pipe
  //////////////////////////////////
  /*
  // the function below is successful only if the chanel was
  // already started (e.g., by using 'do_chanel')
  chanel_pipe_id = chanel_configuration_pipe_open(pipe_data.source);

  if(chanel_pipe_id==ERROR)
    {
      WARNING_("Cannot open deltaQ configuration pipe. 
              Check that chanel was started");
      return ERROR;
    }
  */
  // open chanel configuration file (QOMET output)
  if((fd = fopen(QOMET_FILENAME, "r")) == NULL)
    {
      INFO_("Could not open QOMET output file '%s'", QOMET_FILENAME);
      return ERROR;
    }

  // read and check binary header
  if(io_read_binary_header_from_file(&binary_header, fd)==ERROR)
    {
      INFO_("Aborting on input error (binary header)");
      return ERROR;
    }
  io_binary_print_header(&binary_header);


  for(time_i=0; time_i<binary_header.time_record_number; time_i++)
    {
      if(io_read_binary_time_record_from_file(&binary_time_record, 
					      fd)==ERROR)
	{
	  INFO_("Aborting on input error (time record)");
	  return ERROR;
	}
      io_binary_print_time_record(&binary_time_record);

      //if(binary_time_record.record_number==0)
      //	continue;

      if(binary_time_record.record_number>MAX_RECORDS)
	{
	  INFO_("The number of records to be read exceeds allocated size (%d)",
		MAX_RECORDS);
	  return ERROR;
	}

      if(io_read_binary_records_from_file(binary_records, 
					  binary_time_record.record_number,
					  fd)==ERROR)
	{
	  INFO_("Aborting on input error (records)");
	  return ERROR;
	}

      for(rec_i=0; rec_i<binary_time_record.record_number; rec_i++)
	{
	  if(binary_records[rec_i].from_node == pipe_data.source &&
	     binary_records[rec_i].to_node == pipe_data.destination)
	    io_binary_print_record(&(binary_records[rec_i]));
	}
    }

#else

  //////////////////////////////////
  // Initialize configuration pipe
  //////////////////////////////////

  // set destination
  pipe_data.source = 0;

  // set destination
  pipe_data.destination = 2;

  chanel_pipe_id = chanel_configuration_pipe_open(pipe_data.source);

  if(chanel_pipe_id==ERROR)
    {
      WARNING_("Cannot open deltaQ configuration pipe");
      return ERROR;
    }


  //////////////////////////////////
  // Configure chanel
  //////////////////////////////////

  // clear configuration
  deltaQ_clear(&(pipe_data.deltaQ));

  // set frame error rate
  deltaQ_set_frame_error_rate(&(pipe_data.deltaQ), 0.9);

  // write configuration to pipe
  chanel_configuration_pipe_write(chanel_pipe_id, &pipe_data);


  //////////////////////////////////
  // Wait until reconfiguring
  //////////////////////////////////

  // sleep for a while
  sleep(3);


  //////////////////////////////////
  // Reconfigure chanel
  //////////////////////////////////

  // set frame error rate
  deltaQ_set_frame_error_rate(&(pipe_data.deltaQ), 0.1);

  // write configuration to pipe
  chanel_configuration_pipe_write(chanel_pipe_id, &pipe_data);

#endif

  return SUCCESS;
}
