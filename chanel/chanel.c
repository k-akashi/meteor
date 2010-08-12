
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
 * File name: chanel.c
 * Function: Chanel emulation library main source file
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

#define ENABLE_OUTPUT_THREAD // not needed with RUNE

//double TIME_SUBSTRACT;


////////////////////////////////////////////////////////////
// Auxiliary functions
////////////////////////////////////////////////////////////
/*
// generate a random number in the interval [0,1)
double rand_0_1()
{
  return rand()/((double)RAND_MAX+1.0);
}
*/

////////////////////////////////////////////////////////////
// Functions for the manipulation of "data_class" structure
////////////////////////////////////////////////////////////

// initialize data_class object;
// return SUCCESS on succes, ERROR on error
int data_init(data_class *data_object, int from_node, int to_node, 
	      double time, const void *data, data_type_enum data_type, 
	      size_t data_length)
{
  // check that the data doesn't exceed the maximum size
  if(data_length > MAX_DATA_SIZE)
    {
      WARNING_("Data length (%d) exceeds maximum data size (%d)",
	      data_length, MAX_DATA_SIZE);
      return ERROR;
    }
  
  // initialize fields
  data_object->from_node = from_node;
  data_object->to_node = to_node;
  data_object->time = time;
  data_object->data_type = data_type;

  // initialize data contents and length
  memcpy(data_object->data, data, data_length);
  data_object->data_length = data_length;

  return SUCCESS;
}

// copy data_class object information from data_src to data_dst;
// return SUCCESS on succes, ERROR on error
int data_copy(data_class *data_dst, data_class *data_src)
{
  // check that the data doesn't exceed the maximum size
  if(data_src->data_length > MAX_DATA_SIZE)
    {
      WARNING_("Source object data length (%d) exceeds maximum data size (%d)",
	      data_src->data_length, MAX_DATA_SIZE);
      return ERROR;
    }

  // copy fields
  data_dst->from_node = data_src->from_node;
  data_dst->to_node = data_src->to_node;
  data_dst->time = data_src->time;
  data_dst->data_type = data_src->data_type;

  // copy data contents and length
  memcpy(data_dst->data, data_src->data, data_src->data_length);
  data_dst->data_length = data_src->data_length;

  return SUCCESS;
}

// print data_class object;
// return SUCCESS on succes, ERROR on error
int data_print(data_class *data_object)
{
  int i;

  printf("Data contents: from_node=%d to_node=%d time=%2f data_type=%d \
data_length=%d\n", data_object->from_node, data_object->to_node, 
	 data_object->time, data_object->data_type, data_object->data_length);

  // specific printing of bit-type data sequence
  if(data_object->data_type == DATA_TYPE_BIT)
    {
      printf("               data(bit)=");
      for(i=0; i<data_object->data_length; i++)
	printf("%d", (((char *)(data_object->data))[i]==0)?0:1);
      printf("\n");
    }
  // specific printing of byte-type data sequence
  else if(data_object->data_type == DATA_TYPE_BYTE)
    {
      printf("               data(bytes)=");
      for(i=0; i<data_object->data_length; i++)
	printf("0x%02hhx ", ((char *)(data_object->data))[i]);
      printf("\n");
    }
  else
    {
      printf("<unknown data type> (type=%d)\n", data_object->data_type);
      return ERROR;
    }

  return SUCCESS;
}

// introduce an error in the data field of a data_class object;
// return SUCCESS on succes, ERROR on error
int data_introduce_error(data_class *data_object)
{
  char *data = (char *)(data_object->data);

  // error is currently introduced by negating the last bit or byte
  if(data_object->data_type == DATA_TYPE_BIT)
    data[data_object->data_length-1] = !data[data_object->data_length-1];
  else if(data_object->data_type == DATA_TYPE_BYTE)
    data[data_object->data_length-1] = ~data[data_object->data_length-1];
  else
    {
      WARNING_("Cannot introduce error because of unknown data type (%d)", 
	      data_object->data_type);
      return ERROR;
    }
  
  return SUCCESS;
}


//////////////////////////////////////////////////////////////////
// Functions for the manipulation of "data_queue_class" structure
//////////////////////////////////////////////////////////////////

