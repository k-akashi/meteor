
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
 * File name: routing_info.h
 * Function: Header file of routing_info.c 
 *
 * Authors: Lan Nguyen Tien, Razvan Beuran
 *
 *   $Revision: 128 $
 *   $LastChangedDate: 2009-02-06 10:21:50 +0900 (Fri, 06 Feb 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <net/route.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <sys/param.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/rtnetlink.h>
#ifdef __FreeBSD__
#include <sys/stddef.h>
#include <net/if_dl.h>
#elif __linux
#include <linux/stddef.h>
#include <linux/if_ether.h>
#endif



/////////////////////////////////////////////
// Basic constants
/////////////////////////////////////////////

// size of buffer used when getting route information from kernel
#ifdef __FreeBSD__
#define ROUTE_BUFFER_SIZE (sizeof(struct rt_msghdr) + 512)
#elif __linux
#define ROUTE_BUFFER_SIZE (sizeof(struct rtmsg) + 512)
#endif

// constant used in route message structure
#define RTM_SEQ_CT                    9999


/////////////////////////////////////////////
// Basic macros
/////////////////////////////////////////////

// roundup socket address length (see below)
#define ROUNDUP(a, size) (((a) & ((size)-1)) ? (1 + ((a) | ((size)-1))) : (a))

// get next socket address
#define NEXT_SA(ap) ap = (struct sockaddr *) \
((caddr_t) ap+ (ap->sa_family ? ROUNDUP(ap->sa_family, sizeof(u_long)) : \
sizeof(u_long)))


/////////////////////////////////////////////
// Routing table related functions
/////////////////////////////////////////////

// given the arrays of IP addresses in binary and char format
// 'IP_addresses' and 'IP_char_addresses', get the next hop id
// for the route to destination 'dst_id'; 'direction' can be either
// ROUTE_INCOMING or ROUTE_OUTGOING;
// return the next_hop_id on success, ERROR on error
int get_next_hop_id(in_addr_t *IP_addresses, char *IP_char_addresses, 
		    int id_dst, int direction);

// get from the routing table the next hop associated to destination with
// IP address 'dst'; the result IP address is stored in the argument 
// 'next_hop';
// return SUCCESS on success, ERROR on error
int get_next_hop_ip(char *dst, char *next_hop);

// get addresses from routing table
void get_routing_table_addr(int addrs, struct sockaddr *sa, 
			    struct sockaddr **rti_info);

// transform a socket adress 'sa' with length 'salen'
// into a readable IP address (string);
// return the IP address as a string
char * sock_ntop_host(const struct sockaddr *sa, socklen_t salen);

// map an IP address to internal QOMET node id;
// return id on success, -1 on error
int get_id_from_ip(in_addr_t *IP_addresses, char *s);

// map an internal QOMET node id to an IP address; 
// the address is return in the argument 'ip';
// return SUCCESS on success, ERROR on error
int get_ip_from_id(char *IP_char_addresses, int id, char *ip);


