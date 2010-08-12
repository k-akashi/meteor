
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
 * File name: chanel_space.c
 * Function: Chanel emulation library space object file that is
 *           used when integrating chanel with RUNE
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
#include <sys/sysctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <math.h>

#include <runecommon.h>
#include <runeservice.h>

#include "global.h"
#include "message.h"
#include "io_chanel.h"
#include "chanel.h"
#include "tag_public.h"
#include "timer.h"


////////////////////////////////////////////////////////////
// Various defines and constants
////////////////////////////////////////////////////////////

#define BINARY_INPUT

//#define DO_SLEEP

#ifdef DO_SLEEP
#define SLEEP_TIME             10 // us
#endif

#define OUTPUT_SLEEP_TIME       0

#define NUMBER_NODES           23//8//3                 // MUST CHECK
#define QOMET_FILENAME         "scenario_plt.xml.bin"   // MUST CHECK

#ifndef SCALING_FACTOR
#define SCALING_FACTOR          1.0 //1.0 => no scaling; >1.0 => slow down
#endif  /* !SCALING_FACTOR */
                                       
#define PARAMETERS_UNUSED       13
#define PARAMETERS_TOTAL        19
#define MIN_DBL                 -2147483648.0

#define ACCEPTED_ERROR          0.005

#define MEASURE_ALL


////////////////////////////////////////////////////////////
// RUNE-specific entry point definitions
////////////////////////////////////////////////////////////

// chanel space init entry point
void *chanel_space_init(int gsid);

// chanel space step entry point
int chanel_space_step(void *p);

// chanel space finalize entry point
void chanel_space_fin(void *p);

// chanel space read entry point
void *chanel_space_read(void *p, void *a);

// chanel space write entry point
void *chanel_space_write(void *p, void *a);

// entry points for this dynamic object
// (RUNE-specific structure)
entryPoints ep = 
{
  .init = chanel_space_init,
  .step = chanel_space_step,
  .fin = chanel_space_fin,
  .read = chanel_space_read,
  .write = chanel_space_write
};


////////////////////////////////////////////////////////////
// Function definitions
////////////////////////////////////////////////////////////

// chanel space receive data callback function (declaration)
void chanel_space_recv_data(void *p, data_class *data_object);


////////////////////////////////////////////////////////////
// chanel space definitions
////////////////////////////////////////////////////////////

// chanel space structure
typedef struct
{
  // chanel structure
  chanel_class chanel;

  // global id of the chanel space
  int gsid;

  // used for duration measurement
  //double time1, time2, time2_no_scale;
  double current_time, deadline_time;

  // used for error measurement
  double error;

  // flag showing if it new configurations must be read (when TRUE);
  // when FALSE, configurations may be applied if time is right
  int reset_time;

  // flag showing if configuration file reached its end
  int read_finished;


  // local parameters (no mutex?????)
  float time, bandwidth[NUMBER_NODES], delay[NUMBER_NODES], 
    lossrate[NUMBER_NODES];
  int is_filled[NUMBER_NODES]; // CAN BE REMOVED TO IMPROVE SPEED!!!!!!!
  int tid[NUMBER_NODES];
  int from, to;

  // id of the data node associated to this chanel
  // (currently this is calculated!!!)
  int data_node_id;

  // configuration file descriptor
  FILE *fd;

  // configuration file header data structure
  binary_header_class binary_header;

  // number of configuration time records read so far
  long int time_records_read;

  // start time and end time of a time step
  double step_start_time, step_end_time;

  // number of steps that were executed
  // (used only in delay and error reproting)
  long int step_count;


  uint64_t rdtsc_time1, rdtsc_time2, rdtsc_start_time;
  //double rdtsc_additional;
  unsigned int rdtsc_HZ;

  double TIME_SUBSTRACT;

}chanel_space_class;


// compute modulo difference ofn1 and n2
int modulo_difference(int n1, int n2, int modulo)
{
  if(n1>=n2)
    return n1-n2;
  else
    return n1-n2+modulo;
}

