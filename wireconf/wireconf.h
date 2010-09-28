
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
 * File name: wireconf.h
 * Function: Header file of wired-network emulator configuration
 *
 * Authors: Junya Nakata, Razvan Beuran
 *
 ***********************************************************************/
#ifdef __FreeBSD__
#include <netinet/ip_fw.h>
#include <netinet/ip_dummynet.h>
#endif


#ifndef	__WIRECONF_H
#define	__WIRECONF_H_

#include <sys/queue.h>
#include <sys/socket.h>

#include <sys/types.h>

#include <net/if.h>
#include <netinet/in.h>


/////////////////////////////////////////////
// Basic constants
/////////////////////////////////////////////

// traffic directions to which dummynet configuration
// is to be applied
#define DIRECTION_BOTH                  0
#define DIRECTION_IN                    1
#define DIRECTION_OUT                   2


/////////////////////////////////////////////
// Socket manipulation functions
/////////////////////////////////////////////

// get socket identifier;
// return socket identifier on succes, ERROR on error
int get_socket(void);

// close socket specified by socket_id
void close_socket(int socket_id);


////////////////////////////////////////////////
// Functions implemented by the wireconf library
////////////////////////////////////////////////

// get a rule
// NOT WORKING PROPERLY YET!!!!!!!!!!!!!
#ifdef __FreeBSD__
int get_rule(uint s, int16_t rulenum);
#elif __linux
int get_rule_linux(void);
#endif

// add an ipfw rule containing a dummynet pipe to the firewall
// return SUCCESS on succes, ERROR on error
int add_rule(int s, uint16_t rulenum, int pipe_nr, char *src, char *dst, 
	     int direction);

// delete an ipfw rule;
// return SUCCESS on succes, ERROR on error
#ifdef __FreeBSD__
int delete_rule(uint s, u_int32_t rule_number);
#elif __linux
int delete_netem(uint s, char* dst, u_int32_t rule_number);
#endif

// print a rule structure
#ifdef __FreeBSD__
void print_rule(struct ip_fw *rule);
#endif

// print a pipe structure
#ifdef __FreeBSD__
void print_pipe(struct dn_pipe *pipe);
#endif

// configure a dummynet pipe;
// return SUCCESS on succes, ERROR on error
#ifdef __FreeBSD__
int configure_pipe(int s, int pipe_nr, int bandwidth, int delay, int lossrate);
#elif __linux
int configure_qdisc(int s, char* dst, int handle, int bandwidth, int delay, double lossrate);
#endif

#endif /* !__WIRECONF_H */
