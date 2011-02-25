
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
 * File name: timer.h
 * Function: Header file of the timer library
 *
 * Authors: Junya Nakata, Razvan Beuran
 *
 *   $Revision: 93 $
 *   $LastChangedDate: 2008-09-12 11:16:33 +0900 (Fri, 12 Sep 2008) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/

#ifndef __TIMER_H
#define __TIMER_H

#include <sys/types.h>
#include <sys/sysctl.h>

#include <unistd.h>

// time measurement macro
#define TCHK_START(name)           \
    struct timeval name##_prev;    \
    struct timeval name##_current; \
    gettimeofday(&name##_prev, NULL)

#define TCHK_END(name)                                                             \
gettimeofday(&name##_current, NULL);                                               \
time_t name##_sec;                                                                 \
suseconds_t name##_usec;                                                           \
if (name##_current.tv_sec == name##_prev.tv_sec) {                                 \
    name##_sec = name##_current.tv_sec - name##_prev.tv_sec;                       \
    name##_usec = name##_current.tv_usec - name##_prev.tv_usec;                    \
} else if (name ##_current.tv_sec != name##_prev.tv_sec) {                         \
    int name##_carry = 1000000;                                                    \
    name##_sec = name##_current.tv_sec - name##_prev.tv_sec;                       \
    if (name##_prev.tv_usec > name##_current.tv_usec) {                            \
        name##_usec = name##_carry - name##_prev.tv_usec + name##_current.tv_usec; \
        name##_sec--;                                                              \
        if (name##_usec > name##_carry) {                                          \
            name##_usec = name##_usec - name##_carry;                              \
            name##_sec++;                                                          \
        }                                                                          \
    } else {                                                                       \
        name##_usec = name##_current.tv_usec - name##_prev.tv_usec;                \
    }                                                                              \
}                                                                                  \
printf("%s: sec:%lu usec:%06ld\n", #name, name##_sec, name##_usec); 

///////////////////////////////////
// Structures and macros
///////////////////////////////////

// structure for the timer handle
typedef struct
{
  // stored CPU frequency
  uint32_t cpu_frequency;

  // the relative "zero" of the timer
  uint64_t zero;

  // the time of the next timer event
  uint64_t next_event;

} timer_handle;

// macro to read time stamp counter from CPU
#define rdtsc(t) asm volatile("rdtsc" : "=A" (t))


/////////////////////////////////////////////
// Functions implemented by the timer library
/////////////////////////////////////////////

// get the frequency of the CPU
unsigned int get_cpu_frequency(void);

// init a timer handle and return a pointer to it
// return NULL in case of error
timer_handle *timer_init(void);

// reset the timer (set its relative "zero");
void timer_reset(timer_handle *handle);

// wait for a time to occur (specified in microseconds);
// return SUCCESS on success, ERROR if deadline was missed
int timer_wait(timer_handle *handle, uint64_t time_in_us);

// free timer resources
void timer_free(timer_handle *handle);

#endif /* !__TIMER_H */
