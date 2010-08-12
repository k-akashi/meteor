
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
 * File name: do_chanel.c
 * Function: Example of usage of chanel emulation library
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

#include <sys/time.h>

#include "global.h"
#include "message.h"
#include "chanel.h"

int sent_data_count=0;
int received_data_count=0;
int error_data_count=0;

//#define SLOW_TEST

#ifdef SLOW_TEST

// slow test
#define OUTPUT_SLEEP_TIME        700000//0 // us
#define MAX_DATA_ITEMS           20 //20000

#else
// fast test
#define OUTPUT_SLEEP_TIME        0 // us
#define MAX_DATA_ITEMS           20000

#endif

#define FINAL_SLEEP_TIME         7 //s


// callback function for receive operations (incoming data);
// this function is called from within chanel automatically
void callback_recv_function(void *callback_data, data_class *data_object)
{
  // we ignore the "callback_data" argument here 
  // since it is only used with RUNE

  int j, chksum;
  char *data_bytes=(char *)(data_object->data);

  chksum=0;
  for(j=0; j<data_object->data_length-1; j++)
    chksum+=data_bytes[j];

  if(data_bytes[data_object->data_length-1]==(char)chksum)
    {
      INFO_("Received data in callback function: OK (0x%02hhx)", 
	    data_bytes[0]);
    }
  else
    {
      INFO_("Received data in callback function: ERROR (0x%02hhx)", 
	    data_bytes[0]);
      error_data_count++;
    }

  received_data_count++;
}


// used only for testing basic chanel functionality
int main(void)
{
  data_class data_object;

  data_type_enum data_type = DATA_TYPE_BYTE;
  //char data_bits[]={0, 1, 0, 1};
  int data_length = 8;
  char data_bytes[8];//={0x11,0x22,0x33, 0x44};
  int chanel_source = 1;
  int chanel_destination = 2; //BROADCAST_ID; //2;
  double time;

  int i=0, j, value=0, chksum;

  chanel_class chanel;
  deltaQ_class deltaQ;

  struct timespec time_value;
  struct timespec start_time;
  struct timespec stop_time;

  double TIME_SUBSTRACT         =-1;
  double duration;

  fprintf(stderr, "Starting execution...\n");


  ////////////////////////////////////////////////////////////
  // Starting chanel
  ////////////////////////////////////////////////////////////
  clock_gettime(CLOCK_REALTIME, &time_value);

  TIME_SUBSTRACT = time_value.tv_sec + time_value.tv_nsec/1e9;
  INFO_("READ INITIAL TIME (ONLY FIRST TIME): time_s=%ld time_ns=%ld",
	(long int)time_value.tv_sec, (long int)time_value.tv_nsec);

  // execute chanel for node with id chanel_source and without 
  // callback function
  if(chanel_execute(&chanel, chanel_source, OUTPUT_SLEEP_TIME,
		    &callback_recv_function, TIME_SUBSTRACT)==ERROR)
    {
      WARNING_("Error executing chanel");
      return ERROR;
    }

  // give time for all threads to start execution
  INFO_("Sleeping 1 s...");
  sleep(1);


  ////////////////////////////////////////////////////////////
  // Configuring chanel
  ////////////////////////////////////////////////////////////

  // configure destination for chanel
  deltaQ_set_frame_error_rate(&deltaQ, 0.0);
  chanel_set_deltaQ(&chanel, chanel_destination, &deltaQ);
  

  ////////////////////////////////////////////////////////////
  // Using chanel
  ////////////////////////////////////////////////////////////

  clock_gettime(CLOCK_REALTIME, &start_time);

  // produce some data and send to chanel
  while(i<MAX_DATA_ITEMS)
    {
      // build data_class object (usually done from some 
      // data source)
      chksum=0;
      for(j=0; j<data_length-1; j++)
	{
	  value++;
	  chksum+=value;
	  data_bytes[j]=(char)value;
	}
      data_bytes[data_length-1]=(char)chksum;

      clock_gettime(CLOCK_REALTIME, &time_value);
      time = time_value.tv_sec + time_value.tv_nsec/1e9 - TIME_SUBSTRACT;

      data_init(&data_object, chanel_source, chanel_destination, time, 
		data_bytes, data_type, data_length);
      printf("*** Send data item %4d to chanel at time=%f s\n", 
	     i, data_object.time);
      fflush(stdout);

      sent_data_count++;

      if(chanel_send_data(&chanel, &data_object)==ERROR)
	{
	  WARNING_("Error sending data to chanel");
	}

      /*
      if(i==12)
	{
	  //time+=5.0;
	  //INFO_("Sleeping 5 s...");
	  //sleep(5);

	  INFO_("Reconfiguring chanel...");
	  // configure destination for chanel
	  deltaQ_set_destination(&deltaQ, chanel_destination);
	  deltaQ_set_frame_error_rate(&deltaQ, 0.5);
	  chanel_set_deltaQ(&chanel, &deltaQ);
	}
      */

      i++;

      //usleep(1);
      /*
      {
	unsigned long int adder;
	while(adder<100000)
	  adder++;
      }
      */

    }


  ////////////////////////////////////////////////////////////
  // Stopping chanel
  ////////////////////////////////////////////////////////////
  INFO_("Sleeping %d s...", FINAL_SLEEP_TIME);
  sleep(FINAL_SLEEP_TIME);
  chanel_stop(&chanel);

  clock_gettime(CLOCK_REALTIME, &stop_time);

  // wait for finalization of chanel execution
  if(chanel_finalize(&chanel)==ERROR)
    {
      WARNING_("Error finalizing chanel");
      return ERROR;
    }

  // print statistics
  duration = stop_time.tv_sec + stop_time.tv_nsec/1e9 - 
    start_time.tv_sec - start_time.tv_nsec/1e9 - FINAL_SLEEP_TIME;
  fprintf(stderr, "Experiment duration: %.4f s; sleep time in output \
function: %.4f s\n", duration, (double)OUTPUT_SLEEP_TIME/1e6);

  fprintf(stderr, "Sent %d data items, received %d data items \
(errored items=%d) => %.2f loss rate, %.2f error rate\n", 
	  sent_data_count, received_data_count, error_data_count,
	  (float)(sent_data_count-received_data_count)/sent_data_count,
	  (float)(error_data_count)/received_data_count);

  fprintf(stderr, "Input rate %.4f bps, output rate %.4f bps (error rate \
%.4f bps)\n", sent_data_count * 8 * 8/duration, 
	  received_data_count * 8 * 8/duration, 
	  error_data_count * 8 * 8/duration);

  return SUCCESS;
}