// initialize a data_queue_class object
void data_queue_init(data_queue_class *data_queue)
{
  data_queue->data_head = 0;
  data_queue->data_count = 0;
}

// test whether a data_queue_class object is empty;
// return TRUE on true condition, FALSE on false
int data_queue_is_empty(data_queue_class *data_queue)
{
  if(data_queue->data_count == 0)
    return TRUE;
  else
    return FALSE;
}

// test whether a data_queue_class object is full;
// return TRUE on true condition, FALSE on false
int data_queue_is_full(data_queue_class *data_queue)
{
  if(data_queue->data_count == MAX_DATA_BUFFER_SIZE)
    return TRUE;
  else
    return FALSE;
}

// enqueue an element in a data_queue_class object;
// return SUCCESS on succes, ERROR on error
int data_queue_enqueue(data_queue_class *data_queue, data_class *data_object)
{
  int new_index;

  // check whether the queue is already full
  if(data_queue_is_full(data_queue)==FALSE) 
    {
      // calculate the index for the new element in a circular
      // queue fashion
      new_index = (data_queue->data_head + data_queue->data_count)
	% MAX_DATA_BUFFER_SIZE;

      // copy the new element into the queue
      data_copy(&(data_queue->data_buffer[new_index]), data_object);

      // update element count
      data_queue->data_count++;
    }
  else
    {     
      WARNING_("Enqueue failed because queue is full (%d data units)", 
	      MAX_DATA_BUFFER_SIZE);
      return ERROR;
    }

  return SUCCESS;
}

// dequeue an element from a data_queue_class object;
// return SUCCESS on succes, ERROR on error (i.e., if queue is empty)
int data_queue_dequeue(data_queue_class *data_queue, data_class *data_object)
{
  // check whether the queue is empty
  if(data_queue_is_empty(data_queue)==FALSE)
    {
      // copy element from queue
      data_copy(data_object, 
		&(data_queue->data_buffer[data_queue->data_head]));
      
      // update queue parameters
      data_queue->data_head++;
      data_queue->data_head %= MAX_DATA_BUFFER_SIZE;
      data_queue->data_count--;
    }
  else
    {
      //WARNING_("Dequeue failed because queue is empty");
      return ERROR;
    }

  return SUCCESS;
}

// obtain the last element (tail) from a data_queue_class object;
// return SUCCESS on succes, ERROR on error
int data_queue_tail(data_queue_class *data_queue,  data_class *data_object)
{
  int tail_index;

  // check whether the queue is empty
  if(data_queue_is_empty(data_queue)==FALSE)
    {
      // compute index of tail element
      tail_index = (data_queue->data_head + data_queue->data_count - 1)
	% MAX_DATA_BUFFER_SIZE;

      // copy element from queue but leave queue unchanged
      data_copy(data_object, &(data_queue->data_buffer[tail_index]));
    }
  else
    {
      WARNING_("Tail operation failed because queue is empty");
      return ERROR;
    }

  return SUCCESS;
}


//////////////////////////////////////////////////////////////////
// Functions for the manipulation of "deltaQ_class" structure
//////////////////////////////////////////////////////////////////

// set the bandwidth of a deltaQ object
void deltaQ_set_bandwidth(deltaQ_class *deltaQ, double bandwidth)
{
  deltaQ->bandwidth_configured = TRUE;
  deltaQ->bandwidth = bandwidth;
}

// set the frame error rate of a deltaQ object
void deltaQ_set_frame_error_rate(deltaQ_class *deltaQ, 
				 double frame_error_rate)
{
  deltaQ->fer_configured = TRUE;
  deltaQ->frame_error_rate = frame_error_rate;
}

// set the delay & jitter of a deltaQ object
void deltaQ_set_delay_jitter(deltaQ_class *deltaQ, double delay, double jitter)
{
  deltaQ->delay_jitter_configured = TRUE;
  deltaQ->delay = delay;
  deltaQ->jitter = jitter;
}

// set all parameters of a deltaQ object
void deltaQ_set_all(deltaQ_class *deltaQ, double bandwidth, 
		    double frame_error_rate, double delay, double jitter)
{
  deltaQ->bandwidth_configured = TRUE;
  deltaQ->bandwidth = bandwidth;

  deltaQ->fer_configured = TRUE;
  deltaQ->frame_error_rate = frame_error_rate;

  deltaQ->delay_jitter_configured = TRUE;
  deltaQ->delay = delay;
  deltaQ->jitter = jitter;
}

