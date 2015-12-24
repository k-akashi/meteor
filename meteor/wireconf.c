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
 * File name: wireconf.c
 * Function: Main source file of the wired-network emulator 
 *           configurator library
 *
 * Authors: Junya Nakata, Razvan Beuran
 * Changes: Kunio AKASHI
 *
 ***********************************************************************/

#include <sys/queue.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//#include "wireconf.h"
#include "utils.h"
#include "libnlwrap.h"
#include "config.hpp"

#define FRAME_LENGTH 1522
#define INGRESS 1 // 1 : ingress mode, 0 : egress mode
#define DEV_NAME 256
#define OFFSET_RULE 32767

float priv_delay = 0;
double priv_loss = 0;
float priv_rate = 0;

int32_t
create_ifb(struct nl_sock *sock, int32_t id)
{
    char *ifbdevname;
    int err;
    int if_index;
    struct nl_cache *cache;
    struct rtnl_link *link;

    ifbdevname = (char *)malloc(15);
    sprintf(ifbdevname, "ifb%d", id);
    rtnl_link_alloc_cache(sock, AF_UNSPEC, &cache);
    if_index = rtnl_link_name2i(cache, ifbdevname);
    nl_cache_put(cache);

    if (!if_index) {
        if (!(link = rtnl_link_alloc())) {
            perror("Unable to alloc link");
        }
        rtnl_link_set_name(link, ifbdevname);
        rtnl_link_set_type(link, "ifb");
        rtnl_link_set_num_tx_queues(link, 8);
        rtnl_link_set_num_rx_queues(link, 8);
        if ((err = rtnl_link_add(sock, link, NLM_F_CREATE)) < 0) {
            nl_perror(err, "Unable to add link");
            return err;
        }
        rtnl_link_alloc_cache(sock, AF_UNSPEC, &cache);
        if_index = rtnl_link_name2i(cache, ifbdevname);
        rtnl_link_put(link);
        nl_cache_put(cache);
    }

    uint32_t flags;
    if (!(link = rtnl_link_alloc())) {
        perror("Unable to alloc link");
    }
    rtnl_link_set_name(link, ifbdevname);
    flags = rtnl_link_get_flags(link);
    if (!flags) {
        rtnl_link_set_flags(link, IFF_UP);
        flags = rtnl_link_get_flags(link);
        if ((err = rtnl_link_add(sock, link, RTM_SETLINK)) < 0) {
            nl_perror(err, "Unable to modify link");
            return err;
        }
    }

    //free(ifbdevname);
    rtnl_link_put(link);

    return if_index;
}

int32_t
delete_ifb(struct nl_sock *sock, int32_t if_index)
{
    int err;
    struct rtnl_link *link;

    link = rtnl_link_alloc();
    rtnl_link_set_ifindex(link, if_index);
    if (!(err = rtnl_link_delete(sock, link))) {
        nl_perror(err, "Unable delete link");
        return err;
    }

    return 0;
}

int
init_rule(struct nl_sock *sock, int if_index, int ifb_index, int32_t protocol)
{
    delete_qdisc(sock, if_index, TC_H_INGRESS, 0);
    add_ingress_qdisc(sock, if_index);
    add_mirred_filter(sock, if_index, ifb_index);

    delete_qdisc(sock, ifb_index, TC_H_ROOT, 0);

    add_htb_qdisc(sock, ifb_index, TC_H_ROOT, TC_HANDLE(1, 0), 65535);
    add_htb_class(sock, ifb_index, TC_HANDLE(1, 0), TC_HANDLE(1, 1), 1, 1000000000);

    // Default Drop rule
    add_htb_class(sock, ifb_index, TC_HANDLE(1, 0), TC_HANDLE(1, 1), 1, 1000000000);
    add_htb_class(sock, ifb_index, TC_HANDLE(1, 0), TC_HANDLE(1, 65535), 1, 1000000);

    int delay = 0; // us
    int jitter = 0; // us
    uint32_t loss = 100; // %
    add_netem_qdisc(sock, ifb_index, TC_HANDLE(1, 65535), TC_HANDLE(65535, 0), delay, jitter, loss, 1000);

    return 0;
}

int
add_rule(struct nl_sock *sock, int ifb_index, uint16_t parent, uint16_t handle, 
        int32_t proto, struct node_data *src, struct node_data *dst)
{
    add_htb_class(sock, ifb_index, TC_HANDLE(1, 0), TC_HANDLE(1, parent), 1, 1000000000);

    if (proto == ETH_P_ALL) {
        if (!dst) {
            add_class_macfilter(sock, ifb_index, TC_HANDLE(1, 0), TC_HANDLE(1, parent), 
                    src->mac, NULL);
        }
        else {
            add_class_macfilter(sock, ifb_index, TC_HANDLE(1, 0), TC_HANDLE(1, parent), 
                    src->mac, dst->mac);
        }
    }
    if (proto == ETH_P_IP) {
        if (!dst) {
            add_class_ipv4filter(sock, ifb_index, TC_HANDLE(1, 0), TC_HANDLE(1, parent), 
                    src->ipv4addr.s_addr, src->ipv4prefix, NULL, NULL);
        }
        else {
            add_class_ipv4filter(sock, ifb_index, TC_HANDLE(1, 0), TC_HANDLE(1, parent), 
                    src->ipv4addr.s_addr, src->ipv4prefix, dst->ipv4addr.s_addr, dst->ipv4prefix);
        }
    }

    int delay = 0 ;
    int jitter = 0;
    uint32_t loss = 100; // %
    add_netem_qdisc(sock, ifb_index, TC_HANDLE(1, parent), TC_HANDLE(parent, 0), delay, jitter, loss, 1000);

    return 0;
}

int32_t
configure_rule(struct nl_sock *sock, int ifb_index, uint16_t parent, uint16_t handle, 
        int32_t bandwidth, double delay, double loss)
{
    change_htb_class(sock, ifb_index, TC_HANDLE(1, 0), TC_HANDLE(1, parent), 1, bandwidth);
    change_netem_qdisc(sock, ifb_index, TC_HANDLE(1, parent), TC_HANDLE(parent, 0), delay, 0, loss, 1000);
 
    return 0;
}
