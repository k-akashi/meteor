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
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "global.h"
#include "wireconf.h"

#ifdef __FreeBSD__
#include <netinet/ip_fw.h>
#include <netinet/ip_dummynet.h>
#endif

#ifdef __linux
#include "utils.h"
#include "ip_common.h"
#include "tc_common.h"
#include "tc_util.h"
#endif

#define FRAME_LENGTH 1522
#define INGRESS 1 // 1 : ingress mode, 0 : egress mode
#define MAX_DEV 4096
#define DEV_NAME 256
#define OFFSET_RULE 32767

#ifdef INGRESS
char ifb_devname[DEV_NAME] = "ifb0";
int32_t dev_no = 0;
struct if_list iflist;
struct device_list {
    char dst_address[16];
    char dev[DEV_NAME];
} dev_list[MAX_DEV];
#endif 

typedef union {
    uint8_t octet[4];
    uint32_t word;
} in4_addr;

float priv_delay = 0;
double priv_loss = 0;
float priv_rate = 0;

#ifdef __FreeBSD__
static int 
atoaddr(array_address, ipv4_address, port)
char *array_address;
in4_addr *ipv4_address;
uint16_t *port;
{
    uint32_t c;
    uint32_t o;
  
    c = 0;
    o = 0;
    ipv4_address->word = 0;
    *port = 0;

    for(;;) {
        if(array_address[c] == '.') {
            c++;
            o++;
            continue;
        } 
        else if(isdigit(array_address[c]) == 0) {
            break;
        }
        ipv4_address->octet[o] = ipv4_address->octet[o] * 10 + array_address[c] - '0';
        c++;
    }

    if(array_address[c] == '\0') {
        return SUCCESS;
    }
    else if((o != 3) || (array_address[c] != ':')) {
        return ERROR;
    }
    c++;
    
    for(;;) {
        if(isdigit(array_address[c]) == 0) {
            break;
        }
        *port = *port * 10 + array_address[c] - '0';
        c++;
    }
    return SUCCESS;
}
#endif