// chanel space init entry point
void *chanel_space_init(int gsid)
{
  chanel_space_class *s;
  int i;
  unsigned int size_rdtsc_HZ;

  //int chanel_source = UNDEFINED_ID;
  struct timespec time_value;

  // allocate chanel space structure
  if((s = (chanel_space_class *)malloc(sizeof(*s))) == NULL)
    return NULL;

  // save global id
  s->gsid = gsid;

  s->TIME_SUBSTRACT = -1;

  // compute id of the associated data node
  s->data_node_id=gsid - NUMBER_NODES;

  // initialize error
  s->error = 0;

  // get time
  clock_gettime(CLOCK_REALTIME, &time_value);

  // compute constant to be substracted subsequently
  s->TIME_SUBSTRACT = time_value.tv_sec + time_value.tv_nsec/1e9;
  INFO_("READ INITIAL TIME (ONLY FIRST TIME): time_s=%ld time_ns=%ld",
	    (long int)time_value.tv_sec, (long int)time_value.tv_nsec);
  INFO_("Starting experiment at: %s", ctime(&(time_value.tv_sec)));
  INFO_("\tNUMBER_NODES=%d", NUMBER_NODES);

  // start execution and configure chanel core
  if(chanel_execute(&(s->chanel), s->data_node_id, OUTPUT_SLEEP_TIME,
		    &chanel_space_recv_data, s->TIME_SUBSTRACT)==ERROR)
    {
      INFO_("Error executing chanel (chanel_space_init)");
      return NULL;
    }

  // TO REMOVE
  for(i=0; i<NUMBER_NODES; i++)
    s->is_filled[i] = FALSE;

  // open chanel configuration file (QOMET output)
  if((s->fd = fopen(QOMET_FILENAME, "r")) == NULL)
    {
      INFO_("Could not open QOMET output file '%s'", QOMET_FILENAME);
      return NULL;
    }

  INFO_("Start reading input file %s...", QOMET_FILENAME);

  // read and check binary header
  if(io_read_binary_header_from_file(&(s->binary_header), s->fd)==ERROR)
    {
      INFO_("Aborting on input error (binary header)");
      return NULL;
    }
  io_binary_print_header(&(s->binary_header));

  // init number of time records
  s->time_records_read = 0;

  // prepare variables for first execution
  s->reset_time = TRUE;
  s->read_finished =  FALSE;
  s->time = MIN_DBL;

  s->step_count=0;
  // just give it a big value (SOULD FIX LATER)
  //s->step_end_time=1e12;

  s->rdtsc_time1 = 0;
  s->rdtsc_time2 = 0;
  s->rdtsc_start_time = 0;
  //s->rdtsc_additional = 0.0;
  size_rdtsc_HZ = sizeof(s->rdtsc_HZ);

  // use system call to get CPU frequency
  sysctlbyname("machdep.tsc_freq", &(s->rdtsc_HZ), &size_rdtsc_HZ, NULL, 0);

  // print init finalization message
  INFO_("Chanel (gsid=%d) for Active Tag (data_node_id=%d) init finalized", 
	    s->gsid, s->data_node_id);

  return s;
}

