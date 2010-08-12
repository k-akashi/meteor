
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
 * File name: chanel.h
 * Function: Header file of chanel.c
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 93 $
 *   $LastChangedDate: 2008-09-12 11:16:33 +0900 (Fri, 12 Sep 2008) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#ifndef __CHANEL_H
#define __CHANEL_H


//////////////////////////////////
// Chanel constants
//////////////////////////////////

// maximum number of elements in the chanel buffer
#define MAX_DATA_BUFFER_SIZE   10
// maximum size of a data item
#define MAX_DATA_SIZE          8
// maximu size of a string
#define MAX_STRING             255
// maximum number of destinations
#define MAX_DESTINATIONS       1024 //30


//////////////////////////////////
// Data item ids
//////////////////////////////////

#define BROADCAST_ID     0xFFFFFFFF
#define UNDEFINED_ID     -1


//////////////////////////////////
// Configuration pipe parameters
//////////////////////////////////

// pipe name base string
#define PIPE_NAME_BASE         "/tmp/chanel_deltaQ_pipe"


//////////////////////////////////
// Data item related structures
//////////////////////////////////

// data type indicator
typedef enum
{
  DATA_TYPE_BIT=0, DATA_TYPE_BYTE
} data_type_enum;

// data class structure
typedef struct
{
  int from_node;
  int to_node;
  double time;
  char data[MAX_DATA_SIZE];
  data_type_enum data_type;
  size_t data_length;
} data_class;

// data queue class structure
typedef struct
{
  data_class data_buffer[MAX_DATA_BUFFER_SIZE];
  int data_head;
  int data_count;
} data_queue_class;


//////////////////////////////////
// DeltaQ structure
//////////////////////////////////

// deltaQ class structure
typedef struct
{
  // destination for which this configuration applies
  //int destination;

  // flag showing whether delay & jitter were configured
  int delay_jitter_configured;

  // values of delay & jitter
  double delay;
  double jitter;

  // flag showing whether bandwidth was configured
  int bandwidth_configured;

  // value of bandwidth
  double bandwidth;

  // flag showing whether frame error rate was configured
  int fer_configured;

  // value of frame error rate
  double frame_error_rate;
} deltaQ_class;


//////////////////////////////////
// Pipe data structure
//////////////////////////////////

// pipe data class structure
typedef struct
{
  // source for which this configuration applies
  int source;

  // destination for which this configuration applies
  int destination;

  // deltaQ configuration
  deltaQ_class deltaQ;

} pipe_data_class;


//////////////////////////////////
// Chanel structure
//////////////////////////////////

// chanel class structure
typedef struct
{
  // data source to which this chanel object
  // is associated
  int chanel_source;

  // data queue of this chanel
  data_queue_class data_queue;

  // array of flags showing which destinations are
  // active (TRUE=>active, FALSE=>inactive)
  int active_destinations[MAX_DESTINATIONS];

  // configuration array
  deltaQ_class configurations[MAX_DESTINATIONS];

  // thread for outputing data from chanel
  pthread_t output_thread;

  // sleep time in output thread
  // (if '0', then no sleep occurs)
  float output_sleep_time;

  // thread for configuring chanel
  pthread_t configuration_thread;

  // mutex for protecting chanel 'data_queue' structure
  pthread_mutex_t data_mutex;

  // mutex for protecting chanel 'configurations' and 
  // 'active_destinations' array
  pthread_mutex_t configuration_mutex;

  // flag showing that chanel execution must be stopped
  int do_interrupt;

  // pipe identifier and name
  int deltaQ_pipe_id;
  char pipe_name[MAX_STRING];

  // callback function used to receive data from chanel
  void (*callback_recv_function)(void*,data_class*);


  // used for printing time infromation; should be renamed!!!!!!!!
  double TIME_SUBSTRACT;

} chanel_class;


////////////////////////////////////////////////////////////
// Auxiliary functions
////////////////////////////////////////////////////////////

// generate a random number in the interval [0,1)
double rand_0_1();


////////////////////////////////////////////////////////////
// Functions for the manipulation of "data_class" structure
////////////////////////////////////////////////////////////

// initialize data_class object;
// return SUCCESS on succes, ERROR on error
int data_init(data_class *data_object, int from_node, int to_node, 
	      double time,const void *data, data_type_enum data_type, 
	      size_t data_length);

// copy data_class object information from data_src to data_dst;
// return SUCCESS on succes, ERROR on error
int data_copy(data_class *data_dst, data_class *data_src);

// print data_class object;
// return SUCCESS on succes, ERROR on error
int data_print(data_class *data_object);

// introduce an error in the data field of a data_class object;
// return SUCCESS on succes, ERROR on error
int data_introduce_error(data_class *data_object);