// copy deltaQ object 'deltaQ_src' to 'deltaQ_dest'
void deltaQ_copy(deltaQ_class *deltaQ_dst, deltaQ_class *deltaQ_src)
{
  // configure bandwidth
  if(deltaQ_src->bandwidth_configured == TRUE)
    {
      deltaQ_dst->bandwidth_configured = TRUE;
      deltaQ_dst->bandwidth = deltaQ_src->bandwidth;
    }
  else
    deltaQ_dst->bandwidth_configured = FALSE;

  // configure frame error rate
  if(deltaQ_src->fer_configured == TRUE)
    {
      deltaQ_dst->fer_configured = TRUE;
      deltaQ_dst->frame_error_rate = deltaQ_src->frame_error_rate;
    }
  else
    deltaQ_dst->fer_configured = FALSE;

  // configure delay & jitter
  if(deltaQ_src->delay_jitter_configured)
    {
      deltaQ_dst->delay_jitter_configured = TRUE;
      deltaQ_dst->delay = deltaQ_src->delay;
      deltaQ_dst->jitter = deltaQ_src->jitter;
    }
  else
    deltaQ_dst->delay_jitter_configured = FALSE;
}

// clear the "configured" flags of a deltaQ object
void deltaQ_clear(deltaQ_class *deltaQ)
{
  deltaQ->delay_jitter_configured = FALSE;
  deltaQ->bandwidth_configured = FALSE;
  deltaQ->fer_configured = FALSE;
}

// print a deltaQ object
void deltaQ_print(deltaQ_class *deltaQ)
{
  printf("\tdeltaQ: ");
  if(deltaQ->bandwidth_configured)
    printf("bandwidth=%.2f ", deltaQ->bandwidth);
  if(deltaQ->fer_configured)
    printf("FER=%.3f ", deltaQ->frame_error_rate);
  if(deltaQ->delay_jitter_configured)
    printf("delay=%.3f jitter=%.3f", deltaQ->delay, deltaQ->jitter);
  printf("\n");
}


//////////////////////////////////////////////////////////////////
// chanel functions
//////////////////////////////////////////////////////////////////

// set deltaQ parameters for chanel;
// return SUCCESS on succes, ERROR on error
int chanel_set_deltaQ(chanel_class *chanel, int chanel_destination, 
		      deltaQ_class *deltaQ)
{
  // check that destination is not outside valid range
  if(chanel_destination<0 || chanel_destination>=MAX_DESTINATIONS)
    {
      WARNING_("Cannot set deltaQ because destination index '%d' \
is not in the valid range [%d, %d]", chanel_destination, 0, 
	       MAX_DESTINATIONS-1); 
      return ERROR;
    }

  //INFO_("Setting deltaQ for destination %d...", chanel_destination);
  //deltaQ_print(deltaQ);
  
  // lock mutex on configuration structure
  pthread_mutex_lock(&(chanel->configuration_mutex));

  // enable destination
  chanel->active_destinations[chanel_destination] = TRUE;
  
  // configure the parameters that are present in deltaQ object
  deltaQ_copy(&(chanel->configurations[chanel_destination]), deltaQ);

  // unlock mutex on configuration structure
  pthread_mutex_unlock(&(chanel->configuration_mutex));

  return SUCCESS;
}

// open pipe for chanel configuration;
// return pipe id on succes (positive value), ERROR on error (-1)
int chanel_configuration_pipe_open(int chanel_source)
{
  int pipe_id;
  char pipe_name[MAX_STRING];

  // build pipe name
  snprintf(pipe_name, MAX_STRING, "%s%d", PIPE_NAME_BASE, chanel_source);

  // open pipe
  pipe_id = open(pipe_name, O_WRONLY);
  
  // check for errors
  if(pipe_id == -1)
    {
      perror("chanel");
      WARNING_("Cannot open pipe %s (errno=%d) for write", pipe_name, errno);
      return ERROR;
    }
  else
    return pipe_id;
}