// chanel space step entry point
int chanel_space_step(void *p)
{
  int i;
  chanel_space_class *s = getStorage(p);

  // chanel configuration records
  binary_time_record_class binary_time_record;
  binary_record_class binary_records[NUMBER_NODES*(NUMBER_NODES-1)];

  // count number of executions in one loop
  // (only need to know which is the first one actually!!!!!)
  int counter;

#ifdef MEASURE_ALL
  // used for duration calculation to check too long delays
  double time_diff;
#endif

  struct timespec time_value;

  // get time
  rdtsc(s->rdtsc_time1);

  // compute current time by adding to the initial time 
  //the elapsed time since last computation

  // first time we need to initialize
  if(s->time == MIN_DBL)
    {
      s->rdtsc_start_time = s->rdtsc_time1;
      s->current_time = 0;
    }

#ifdef MEASURE_ALL
  // compute duration spent outside the loop
  // (note unusual order time1-time2)
  if(s->time != MIN_DBL)//first time
    time_diff = (s->rdtsc_time1 - s->rdtsc_time2)/(double)s->rdtsc_HZ;
  else
    time_diff = 0;
  
  if(time_diff>ACCEPTED_ERROR)
    INFO_("%f STEP[%ld]: Time outside step function=%f", 
	  (s->rdtsc_time1 - s->rdtsc_start_time)/(double)s->rdtsc_HZ,
	  s->step_count, time_diff);
#endif

  ////////////////////////////////////////////////////////////
  // Output section
  ////////////////////////////////////////////////////////////

  // increment number of steps
  s->step_count++;

  // get time again (because of print!)
  rdtsc(s->rdtsc_time1);

  // chanel output step (when we try to send data if any is available)
  chanel_output_step(&(s->chanel), p);

  // compute time that was spent in the output function 
  rdtsc(s->rdtsc_time2);

#ifdef MEASURE_ALL
  // compute duration of output step
  time_diff = (s->rdtsc_time2 - s->rdtsc_time1)/(double)s->rdtsc_HZ;
  if(time_diff>ACCEPTED_ERROR)
    INFO_("%f STEP[%ld]: Time in output function=%f", 
	  (s->rdtsc_time2 - s->rdtsc_start_time)/(double)s->rdtsc_HZ,
	  s->step_count, time_diff);
#endif

  ////////////////////////////////////////////////////////////
  // Reading configuration section
  ////////////////////////////////////////////////////////////

  // get time again (because of print!!!!!!!!!!1111)
  rdtsc(s->rdtsc_time1);

  //clock_gettime(CLOCK_REALTIME, &time_value);


  // configuration reading section
  if(s->reset_time == TRUE && s->read_finished == FALSE)
    {
      INFO_("Reading configuration records...");
      counter = 0;

      if(io_read_binary_time_record_from_file(&binary_time_record, 
						 s->fd)==ERROR)
	{
	  INFO_("Aborting on input error (time record)");
	  return -1;
	}
      //io_binary_print_time_record(&binary_time_record);

      // to do on first iteration of each batch of descriptors
      // NOTE: COUNTER VARIABLE NOT NEEDED APPARENTLY!!!!!!!
      if(counter==0)
	{
	  s->reset_time = FALSE;
	  
	  if(s->time == MIN_DBL)//first time
	    {
	      s->deadline_time=s->current_time;
	      INFO_("COMPUTED FIRST DEADLINE TIME = %f", s->deadline_time);
	    }
	  else
	    {
	      s->deadline_time = ((s->deadline_time/SCALING_FACTOR) + 
				  (binary_time_record.time-s->time)) * 
		SCALING_FACTOR;
	      INFO_("COMPUTED NEXT DEADLINE TIME = %f (CONFIG FILE TIME=%f)", 
		    s->deadline_time, binary_time_record.time);
	    }
	  s->time = binary_time_record.time;
	}

      if(binary_time_record.record_number>(NUMBER_NODES*(NUMBER_NODES-1)))
	{
	  INFO_("The number of records to be read exceeds allocated size (%d)",
		NUMBER_NODES*(NUMBER_NODES-1));
	  return ERROR;
	}

      if(io_read_binary_records_from_file(binary_records, 
					  binary_time_record.record_number,
					  s->fd)==ERROR)
	{
	  INFO_("Aborting on input error (records)");
	  return -1;
	}

      for(i=0; i<binary_time_record.record_number; i++)
	{
	  // check whether the from_node and to_node from the file
	  // match the user selected ones
	  if(binary_records[i].from_node == s->data_node_id)
	    {
	      //io_binary_print_record(&(binary_records[i]));

#ifdef PREDEFINED_EXPERIMENT
	      bandwidth=BANDWIDTH;
	      delay=DELAY;
	      lossrate=PACKET_LOSS_RATE;
#endif
	      
	      if(binary_records[i].to_node<0 || binary_records[i].to_node> NUMBER_NODES-1)
		{
		  INFO_("Destination with id = %d is out of the valid range \
[%d, %d]", binary_records[i].to_node, 0, NUMBER_NODES-1);
		  return -1;
		}
	      
	      s->tid[binary_records[i].to_node] = binary_records[i].to_node;
	      s->is_filled[binary_records[i].to_node] = TRUE;
	      //s->bandwidth[binary_record.to_node] = bandwidth;
	      //s->delay[binary_record.to_node] = delay;
	      s->lossrate[binary_records[i].to_node] = 
		binary_records[i].loss_rate;
	      s->from = binary_records[i].from_node;
	      /*
		INFO_("AT %.4f (counter=%d) READ chanel %d->%d configuration (time=%.2f s): bandwidth=%.2fbit/s \
		loss_rate=%.4f delay=%.4f ms (WAIT=%.4fs)", s->td1, counter, s->from, s->tid[to], time, s->bandwidth[to], s->lossrate[to], s->delay[to], (s->td2-s->td1));
	      */

	      counter++;

	      //fflush(stdout);  
	    }
	}

      s->time_records_read++;
      //printf("*********s->time_records_read=%d total=%ld\n", s->time_records_read, s->binary_header.time_record_number);
      //fflush(stdout);  
      if(s->time_records_read >= s->binary_header.time_record_number)
      {
	INFO_("Finished reading configuration file");
	fflush(stdout);
	s->read_finished = TRUE;
      }
    }

  // get time
  rdtsc(s->rdtsc_time2);

#ifdef MEASURE_ALL
  // compute duration of reading
  time_diff = (s->rdtsc_time2 - s->rdtsc_time1)/(double)s->rdtsc_HZ;
  if(time_diff>ACCEPTED_ERROR)
    INFO_("%f STEP[%ld]: Time in reading function=%f", 
	  (s->rdtsc_time2 - s->rdtsc_start_time)/(double)s->rdtsc_HZ,
	  s->step_count, time_diff);
#endif

  ////////////////////////////////////////////////////////////
  // Applying configuration section
  ////////////////////////////////////////////////////////////

  // get current time
  rdtsc(s->rdtsc_time1);

  // compute current time by adding to the initial time 
  // the elapsed time since last
  s->current_time =/*s->TIME_SUBSTRACT + */ /*s->rdtsc_additional + */
    (s->rdtsc_time1 - s->rdtsc_start_time)/(double)s->rdtsc_HZ;

  // applying configuration step
  if(s->reset_time == FALSE && s->read_finished == FALSE)
    {
      if((s->deadline_time-ACCEPTED_ERROR) < s->current_time)
	{
	  if(fmod(s->deadline_time, 10.0)==0)
	    {
	      // get time
	      clock_gettime(CLOCK_REALTIME, &time_value);
	      INFO_("CHECK ACCURACY: current_time=%f gettime=%f \
accuracy_difference=%f", s->current_time, time_value.tv_sec + 
		    time_value.tv_nsec/1e9 - s->TIME_SUBSTRACT,
		    time_value.tv_sec + time_value.tv_nsec/1e9 - 
		    s->TIME_SUBSTRACT - s->current_time);
	    }

	  INFO_("Applying configuration");
	  s->error = s->current_time - s->deadline_time;
	  if(s->error>ACCEPTED_ERROR) //error > 1%
	    INFO_("%f STEP[%ld]: Deadline ERROR is equal=%f", 
		  (s->rdtsc_time1 - s->rdtsc_start_time)/(double)s->rdtsc_HZ,
		  s->step_count, s->error);

	  for(i=0; i<NUMBER_NODES; i++)
	    {
	      //is_filled necessary?????
	      if(i!=s->data_node_id && s->is_filled[i] == TRUE) 
		{
		  deltaQ_class deltaQ;

		  deltaQ_set_frame_error_rate(&deltaQ, s->lossrate[i]);
		  chanel_set_deltaQ(&(s->chanel), s->tid[i], &deltaQ);
		  s->is_filled[i] = FALSE;
		}
	    }
	  s->reset_time = TRUE;
	}
    }
    
  rdtsc(s->rdtsc_time2);

#ifdef MEASURE_ALL
  time_diff = (s->rdtsc_time2 - s->rdtsc_time1)/(double)s->rdtsc_HZ;
  if(time_diff>ACCEPTED_ERROR)
    INFO_("%f STEP[%ld]: Time in configuring function=%f", 
	  (s->rdtsc_time2 - s->rdtsc_start_time)/(double)s->rdtsc_HZ,
	  s->step_count, time_diff);
#endif

  //#ifdef DO_SLEEP
  //usleep(SLEEP_TIME);
  //#endif

  // USED TO EXIT THE SPACE COMPLETELY!!!!!!!!?????????????
  //releaseExec();

  return 0;
}