int
get_socket(void)
{
#ifdef __FreeBSD__
    uint32_t socket_id;
  
    if((socket_id = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
        WARNING("Error creating socket");
        perror("socket");

        return ERROR;
    }

    return socket_id;
#elif __linux
    return rtnl_open(&rth, 0);
#endif
}

void
close_socket(socket_id)
int socket_id;
{
#ifdef __FreeBSD__
    close(socket_id);
#elif __linux
    rtnl_close(&rth);
#endif
}

#ifdef __FreeBSD__
static int
apply_socket_options(s, option_id, option, socklen_t, option_length)
int s;
int option_id;
void *option;
socklen_t option_length;
{
    switch(option_id) {
        case IP_FW_ADD:
            DEBUG("Add ipfw rule: "); 
            print_rule((struct ip_fw*)option);

            if(getsockopt(s, IPPROTO_IP, option_id, option, &option_length) < 0) {
                WARNING("Error getting socket options");
                perror("getsockopt");
    
                return ERROR;
            }
            break;
        case IP_FW_DEL:
            DEBUG("Delete ipfw rule #%d", (*(u_int32_t *)option));

            if(setsockopt(s, IPPROTO_IP, option_id, option, option_length) < 0) {
                WARNING("Error setting socket options");
                perror("setsockopt");

                return ERROR;
            }
            break;
        case IP_DUMMYNET_CONFIGURE:
            DEBUG("Configure ipfw dummynet pipe: "); 
            print_pipe((struct dn_pipe*)option);
            if(setsockopt(s, IPPROTO_IP, option_id, option, option_length) < 0) {
                WARNING("Error setting socket options");
                perror("setsockopt");
                return ERROR;
            }
            break;
        /* FUTURE PLAN: DELETE _PIPES_ INSTEAD OF RULES
        case IP_DUMMYNET_DEL:
            printf("delete pipe: option_id=%d option=%d option_length=%d\n", option_id, (*(int16_t*)(option)), option_length);
            if(setsockopt(s, IPPROTO_IP, option_id, option, option_length)<0) {
                WARNING("Error setting socket options");
                perror("setsockopt");

                return ERROR;
            }
            break;
        */
        default:
            WARNING("Unrecognized ipfw socket option: %d", option_id);

            return -1;
    }
 
   return 0;
}
#endif


// NOT WORKING PROPERLY YET!!!!!!!!!!!!!
int
get_rule(s, rulenum)
uint s;
int16_t rulenum;
{
#ifdef __FreeBSD__
    int i = 0;
    struct ip_fw rules[10];

    socklen_t rules_size;

    bzero(&rules, sizeof(rules));
    rules_size=sizeof(rules);

    if(getsockopt(s, IPPROTO_IP, IP_FW_GET, &rules, &rules_size) < 0) {
        WARNING("Error getting socket options");
        perror("getsockopt");
        return -1;
    }
    printf("Got rules:\n");
    for(i = 0; i < 10; i++) {
        print_rule(&rules[i]);
    }

#elif __linux
    uint32_t ret;
    ret = system("tc qdisc show");
    if(ret != 0) {
        dprintf(("Cannot print qdisc setting\n"));
    }
#endif
    return 0;
}

#ifdef __FreeBSD__
int
add_rule_ipfw(dsock, rulenum, pipe_nr, src, dst, direction)
int dsock;
uint16_t rulenum;
int pipe_nr;
char *src;
char *dst;
int direction;
{
    int clen;
    in4_addr addr;
    uint16_t port;
    ipfw_insn cmd[10];
    struct ip_fw *p;

    clen = 0;

    if(strncmp(src, "any", strlen(src))==0) {
        cmd[clen].opcode = O_IP_SRC;
        cmd[clen].len = 0;
        cmd[clen].arg1 = 0;
    }
    else {
        if(atoaddr(src, &addr, &port) < 0) {
            WARNING("Invalid argument to add_rule: %s\n", src);
            return ERROR;
        }
        cmd[clen].opcode = O_IP_SRC;
        cmd[clen].len = 2;
        cmd[clen].arg1 = 0;
        ((uint32_t *)cmd)[clen + 1] = addr.word;
        clen += 2;
        if(port > 0) {
            cmd[clen].opcode = O_IP_SRCPORT;
            cmd[clen].len = 2;
            cmd[clen].arg1 = 0;
            ((uint32_t *)cmd)[clen + 1]
                = port | port << 16;
            clen += 2;
        }
    }

    if(strncmp(dst, "any", strlen(dst)) == 0) {
        cmd[clen].opcode = O_IP_DST;
        cmd[clen].len = 0;
        cmd[clen].arg1 = 0;
    }
    else {
        if(atoaddr(dst, &addr, &port) < 0) {
            WARNING("Invalid argument to add_rule: %s", dst);
            return ERROR;
        }

        cmd[clen].opcode = O_IP_DST;
        cmd[clen].len = 2;
        cmd[clen].arg1 = 0;
        ((uint32_t *)cmd)[clen + 1] = addr.word;
        clen += 2;
        if(port > 0) {
            cmd[clen].opcode = O_IP_DSTPORT;
            cmd[clen].len = 2;
            cmd[clen].arg1 = 0;
            ((uint32_t *)cmd)[clen + 1]
                = port | port << 16;
            clen += 2;
        }
    }

    if(direction != DIRECTION_BOTH) {
        cmd[clen].opcode = O_IN; 
        cmd[clen].len = 1;

        if(direction == DIRECTION_OUT) {
            cmd[clen].len |= F_NOT;
        }
        clen += 1;
    }

    cmd[clen].opcode = O_PIPE;
    cmd[clen].len = 2;
    cmd[clen].arg1 = pipe_nr;
    ((uint32_t *)cmd)[clen + 1] = 0;
    clen += 1;  /* trick! */
    if((p = (struct ip_fw *)malloc(sizeof(struct ip_fw) + clen * 4)) == NULL) {
        WARNING("Could not allocate memory for a new rule");
        return ERROR;
    }
    bzero(p, sizeof(struct ip_fw));
    p->act_ofs = clen - 1;
    p->cmd_len = clen + 1;
    p->rulenum = rulenum;
    bcopy(cmd, &p->cmd, clen * 4);

    if(apply_socket_options(dsock, IP_FW_ADD, p, sizeof(struct ip_fw) + clen * 4) < 0) {
        WARNING("Adding rule operation failed");
        return ERROR;
    }
    return 0;
}
#endif

int
init_rule(dst)
char *dst;
{
#ifdef __linux
    char *devname;
    uint32_t htb_qdisc_id[4];
    uint32_t htb_class_id[4];
    //struct qdisc_params qp;
    //struct u32_params ufp;

    if(!INGRESS) {
        devname =  get_route_info("dev", dst);
        devname = "eth5";
    }
    if(INGRESS) {
        set_ifb(ifb_devname, IF_UP);

        devname = "eth4";
        delete_netem_qdisc(devname, 0);
        add_ingress_qdisc(devname);
        add_ingress_filter(devname, ifb_devname);

        devname = "eth5";
        delete_netem_qdisc(devname, 0);
        add_ingress_qdisc(devname);
        add_ingress_filter(devname, ifb_devname);

/*
        int i;
        int nifaces;
        struct ifconf ifconf;
        static struct ifreq ifreqs[MAX_DEV];

        memset(&ifconf, 0, sizeof(ifconf));
        ifconf.ifc_buf = (char*) (ifreqs);
        ifconf.ifc_len = sizeof(ifreqs);

        if(get_iface_list(&ifconf) < 0) {
            return -1;
        }
        nifaces =  ifconf.ifc_len / sizeof(struct ifreq);
        dprintf(("Interfaces (count = %d)\n", nifaces));
        for(i = 0; i < nifaces; i++) {
            devname = ifreqs[i].ifr_name;
            dprintf(("[init_rule] add ingress device %s\n", devname));

            delete_netem_qdisc(devname, 0);
            add_ingress_qdisc(devname);
            add_ingress_filter(devname, ifb_devname);
        }
*/
        devname = ifb_devname;
    }
 
    htb_qdisc_id[0] = TC_H_ROOT;
    htb_qdisc_id[1] = 0;
    htb_qdisc_id[2] = 1;
    htb_qdisc_id[3] = 0;
 
    delete_netem_qdisc(devname, 0);
    add_htb_qdisc(devname, htb_qdisc_id);

/* XXX
        ufp.match.type = "u32";
        ufp.classid[0] = 1;
        ufp.classid[1] = 1;
        ufp.action = "mirred";
        ufp.rdev = ifb_devname;
        //tc_filter_modify(RTM_NEWTFILTER, NLM_F_EXCL|NLM_F_CREATE, devname, parent_filterid, "NULL", "ip", "u32", &ufp);
*/

#endif
    return 0;
}

#ifdef __linux
int
add_rule_netem(rulenum, handle_nr, src, dst, direction)
uint16_t rulenum;
int handle_nr;
char *src;
char *dst;
int direction;
{
    char *devname;
    uint32_t htb_class_id[4];
    uint32_t netem_qdisc_id[4];
    uint32_t filter_id[4];
    struct qdisc_params qp;
    struct u32_params ufp;

    if(!INGRESS) {
        devname = malloc(DEV_NAME);
        devname =  (char* )get_route_info("dev", dst);
        devname = "eth5";
    }
    else if(INGRESS) {
        devname = "ifb0";
    }

    memset(&qp, 0, sizeof(struct qdisc_params));
    memset(&ufp, 0, sizeof(struct u32_params));

/*
    dprintf(("[add_rule] devname : %s\n", devname));
    strcpy(device_list[dev_no].dst_address, dst);
    strcpy(device_list[dev_no].device, devname);
    dev_no++;
*/

    dprintf(("[add_rule] rulenum = %d\n", handle_nr));
    qp.limit = 100000;
    qp.delay = 0.001;
    qp.rate = Gigabit;
    qp.buffer = Gigabit / 1000;

    char srcaddr[20];
    char dstaddr[20];

    htb_class_id[0] = 1;
    htb_class_id[1] = 0;
    htb_class_id[2] = 1;
    htb_class_id[3] = handle_nr;

    netem_qdisc_id[0] = 1;
    netem_qdisc_id[1] = handle_nr;
    netem_qdisc_id[2] = handle_nr;
    netem_qdisc_id[3] = 0;

    filter_id[0] = 1;
    filter_id[1] = 0;
    filter_id[2] = 1;
    filter_id[3] = handle_nr;

    if(src != NULL) {
        sprintf(srcaddr, "%s/32", src);
        dprintf(("[add_rule] filter source address : %s\n", srcaddr));
    }
    else {
        dprintf(("[add_rule] source address is NULL\n"));
    }
    if(dst != NULL) {
        sprintf(dstaddr, "%s/32", dst);
        dprintf(("[add_rule] filter dstination address : %s\n", dstaddr));
    }
    else {
        dprintf(("[add_rule] destination address is NULL\n"));
    }

    if(INGRESS) {
        devname = ifb_devname;
    }

    if(strcmp(src, "any") == 0) {
        strcpy(srcaddr, "0.0.0.0/0");
        ufp.match[IP_SRC].type = NULL;
    }
    else {
        ufp.match[IP_SRC].type = "u32";
    }
    ufp.match[IP_SRC].proto = "ip";
    ufp.match[IP_SRC].filter = "src";
    ufp.match[IP_SRC].arg = srcaddr;

    if(strcmp(dst, "any") == 0) {
        strcpy(dstaddr, "0.0.0.0/0");
        ufp.match[IP_DST].type = NULL;
    }
    else {
        ufp.match[IP_DST].type = "u32";
    }
    ufp.match[IP_DST].proto = "ip";
    ufp.match[IP_DST].filter = "dst";
    ufp.match[IP_DST].arg = dstaddr;
    ufp.classid[0] = filter_id[2];
    ufp.classid[1] = filter_id[3];

    add_htb_class(devname, htb_class_id, 1000000000);
    add_netem_qdisc(devname, netem_qdisc_id, qp);
    add_tc_filter(devname, filter_id, "ip", "u32", &ufp);

    if(strcmp(dst, "any") == 0) {
        ufp.match[IP_SRC].type = NULL;
    }
    else {
        ufp.match[IP_SRC].type = "u32";
    }
    ufp.match[IP_SRC].arg = dstaddr;
    if(strcmp(src, "any") == 0) {
        ufp.match[IP_DST].type = NULL;
    }
    else {
        ufp.match[IP_DST].type = "u32";
    }
    ufp.match[IP_DST].arg = srcaddr;
    add_tc_filter(devname, filter_id, "ip", "u32", &ufp);

    return 0;
}
#endif

int 
add_rule(s, rulenum, pipe_nr, src, dst, direction)
int s;
uint16_t rulenum;
int pipe_nr;
char *src;
char *dst;
int direction;
{
#ifdef __FreeBSD
    return add_rule_ipfw(s, rulenum, pipe_nr, src, dst, direction);
#elif __linux
    return add_rule_netem(rulenum, pipe_nr, src, dst, direction);
#endif
}

#ifdef __FreeBSD
int
delete_ipfw(s, rule_number)
uint32_t s;
uint32_t rule_number;
{
    if(apply_socket_options(s, IP_FW_DEL, &rule_number, sizeof(rule_number)) < 0) {
        WARNING("Delete rule operation failed");
        return ERROR;
    }

    return SUCCESS;
}

#elif __linux
int
delete_netem(s, dst, rule_number)
uint32_t s;
char* dst;
uint32_t rule_number;
{
    struct qdisc_params qp; 
    char* devname;

    memset(&qp, 0, sizeof(qp));
    devname = malloc(DEV_NAME);


    if(!INGRESS) {
        devname = (char*)get_route_info("dev", dst);
        //delete_netem_qdisc(devname, 0);
        delete_netem_qdisc("eth5", INGRESS);
    }
    if(INGRESS) {
/*
        int ret;
        int cmd_len = 1024;
        char *cmd;

        cmd = malloc(cmd_len);
        tc_cmd(RTM_DELQDISC, 0, ifb_devname, "1", "root", qp, "pfifo");
        sprintf(cmd, "tc qdisc del dev %s root", devname);
        ret = system(cmd);
        if(ret != 0) {
            exit(1);
        }
*/
        //delete_netem_qdisc(devname, 0);
        //delete_netem_qdisc(ifb_devname, 1);
        delete_netem_qdisc("eth4", INGRESS);
        delete_netem_qdisc("eth5", INGRESS);
        delete_netem_qdisc("ifb0", 0);
    }

    return SUCCESS;
}
#endif

int
delete_rule(s, dst, rule_number)
uint32_t s;
char* dst;
uint32_t rule_number;
{
#ifdef __FreeBSD
    return delete_ipfw(s, rule_number);
#elif __linux
    return delete_netem(s, dst, rule_number);
#endif
}

#ifdef __FreeBSD
void
print_rule(rule)
struct ip_fw *rule;
{
    printf("Rule #%d (size=%d):\n", rule->rulenum, sizeof(*rule));
    printf("\tnext=%p next_rule=%p\n", rule->next, rule->next_rule);
    printf("\tact_ofs=%u cmd_len=%u rulenum=%u set=%u _pad=%u\n", 
        rule->act_ofs, rule->cmd_len, rule->rulenum, rule->set, rule->_pad);
    printf("\tpcnt=%llu bcnt=%llu timestamp=%u\n", rule->pcnt, rule->bcnt, rule->timestamp);
}

void
print_pipe(pipe)
struct dn_pipe *pipe;
{
    printf("Pipe #%d (size=%d):\n", pipe->pipe_nr, sizeof(*pipe));
    //printf("\tnext=%p pipe_nr=%u\n", pipe->next.sle_next, pipe->pipe_nr);

#ifdef NEW_DUMMYNET
    printf("\tbandwidth=%u delay=%u delay_type=%u fs.plr=%u fs.qsize=%u\n", 
            pipe->bandwidth, pipe->delay, pipe->delay_type, pipe->fs.plr, 
            pipe->fs.qsize);
#else
    printf("\tbandwidth=%u delay=%u fs.plr=%u fs.qsize=%u\n", 
            pipe->bandwidth, pipe->delay, pipe->fs.plr, pipe->fs.qsize);
#endif
}
#endif

#ifdef __FreeBSD
int
configure_pipe(s, pipe_nr, bandwidth, delay, lossrate)
int s;
int pipe_nr;
int bandwidth;
int delay;
int lossrate;
{
    struct dn_pipe p;

    bzero(&p, sizeof(p));

    p.pipe_nr = pipe_nr;
    p.fs.plr = lossrate;
    p.bandwidth = bandwidth;
    p.delay = delay;
    p.fs.qsize = 2;

    if(apply_socket_options(s, IP_DUMMYNET_CONFIGURE, &p, sizeof(p)) < 0) {
        WARNING("Pipe configuration could not be applied");
        return ERROR;
    }
    return SUCCESS;
}

#elif __linux
int
configure_qdisc(dst, handle, bandwidth, delay, lossrate)
char* dst;
int32_t handle;
int32_t bandwidth;
float delay;
double lossrate;
{
    struct qdisc_params qp;
    char *devname;
    int config_netem = 0;
    int config_bw = 0;
    uint32_t htb_class_id[4];
    uint32_t netem_qdisc_id[4];
    char buffer[20];

    memset(&qp, 0, sizeof(qp));
    devname = malloc(DEV_NAME);

#ifndef INGRESS
    int i;
    for(i = 0; i <= dev_no; i++) {
        if(strcmp(dev_list[i].dst_address, dst) == 0) {
            strcpy(devname, dev_list[i].dev);
            break;
        }
    }
#else
    devname = "eth5";
#endif

    dprintf(("[configure_qdisc] handle : %d, delay : %.6f, bandwidth %d, lossrate %.6f\n", 
        handle, delay, bandwidth, lossrate));
    htb_class_id[0] = 1;
    htb_class_id[1] = 0;
    htb_class_id[2] = 1;
    htb_class_id[3] = handle;

    netem_qdisc_id[0] = 1;
    netem_qdisc_id[1] = handle;
    netem_qdisc_id[2] = handle;
    netem_qdisc_id[3] = 0;

    if(priv_delay != delay || priv_loss != lossrate) {
        config_netem = 1;
        qp.delay = delay;
        dprintf(("[configure_qdisc] delay : %.6f\n", qp.delay));
        priv_delay = delay;
        if(lossrate > 1 || lossrate < 0) {
            qp.loss = lossrate;
        }
        else {
            qp.loss = 0;
        }
        priv_loss = lossrate;
    }

    if(priv_rate != bandwidth) {
        if((bandwidth / 1024) < FRAME_LENGTH) {
            sprintf(buffer, "%d", FRAME_LENGTH);
        }
        else {
            sprintf(buffer, "%d", bandwidth / 1024);
        }
        priv_rate = bandwidth;
        config_bw = 1;
    }

    config_netem = 1;
    qp.delay = delay;
    qp.loss = lossrate;
    config_bw = 1;
    qp.rate = bandwidth;
    if(INGRESS) {
        devname = ifb_devname;
    }

    dprintf(("[configure_qdisc] change rule interface name : %s\n", devname));
    if(config_netem) {
        qp.limit = 100000;
        change_netem_qdisc(devname, netem_qdisc_id, qp);
    }

    if(INGRESS) {
            devname = ifb_devname;
    }
    if(config_bw) {
        qp.rate = bandwidth;
        qp.buffer = buffer;
        change_htb_class(devname, htb_class_id, 1 * 1000 * 1000 * 1000);
//        change_htb_class(devname, htb_class_id, rate);
    }

    return SUCCESS;
}
#endif

int
configure_rule(dsock, dst, pipe_nr, bandwidth, delay, lossrate)
int dsock;
char* dst;
int32_t pipe_nr;
int32_t bandwidth;
float delay;
double lossrate;
{
#ifdef __FreeBSD
    return configure_pipe(dsock, pipe_nr, bandwidth, delay, lossrate);
#elif __linux
    return configure_qdisc(dst, pipe_nr, bandwidth, delay, lossrate);
#endif
}
