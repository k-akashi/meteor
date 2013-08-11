
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
 *
 ***********************************************************************/

#include <sys/queue.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>

#ifdef __FreeBSD__
#include <netinet/ip_fw.h>
#include <netinet/ip_dummynet.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "global.h"
#include "wireconf.h"
#ifdef __linux
#include "utils.h"
#include "ip_common.h"
#include "tc_common.h"
#include "tc_util.h"
#endif

#define FRAME_LENGTH 1522
// ether header 18 bytes
// VLAN ID 4 bytes
// payload 1500 bytes

/////////////////////////////////////////////
// Special defines
/////////////////////////////////////////////

// this is only used in conjunction with the new
// modified version of dummynet that is still in
// experimental phase
//#define NEW_DUMMYNET #(version 6??)


/////////////////////////////////////////////
// Message-handling functions
/////////////////////////////////////////////

#ifdef MESSAGE_WARNING
#define WARNING(message...) do {                                             \
  fprintf(stdout, "libwireconf WARNING: %s, line %d: ", __FILE__, __LINE__); \
  fprintf(stdout, message); fprintf(stdout,"\n");                            \
} while(0)
#else
#define WARNING(message...) /* message */
#endif

#ifdef MESSAGE_DEBUG
#define DEBUG(message...) do {                                             \
  fprintf(stdout, "libwireconf DEBUG: %s, line %d: ", __FILE__, __LINE__); \
  fprintf(stdout, message); fprintf(stdout,"\n");                          \
} while(0)
#else
#define DEBUG(message...) /* message */
#endif

#ifdef MESSAGE_INFO
#define INFO(message...) do {                                             \
  fprintf(stdout, "libwireconf INFO: %s, line %d: ", __FILE__, __LINE__); \
  fprintf(stdout, message); fprintf(stdout,"\n");                         \
} while(0)
#else
#define INFO(message...) /* message */
#endif

#ifdef __linux

#ifndef TBF
#define TBF 0
#endif // TBF

#define INGRESS 0 // 1 : ingress mode, 0 : egress mode
#ifdef INGRESS
#define MAX_IFB 1
char* ifb_device[MAX_IFB];
#endif // INGRESS 

#define MAX_DEV 16
#define TEST 1
#ifdef TEST
char* device_list;
#else
int dev_no = 0;
struct DEVICE_LIST {
    char dst_address[20];
    char device[20];
} device_list[MAX_DEV];
#endif // TEST
#endif // __linux

//static struct 
//{
//  char netem_child[20];
//  char tbf_child[20];
//}rootid;

///////////////////////////////////////////////
// IPv4 address data structure and manipulation
///////////////////////////////////////////////

// IPv4 address structure
typedef union {
    uint8_t octet[4];
    uint32_t word;
} in4_addr;

