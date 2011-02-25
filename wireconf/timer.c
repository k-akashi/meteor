
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
 * File name: timer.c
 * Function: Main file of the timer library
 *
 * Authors: Junya Nakata, Razvan Beuran
 *
 *   $Revision: 128 $
 *   $LastChangedDate: 2009-02-06 10:21:50 +0900 (Fri, 06 Feb 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <sys/sysctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "global.h"
#include "timer.h"

#ifdef __linux
#include <string.h>
#endif

///////////////////////////////////
// Message-handling macros
///////////////////////////////////

#ifdef MESSAGE_WARNING
#define WARNING(message...) do {                                          \
  fprintf(stdout, "libtimer WARNING: %s, line %d: ", __FILE__, __LINE__); \
  fprintf(stdout, message); fprintf(stderr,"\n");                       \
} while(0)
#else
#define WARNING(message...) /* message */
#endif

/////////////////////////////////////////////
// Functions implemented by the timer library
/////////////////////////////////////////////

// get the frequency of the CPU
unsigned int get_cpu_frequency(void)
{
  uint32_t hz = 0;

  // use system call to get CPU frequency
#ifdef __FreeBSD__
  unsigned int s = sizeof(hz);
  sysctlbyname("machdep.tsc_freq", &hz, &s, NULL, 0);
#elif __linux
//  hz = sysconf(_SC_CLK_TCK);
	FILE *cpuinfo;
	double tmp_hz;
	char str[256];
	char buff[10];

    if((cpuinfo = fopen("/proc/cpuinfo", "rb")) == NULL)
	{
		printf("file open error!!\n");
		exit(EXIT_FAILURE);
	}

	while(fgets(str, 256, cpuinfo) != NULL)
	{
		if(strstr(str, "cpu MHz") != NULL){
			strncpy(buff, str+11, 30);
			tmp_hz = atof(buff);
			hz = tmp_hz * 1000 * 1000;
			break;
		}
	}
#endif

  return hz;
}

// init a timer handle and return a pointer to it;
// return NULL in case of error
timer_handle *timer_init(void)
{
  timer_handle *handle;
  
  // allocte the timer handle
  if((handle = (timer_handle *)malloc(sizeof(timer_handle))) == NULL) 
    {
      WARNING("Could not allocate memory for the timer");
      return NULL;
    }

  // store CPU frequency
  handle->cpu_frequency = get_cpu_frequency();

  // reset the timer so that 'zero' has a reasonable value
  timer_reset(handle);

  return handle;
}

// reset the timer (set its relative "zero")
void timer_reset(timer_handle *handle)
{
  // init the 'next_event' and 'zero' values
  rdtsc(handle->next_event);
  rdtsc(handle->zero);
}

// wait for a time to occur (specified in microseconds);
// return SUCCESS on success, ERROR if deadline was missed
int timer_wait(timer_handle *handle, uint64_t time_in_us)
{
  // the current time
  uint64_t crt_time;

  // compute the value of the next timer event  
  handle->next_event = handle->zero + 
    (handle->cpu_frequency * time_in_us) / 1000000;

  // get current time
  rdtsc(crt_time);

  // if current time exceeds next event time
  // the deadline was missed
  if(handle->next_event < crt_time)
    return ERROR;

  // wait for the next event to occur
  do
    {
      // get current time
      rdtsc(crt_time);
	  usleep(1);
    } 
  while(handle->next_event >= crt_time);

  return SUCCESS;
}

// free timer resources
void timer_free(timer_handle *handle)
{
  free(handle);
}

