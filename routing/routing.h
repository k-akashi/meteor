
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
 * File name: routing.h
 * Function: Header file of routing.c
 *
 * Authors: Takashi Okada, Razvan Beuran
 *
 *   $Revision: 128 $
 *   $LastChangedDate: 2009-02-06 10:21:50 +0900 (Fri, 06 Feb 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#ifndef __ROUTING_H
#define __ROUTING_H

#include <sys/types.h>	/* basic system data types */
#include <sys/socket.h>	/* basic socket definitions */
#include <sys/time.h>	/* timeval{} for select() */
#include <time.h>	/* timespec{} for pselect() */
#include <netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>	/* inet(3) functions */
#include <errno.h>
#include <fcntl.h>	/* for nonblocking */
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>	/* for S_xxx file mode constants */
#include <sys/uio.h>	/* for iovec{} and readv/writev */
#include <unistd.h>
#include <sys/wait.h>
#include <sys/un.h>	/* for Unix domain sockets */
#include <net/if_dl.h>
#include <net/route.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <sys/param.h>
#include <sys/sysctl.h>

#define RTCTL_ADD	1
#define RTCTL_DELETE	2
#define RTCTL_CHANGE	4
#define RTCTL_GATEWAY	8
#define RTCTL_IF	16
#define RTCTL_NETMASK	32
#define RTCTL_UNKNOWN	64
#define RTCTL_CMD_LEN	10


#define BUFLEN  (sizeof(struct rt_msghdr) + 512)
        /* sizeof(struct sockaddr_in6) * 8 = 192 */
#define SEQ     9999

/*
 * Round up 'a' to next multiple of 'size'
 */
#define ROUNDUP(a, size) (((a) & ((size)-1)) ? (1 + ((a) | ((size)-1))) : (a))

/*
 * Step to next socket address structure;
 * if sa_family is 0, assume it is sizeof(u_long).
 *
 * advance to next socket address
 */
#define NEXT_SA(ap) ap = (struct sockaddr *) \
  ((caddr_t) ap + (ap->sa_family ? ROUNDUP(ap->sa_family, sizeof (u_long)) : \
  sizeof(u_long)))


//#define RTCTL_PRINT_RESPONSE

//
struct addrinfo * host_serv(const char *host, const char *serv, int family, 
			    int socktype);

//
struct addrinfo * Host_serv(const char *host, const char *serv, int family, 
			    int socktype);

//
char * sock_ntop_host(const struct sockaddr *sa, socklen_t salen);

//
char * Sock_ntop_host(const struct sockaddr *sa, socklen_t salen);

//
char * sock_masktop(struct sockaddr *sa, socklen_t salen);

//
void get_rtaddrs(int addrs, struct sockaddr *sa, struct sockaddr **rti_info);

//
char * net_rt_iflist(int family, int flags, size_t *lenp);

//
char * Net_rt_iflist(int family, int flags, size_t *lenp);

//
unsigned int if_nametoindex(const char *name);

//
int rtctl_add(char *daddr, char *gaddr, char *maddr, char *ifname);

//
int rtctl_delete(char *daddr, char *gaddr, char *maddr, char *ifname);

//
int rtctl(char *daddr, char *gaddr, char *maddr, char *ifname, u_char cmd);

//
int rtctl2(uint32_t *daddr, uint32_t *gaddr, uint32_t *maddr, char *ifname, 
	   u_char cmd);

//
int getrt(char *ipaddr);


#endif