#ifdef __linux
int priv_delay = 0;
double priv_loss = 0;
int priv_rate = 0;
#endif
// convert a string containing an IPv4 address 
// to an IPv4 data structure;
// return SUCCESS on success, ERROR on error
#ifdef __FreeBSD__
static int 
atoaddr(array_address, ipv4_address, port)
char *array_address;
in4_addr *ipv4_address;
uint16_t *port;
{
    uint32_t c, o;
  
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


/////////////////////////////////////////////
// Socket manipulation functions
/////////////////////////////////////////////

// get a socket identifier;
// return socket identifier on succes, ERROR on error
int
get_socket(void)
{
    uint32_t socket_id;
  
    if((socket_id = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
        WARNING("Error creating socket");
        perror("socket");

        return ERROR;
    }

    return socket_id;
}

// close socket specified by socket_id
void
close_socket(socket_id)
int socket_id;
{
  close(socket_id);
}

// apply socket options related to ipfw commands
#ifdef __FreeBSD__
static int
apply_socket_options(s, option_id, option, socklen_t, option_length)
int s;
int option_id;
void *option;
socklen_t option_length;
{
    // choose the action corresponding to the given command
    switch(option_id) {
        // add a rule to the firewall (the rule contains dummynet pipes);
        // option should be of type "struct ip_fw*"
        case IP_FW_ADD:
#ifdef MESSAGE_DEBUG
            DEBUG("Add ipfw rule: "); 
            print_rule((struct ip_fw*)option);
#endif

            if(getsockopt(s, IPPROTO_IP, option_id, option, &option_length) < 0) {
                WARNING("Error getting socket options");
                perror("getsockopt");
    
                return ERROR;
            }
            break;

        // delete a firewall rule (the rule contains dummynet pipes);
        // option should be of type "u_int32_t *"
        case IP_FW_DEL:
            DEBUG("Delete ipfw rule #%d", (*(u_int32_t *)option));

            if(setsockopt(s, IPPROTO_IP, option_id, option, option_length) < 0) {
                WARNING("Error setting socket options");
                perror("setsockopt");

                return ERROR;
            }
            break;

        // configure a dummynet pipe;
        // option should be of type "dn_pipe *"
        case IP_DUMMYNET_CONFIGURE:
#ifdef MESSAGE_DEBUG
            DEBUG("Configure ipfw dummynet pipe: "); 
            print_pipe((struct dn_pipe*)option);
#endif

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


////////////////////////////////////////////////
// Functions implemented by the wireconf library
////////////////////////////////////////////////

// get a rule
// NOT WORKING PROPERLY YET!!!!!!!!!!!!!
int
get_rule(s, rulenum)
uint s;
int16_t rulenum;
{
#ifdef __FreeBSD__
    //int RULES=100;
    int i = 0;
    struct ip_fw rules[10];

    socklen_t rules_size;

    bzero(&rules, sizeof(rules));
    rules_size=sizeof(rules);
    //rule.rulenum = rulenum;

    //  printf("Get rule: "); print_rule(&rule);
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

// add an ipfw rule containing a dummynet pipe to the firewall
// return SUCCESS on succes, ERROR on error
#ifdef __FreeBSD__
int
add_rule(s, rulenum, pipe_nr, src, dst, direction)
int s;
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

    // additional rule length counter
    clen = 0;

    // process source address

    if(strncmp(src, "any", strlen(src))==0) {
        // source address was "any"
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

    // process destination address
    if(strncmp(dst, "any", strlen(dst)) == 0) {
        // destination address was "any"
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

    // use in/out direction indicators
    if(direction != DIRECTION_BOTH) {
        // basic command code for in/out operation
        cmd[clen].opcode = O_IN; 
        cmd[clen].len = 1;

        // a negation mask is used for meaning "out"
        if(direction == DIRECTION_OUT) {
            cmd[clen].len |= F_NOT;
        }
        //printf("len=0x%x len&F_NOT=0x%x flen=%d\n", cmd[clen].len, cmd[clen].len & F_NOT, F_LEN(cmd+clen));
        clen += 1;
    }

    // configure pipe
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

    // apply appropriate socket options
    if(apply_socket_options(s, IP_FW_ADD, p, sizeof(struct ip_fw) + clen * 4) < 0) {
        WARNING("Adding rule operation failed");
        return ERROR;
    }
    return 0;
}
#elif __linux
int
add_rule(s, rulenum, handle_nr, src, dst, direction)
int s;
uint16_t rulenum;
int handle_nr;
char *src;
char *dst;
int direction;
{
    int i;
    int32_t find_ifb_device = 0;
    struct qdisc_parameter qp;
    char* device_name =  (char* )get_route_info("dev", dst);
    char handleid[10];

#ifdef TEST
    device_list = device_name;
#else
    device_list[dev_no].dst_address = dst;
    device_list[dev_no].device = device_name;
    dev_no++;
#endif

    // Qdisc Parameter set
    memset(&qp, 0, sizeof(qp));

    sprintf(handleid, "%d", handle_nr);
    dprintf(("[add_rule] rulenum = %s\n", handleid));

    // initialize Qdisc Parameter
    qp.limit = "100000";
    qp.delay = "1us";
    qp.jitter = "0";
    qp.delay_corr = "0";
    qp.loss = "0";
    qp.loss_corr = "0";
    qp.reorder_prob = "0";
    qp.reorder_corr = "0";
    qp.rate = "1Gbit";
    qp.buffer = "1Mbit";

    char netem_parent_id[10];
    char netem_handle_id[10];

    char htb_qdisc_parent_id[10];
    char htb_qdisc_handle_id[10];

    char htb_class_1_parent_id[10];
    char htb_class_1_handle_id[10];

    char htb_class_2_parent_id[10];
    char htb_class_2_handle_id[10];

#ifdef PFIFO
    char pfifo_handle_id[10];
    char pfifo_parent_id[10];
#endif

    char filter_parent_id[10];
    char filter_handle_id[10];
    char dstaddr[20];

    sprintf(htb_qdisc_parent_id, "root");
    sprintf(htb_qdisc_handle_id, "%d:", handle_nr);

    sprintf(htb_class_1_parent_id, "%d:",  handle_nr);
    sprintf(htb_class_1_handle_id, "%d:1", handle_nr);

    sprintf(htb_class_2_parent_id, "%d:",   handle_nr);
    sprintf(htb_class_2_handle_id, "%d:11", handle_nr);

    sprintf(netem_parent_id, "%d:11", handle_nr);
    sprintf(netem_handle_id, "%d:", handle_nr + 1000);

    sprintf(filter_parent_id, "%d:", handle_nr);
    sprintf(filter_handle_id, "%d:11", handle_nr);

/* master branch rule
    sprintf(netemid, "%d", handle_nr + 1000);
    sprintf(parent_netemid, "%d:1", handle_nr);

    if(!TBF) {
        sprintf(pfifoid, "%d", handle_nr + 2000);
        sprintf(parent_pfifoid, "%s:1", netemid);
    }
    else {
        sprintf(bwid, "%d", handle_nr + 2000);
        sprintf(parent_bwid, "%s:1", netemid);
        sprintf(pfifoid, "%d", handle_nr + 3000);
        sprintf(parent_pfifoid, "%s:1", bwid);
    }
*/

    sprintf(dstaddr, "%s/32", dst);

    dprintf(("[add_rule] filter dstination address : %s\n", dstaddr));

    struct u32_parameter ufp;

    memset(&ufp, 0, sizeof(struct u32_parameter));

    ufp.match.type = "u32";
    ufp.match.protocol = "ip";
    ufp.match.filter = "dst";
    ufp.match.arg = dstaddr;
    ufp.offset = NULL;
    ufp.hashkey = NULL;
    ufp.classid = "200:11";
    //ufp.classid = parent_netemid;
    ufp.divisor = NULL;
    ufp.order = NULL;
    ufp.link = NULL;
    ufp.ht = NULL;
    ufp.indev = NULL;
    ufp.action = NULL;
    ufp.police = NULL;
    ufp.rdev = NULL;

    // configure netem egress filter
    if(!INGRESS) {
//      dprintf(("\n\n[add_rule] add tbf qdisc\n"));
//      add_tbf_qdisc(device_name, "root", handleid, qp);

        dprintf(("\n\n[add_rule] add htb qdisc\n"));
        add_htb_qdisc(device_name, htb_qdisc_parent_id, htb_qdisc_handle_id);
        dprintf(("\n\n[add_rule] add htb class\n"));
        add_htb_class(device_name, htb_class_1_parent_id, htb_class_1_handle_id, "10000000");
        dprintf(("\n\n[add_rule] add htb class\n"));
        add_htb_class(device_name, htb_class_2_parent_id, htb_class_2_handle_id, "10000000");
        dprintf(("\n\n[add_rule] add netem qdisc\n"));
        add_netem_qdisc(device_name, netem_parent_id, netem_handle_id, qp);
        tc_filter_modify(RTM_NEWTFILTER, NLM_F_EXCL|NLM_F_CREATE, device_name, filter_parent_id, filter_handle_id, "ip", "u32", &ufp);
/*
        tc_cmd(RTM_NEWQDISC, NLM_F_EXCL|NLM_F_CREATE, device_name, handleid, "root", qp, "prio");
        tc_cmd(RTM_NEWQDISC, NLM_F_EXCL|NLM_F_CREATE, device_name, netemid, parent_netemid, qp, "netem");
        if(!TBF) {
            tc_cmd(RTM_NEWQDISC, NLM_F_EXCL|NLM_F_CREATE, device_name, pfifoid, parent_pfifoid, qp, "pfifo");
        }
        else {
            tc_cmd(RTM_NEWQDISC, NLM_F_EXCL|NLM_F_CREATE, device_name, bwid, parent_bwid, qp, "tbf");
            tc_cmd(RTM_NEWQDISC, NLM_F_EXCL|NLM_F_CREATE, device_name, pfifoid, parent_pfifoid, qp, "pfifo");
        }
        tc_filter_modify(RTM_NEWTFILTER, NLM_F_EXCL|NLM_F_CREATE, device_name, parent_filterid, parent_netemid, "ip", "u32", &ufp);
*/
    }

    // ingress filter
    if(INGRESS) {
        tc_cmd(RTM_NEWQDISC, NLM_F_EXCL|NLM_F_CREATE, device_name, handleid, "ingress", qp, "ingress");
        char ifb_device_name[5];
        for(i = 0; i <= MAX_IFB; i++) {
            if(!ifb_device[i]) {
                ifb_device[i] = device_name;
                find_ifb_device = 1;
                sprintf(ifb_device_name, "ifb%d", i);
                break;
            }
        }
        if(!find_ifb_device) {
            fprintf(stderr, "Cannot file ifb device\n");
            exit(1);
        }
        // filter-rule parameter
        struct u32_parameter* up;
        up->match.type = "u32";
        up->offset = NULL;
        up->hashkey = NULL;
        up->classid = "1:1";
        up->divisor = NULL;
        up->order = NULL;
        up->link = NULL;
        up->ht = NULL;
        up->indev = NULL;
        up->action = "mirred";
        up->police = NULL;
        up->rdev = ifb_device_name;
//        tc_filter_modify(RTM_NEWTFILTER, NLM_F_EXCL|NLM_F_CREATE, device_name, "ffff:", NULL, "ip", "u32", up);
//        tc_cmd(RTM_NEWQDISC, NLM_F_EXCL|NLM_F_CREATE, ifb_device_name, handleid, "root", qp, "netem");
//        tc_cmd(RTM_NEWQDISC, NLM_F_EXCL|NLM_F_CREATE, ifb_device_name, bwid, parent_netemid, qp, "tbf");
//        tc_cmd(RTM_NEWQDISC, NLM_F_EXCL|NLM_F_CREATE, ifb_device_name, pfifoid, parent_bwid, qp, "pfifo");
//        tc_filter_modify(RTM_NEWTFILTER, NLM_F_EXCL|NLM_F_CREATE, device_name, parent_filterid, "NULL", "ip", "u32", &ufp);
    }

    return 0;
}
#endif

// delete an ipfw rule;
// return SUCCESS on succes, ERROR on error
#ifdef __FreeBSD__
int
delete_rule(s, rule_number)
uint32_t s;
uint32_t rule_number;
{
    // Note: rule number is of type u_int16_t in ip_fw.h,
    // but a comment in ip_fw2.c shows that the expected size
    // when applying socket options is u_int32_t (see comment below)

    /* COMMENT FROM IP_FW2.C
     *
     * IP_FW_DEL is used for deleting single rules or sets,
     * and (ab)used to atomically manipulate sets. Argument size
     * is used to distinguish between the two:
     *    sizeof(u_int32_t)
     *  delete single rule or set of rules,
     *  or reassign rules (or sets) to a different set.
     *    2*sizeof(u_int32_t)
     *  atomic disable/enable sets.
     *  first u_int32_t contains sets to be disabled,
     *  second u_int32_t contains sets to be enabled.
     */

    // do delete rule
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
    struct qdisc_parameter qp; // 1 : ingress mode, 0 : egress mode
    char* device_name;

    memset(&qp, 0, sizeof(qp));

    device_name = (char* )get_route_info("dev", dst);
    /*
    qp.limit = "1000";
    qp.delay = "1us";
    qp.jitter = "0";
    qp.delay_corr = "0";
    qp.loss = "0";
    qp.loss_corr = "0";
    qp.reorder_prob = "0";
    qp.reorder_corr = "0";
    qp.rate = "1Gbit";
    qp.buffer = "1Mbit";
    */

    //system("tc qdisc del dev lo root");
    if(!INGRESS) {
//        tc_cmd(RTM_DELQDISC, 0, device_name, "1", "root", qp, "pfifo");
        delete_netem_qdisc(device_name);
    }

    if(INGRESS) {
        int i;
        int ret;
        char *cmd;
        char ifb_device_name[5];
        for(i = 0; i <= MAX_IFB; i++) {
            if(strcmp(ifb_device[i], device_name) == 0) {
                sprintf(ifb_device_name, "ifb%d", i);
                break;
            }
        }
        tc_cmd(RTM_DELQDISC, 0, ifb_device_name, "1", "root", qp, "pfifo");
        //tc_cmd(RTM_DELQDISC, 0, device_name, "1", "root", qp, "ingress");
        //sprintf(cmd, "tc qdisc del dev %s root", ifb_device_name);
        //system(cmd);
        sprintf(cmd, "tc qdisc del dev %s ingress", device_name);
        ret = system(cmd);
    }
    //tc_cmd(RTM_DELQDISC, 0, (char* )get_route_info("dev", dst), "1", "0", qp, "netem");
    //tc_cmd(RTM_DELQDISC, 0, (char* )get_route_info("dev", "127.0.0.1"), "1", "0", qp, "netem");
#endif

    return SUCCESS;
}

// print a rule structure
#ifdef __FreeBSD__
void
print_rule(rule)
struct ip_fw *rule;
{
    printf("Rule #%d (size=%d):\n", rule->rulenum, sizeof(*rule));
    printf("\tnext=%p next_rule=%p\n", rule->next, rule->next_rule);
    printf("\tact_ofs=%u cmd_len=%u rulenum=%u set=%u _pad=%u\n", 
            rule->act_ofs, rule->cmd_len, rule->rulenum, rule->set, rule->_pad);
    printf("\tpcnt=%llu bcnt=%llu timestamp=%u\n", rule->pcnt, rule->bcnt, 
            rule->timestamp);
}
#endif

// print a pipe structure
#ifdef __FreeBSD__
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

// configure a dummynet pipe;
// return SUCCESS on succes, ERROR on error
#ifdef __FreeBSD__
int
configure_pipe(s, pipe_nr, bandwidth, delay, lossrate)
int s;
int pipe_nr;
int bandwidth;
int delay;
int lossrate;
{
    struct dn_pipe p;

    // reset data structure  
    bzero(&p, sizeof(p));

    // initialize appropriate fields
    p.pipe_nr = pipe_nr;
    p.fs.plr = lossrate;
    p.bandwidth = bandwidth;
    p.delay = delay;

    // set queue size to a small value to avoid large delays 
    // in case bandwidth limitation is enforced
    p.fs.qsize = 2;

    if(apply_socket_options(s, IP_DUMMYNET_CONFIGURE, &p, sizeof(p)) < 0) {
        WARNING("Pipe configuration could not be applied");
        return ERROR;
    }
    return SUCCESS;
}
#elif __linux
int
convert_netemid(handle)
int handle;
{
    int id;

    id = handle;
    id <<= 16;
    id |= handle;

    return id;
}

int
configure_qdisc(s, dst, handle, bandwidth, delay, lossrate)
int s;
char* dst;
int handle;
int bandwidth;
int delay;
double lossrate;
{
    struct qdisc_parameter qp;
    //char* device_name =  (char* )get_route_info("dev", dst);
    char* device_name;
    int config_netem = 0;
    int config_tbf = 0;
    char delaystr[20];
    char loss[20];
    char handleid[10];
    char rate[20];
    char buffer[20];

    memset(&qp, 0, sizeof(qp));

#ifdef TEST
    device_name = device_list;
#else
    int i;
    for(i = 0; i < dev_no; i++) {
        if(strcmp(device_list[dev_no].dst_address, dst) == 0) {
            device_name = device_list[dev_no].device;
            break;
        }
    }
#endif

    /*
    int netemid;
    netemid = convert_netemid(handle);
    netemid |= handle;
    int parent;
    parent = netemid;
    parent >>= 16;
    int child;
    child = netemid;
    child <<= 16;
    child >>= 16;
    dprintf(("[configure_qdisc] netem id = %d\n", netemid));
    dprintf(("[configure_qdisc] parent id = %d\n", parent));
    dprintf(("[configure_qdisc] child id = %d\n", child));
    dprintf(("[configure_qdisc] rulenum = %d\n", handle));
    */

    sprintf(handleid, "%d", handle);

    char htb_class_2_parent_id[10];
    char htb_class_2_handle_id[10];
    char netem_parent_id[10];
    char netem_handle_id[10];

    sprintf(htb_class_2_parent_id, "%d:",   handle);
    sprintf(htb_class_2_handle_id, "%d:11", handle);
    sprintf(netem_parent_id, "%d:11", handle);
    sprintf(netem_handle_id, "%d:", handle + 1000);

    // tmp
    char netemid[10];
    char parent_netemid[10];
    char bwid[10];
    char parent_bwid[10];
    sprintf(netemid, "%d", handle + 1000);
    sprintf(parent_netemid, "%d:1", handle);
    sprintf(bwid, "%d", handle + 2000);
    sprintf(parent_bwid, "%s:1", netemid);
    //

    if(priv_delay != delay || priv_loss != lossrate) {
         sprintf(delaystr, "%dms", delay);
         qp.delay = delaystr;
         priv_delay = delay;
         config_netem = 1;
         sprintf(loss, "%f", lossrate);
         qp.loss = loss;
         priv_loss = lossrate;
    }

    if(priv_rate != bandwidth) {
        sprintf(rate, "%d", bandwidth);
        if((bandwidth / 1024) < FRAME_LENGTH) {
            sprintf(buffer, "%d", FRAME_LENGTH);
        }
        else {
            sprintf(buffer, "%d", bandwidth / 1024);
        }
        priv_rate = bandwidth;
        config_tbf = 1;
    }

    if(INGRESS) {
        int i;
        for(i = 0; i <= MAX_IFB; i++) {
            if(strcmp(ifb_device[i], device_name) == 0) {
                sprintf(device_name, "ifb%d", i);
                break;
            }
        }
    }

    if(config_netem) {
        qp.limit = "100000";
        qp.jitter = "0";
        qp.delay_corr = "0";
        qp.loss_corr = "0";
        qp.reorder_prob = "0";
        qp.reorder_corr = "0";

        //tc_cmd(RTM_NEWQDISC, 0, device_name, "1200:", "200:1", qp, "netem");
        //tc_cmd(RTM_NEWQDISC, 0, device_name, netemid, parent_netemid, qp, "netem");
        change_netem_qdisc(device_name, netem_parent_id, netem_handle_id, qp);
    }

    if(TBF) {
        if(INGRESS) {
            int i;
            for(i = 0; i <= MAX_IFB; i++) {
                if(strcmp(ifb_device[i], device_name) == 0) {
                    sprintf(device_name, "ifb%d", i);
                    break;
                }
            }
        }
        if(config_tbf) {
            qp.rate = rate;
            qp.buffer = buffer;
//            tc_cmd(RTM_NEWQDISC, 0, device_name, "200:", "root", qp, "tbf");
//            tc_cmd(RTM_NEWQDISC, 0, device_name, bwid, parent_bwid, qp, "tbf");
            change_htb_class(device_name, htb_class_2_parent_id, htb_class_2_handle_id, rate);
//          change_tbf_qdisc(device_name, "root", "200:", qp);
        }
    }

    return SUCCESS;
}
#endif
