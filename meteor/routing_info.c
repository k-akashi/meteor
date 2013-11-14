
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
 * File name: routing_info.c
 * Function: Get routing information from kernel routing table and map it 
 *           to and from internal QOMET node id
 *
 * Authors: Lan Nguyen Tien, Razvan Beuran
 *
 *   $Revision: 128 $
 *   $LastChangedDate: 2009-02-06 10:21:50 +0900 (Fri, 06 Feb 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/

#include "wireconf.h"
#include "global.h"
#include "routing_info.h"
#include "message.h"


///////////////////////////////////
// Routing table related functions
///////////////////////////////////

// given the arrays of IP addresses in binary and char format
// 'IP_addresses' and 'IP_char_addresses', get the next hop id
// for the route to destination 'dst_id'; 'direction' can be either
// DIRECTION_IN (incoming route) or DIRECTION_OUT (outgoing route);
// return the next_hop_id on success, ERROR on error
int
get_next_hop_id(IP_addresses, IP_char_addresses, dst_id, direction)
in_addr_t *IP_addresses;
char *IP_char_addresses;
int dst_id;
int direction;
{
    char dst_ip[IP_ADDR_SIZE];
    char next_hop_ip[IP_ADDR_SIZE];
    int next_hop_id;

    // for incoming routes, the route is the destination itself
    if(direction == DIRECTION_IN || direction == DIRECTION_BR) {
        return dst_id;
    }

    // convert destination id to IP address
    if(get_ip_from_id(IP_char_addresses, dst_id, dst_ip) == ERROR) {
        return ERROR;
    }

    // get the next hop IP address for a destination IP address
#ifdef __FreeBSD
    if(get_next_hop_ip(dst_ip, next_hop_ip) == ERROR) {
        return ERROR;
    }
#endif

    // get the next hop id from the next hop IP address
    if((next_hop_id = get_id_from_ip(IP_addresses, next_hop_ip)) != ERROR) {
        return next_hop_id;
    }

    return ERROR;
}

// get from the routing table the next hop associated to destination with
// IP address 'dst'; the result IP address is stored in the argument 
// 'next_hop';
// return SUCCESS on success, ERROR on error
int
get_next_hop_ip(dst, next_hop)
char *dst;
char *next_hop;
{
    int sockfd;
    char *buf;
    pid_t pid;

    struct sockaddr *sa; 
    struct sockaddr *rti_info[RTAX_MAX];
    struct sockaddr_in *sin;
    char *gw;

#ifdef __FreeBSD__
    struct rt_msghdr *rtm;
#elif __linux
    memset(rti_info, 0, sizeof(rti_info));
    struct rtmsg *rtm;
#endif

#ifdef __FreeBSD__
    ssize_t n;
    char *rt_dst;
    
    buf = calloc(1,ROUTE_BUFFER_SIZE);
    rtm = (struct rt_msghdr *) buf;
    sin = (struct sockaddr_in *) (rtm + 1);
#elif __linux
    buf = calloc(1, ROUTE_BUFFER_SIZE);
    rtm = (struct rtmsg *)buf;
    sin = (struct sockaddr_in *) (rtm + 1);
#endif

    sin->sin_family = AF_INET;
#ifdef __FreeBSD__
    sin->sin_len = sizeof(struct sockaddr_in);
#endif
    inet_pton(AF_INET, dst, &(sin->sin_addr));

    sockfd = socket(PF_ROUTE, SOCK_RAW, 0);

    if(sockfd == -1) {
        WARNING("Cannot open socket to routing table");
        exit(1);
    }

#ifdef __FreeBSD__
    rtm = (struct rt_msghdr *) buf;
    rtm->rtm_msglen = sizeof(struct rt_msghdr) + sizeof(struct sockaddr_in);
    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_GET;
    rtm->rtm_addrs = RTA_DST;
    rtm->rtm_pid = pid = getpid();
    rtm->rtm_seq = RTM_SEQ_CT;

    // send request through routing socket
    write(sockfd, rtm, rtm->rtm_msglen);

    // receive the return value from the socket
    do {
        n = read(sockfd, rtm, ROUTE_BUFFER_SIZE);
    } while(rtm->rtm_type != RTM_GET || rtm->rtm_seq != RTM_SEQ_CT || rtm->rtm_pid != pid);

    rtm = (struct rt_msghdr *) buf;
    sa = (struct sockaddr *) (rtm + 1);

    // read from sockaddr sa & put result in to rti_info
    get_routing_table_addr(rtm->rtm_addrs, sa, rti_info);

    // free buffer
    free(buf);
  
    // return gateway IP address
    if((sa = rti_info[RTAX_DST]) != NULL) {
        rt_dst = sock_ntop_host(sa, sa->sa_family);
    }
    else {
        return ERROR;
    }

#elif __linux
	rtm = (struct rtmsg *)buf;
	rtm->rtm_type = RTM_GETROUTE;
	pid = getpid();
    if(pid != 0) {
        fprintf(stderr, "Operation not permitted\n");
    }
#endif
    // if rt_dst != NULL and rt_dst equals dst then gw is considered

    //if (!strcmp(rt_dst, dst)) {
    if(1) {
        int rtf_gateway;;
#ifdef __FreeBSD__
        rtf_gateway = RTAX_GATEWAY;
#elif __linux
        rtf_gateway = RTF_GATEWAY;
#endif
        if((sa = rti_info[rtf_gateway]) != NULL) {
	        gw = (char *) sock_ntop_host(sa, sa->sa_family);
	
	        if (strncmp(gw, "AF_LINK", 7)) {
                strncpy(next_hop, gw, 20);
	            close(sockfd);

	            return SUCCESS;
	        }
	        else {
	            strncpy(next_hop, dst, 20);
	            close(sockfd);

	            return SUCCESS;
	        }
        }
        else {
	        close(sockfd);
	        return ERROR;
	    }
    }
    else {
        close(sockfd);

        return ERROR;
    }

    close(sockfd);

    /*
    if ( (sa = rti_info[RTAX_DST]) != NULL)
      printf("dest: %s\n", sock_ntop_host(sa, sa->sa_family));
    if ( (sa = rti_info[RTAX_GATEWAY]) != NULL)
      printf("gateway: %s\n", sock_ntop_host(sa, sa->sa_family));
    if ( (sa = rti_info[RTAX_NETMASK]) != NULL)
      printf("netmask: %s\n", sock_masktop(sa, sa->sa_family));
    if ( (sa = rti_info[RTAX_GENMASK]) != NULL)
      printf("genmask: %s\n", sock_masktop(sa, sa->sa_family));
    */
}

// get addresses from routing table
void
get_routing_table_addr(addrs, sa, rti_info)
int addrs;
struct sockaddr *sa;
struct sockaddr **rti_info;
{
    int i;
    for(i = 0; i < RTAX_MAX; i++) {
        if(addrs &(1 << i)) {
            rti_info[i] = sa;
            NEXT_SA(sa);
        }
        else {
	        rti_info[i] = NULL;
        }
    }
}

// transform a socket adress 'sa' with length 'salen'
// into a readable IP address (string);
// return the IP address as a string
char
*sock_ntop_host(sa, salen)
const struct sockaddr *sa;
socklen_t salen;
{
    static char str[128]; // Unix domain is largest

    switch (sa->sa_family) {
        case AF_INET: {
            struct sockaddr_in *sin = (struct sockaddr_in*)sa;
	
            if(inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL) {
                return(NULL);
            }
            return(str);
        }

        case AF_INET6: {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)sa;

            if(inet_ntop(AF_INET6, &sin6->sin6_addr, str, sizeof(str)) == NULL) {
                return(NULL);
            }
            return(str);
        }
#if 0
        case AF_UNIX:
            struct sockaddr_un*unp = (struct sockaddr_un *) sa;

            /*
            OK to have no pathname bound to the socket: happens on
            every connect() unless client calls bind() first.
            */
            if(unp->sun_path[0] == 0) {
              strcpy(str, "(no pathname bound)"); 
            }
            else {
                snprintf(str, sizeof(str), "%s", unp->sun_path);
            }
            return(str);
#endif

#ifdef __FreeBSD__
        //case AF_LINK:
        case AF_PACKET:
            struct sockaddr_dl*sdl = (struct sockaddr_dl *) sa;
            //struct sockaddr_dl*sll = (struct sockaddr_dl *) sa;
        
            if (sdl->sdl_nlen > 0) {
            //if (sll->sll_family > 0)
                snprintf(str, sizeof(str), "%*s", sdl->sdl_nlen, &sdl->sdl_data[0]);
            }
            else {
                snprintf(str, sizeof(str), "AF_LINK, index=%d", sdl->sdl_index);
                return(str);
            }
#endif

        default:
            snprintf(str, sizeof(str), "sock_ntop_host: unknown AF_xxx: %d, len %d", sa->sa_family, salen);
            return(str);
    }
    return NULL;
}