// write to chanel configuration pipe;
// return SUCCESS on succes, ERROR on error
int chanel_configuration_pipe_write(int chanel_pipe_id, 
				    pipe_data_class *pipe_data)
{
  size_t intended_count;
  ssize_t wrote_count;

  // check whether pipe id is valid
  if(chanel_pipe_id==ERROR)
    {
      WARNING_("Invalid deltaQ configuration pipe id (%d)", chanel_pipe_id); 
      return ERROR;
    }

  // compute intended size for writing
  intended_count = sizeof(pipe_data_class);

  // write to pipe
  wrote_count = write(chanel_pipe_id, pipe_data, intended_count);
  INFO_("Wrote %d bytes to pipe", wrote_count);

  // check that intended and wrote data sizes are the same
  if(wrote_count != intended_count)
    {
      WARNING_("Error writing to deltaQ configuration pipe \
(intended=%d bytes, wrote=%d bytes", intended_count, wrote_count); 
      return ERROR;
    }

  return SUCCESS;
}

// send data to chanel (incoming in chanel);
// return SUCCESS on succes, ERROR on error
int chanel_send_data(chanel_class *chanel, data_class *data_object)
{
  struct timespec before;
  
  int enqueue_status;

  // change to something more general!!!!!!!!!!!!!!!!!
  INFO_("Incoming data [%d]->[%d]... \
(0x%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx)",
	data_object->from_node, data_object->to_node,
	((char *)(data_object->data))[0], ((char *)(data_object->data))[1], 
	((char *)(data_object->data))[2], ((char *)(data_object->data))[3],
	((char *)(data_object->data))[4], ((char *)(data_object->data))[5], 
	((char *)(data_object->data))[6], ((char *)(data_object->data))[7]);

  /*for(i=0; i<data_object->data_length; i++)
	printf("0x%x ", ((char *)(data_object->data))[i]);
  */

#ifdef DEBUG
  data_print(data_object);
#endif

  // lock mutex on data structure
  pthread_mutex_lock(&(chanel->data_mutex));

  // try to enqueue data
  enqueue_status = data_queue_enqueue(&(chanel->data_queue), data_object);

  // unlock mutex on data structure
  pthread_mutex_unlock(&(chanel->data_mutex));

  // check enqueue status
  if(enqueue_status == SUCCESS)
    {
      clock_gettime(CLOCK_REALTIME, &before);
      //rdtsc(chanel->rdtsc_time1);

      INFO_("AT time=%f: Enqueued data [%d]->BROADCAST... \
(data[0]=0x%02hhx, time=%f)", 
	    before.tv_sec+before.tv_nsec/1e9-chanel->TIME_SUBSTRACT, 
	    data_object->from_node, 
	    ((char *)(data_object->data))[0], data_object->time);
      //data_print(data_object);
    }
  else
    {
      WARNING_("Unable to send data to chanel"); 

      return ERROR;
    }

  return SUCCESS;
}

