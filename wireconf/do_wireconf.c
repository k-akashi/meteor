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
 * File name: do_wireconf.c
 * Function: Main source file of the real-time wired-network emulator 
 *           configuration program. At the moment it can be used to 
 *           drive the netwrok emulator dummynet on FreeBSD 
 *
 * Authors: Junya Nakata, Lan Nguyen Tien, Razvan Beuran
 * Changes : Kunio AKASHI
 *
 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>

#include "global.h"
#include "wireconf.h"
#include "timer.h"
#include "message.h"
#include "routing_info.h"

#ifdef __linux
#include "tc_util.h"
#include <sched.h>
#endif

//#define OLSR_ROUTING

#define BIN_SC 1
#define TXT_SC 2

#define PARAMETERS_TOTAL        19
#define PARAMETERS_UNUSED       13

#define MIN_PIPE_ID_IN          10000
#define MIN_PIPE_ID_IN_BCAST	20000
#define MIN_PIPE_ID_OUT         30000

#define MAX_RULE_NUM            100

#define TCHK_START(name)           \
struct timeval name##_prev;        \
struct timeval name##_current;     \
gettimeofday(&name##_prev, NULL)

#define TCHK_END(name)                                                             \
    gettimeofday(&name##_current, NULL);                                           \
time_t name##_sec;                                                                 \
suseconds_t name##_usec;                                                           \
if(name##_current.tv_sec == name##_prev.tv_sec) {                                  \
    name##_sec = name##_current.tv_sec - name##_prev.tv_sec;                       \
    name##_usec = name##_current.tv_usec - name##_prev.tv_usec;                    \
}                                                                                  \
else if(name ##_current.tv_sec != name##_prev.tv_sec) {                            \
    int name##_carry = 1000000;                                                    \
    name##_sec = name##_current.tv_sec - name##_prev.tv_sec;                       \
    if(name##_prev.tv_usec > name##_current.tv_usec) {                             \
        name##_usec = name##_carry - name##_prev.tv_usec + name##_current.tv_usec; \
        name##_sec--;                                                              \
        if(name##_usec > name##_carry) {                                           \
            name##_usec = name##_usec - name##_carry;                              \
            name##_sec++;                                                          \
        }                                                                          \
    }                                                                              \
    else {                                                                         \
        name##_usec = name##_current.tv_usec - name##_prev.tv_usec;                \
    }                                                                              \
}                                                                                  \
printf("%s: sec:%lu usec:%06ld\n", #name, name##_sec, name##_usec);

typedef struct {
    float time;
    int32_t next_hop_id;
    float bandwidth;
    float delay;
    float lossrate;
} qomet_param;

void
usage(argv0)
char *argv0;
{
    fprintf(stderr, "\ndo_wireconf. Drive network emulation using QOMET  data.\n\n");
    fprintf(stderr, "do_wireconf can use the QOMET data in <qomet_output_file> in two different ways:\n");
    fprintf(stderr, "(1) Configure a specified pair <from_node_id> (IP address <from_node_addr>)\n");
    fprintf(stderr, "    <to_node_id> (IP address <to_node_addr>) using the dummynet rule\n");
    fprintf(stderr, "    <rule_number> and pipe <pipe_number>, optionally in direction 'in' or 'out'.\n");
    fprintf(stderr, "    Usage: %s -q <qomet_output_file>\n"
            "\t\t\t-f <from_node_id> \t-F <from_node_addr>\n"
            "\t\t\t-t <to_node_id> \t-T <to_node_addr>\n"
            "\t\t\t-r <rule_number> \t-p <pipe_number>\n"
            "\t\t\t[-d {in|out}]\n", argv0);
    fprintf(stderr, "(2) Configure all connections from the current node <current_id> given the\n");
    fprintf(stderr, "    IP settings in <settings_file> and the OPTIONAL broadcast address\n");
    fprintf(stderr, "    <broadcast_address>; Interval between configurations is <time_period>.\n");
    fprintf(stderr, "    The settings file contains on each line pairs of ids and IP addresses.\n");
    fprintf(stderr, "    Usage: %s -q <qomet_output_file>\n"
            "\t\t\t-i <current_id> \t-s <settings_file>\n"
            "\t\t\t-m <time_period> \t[-b <broadcast_address>]\n", argv0);
    fprintf(stderr, "NOTE: If option '-s' is used, usage (2) is inferred, otherwise usage (1) is assumed.\n");
}

int
read_settings(path, p, p_size)
char *path;
in_addr_t *p;
int p_size;
{
    static char buf[BUFSIZ];
    int i = 0;
    int line_nr = 0;
    FILE *fd;

    char node_name[20];
    char interface[20];
    int node_id;
    char node_ip[IP_ADDR_SIZE];

    if((fd = fopen(path, "r")) == NULL) {
        WARNING("Cannot open settings file '%s'", path);
        return -1;
    }

    while(fgets(buf, BUFSIZ, fd) != NULL) {
        line_nr++;

        if(i >= p_size) {
            WARNING("Maximum number of IP addresses (%d) exceeded", p_size);
            fclose(fd);
            return -1;
        }
        else {
            int scaned_items;
            scaned_items = sscanf(buf, "%s %s %d %16s",node_name, interface,  &node_id, node_ip);
            if(scaned_items < 2) {
                WARNING("Skipped invalid line #%d in settings file '%s'", line_nr, path);
                continue;
            }
            if(node_id < 0 || node_id < FIRST_NODE_ID || node_id >= MAX_NODES) {
                WARNING("Node id %d is not within the permitted range [%d, %d]", node_id, FIRST_NODE_ID, MAX_NODES);
                fclose(fd);
                return -1;
            }
            if((p[node_id] = inet_addr(node_ip)) != INADDR_NONE) {
                i++;
            }
        }
    }

    fclose(fd);
    return i;
}

int
lookup_param(next_hop_id, p, rule_count)
int next_hop_id;
qomet_param *p;
int rule_count;
{
    int i;

    for(i = 0; i < rule_count; i++) {
        if(p[i].next_hop_id == next_hop_id)
            return i;
    }

    return -1;
}

void
dump_param_table(p, rule_count)
qomet_param *p;
int rule_count;
{
    int i;

    INFO("Dumping the param_table (rule count=%d)...", rule_count);
    for(i = 0; i < rule_count; i++) {
        INFO("%f \t %d \t %f \t %f \t %f", p[i].time, p[i].next_hop_id, p[i].bandwidth, p[i].delay, p[i].lossrate);
    }
}

int
main(argc, argv)
int argc;
char **argv;
{
    char ch;
    char *p;
    char *argv0;
    char *faddr, *taddr;
    char buf[BUFSIZ];
    uint32_t s;
    uint32_t fid, tid;
    uint32_t from, to;
    uint32_t pipe_nr;
    uint16_t rulenum;

    uint32_t sc_type;
    uint32_t usage_type;

    float time, dummy[PARAMETERS_UNUSED], bandwidth, delay, lossrate;
    FILE *qomet_fd;
    timer_handle *timer;
    int32_t loop_count = 0;

    int32_t direction = DIRECTION_BOTH;

    struct timeval tp_begin, tp_end;

    int32_t i, j;
    int32_t my_id;
    in_addr_t IP_addresses[MAX_NODES];
    char IP_char_addresses[MAX_NODES*IP_ADDR_SIZE];
    int32_t node_number;

    int32_t offset_num, rule_count;
    int32_t next_hop_id, rule_num;
    uint32_t over_read;
    float time_period, next_time;
    qomet_param param_table[MAX_RULE_NUM];
    qomet_param param_over_read;
    char broadcast_address[IP_ADDR_SIZE];

    usage_type = 1;

    memset(param_table, 0, sizeof(qomet_param) * MAX_RULE_NUM);
    memset(&param_over_read, 0, sizeof(qomet_param));
    over_read = 0;
    rule_count = 0;
    next_time = 0.0;
    next_hop_id = 0;
    rule_num = -1;

    argv0 = argv[0];
    qomet_fd = NULL;

    faddr = taddr = NULL;
    fid = tid = pipe_nr = -1;
    rulenum = 65535;

    my_id = -1;
    node_number = -1;
    time_period = -1;
    strncpy(broadcast_address, "255.255.255.255", IP_ADDR_SIZE);

/* cpu affinity
#ifdef __linux
    uint32_t core_num;
    core_num = sysconf(_SC_NPROCESSORS_CONF);
    
    if(core_num != 1) {
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(1, &mask);
        sched_setaffinity(0, sizeof(mask), &mask);
    }
#endif
*/

    if(argc < 2) {
        WARNING("No arguments provided");
        usage(argv0);
        exit(1);
    }

    while((ch = getopt(argc, argv, "hq:a:f:F:t:T:r:p:d:i:s:m:b:")) != -1) {
        switch(ch) {
            case 'h':
                usage();
                exit(0);
            case 'a':
                if(sc_type == TXT_SC) {
                    WARNING("Already read scenario data.");
                    exit(1);
                }
                sc_type = BIN_SC;
                if((qomet_fd = fopen(optarg, "rb")) == NULL) {
                    WARNING("Could not open QOMET output file '%s'", optarg);
                    exit(1);
                }
                break;
            case 'q':
                if(sc_type == BIN_SC) {
                    WARNING("Already read scenario data.");
                    exit(1);
                }
                sc_type = TXT_SC;
                printf("sc_type : %d\n", sc_type);
                if((qomet_fd = fopen(optarg, "r")) == NULL) {
                    WARNING("Could not open QOMET output file '%s'", optarg);
                    exit(1);
                }
                break;
            case 'f':
                fid = strtol(optarg, &p, 10);
                if((*optarg == '\0') || (*p != '\0')) {
                    WARNING("Invalid from_node_id '%s'", optarg);
                    exit(1);
                }
                my_id = fid;
                node_number = 2; // add node_number tmp Fix me
                break;
            case 'F':
                faddr = optarg;
                break;
            case 't':
                tid = strtol(optarg, &p, 10);
                if((*optarg == '\0') || (*p != '\0')) {
                    WARNING("Invalid to_node_id '%s'", optarg);
                    exit(1);
                }
                break;
            case 'T':
                taddr = optarg;
                break;
            case 'r':
                rulenum = strtol(optarg, &p, 10);
                if((*optarg == '\0') || (*p != '\0')) {
                    WARNING("Invalid rule_number '%s'", optarg);
                    exit(1);
                }
                break;
            case 'p':
                pipe_nr = strtol(optarg, &p, 10);
                if((*optarg == '\0') || (*p != '\0')) {
                    WARNING("Invalid pipe_number '%s'", optarg);
                    exit(1);
                }
                break;
            case 'd':
                if(strcmp(optarg, "in") == 0) {
                    direction = DIRECTION_IN;
                }
                else if(strcmp(optarg, "out") == 0) {
                    direction = DIRECTION_OUT;
                }
                else {
                    WARNING("Invalid direction '%s'", optarg);
                    exit(1);
                }
                break;
            case 'i':
                my_id = strtol(optarg, NULL, 10);
                break;
            case 's':
                usage_type = 2;
                if((node_number = read_settings(optarg, IP_addresses, MAX_NODES)) < 1) {
                    WARNING("Settings file '%s' is invalid", optarg);
                    exit(1);
                }
                for(i = 0; i < node_number; i++) {
                    snprintf(IP_char_addresses + i * IP_ADDR_SIZE, IP_ADDR_SIZE, 
                            "%hu.%hu.%hu.%hu",
                            *(((uint8_t *)&IP_addresses[i]) + 0),
                            *(((uint8_t *)&IP_addresses[i]) + 1),
                            *(((uint8_t *)&IP_addresses[i]) + 2),
                            *(((uint8_t *)&IP_addresses[i]) + 3));
                }
                break;
            case 'm':
                if((time_period = strtod(optarg, NULL)) == 0) {
                    WARNING("Invalid time period");
                    exit(1);
                }
                break;
            case 'b':
                strncpy(broadcast_address, optarg, IP_ADDR_SIZE);
                break;
            default:
                usage(argv0);
                exit(1);
        }
    }

    if((my_id < FIRST_NODE_ID) || (my_id >= node_number + FIRST_NODE_ID)) {
        WARNING("Invalid ID '%d'. Valid range is [%d, %d]", my_id, FIRST_NODE_ID, node_number+FIRST_NODE_ID - 1);
        exit(1);
    }

    argc -= optind;
    argv += optind;

    if(qomet_fd == NULL) {
        WARNING("No QOMET data file was provided");
        usage(argv0);
        exit(1);
    }

    if((usage_type == 1) && ((fid == -1) || (faddr == NULL) || (tid == -1) || (taddr == NULL) || (pipe_nr == -1) || (rulenum == 65535))) {
        WARNING("Insufficient arguments were provided for usage (1)");
        usage(argv0);
        fclose(qomet_fd);
        exit(1);
    }
/*
    else if ((my_id == -1) || (node_number == -1) || (time_period == -1)) {
        WARNING("Insufficient arguments were provided for usage (2)");
        usage(argv0);
        fclose(qomet_fd);
        exit(1);
    }
*/


    DEBUG("Initialize timer...");
    if((timer = timer_init()) == NULL) {
        WARNING("Could not initialize timer");
        exit(1);
    }
    DEBUG("Open control socket...");
    if((s = get_socket()) < 0) {
        WARNING("Could not open control socket (requires root priviledges)\n");
        exit(1);
    }

#ifdef __linux
    init_rule(taddr);
#endif

    if(usage_type == 1) {
        INFO("Add rule #%d with pipe #%d from %s to %s", rulenum, pipe_nr, faddr, taddr);

        if(add_rule(s, rulenum, pipe_nr, faddr, taddr, direction) < 0) {
            WARNING("Could not add rule #%d with pipe #%d from %s to %s", 
                    rulenum, pipe_nr, faddr, taddr);
            exit(1);
        }
    }
    else {
        for(j = FIRST_NODE_ID; j < node_number + FIRST_NODE_ID; j++) {
            if(j == my_id) {
                continue;
            }
            offset_num = j; 
            INFO("Node %d: Add rule #%d with pipe #%d to destination %s", 
                    my_id, MIN_PIPE_ID_OUT + offset_num, MIN_PIPE_ID_OUT + offset_num, 
                    IP_char_addresses + (j - FIRST_NODE_ID) * IP_ADDR_SIZE);
            if(add_rule(s, MIN_PIPE_ID_OUT + offset_num, MIN_PIPE_ID_OUT + offset_num,
                        "any", IP_char_addresses + (j - FIRST_NODE_ID) * IP_ADDR_SIZE, 
                        DIRECTION_OUT) < 0) {
                WARNING("Node %d: Could not add rule #%d with pipe #%d to \
                        destination %s", my_id, MIN_PIPE_ID_OUT + offset_num, 
                        MIN_PIPE_ID_OUT + offset_num, 
                        IP_char_addresses + (j-FIRST_NODE_ID) * IP_ADDR_SIZE);
                exit(1);
            }
        }
        for(j = FIRST_NODE_ID; j < node_number + FIRST_NODE_ID; j++)
        {
            if(j == my_id) {
                continue;
            }
            offset_num = j; 

            INFO("Node %d: Add rule #%d with pipe #%d to destination %s",
                    my_id, MIN_PIPE_ID_IN_BCAST + offset_num, 
                    MIN_PIPE_ID_IN_BCAST + offset_num, broadcast_address);
            if(add_rule(s, MIN_PIPE_ID_IN_BCAST + offset_num, 
                        MIN_PIPE_ID_IN_BCAST + offset_num, 
                        IP_char_addresses+(j-FIRST_NODE_ID)*IP_ADDR_SIZE,
                        broadcast_address, DIRECTION_IN) < 0)
            {
                WARNING("Node %d: Could not add rule #%d with pipe #%d from %s to \
                        destination %s", my_id, MIN_PIPE_ID_IN_BCAST + offset_num, 
                        MIN_PIPE_ID_IN_BCAST + offset_num, 
                        IP_char_addresses+(j - FIRST_NODE_ID) * IP_ADDR_SIZE, broadcast_address);
                exit(1);
            }
        }
    }
    gettimeofday(&tp_begin, NULL);

    if(usage_type == 1) {
#ifdef OLSR_ROUTING
        if((next_hop_id = get_next_hop_id(tid, direction)) == ERROR) {
            WARNING("Time=%.2f: Couldn't locate the next hop for destination node %i, direction=%i", time, tid, direction);
            exit(1);
        } 
        else {
            INFO("Time=%.2f: Next_hop=%i for destination=%i", time, next_hop_id, tid);
        }
#else
        next_hop_id = tid;
#endif
    }

    INFO("Reading QOMET data from file...");
    next_time = 0;

    if(sc_type == BIN_SC) {
        printf("not implemented. %d\n", sc_type);
    }
    else if(sc_type == TXT_SC) {
        while(fgets(buf, BUFSIZ, qomet_fd) != NULL) {
            if(sscanf(buf, "%f %d %f %f %f %d %f %f %f %f %f %f "
                "%f %f %f %f %f %f %f", &time, &from, &dummy[0],
                &dummy[1], &dummy[2], &to, &dummy[3], &dummy[4],
                &dummy[5],  &dummy[6], &dummy[7], &dummy[8],
                &dummy[9], &dummy[10], &dummy[11], &bandwidth, 
                &lossrate, &delay, &dummy[12]) != PARAMETERS_TOTAL) {
                INFO("Skipped non-parametric line");
                continue;
            }
            if(usage_type == 1) {
                if((from == fid) && (to == next_hop_id)) {
                    INFO("* Wireconf configuration (time=%.2f s): bandwidth=%.2fbit/s loss_rate=%.4f delay=%.4f ms", \
                        time, bandwidth, lossrate, delay);
    
                    if(time == 0.0) {
                        timer_reset(timer);
                    }
                    else {
                        if(timer_wait(timer, time * 1000000) < 0) {
                            WARNING("Timer deadline missed at time=%.2f s", time);
                        }
                    }
    
    
#ifdef OLSR_ROUTING
                    if((next_hop_id = get_next_hop_id(tid, direction)) == ERROR) {
                        WARNING("Time=%.2f: Couldn't locate the next hop for destination node %i,direction=%i", \
                            time, tid, direction);
                        exit(1);
                    }
                    else {
                        INFO("Time=%.2f: Next_hop=%i for destination=%i", time, next_hop_id, tid);
                    }
#endif
    
#ifdef __FreeBSD__
                    bandwidth = (int)round(bandwidth);
                    delay = (int) round(delay);
                    lossrate = (int)round(lossrate * 0x7fffffff);
    
                    configure_pipe(s, pipe_nr, bandwidth, delay, lossrate);
#elif __linux
                    bandwidth = (int)round(bandwidth);
                    //delay = (int)round(delay);
                    lossrate = (int)rint(lossrate * 0x7fffffff);
                    lossrate = 0;
    
    				//TCHK_START(time);
                    configure_qdisc(s, taddr, pipe_nr, bandwidth, delay, lossrate);
    				//TCHK_END(time);
#endif
                    loop_count++;
                }
            }
            else { 
                if(time == next_time) {
                    if(from == my_id) {
                        param_table[rule_count].time = time;
                        param_table[rule_count].next_hop_id = to;
                        param_table[rule_count].bandwidth = bandwidth;
                        param_table[rule_count].delay = delay;
                        param_table[rule_count].lossrate = lossrate;
                        rule_count++;
                    }
                    continue;
                }
                else {
                    if(from == my_id) {
                        over_read = 1;
                        param_over_read.time = time;
                        param_over_read.next_hop_id = to;
                        param_over_read.bandwidth = bandwidth;
                        param_over_read.delay = delay;
                        param_over_read.lossrate = lossrate;
                    }
                    else {
                        over_read = 0;
                    }
    
                    if(time == 0.0) {
                        timer_reset(timer);
                    }
                    else {
                        if(timer_wait(timer, time * 1000000) < 0) {
                            WARNING("Timer deadline missed at time=%.2f s", time);
                        }
                    }
    
                    for(i = FIRST_NODE_ID; i < (node_number+FIRST_NODE_ID); i++) {
                        if(i == my_id) {
                            continue;
                        }
    
                        if((next_hop_id = get_next_hop_id(IP_addresses, IP_char_addresses, i, DIRECTION_OUT)) == ERROR) {
                            WARNING("Could not locate the next hop for destination node %i", i);
                            exit(1);
                        }
    
                        offset_num = i;
                        if((rule_num = lookup_param(next_hop_id, param_table, rule_count)) == -1) {
                            WARNING("Could not locate the rule number for next hop = %i", 
                                    next_hop_id);
                            dump_param_table(param_table, rule_count);
                            exit(1);
                        }
    
                        time = param_table[rule_num].time;
                        bandwidth = param_table[rule_num].bandwidth;
                        delay = param_table[rule_num].delay;
                        lossrate = param_table[rule_num].lossrate;
    
                        INFO("* Wireconf: #%d UCAST to #%d [%s] (time=%.2f s): bandwidth=%.2fbit/s \
                                loss_rate=%.4f delay=%.4f ms, offset=%d", \
                                my_id, i, IP_char_addresses + (i - FIRST_NODE_ID) * IP_ADDR_SIZE, 
                                time, bandwidth, lossrate, delay, offset_num);
    
#ifdef __FreeBSD__
                        bandwidth = (int)round(bandwidth);// * 2.56);
                        delay = (int) round(delay);//(delay / 2);
                        lossrate = (int)round(lossrate * 0x7fffffff);
    
                        configure_pipe(s, MIN_PIPE_ID_OUT + offset_num, bandwidth, delay, lossrate);
#elif __linux
                        bandwidth = (int)rint(bandwidth);// * 2.56);
                        delay = (int) rint(delay);//(delay / 2);
                        lossrate = (int)rint(lossrate * 0x7fffffff);

                        configure_qdisc(s, taddr, MIN_PIPE_ID_OUT + offset_num, bandwidth, delay, lossrate);
#endif
                        loop_count++;
                    }
    
                    for(i = FIRST_NODE_ID; i < (node_number+FIRST_NODE_ID); i++) {
                        if(i == my_id)
                            continue;
    
                        offset_num = i;
                        if((rule_num = lookup_param(i, param_table, rule_count)) == -1) {
                            WARNING("Could not locate the rule number for next hop = %d with rule count = %d", 
                                    i, rule_count);
                            dump_param_table(param_table, rule_count);
                            exit(1);
                        }
    
                        time = param_table[rule_num].time;
                        bandwidth = param_table[rule_num].bandwidth;
                        delay = param_table[rule_num].delay;
                        lossrate = param_table[rule_num].lossrate;
    
                        INFO("* Wireconf: #%d BCAST from #%d [%s] (time=%.2f s): bandwidth=%.2fbit/s \
                                loss_rate=%.4f delay=%.4f ms, offset=%d", my_id, i, \
                                IP_char_addresses+(i-FIRST_NODE_ID)*IP_ADDR_SIZE, 
                                time, bandwidth, lossrate, delay, offset_num);
#ifdef __FreeBSD__
                        bandwidth = (int)round(bandwidth);// * 2.56);
                        delay = (int) round(delay);//(delay / 2);
                        lossrate = (int)round(lossrate * 0x7fffffff);
    
                        configure_pipe(s, MIN_PIPE_ID_IN_BCAST + offset_num, bandwidth, delay, lossrate);
#elif __linux
                        bandwidth = (int)rint(bandwidth);// * 2.56);
                        delay = (int)rint(delay);//(delay / 2);
                        lossrate = (int)rint(lossrate * 0x7fffffff);

                        configure_qdisc(s, taddr, MIN_PIPE_ID_IN_BCAST + offset_num, bandwidth, delay, lossrate);
#endif
                        loop_count++;
                    }
    
                    memset(param_table, 0, sizeof(qomet_param) * MAX_RULE_NUM);
                    rule_count = 0;
                    next_time += time_period;
                    INFO("New timer deadline=%f", next_time);

                    if(over_read == 1) {
                        param_table[0].time = param_over_read.time;
                        param_table[0].next_hop_id = param_over_read.next_hop_id;
                        param_table[0].bandwidth = param_over_read.bandwidth;
                        param_table[0].delay = param_over_read.delay;
                        param_table[0].lossrate = param_over_read.lossrate;
                        rule_count++;
                    }
                    memset(&param_over_read, 0, sizeof(qomet_param));
                    over_read = 0;
                }
            }
        } 
    }

    if(loop_count == 0) {
        if(usage_type == 1) {
            WARNING("The specified pair from_node_id=%d & to_node_id=%d could not be found", fid, tid);
        }
        else {
            WARNING("No valid line was found for the node %d", my_id); 
        }
    }
    timer_free(timer);

    if(usage_type==1) {
#ifdef __FreeBSD__
        if(delete_rule(s, rulenum) == ERROR) {
            WARNING("Could not delete rule #%d", rulenum);
            exit(1);
        }
#elif __linux
        if(delete_netem(s, taddr, rulenum) == ERROR) {
            WARNING("Could not delete rule #%d", rulenum);
            exit(1);
        }
#endif
    }
    else {
#ifdef __FreeBSD__
        for (j = FIRST_NODE_ID; j < node_number + FIRST_NODE_ID; j++) {
            if(j == my_id) {
                continue;
            }

            offset_num = j;
            INFO("Deleting rule %d...", MIN_PIPE_ID_OUT + offset_num);
            if(delete_rule(s, MIN_PIPE_ID_OUT+offset_num) == ERROR) {
                WARNING("Could not delete rule #%d", MIN_PIPE_ID_OUT + offset_num);
            }
        }

        //delete broadcast dummynet rules
        for (j = FIRST_NODE_ID; j < node_number + FIRST_NODE_ID; j++) {
            if(j == my_id) {
                continue;
            }

            offset_num = j;
            INFO("Deleting rule %d...", MIN_PIPE_ID_IN_BCAST + offset_num);
            if(delete_rule(s, MIN_PIPE_ID_IN_BCAST+offset_num) == ERROR) {
                WARNING("Could not delete rule #%d", MIN_PIPE_ID_IN_BCAST+offset_num);
            }
        }
#elif __linux
        if(delete_netem(s, taddr, rulenum) == ERROR) {
            WARNING("Could not delete netem rule");
        }
#endif
    }

    gettimeofday(&tp_end, NULL);
    INFO("Experiment execution time=%.4f s", \
        (tp_end.tv_sec+tp_end.tv_usec / 1.0e6) - (tp_begin.tv_sec + tp_begin.tv_usec / 1.0e6));
    DEBUG("Closing socket...");

    close_socket(s);
    fclose(qomet_fd);

    return 0;
}