// chanel space finalize entry point
void chanel_space_fin(void *p)
{
  chanel_space_class *s = getStorage(p);

  free(s);

  if(chanel_finalize(&(s->chanel))==ERROR)
    INFO_("Error finalizing chanel (chanel_space_fin)");
}

// chanel space read entry point
void *chanel_space_read(void *p, void *a)
{

  return NULL;
}

void chanel_space_recv_data(void *p, data_class *data_object)
{
  chanel_space_class *s = getStorage(p);

  int conduit_id;
  tagframe frame;
  int i;
#ifdef MEASURE_ALL
  double time_diff;
#endif

  conduit_id = modulo_difference(data_object->to_node, 
				 data_object->from_node, NUMBER_NODES)-1;

  frame.len=htonl(data_object->data_length);
  //printf("Sending %d bytes\n", data_object->data_length);
  for(i=0; i<data_object->data_length; i++)
    {
      frame.payload[i] = (uint8_t)data_object->data[i];
      //printf("internal=%d sent=%d\n", 
	//     data_object->data[i], frame.data[i]);
    }

/*
  fprintf(stdout, "|Chanel (gsid=%d)| is sending\n\t"
	  "%02x %02x %02x %02x %02x %02x %02x %02x\n", 
	  s->gsid,
	  frame.data[0], frame.data[1],
	  frame.data[2], frame.data[3],
	  frame.data[4], frame.data[5],
	  frame.data[6], frame.data[7]);
*/
  //fflush(stdout);


  // get time again (because of print!)
  rdtsc(s->rdtsc_time1);

  runeWrite(p, conduit_id, (condData *)&frame, NULL);

  // compute time that was spent in the output function 
  rdtsc(s->rdtsc_time2);

#ifdef MEASURE_ALL
  // compute duration of output step
  time_diff = (s->rdtsc_time2 - s->rdtsc_time1)/(double)s->rdtsc_HZ;
  if(time_diff>ACCEPTED_ERROR)
    INFO_("%f Time in runeWrite step=%f", 	  
	  (s->rdtsc_time2 - s->rdtsc_start_time)/(double)s->rdtsc_HZ,
	  time_diff);
#endif

}