// receive data from chanel (outgoing from chanel);
// the callback structure callback_data is 
// used when providing the data to chanel user;
// return SUCCESS on succes, ERROR on error
int chanel_recv_data(chanel_class *chanel, void *callback_data)
{
  double success_probability;
  int chanel_destination;

  data_class data_object, tmp_data_object;

  int dequeue_status;

  // lock mutex on data structure
  pthread_mutex_lock(&(chanel->data_mutex));

  // try to dequeue data
  dequeue_status = data_queue_dequeue(&(chanel->data_queue), &data_object);

  // unlock mutex on data structure
  pthread_mutex_unlock(&(chanel->data_mutex));

  // check dequeue status
  if(dequeue_status == SUCCESS)
    {

      // PUT CODE FOR SENDING UNICAST AS WELL!!!!!!!!!!!!!!!!!!!!!!

      /* Use this code for special unicast
      chanel_destination = data_object->to_node;
      if(chanel_destination>MAX_DESTINATIONS)
	{
	  WARNING_("Destination index '%d' exceeds the limit (%d)", 
		  chanel_destination, MAX_DESTINATIONS); 
	  return ERROR;
	}
      */
      /*
      if(data_object.to_node != BROADCAST_ID)
	{
	  WARNING_("ONLY broadcast destination supported: \
discarding data for destination %d", data_object.to_node); 
	  return ERROR;
	}
      */

      //data_print(&data_object);


      // send data to all destinations
      for(chanel_destination=0; chanel_destination<MAX_DESTINATIONS; 
	  chanel_destination++)
	{
	  // ignore destinations equal to the sending node
	  if(data_object.from_node == chanel_destination)
	    continue;

	  // lock mutex on configuration structure
	  pthread_mutex_lock(&(chanel->configuration_mutex));

	  // ignore destinations that were not enabled
	  if(chanel->active_destinations[chanel_destination]==FALSE)
	    {
//	      INFO_("Ignored packet for %d because destination was not enabled", chanel_destination);

	      // unlock mutex on configuration structure
	      pthread_mutex_unlock(&(chanel->configuration_mutex));

	      continue;
	    }

	  // copy data to temporary data object
	  data_copy(&tmp_data_object, &data_object);

	  //INFO_("Analyzing data for destination %d...", chanel_destination);

	  // special processing for FER>0; skip this for FER==0
	  if(chanel->configurations[chanel_destination].frame_error_rate > 0)
	    {
	      // FER==1 means data will be dropped
	      if(chanel->configurations[chanel_destination].
		 frame_error_rate == 1.0)
		{
		  //INFO_("Dropping frame...");
		  //fflush(stdout);
		  tmp_data_object.data_length = 0;
		  continue;
		}
	      // FER>0.999999 means error will be introduced
	      else if(chanel->configurations[chanel_destination].
		      frame_error_rate>0.999999)
		success_probability = 0;// sure error
	      // otherwise compute success probability
	      else
		success_probability = rand_0_1();

	      // if success probability is less than FER introduce error
	      if(success_probability < 
		 chanel->configurations[chanel_destination].frame_error_rate)
		{
		  INFO_("Applying frame error for [%d]->[%d]...",
		       tmp_data_object.from_node, chanel_destination);
		  //fflush(stdout);
		  data_introduce_error(&tmp_data_object);
		}
	    }

	  // unlock mutex on configuration structure
	  pthread_mutex_unlock(&(chanel->configuration_mutex));


	  //internal_data_object.to_node = chanel_destination;
	  //INFO_("Outgoing data [%d]->[%d]... (first byte=0x%hh02x)", 
	  //     internal_data_object.from_node, internal_data_object.to_node,
	  //     ((char *)(internal_data_object.data))[0]);

	  // set destination for this data item
	  tmp_data_object.to_node = chanel_destination;

	  // change to something more general!!!!!!!!!!!!!!!!!
	  INFO_("Outgoing data [%d]->[%d]... (0x%02hhx%02hhx%02hhx%02hhx\
%02hhx%02hhx%02hhx%02hhx)", //0x%hh02x)",
		tmp_data_object.from_node, tmp_data_object.to_node,
		((char *)(tmp_data_object.data))[0],
		((char *)(tmp_data_object.data))[1],
		((char *)(tmp_data_object.data))[2],
		((char *)(tmp_data_object.data))[3],
		((char *)(tmp_data_object.data))[4],
		((char *)(tmp_data_object.data))[5],
		((char *)(tmp_data_object.data))[6],
		((char *)(tmp_data_object.data))[7]);

	  //data_print(tmp_data_object);

	  // check if a callback function has been defined
	  if(chanel->callback_recv_function!=NULL)
	    {
	      //struct timespec before, after;
	      //double time_diff;

	      //clock_gettime(CLOCK_REALTIME, &before);
	      (*chanel->callback_recv_function)(callback_data, 
						&tmp_data_object);
	      //clock_gettime(CLOCK_REALTIME, &after);
	      //time_diff = (after.tv_sec-before.tv_sec)+
	      //            (after.tv_nsec-before.tv_nsec)/1e9;
	      //if(time_diff>0.005)
	      //  INFO("Time in callback =%f", time_diff);
	    }
	  else
	    {
	      // no callback function was defined

	      // use different way when operating standalone
	      // and replace the line below
	      WARNING_("No callback data structure defined for chanel output: \
data is not sent any further");
	    }
	}
    }
  else
    {
      //DEBUG_("Unable to receive data from chanel");
      return ERROR;
    }

  return SUCCESS;
}