//////////////////////////////////////////////////////////////////
// Functions for the manipulation of "data_queue_class" structure
//////////////////////////////////////////////////////////////////

// initialize a data_queue_class object
void data_queue_init(data_queue_class *data_queue);

// test whether a data_queue_class object is empty;
// return TRUE on true condition, FALSE on false
int data_queue_is_empty(data_queue_class *data_queue);

// test whether a data_queue_class object is full;
// return TRUE on true condition, FALSE on false
int data_queue_is_full(data_queue_class *data_queue);

// enqueue an element in a data_queue_class object;
// return SUCCESS on succes, ERROR on error
int data_queue_enqueue(data_queue_class *data_queue, data_class *data_object);

// dequeue an element from a data_queue_class object;
// return SUCCESS on succes, ERROR on error
int data_queue_dequeue(data_queue_class *data_queue, data_class *data_object);

// obtain the last element (tail) from a data_queue_class object;
// return SUCCESS on succes, ERROR on error
int data_queue_tail(data_queue_class *data_queue,  data_class *data_object);


//////////////////////////////////////////////////////////////////
// Functions for the manipulation of "deltaQ_class" structure
//////////////////////////////////////////////////////////////////

// set the bandwidth of a deltaQ object
void deltaQ_set_bandwidth(deltaQ_class *deltaQ, double bandwidth);

// set the frame error rate of a deltaQ object
void deltaQ_set_frame_error_rate(deltaQ_class *deltaQ, 
				 double frame_error_rate);

// set the delay & jitter of a deltaQ object
void deltaQ_set_delay_jitter(deltaQ_class *deltaQ, 
			     double delay, double jitter);

// set all parameters of a deltaQ object
void deltaQ_set_all(deltaQ_class *deltaQ, double bandwidth, 
		    double frame_error_rate, double delay, double jitter);

// copy deltaQ object 'deltaQ_src' to 'deltaQ_dest'
void deltaQ_copy(deltaQ_class *deltaQ_dest, deltaQ_class *deltaQ_src);

// clear the "configured" flags of a deltaQ object
void deltaQ_clear(deltaQ_class *deltaQ);

// print a deltaQ object
void deltaQ_print(deltaQ_class *deltaQ);


//////////////////////////////////////////////////////////////////
// chanel functions
//////////////////////////////////////////////////////////////////

// set deltaQ parameters for 'chanel_destination' in 'chanel' object;
// return SUCCESS on succes, ERROR on error
int chanel_set_deltaQ(chanel_class *chanel, int chanel_destination,
		      deltaQ_class *deltaQ);

// open pipe for chanel configuration;
// return pipe id on succes (positive value), ERROR on error (-1)
int chanel_configuration_pipe_open(int chanel_source);

// write to chanel configuration pipe;
// return SUCCESS on succes, ERROR on error
int chanel_configuration_pipe_write(int chanel_pipe_id, 
				    pipe_data_class *pipe_data);

// send data to chanel (incoming in chanel);
// return SUCCESS on succes, ERROR on error
int chanel_send_data(chanel_class *chanel, data_class *data_object);

// receive data from chanel (outgoing from chanel);
// the callback structure callback_data is 
// used when providing the data to chanel user;
// return SUCCESS on succes, ERROR on error
int chanel_recv_data(chanel_class *chanel, void *callback_data);

// catch signals for stopping chanel operation
void chanel_catch_signal(int signal_number);

// do one chanel output step and try to receive data from 
// chanel (outgoing from chanel); the callback function
// callback_recv_function is used to provide the data
// to chanel user;
// return SUCCESS on succes, ERROR on error
int chanel_output_step(chanel_class *chanel, void *callback_data);

// chanel output thread that repeatedly executes
// the chanel_output_step function; this thread is
// started for standalone execution only
void *chanel_output_thread(void *arg);

// chanel configuration thread that reads deltaQ data
// from the configuration pipe
void *chanel_configuration_thread(void *arg);

// execute a chanel instance associated with the source
// init_source_id and using the callback function 
// callback_recv_function to provide the data to chanel user;
// init_source_id==UNDEFINED_ID signifies source is undefined;
// return SUCCESS on succes, ERROR on error
int chanel_execute(chanel_class *chanel, int init_source_id, 
		   float output_sleep_time,
		   void (*callback_recv_function)(void*,data_class*), 
		   double TIME_SUBSTRACT_param);

// function called to stop chanel execution
// by triggering the end of thread execution
void chanel_stop(chanel_class *chanel);

// function called to finalize chanel execution
// and wait for end of thread execution;
// return SUCCESS on succes, ERROR on error
int chanel_finalize(chanel_class *chanel);


#endif