// chanel space write entry point
// this function is called when a write operation is performed to this
// space (i.e. it receives data on a conduit)
void *chanel_space_write(void *p, void *a)
{
  chanel_space_class *s = getStorage(p);
  condPacket *packet = (condPacket *)a;
  tagframe *frame = (tagframe *)&packet->data;

  //chanel_space_class *s = getStorage(p);
  static tagresp resp;

  int i;

  data_class data_object;

  data_type_enum data_type = DATA_TYPE_BYTE;
  size_t data_length;
  char data_bytes[MAX_DATA_SIZE];
  int chanel_source =  ntohl(packet->sgsid);
  int chanel_destination = BROADCAST_ID;
  
  /*
  fprintf(stdout, "|Chanel (gsid=%d)| received from gsid %d\n\t"
  	  "%02x %02x %02x %02x %02x %02x %02x %02x\n", 
	  s->gsid, ntohl(packet->sgsid),
	  frame->data[0], frame->data[1],
	  frame->data[2], frame->data[3],
	  frame->data[4], frame->data[5],
	  frame->data[6], frame->data[7]);
  */
  //fflush(stdout);

  resp.status = 0;
  resp.packet.data.len = htonl(sizeof(resp.status));

  data_length=ntohl(frame->len);
  //printf("Received %d bytes\n", data_length);
  for(i=0; i<data_length; i++)
    {
      data_bytes[i]=(char)(frame->payload[i]);
      //printf("internal=%d received=%d\n", 
      //	     data_bytes[i], (frame->data[i]));
    }

  data_init(&data_object, chanel_source, chanel_destination, 
	    frame->payload[2]*256+frame->payload[3],
	    data_bytes, data_type, data_length);

  chanel_send_data(&(s->chanel), &data_object);

  return &resp;
}