// map an IP address to internal QOMET node id;
// return id on success, ERROR on error
int
get_id_from_ip(IP_addresses, s)
in_addr_t *IP_addresses;
char *s;
{
    uint32_t ip;
    int i;

    if((ip = inet_addr(s)) != INADDR_NONE) {
        for(i = 0; i < MAX_NODES; i++) {
            //INFO("Address %s: check %d and %d", s, ip, ipaddr[i]);
            if(ip == IP_addresses[i]) {
                return i;
            }
        }
    }
    else {
        WARNING("The address provided (%s) cannot be maped to an id", s);
    }

    return ERROR;
}

// map an internal QOMET node id to an IP address; 
// the address is return in the argument 'ip';
// return SUCCESS on success, ERROR on error
int
get_ip_from_id(IP_char_addresses, id, ip)
char *IP_char_addresses;
int id;
char *ip;
{
  //uint32_t ipaddr[MAX_NODES];

    if(id < 0 || id >= MAX_NODES ) {
        WARNING("Id %d is not within the valid range [%d,%d]", id, 0, MAX_NODES);
        return ERROR;
    }

    strncpy(ip, IP_char_addresses + (id - FIRST_NODE_ID) * IP_ADDR_SIZE, IP_ADDR_SIZE);
  
    return SUCCESS;
}