/*
// catch signals for stopping chanel operation
void chanel_catch_signal(int signal_number)
{
  fprintf(stderr, "chanel INFO: Caught signal number %d. \
Stopping chanel threads...\n", signal_number);

  // set flag to interrupt operation
  chanel.do_interrupt = TRUE;

  // restore default signal handler
  signal(signal_number, SIG_DFL);
}
*/


// do one chanel output step and try to receive data from 
// chanel (outgoing from chanel); the callback function
// callback_recv_function is used to provide the data
// to chanel user;
// return SUCCESS on succes, ERROR on error
int chanel_output_step(chanel_class *chanel, void *callback_data)
{
  /*
  int queue_status;

  // lock mutex on data structure
  pthread_mutex_lock(&(chanel->data_mutex));

  // check queue status
  queue_status = data_queue_is_empty(&(chanel->data_queue));

  // unlock mutex on data structure
  pthread_mutex_unlock(&(chanel->data_mutex));

  // check whether queue is empty
  if(queue_status == TRUE)
    {
      INFO_("Queue is empty");
      return SUCCESS;
    }
  else
    {
      INFO_("Queue has data");

      // try to receive data
      if(chanel_recv_data(chanel, callback_data)==ERROR)
	{
	  WARNING_("Error receiving data from chanel");
	  return ERROR;
	}
    }
  */

  // return error status indicates that queue was empty,
  // but this is not an error in our case
  chanel_recv_data(chanel, callback_data);

  return SUCCESS;
}

// chanel output thread that repeatedly executes
// the chanel_output_step function; this thread is
// started for standalone execution only
void *chanel_output_thread(void *arg)
{
  float time = 0;

  chanel_class *chanel = (chanel_class*)arg;

  INFO_("Output thread begins execution (chanel=%p)", chanel);

  // execute until interrupted
  while(chanel->do_interrupt==FALSE)
    {
      if(chanel->output_sleep_time>=250000)
	{
	  INFO_("Output thread: time=%.2f s", time);
	  fflush(stdout);
	}

      // no "callback_data" is needed in standalone version
      chanel_output_step(chanel, NULL);

      if(chanel->output_sleep_time!=0)
	{
	  // sleep OUTPUT_SLEEP_TIME in between executions
	  time+=chanel->output_sleep_time/1e6;
	  usleep(chanel->output_sleep_time);
	}
    }

  return NULL;
}

// chanel configuration thread that reads deltaQ data
// from the configuration pipe; this thread is
// started for standalone execution only (NOW)
void *chanel_configuration_thread(void *arg)
{
  pipe_data_class pipe_data;
  size_t expected_count=sizeof(pipe_data_class);
  ssize_t read_count;
  chanel_class *chanel = (chanel_class*)arg;

  INFO_("Configuration thread begins execution (chanel=%p)", chanel);

  // execute until interrupted
  while(chanel->do_interrupt==FALSE)
    {
      // read deltaQ data from pipe
      // (THIS IS BLOCKING READ!!!!!!!!!)
      read_count = read(chanel->deltaQ_pipe_id, &pipe_data, expected_count);

      // check whether enough data was read
      if(read_count==expected_count)
	{
	  // configure chanel deltaQ parameters
	  chanel_set_deltaQ(chanel, pipe_data.destination, 
			    &(pipe_data.deltaQ));

	  //printf("*** Pipe data:\n");
	  //deltaQ_print(deltaQ);
	}
      else
	{
	  WARNING_("*** Pipe data <incorrect> or <no data>\n");
	}
    }

  return NULL;
}

// execute a chanel instance associated with the source
// init_source_id and using the callback function 
// callback_recv_function to provide the data to chanel user;
// init_source_id==UNDEFINED_ID signifies source is undefined;
// return SUCCESS on succes, ERROR on error
int chanel_execute(chanel_class *chanel, int init_source_id,
		   float output_sleep_time,
		   void (*callback_recv_function)(void*,data_class*),
		   double TIME_SUBSTRACT_param)
{
  int i;

  // copy function pointer
  chanel->callback_recv_function = callback_recv_function;

  // initialize source
  chanel->chanel_source = init_source_id;
  chanel->output_sleep_time = output_sleep_time;

  // initialize do_interrupt flag
  chanel->do_interrupt = FALSE;

  chanel->TIME_SUBSTRACT = TIME_SUBSTRACT_param;

  // check if source is defined
  if(chanel->chanel_source!=UNDEFINED_ID)
    INFO_("Executing chanel for node %d", chanel->chanel_source);
  else
    INFO_("Executing chanel for undefined node");

  // initialize queue
  data_queue_init(&(chanel->data_queue));

  // initialize chanel configuration
  for(i=0; i< MAX_DESTINATIONS; i++)
    {
      // set active destination to false
      chanel->active_destinations[i]=FALSE;

      // use line below if default values are to be provided
      //deltaQ_set_all(&(chanel->configurations[i]), 1e9, 0.0, 0.0, 0.0);

      // clear configurations
      deltaQ_clear(&(chanel->configurations[i]));
    }

  // initialize random seed
  srand(0x12345678);

  // install signal handler
  //signal(SIGINT, chanel_catch_signal);

  if(pthread_mutex_init(&(chanel->data_mutex), NULL))
    {
      WARNING_("Cannot init data mutex");
      return ERROR;
    }

  if(pthread_mutex_init(&(chanel->configuration_mutex), NULL))
    {
      WARNING_("Cannot init configuration mutex");
      return ERROR;
    }

  // build pipe name and remove previous instance if necessary
  snprintf(chanel->pipe_name, MAX_STRING, "%s%d", PIPE_NAME_BASE, 
	   chanel->chanel_source);
  unlink(chanel->pipe_name);

  // create deltaQ configuration pipe
  if(mkfifo(chanel->pipe_name, 0600) < 0)
    {
      perror("mkfifo()");
      WARNING_("Cannot create deltaQ configuration pipe '%s'", 
	       chanel->pipe_name);
      return ERROR;
    }

  // open deltaQ configuration pipe
  if((chanel->deltaQ_pipe_id=open(chanel->pipe_name, O_RDWR)) == -1) 
    {
      WARNING_("Cannot open deltaQ configuration pipe '%s' for read", 
	      chanel->pipe_name);
      return ERROR;
    }

  INFO_("Starting configuration thread (chanel=%p)", chanel);
  // start configuration thread
  if(pthread_create(&chanel->configuration_thread, NULL, 
		    chanel_configuration_thread, (void *)chanel)!=0)
    {
      WARNING_("Cannot create deltaQ configuration thread");
      return ERROR;
    }

#ifdef ENABLE_OUTPUT_THREAD
  /*  NOT NEEDED FOR RUNE VERSION (also look at finalize) */
  INFO_("Starting output thread (chanel=%p)", chanel);
  // start output thread
  if(pthread_create(&chanel->output_thread, NULL, chanel_output_thread, 
		    (void *)chanel)!=0)
    {
      WARNING_("Error creating output thread");
      return ERROR;
    }
#endif

  return SUCCESS;
}

// function called to stop chanel execution
// by triggering the end of thread execution
void chanel_stop(chanel_class *chanel)
{
  INFO_("Stopping chanel threads...");
  chanel->do_interrupt = TRUE;
}

// function called to finalize chanel execution
// and wait for end of thread execution
int chanel_finalize(chanel_class *chanel)
{

#ifdef ENABLE_OUTPUT_THREAD
  /* NOT NEEDED WITH RUNE (also look at starting place) */
  // wait for output thread to end
  if(pthread_join(chanel->output_thread, NULL)!=0)
    {
      WARNING_("Error joining output thread");
      return ERROR;
    }
#endif

  // stop and wait for configuration thread to end
  pthread_cancel(chanel->configuration_thread);
  if(pthread_join(chanel->configuration_thread, NULL)!=0)
    {
      WARNING_("Error joining deltaQ configuration thread");
      return ERROR;
    }

  // now we should close the configuration pipe
  close(chanel->deltaQ_pipe_id);
  unlink(chanel->pipe_name);

  // destroy mutexes
  pthread_mutex_destroy(&(chanel->data_mutex));
  pthread_mutex_destroy(&(chanel->configuration_mutex));

  return SUCCESS;
}
