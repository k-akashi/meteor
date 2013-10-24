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
#include "message.h"
#include "routing_info.h"
#include "statistics.h"
#include "timer.h"

#ifdef __linux
#include "tc_util.h"
#include <sched.h>
#endif

#define BIN_SC 1
#define TXT_SC 2

#define PARAMETERS_TOTAL        19
#define PARAMETERS_UNUSED       13

#define MIN_PIPE_ID_IN          10000
#define MIN_PIPE_ID_IN_BCAST	20000
#define MIN_PIPE_ID_OUT         30000

#define MAX_RULE_NUM            100
#define MAX_RULE_COUNT          500

#define UNDEFINED_SIGNED        -1
#define UNDEFINED_UNSIGNED      65535
#define UNDEFINED_BANDWIDTH     -1.0
#define DEFAULT_FRAME_SIZE      1500
#define SCALING_FACTOR          1.0

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
    double bandwidth;
    double delay;
    double lossrate;
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
    fprintf(stderr, "    <baddr>; Interval between configurations is <time_period>.\n");
    fprintf(stderr, "    The settings file contains on each line pairs of ids and IP addresses.\n");
    fprintf(stderr, "    Usage: %s -q <qomet_output_file>\n"
            "\t\t\t-i <current_id> \t-s <settings_file>\n"
            "\t\t\t-m <time_period> \t[-b <baddr>]\n", argv0);
    fprintf(stderr, "NOTE: If option '-s' is used, usage (2) is inferred, otherwise usage (1) is assumed.\n");
}

#ifdef __amd64__
#define rdtsc(t)                                                              \
    __asm__ __volatile__ ("rdtsc; movq %%rdx, %0; salq $32, %0;orq %%rax, %0" \
    : "=r"(t) : : "%rax", "%rdx"); 
#else
#define rdtsc(t) asm volatile("rdtsc" : "=A" (t))
#endif

//typedef struct
//{
//    uint32_t cpu_frequency;
//    uint64_t zero;
//    uint64_t next_event;
//} timer_handle;

int
timer_wait_rdtsc(handle, time_in_us)
struct timer_handle* handle;
uint64_t time_in_us;
{
    uint64_t crt_time;

    dprintf(("[timer_wait] handle->zero \t\t: %lu\n", handle->zero));
    dprintf(("[timer_wait] handle->cpu_frequency \t: %u\n", handle->cpu_frequency));
    dprintf(("[timer_wait] time_in_us \t\t: %lu\n", time_in_us));

    handle->next_event = handle->zero + (handle->cpu_frequency * time_in_us) / 1000000;

    rdtsc(crt_time);

    dprintf(("[timer_wait] handle->next_event \t: %lu\n", handle->next_event));
    dprintf(("[timer_wait] crt_time \t\t\t: %lu\n", crt_time));

    if(handle->next_event < crt_time) {
        return ERROR;
    }

    do {
        usleep(1);
        rdtsc(crt_time);
    } while(handle->next_event >= crt_time);

    return SUCCESS;
}

uint32_t
get_cpu_frequency(void)
{
    uint32_t hz = 0;

#ifdef __FreeBSD__
    unsigned int s = sizeof(hz);
    sysctlbyname("machdep.tsc_freq", &hz, &s, NULL, 0);
#elif __linux
    FILE *cpuinfo;
    double tmp_hz;
    char str[256];
    char buff[256];

    if((cpuinfo = fopen("/proc/cpuinfo", "rb")) == NULL) {
        printf("/proc/cpuinfo file cannot open\n");
        exit(EXIT_FAILURE);
    }

    while(fgets(str, sizeof(str), cpuinfo) != NULL) {
        if(strstr(str, "cpu MHz") != NULL) {
            memset(buff, 0, sizeof(buff));
            strncpy(buff, str + 11, strlen(str + 11));
            tmp_hz = atof(buff);
            hz = tmp_hz * 1000 * 1000;
            break;
        }
    }
#endif

  return hz;
}

void
timer_free(handle)
struct timer_handle *handle;
{
  free(handle);
}

void
timer_reset_rdtsc(handle)
struct timer_handle *handle;
{
    rdtsc(handle->next_event);
    rdtsc(handle->zero);
    dprintf(("[timer_reset] next_event : %lu\n", handle->next_event));
    dprintf(("[timer_reset] zero : %lu\n", handle->zero));
}

struct timer_handle
*timer_init_rdtsc(void)
{
    struct timer_handle *handle;
    
    if((handle = (struct timer_handle *)malloc(sizeof(struct timer_handle))) == NULL) {
        WARNING("Could not allocate memory for the timer");

        return NULL;
    }
    
    handle->cpu_frequency = get_cpu_frequency();
    dprintf(("[timer_init] handle->cpu_frequency : %u\n", handle->cpu_frequency));
    
    timer_reset_rdtsc(handle);
    
    return handle;
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
lookup_param(next_hop_id, p, rule_cnt)
int next_hop_id;
qomet_param *p;
int rule_cnt;
{
    int i;

    for(i = 0; i < rule_cnt; i++) {
        if(p[i].next_hop_id == next_hop_id)
            return i;
    }

    return -1;
}

void
dump_param_table(p, rule_cnt)
qomet_param *p;
int rule_cnt;
{
    int i;

    INFO("Dumping the param_table (rule cnt=%d)...", rule_cnt);
    for(i = 0; i < rule_cnt; i++) {
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
    char settings_file_name[BUFSIZ];
    int32_t loop = FALSE;
    int32_t dsock;
    int32_t node_cnt = 0;
    uint16_t rulenum;
    uint32_t fid, tid;
    uint32_t from, to;
    uint32_t pipe_nr;
    int64_t time_i;
    struct bin_hdr_cls bin_hdr;
    struct wireconf_class wireconf;

    float crt_record_time = 0.0;
    struct bin_time_rec_cls bin_time_rec;
    struct bin_rec_cls *bin_recs = NULL;
    int32_t bin_recs_max_cnt;

    int32_t *next_hop_ids = NULL;
    int32_t node_i;
  
    uint32_t *last_pkt_cnt = NULL;
    uint32_t *last_byte_cnt = NULL;
  
    int do_adjust_deltaQ = TRUE;
    int use_mac_addr = FALSE;
    float *avg_frame_sizes = NULL;
  
    
    struct bin_rec_cls **my_recs_ucast = NULL;
    struct bin_rec_cls *adjusted_recs_ucast = NULL;
    int *my_recs_ucast_changed = NULL;
    struct bin_rec_cls *my_recs_bcast = NULL;
    int *my_recs_bcast_changed = NULL;
    
#ifdef __FreeBSD
    int rule_data_alloc_size = -1;
    float my_total_channel_utilization;
    float my_total_transmission_probability;
    struct ip_fw *rule_data = NULL;
    struct ip_fw *rules[MAX_RULE_COUNT];
#endif
    
    int pkt_i;

    uint32_t sc_type;
    uint32_t usage_type;

    float time, dummy[PARAMETERS_UNUSED], bandwidth, delay, lossrate;
    FILE *qomet_fd;
    struct timer_handle *timer;
    int32_t loop_cnt = 0;

#ifdef __linux
    int32_t direction = DIRECTION_BR;
#else
    int32_t direction = DIRECTION_BOTH;
#endif

    struct timeval tp_begin, tp_end;

    int32_t i, j;
    int32_t my_id;
    in_addr_t ipaddrs[MAX_NODES];
    char ipaddrs_c[MAX_NODES*IP_ADDR_SIZE];
    int32_t node_number;

    int32_t offset_num;
    int32_t rule_cnt;
    int32_t next_hop_id, rule_num;
    uint32_t over_read;
    float time_period, next_time;
    qomet_param param_table[MAX_RULE_NUM];
    qomet_param param_over_read;
    char baddr[IP_ADDR_SIZE];

    usage_type = 1;

    memset(param_table, 0, sizeof(qomet_param) * MAX_RULE_NUM);
    memset(&param_over_read, 0, sizeof(qomet_param));

    memset(&device_list, 0, sizeof(struct DEVICE_LIST) * MAX_IFS);
    if_num = 0;
    over_read = 0;
    rule_cnt = 0;
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
    strncpy(baddr, "255.255.255.255", IP_ADDR_SIZE);

    if(argc < 2) {
        WARNING("No arguments provided");
        usage(argv0);
        exit(1);
    }

    i = 0;
    while((ch = getopt(argc, argv, "hc:q:a:f:F:t:T:r:p:d:i:z:s:m:b:l")) != -1) {
        switch(ch) {
            case 'h':
                usage("sample");
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
                else if(strcmp(optarg, "bridge") == 0) {
#ifdef __linux
                    direction = DIRECTION_BR;
#elif __FreeBSD
                    WARNING("Invalid direction '%s'", optarg);
                    exit(1);
#endif
                }
                else {
                    WARNING("Invalid direction '%s'", optarg);
                    exit(1);
                }
                break;
            case 'i':
                my_id = strtol(optarg, NULL, 10);
                break;
            case 'z':
#ifdef __linux
                strcpy(device_list[if_num].dev_name, optarg);
                if_num++;

#elif __FreeBSD
                fprintf(stderr, "support only linux\n");
#endif
                break;
            case 's':
                strncpy(settings_file_name, optarg, MAX_STRING - 1);
                usage_type = 2;

                if((node_number = read_settings(optarg, ipaddrs, MAX_NODES)) < 1) {
                    WARNING("Settings file '%s' is invalid", optarg);
                    exit(1);
                }
                for(i = 0; i < node_number; i++) {
                    snprintf(ipaddrs_c + i * IP_ADDR_SIZE, IP_ADDR_SIZE, 
                            "%d.%d.%d.%d",
                            *(((uint8_t *)&ipaddrs[i]) + 0),
                            *(((uint8_t *)&ipaddrs[i]) + 1),
                            *(((uint8_t *)&ipaddrs[i]) + 2),
                            *(((uint8_t *)&ipaddrs[i]) + 3));
                }
                break;
            case 'm':
                if((time_period = strtod(optarg, NULL)) == 0) {
                    WARNING("Invalid time period");
                    exit(1);
                }
                break;
            case 'b':
                strncpy(baddr, optarg, IP_ADDR_SIZE);
                break;
            case 'l':
                loop = TRUE;
                break;
            default:
                usage(argv0);
                exit(1);
        }
    }

    argc -= optind;
    argv += optind;

    if(qomet_fd == NULL) {
        WARNING("No QOMET data file was provided");
        usage(argv0);
        exit(1);
    }

    if(sc_type == BIN_SC) {
        if(use_mac_addr == TRUE) {
        /*
            if((node_cnt = io_read_settings_file_mac (settings_file_name,
                ipaddrs, ipaddrs_c, MAC_addresses, MAC_char_addresses, MAX_NODES_W)) < 1) {
                WARNING("Invalid MAC address settings file: '%s'", settings_file_name);
                exit(1);
            }
        */
            /*
               for (node_i=0; node_i<MAX_NODES_W; node_i++)
               {
               ipaddrs[node_i]=node_i;
               sprintf(ipaddrs_c + (node_i*IP_ADDR_SIZE), 
               "0.0.0.%d", node_i);
               }
            */
        }
        else {
            if((node_cnt = io_read_settings_file (settings_file_name, ipaddrs,
                ipaddrs_c, MAX_NODES)) < 1) {
                WARNING("Invalid IP address settings file: '%s'", settings_file_name);
                exit(1);
            }
        }
    }

    DEBUG("Initialize timer...");
    if((timer = timer_init_rdtsc()) == NULL) {
        WARNING("Could not initialize timer");
        exit(1);
    }
    DEBUG("Open control socket...");
    if((dsock = get_socket()) < 0) {
        WARNING("Could not open control socket (requires root priviledges)\n");
        exit(1);
    }

    init_rule(taddr);

    if(usage_type == 1) {
        INFO("Add rule #%d with pipe #%d from %s to %s", rulenum, pipe_nr, faddr, taddr);

        if(add_rule(dsock, rulenum, pipe_nr, faddr, taddr, direction) < 0) {
            WARNING("Could not add rule #%d with pipe #%d from %s to %s", 
                    rulenum, pipe_nr, faddr, taddr);
            exit(1);
        }
    }
    else {
        for(j = FIRST_NODE_ID; j < node_number + FIRST_NODE_ID; j++) {
            if(direction == DIRECTION_BR) {
                int k;
                uint32_t add_rule_num;
                char *add_filter_saddr = (char*)calloc(1, IP_ADDR_SIZE);
                char *add_filter_daddr = (char*)calloc(1, IP_ADDR_SIZE);

                for(k = 0; k < node_number + FIRST_NODE_ID; k++) {
                    if(j <= k) {
                        continue;
                    }
                    offset_num = j + (k * (node_number + 1));
                    add_rule_num = MIN_PIPE_ID_OUT + offset_num;
                    strcpy(add_filter_saddr, ipaddrs_c + k * IP_ADDR_SIZE);
                    strcpy(add_filter_daddr, ipaddrs_c + (j - FIRST_NODE_ID) * IP_ADDR_SIZE);

                    if(add_rule(dsock, add_rule_num, add_rule_num, add_filter_saddr, add_filter_daddr, DIRECTION_OUT) < 0) {
                        printf("Node %d: Could not add rule #%d", j, add_rule_num);
                        printf("\tSource Address %s", add_filter_saddr);
                        printf("\tDestination Address %s", add_filter_daddr);
                        exit(1);
                    }
                }
            }
            else {
                if(j == my_id) {
                    continue;
                }
                offset_num = j;
                INFO("Node %d: Add rule #%d with pipe #%d to destination %s", 
                        my_id, MIN_PIPE_ID_OUT + offset_num, MIN_PIPE_ID_OUT + offset_num, 
                        ipaddrs_c + (j - FIRST_NODE_ID) * IP_ADDR_SIZE);
                if(add_rule(dsock, MIN_PIPE_ID_OUT + offset_num, MIN_PIPE_ID_OUT + offset_num,
                            "any", ipaddrs_c + (j - FIRST_NODE_ID) * IP_ADDR_SIZE, 
                            DIRECTION_OUT) < 0) {
                    WARNING("Node %d: Could not add rule #%d with pipe #%d to \
                            destination %s", my_id, MIN_PIPE_ID_OUT + offset_num, 
                            MIN_PIPE_ID_OUT + offset_num, 
                            ipaddrs_c + (j-FIRST_NODE_ID) * IP_ADDR_SIZE);
                    exit(1);
                }
                if(add_rule(dsock, MIN_PIPE_ID_OUT + offset_num + 1, MIN_PIPE_ID_OUT + offset_num + 1,
                            "any", ipaddrs_c + (j - FIRST_NODE_ID) * IP_ADDR_SIZE, 
                            DIRECTION_OUT) < 0) {
                    WARNING("Node %d: Could not add rule #%d with pipe #%d to \
                            destination %s", my_id, MIN_PIPE_ID_OUT + offset_num, 
                            MIN_PIPE_ID_OUT + offset_num, 
                            ipaddrs_c + (j-FIRST_NODE_ID) * IP_ADDR_SIZE);
                    exit(1);
                }
            }
        }
        if(direction != DIRECTION_BR) {
            for(j = FIRST_NODE_ID; j < node_number + FIRST_NODE_ID; j++) {
                if(j == my_id || direction != DIRECTION_BR) {
                    continue;
                }
                offset_num = j; 
    
                INFO("Node %d: Add rule #%d with pipe #%d to destination %s",
                        my_id, MIN_PIPE_ID_IN_BCAST + offset_num, 
                        MIN_PIPE_ID_IN_BCAST + offset_num, baddr);
                if(add_rule(dsock, MIN_PIPE_ID_IN_BCAST + offset_num, 
                            MIN_PIPE_ID_IN_BCAST + offset_num, 
                            ipaddrs_c+(j-FIRST_NODE_ID)*IP_ADDR_SIZE,
                            baddr, DIRECTION_IN) < 0)
                {
                    WARNING("Node %d: Could not add rule #%d with pipe #%d from %s to \
                            destination %s", my_id, MIN_PIPE_ID_IN_BCAST + offset_num, 
                            MIN_PIPE_ID_IN_BCAST + offset_num, 
                            ipaddrs_c + (j - FIRST_NODE_ID) * IP_ADDR_SIZE, baddr);
                    exit(1);
                }
                if(add_rule(dsock, MIN_PIPE_ID_IN_BCAST + offset_num + 0, 
                            MIN_PIPE_ID_IN_BCAST + offset_num + 1, 
                            ipaddrs_c+(j-FIRST_NODE_ID)*IP_ADDR_SIZE,
                            baddr, DIRECTION_IN) < 0)
                {
                    WARNING("Node %d: Could not add rule #%d with pipe #%d from %s to \
                            destination %s", my_id, MIN_PIPE_ID_IN_BCAST + offset_num, 
                            MIN_PIPE_ID_IN_BCAST + offset_num, 
                            ipaddrs_c + (j - FIRST_NODE_ID) * IP_ADDR_SIZE, baddr);
                    exit(1);
                }
            }
        }
    }
    gettimeofday(&tp_begin, NULL);

    if(usage_type == 1) {
        next_hop_id = tid;
    }

    INFO("Reading QOMET data from file...");
    next_time = 0;

    emulation_start:
    if(sc_type == BIN_SC) {
        if(io_binary_read_header_from_file(&bin_hdr, qomet_fd) == ERROR) {
            WARNING("Aborting on input error (binary header)");
            exit(1);
        }
        io_binary_print_header(&bin_hdr);

        if(node_cnt != bin_hdr.if_num) {
            WARNING("Number of nodes according to the settings file (%d) and \
                number of nodes according to QOMET scenario (%d) differ", 
                node_cnt, bin_hdr.if_num);
            exit(1);
        }

        bin_recs_max_cnt = bin_hdr.if_num * (bin_hdr.if_num - 1);

        bin_recs = (struct bin_rec_cls *)calloc(bin_recs_max_cnt, sizeof(struct bin_rec_cls));
        if(bin_recs == NULL) {
            WARNING("Cannot allocate memory for binary records");
            exit(1);
        }

        next_hop_ids = (int32_t *)calloc(bin_hdr.if_num, sizeof(int));
        if(next_hop_ids == NULL) {
            WARNING("Cannot allocate memory for next_hop_ids");
            exit(1);
        }

        my_recs_ucast = (struct bin_rec_cls**)calloc(bin_hdr.if_num, sizeof(struct bin_rec_cls*));
        if(my_recs_ucast == NULL) {
            WARNING("Cannot allocate memory for my_recs_ucast");
            exit(1);
        }

        for(node_i = 0; node_i < bin_hdr.if_num; node_i++) {
            int j;

            my_recs_ucast[node_i] = (struct bin_rec_cls *)calloc(bin_hdr.if_num, sizeof(struct bin_rec_cls));
            if(my_recs_ucast[node_i] == NULL) {
                WARNING("Cannot allocate memory for my_recs_ucast[%d]", node_i);
                exit(1);
            }
            for(j = 0; j < bin_hdr.if_num; j++) {
                my_recs_ucast[node_i][j].bandwidth = UNDEFINED_BANDWIDTH;
            }
        }

        if(direction == DIRECTION_BR) {
            adjusted_recs_ucast = (struct bin_rec_cls *)calloc(bin_hdr.if_num * bin_hdr.if_num, sizeof(struct bin_rec_cls));
        }
        else {
            adjusted_recs_ucast = (struct bin_rec_cls *)calloc(bin_hdr.if_num, sizeof(struct bin_rec_cls));
        }
        if(adjusted_recs_ucast == NULL) {
            WARNING("Cannot allocate memory for adjusted_recs_ucast");
            exit(1);
        }

        my_recs_ucast_changed = (int32_t *)calloc(bin_hdr.if_num, sizeof(int32_t));
        if(my_recs_ucast_changed == NULL) {
            WARNING("Cannot allocate memory for my_recs_ucast_changed");
            exit(1);
        }

        my_recs_bcast = (struct bin_rec_cls *)calloc(bin_hdr.if_num, sizeof(struct bin_rec_cls));
        if(my_recs_bcast == NULL) {
            WARNING("Cannot allocate memory for my_recs_bcast");
            exit(1);
        }
        for(node_i = 0; node_i < bin_hdr.if_num; node_i++) {
            my_recs_bcast[node_i].bandwidth = UNDEFINED_BANDWIDTH;
        }

        my_recs_bcast_changed = (int32_t *)calloc(bin_hdr.if_num, sizeof(int32_t));
        if(my_recs_bcast_changed == NULL) {
            WARNING("Cannot allocate memory for my_recs_bcast_changed");
            exit(1);
        }
        
        last_byte_cnt = (uint32_t *)calloc(bin_hdr.if_num, sizeof(uint32_t));
        if(last_byte_cnt == NULL) {
            WARNING("Cannot allocate memory for last_byte_cnt");
            exit(1);
        }
        
        last_pkt_cnt = (uint32_t *)calloc(bin_hdr.if_num, sizeof(uint32_t));
        if(last_pkt_cnt == NULL) {
            WARNING("Cannot allocate memory for last_pkt_cnt");
            exit(1);
        }
        
        avg_frame_sizes = (float *)calloc(bin_hdr.if_num, sizeof(float));
        if(avg_frame_sizes == NULL) {
            WARNING("Cannot allocate memory for avg_frame_sizes");
            exit(1);
        }
        for(pkt_i = 0; pkt_i < bin_hdr.if_num; pkt_i++) {
            //last_pkt_cnt[pkt_i] = 0; // calloc inits to 0 => not needed!
            //last_byte_cnt[pkt_i] = 0;
            avg_frame_sizes[pkt_i] = DEFAULT_FRAME_SIZE;
        }
        gettimeofday(&tp_begin, NULL);

        for(time_i = 0; time_i < bin_hdr.time_rec_num; time_i++) {
            int rec_i;
            DEBUG("Reading QOMET data from file...");

            if(io_binary_read_time_record_from_file(&bin_time_rec, qomet_fd) == ERROR) {
                WARNING("Aborting on input error (time record)");
                exit (1);
            }
            io_binary_print_time_record(&bin_time_rec);
            crt_record_time = bin_time_rec.time;

            if(bin_time_rec.record_number > bin_recs_max_cnt) {
                WARNING("The number of records to be read exceeds allocated size (%d)", bin_recs_max_cnt);
                exit (1);
            }

            if(io_binary_read_records_from_file(bin_recs, bin_time_rec.record_number, qomet_fd) == ERROR) {
                WARNING("Aborting on input error (records)");
                exit (1);
            }

            for(rec_i = 0; rec_i < bin_time_rec.record_number; rec_i++) {
                if(bin_recs[rec_i].from_id < FIRST_NODE_ID || 
                    (bin_recs[rec_i].from_id > bin_hdr.if_num + FIRST_NODE_ID - 1)) {
                    INFO ("Source with id = %d is out of the valid range [%d, %d]", 
                        bin_recs[rec_i].from_id, FIRST_NODE_ID, 
                        bin_hdr.if_num + FIRST_NODE_ID - 1);
                    exit (1);
                }

                if(bin_recs[rec_i].to_id < FIRST_NODE_ID || (bin_recs[rec_i].to_id > bin_hdr.if_num + FIRST_NODE_ID - 1)) {
                    exit (1);
                }

                if(bin_recs[rec_i].from_id == my_id || direction == DIRECTION_BR) {
                    io_binary_copy_record(&(my_recs_ucast[bin_recs[rec_i].from_id][bin_recs[rec_i].to_id]), 
                      &bin_recs[rec_i]);
                    my_recs_ucast_changed[bin_recs[rec_i].to_id] = TRUE;

                    //io_binary_print_record (&(my_recs_ucast[bin_recs[rec_i].from_node][bin_recs[rec_i].to_node]));
                }

                if(bin_recs[rec_i].to_id == my_id || direction == DIRECTION_BR) {
                    io_binary_copy_record(&(my_recs_bcast[bin_recs[rec_i].from_id]), &bin_recs[rec_i]);
                    my_recs_bcast_changed[bin_recs[rec_i].from_id] = TRUE;
                    //io_binary_print_record (&(my_recs_bcast[bin_recs[rec_i].from_node]));
                }
            }

            if(bin_time_rec.record_number == 0) {
                // NOT IMPLEMENTED YET
            }

            if(time_i == 0) {
                uint32_t rec_index;
                for(rec_i = 0; rec_i < node_cnt; rec_i++) {
                    // do not consider the node itself
                    if(direction == DIRECTION_BR) {
                        int rec_j;
                        for(rec_j = 0; rec_j < node_cnt; rec_j++) {
                            rec_index = rec_i * (node_cnt + 1) + rec_j;
                            io_binary_copy_record(&(adjusted_recs_ucast[rec_index]), &(my_recs_ucast[rec_j][rec_i]));
                            DEBUG("Copied my_recs_ucast to adjusted_recs_ucast (index is rec_i=%d).", rec_i);
                        }
                    }
                    else if(rec_i != my_id && direction != DIRECTION_BR) {
                        io_binary_copy_record(&(adjusted_recs_ucast[rec_i]), &(my_recs_ucast[my_id][rec_i]));
                        DEBUG("Copied my_recs_ucast to adjusted_recs_ucast (index is rec_i=%d).", rec_i);
                        //io_binary_print_record (&(adjusted_recs_ucast[rec_i]));
                    }
                }
            }
            else {
                if(do_adjust_deltaQ == FALSE) {
                    WARNING("Adjustment of deltaQ is disabled.");

                    for(rec_i = 0; rec_i < node_cnt; rec_i++) {
                        if(rec_i != my_id) {
                            io_binary_copy_record(&(adjusted_recs_ucast[rec_i]), &(my_recs_ucast[my_id][rec_i]));
                            DEBUG("Copied my_recs_ucast to adjusted_recs_ucast (index is rec_i=%d).", rec_i);
                            //io_binary_print_record (&(adjusted_recs_ucast[rec_i]));
                        }
                    }
                }
                else {
                    DEBUG("Adjustment of deltaQ is enabled.");
                    adjust_deltaQ(&wireconf, my_recs_ucast,
                       adjusted_recs_ucast, my_recs_ucast_changed,
                       avg_frame_sizes);
                }
            }

            if (time_i == 0) {
                timer_reset(timer, crt_record_time);
            }
            else {
                if(SCALING_FACTOR == 10.0) {
                    INFO("Waiting to reach time %.2f s...", crt_record_time);
                }
                else {
                    INFO ("Waiting to reach real time %.2f s (scenario time %.2f)...\n", 
                        crt_record_time * SCALING_FACTOR, crt_record_time);

                    timer_wait(timer, crt_record_time * SCALING_FACTOR);
                }

                for(node_i = FIRST_NODE_ID; node_i < (node_cnt + FIRST_NODE_ID); node_i++) {
                    if(direction == DIRECTION_BR) {
                        int i;
                        int k;
                        i = node_i - FIRST_NODE_ID;
                        for(k = 0; k < node_cnt; k++) {
                            if(i <= k) {
                                continue;
                            }
                            next_hop_id = i * (k + 1) + k;
                            bandwidth = adjusted_recs_ucast[next_hop_id].bandwidth;
                            delay = adjusted_recs_ucast[next_hop_id].delay;
                            lossrate = adjusted_recs_ucast[next_hop_id].loss_rate;

                            if(configure_rule(dsock, taddr, MIN_PIPE_ID_OUT + node_i, 
                                    bandwidth, delay, lossrate) == ERROR) {
                                fprintf(stderr, "Error configuring UCAST rule %d.", MIN_PIPE_ID_OUT + node_i);
                                exit(1);
                            }
                        }
                    }
                    else {
                        int next_hop_id;
    
                        if(node_i == my_id && direction != DIRECTION_BR) {
                            continue;
                        }
    
                        if(use_mac_addr == TRUE) {
                            next_hop_id = node_i;
                        }
                        else {
                            if(direction == DIRECTION_BR) {
                                next_hop_id = get_next_hop_id(ipaddrs, ipaddrs_c, node_i, DIRECTION_BR);
                                printf("node_i : %d\n", node_i);
                                printf("next_hop_id : %d\n", next_hop_id);
                            }
                            else {
                                next_hop_id = get_next_hop_id(ipaddrs, ipaddrs_c, node_i, DIRECTION_OUT);
                            }
                        }
    
                        if(next_hop_id == ERROR) {
                            WARNING("Could not locate the next hop for destination node %i", node_i);
                            next_hop_id = node_i;
                        }
                        DEBUG("Next_hop=%i for destination=%i", next_hop_id, node_i);
    
                        if(next_hop_id < 0 || (next_hop_id > bin_hdr.if_num - 1)) {
                            WARNING("Next hop with id = %d is out of the valid range [%d, %d]", 
                                bin_recs[rec_i].to_id, 0, bin_hdr.if_num - 1);
                            exit(1);
                        }
    
                        next_hop_ids[node_i] = next_hop_id;
    
                        bandwidth = adjusted_recs_ucast[next_hop_id].bandwidth;
                        delay = adjusted_recs_ucast[next_hop_id].delay;
                        lossrate = adjusted_recs_ucast[next_hop_id].loss_rate;
    
                        if(bandwidth != UNDEFINED_BANDWIDTH) {
                            INFO ("-- Wireconf pipe=%d: #%d UCAST to #%d (next_hop_id=%d) \
                                [%s] (time=%.2f s): bandwidth=%.2fbit/s lossrate=%.4f delay=%.4f ms",
                                MIN_PIPE_ID_OUT + node_i, my_id, node_i, next_hop_id,
                                ipaddrs_c + (node_i - FIRST_NODE_ID) * IP_ADDR_SIZE,
                                crt_record_time, bandwidth, lossrate, delay);
                        }
                        else {
                            INFO ("-- Wireconf pipe=%d: #%d UCAST to #%d (next_hop_id=%d) \
                                [%s] (time=%.2f s): no valid record could be found => configure with no degradation", 
                                MIN_PIPE_ID_OUT + node_i, my_id, node_i, next_hop_id, 
                                ipaddrs_c + (node_i - FIRST_NODE_ID) * IP_ADDR_SIZE, crt_record_time);
    
                                //bandwidth = 0.0;
                                //delay = 0.0;
                                //lossrate = 0.0;
                        }
    
                        if(configure_rule(dsock, taddr, MIN_PIPE_ID_OUT + node_i, 
                                bandwidth, delay, lossrate) == ERROR) {
                            WARNING("Error configuring UCAST pipe %d.", MIN_PIPE_ID_OUT + node_i);
                            exit (1);
                        }
                    }
                }

                if(direction != DIRECTION_BR) {
                    for (node_i = FIRST_NODE_ID; node_i < (node_cnt + FIRST_NODE_ID); node_i++) {
                        if(node_i == my_id) {
                            continue;
                        }
                        
                        if(use_mac_addr == TRUE) {
                            continue;
                        }
    
                        bandwidth = my_recs_bcast[node_i].bandwidth;
                        delay = my_recs_bcast[node_i].delay;
                        lossrate = my_recs_bcast[node_i].loss_rate;
    
                        DEBUG("Used contents of my_recs_bcast for deltaQ parameters (index is node_i=%d).", node_i);
                            //io_binary_print_record (&(my_recs_bcast[node_i]));
    
                        if(bandwidth != UNDEFINED_BANDWIDTH) {
                            INFO ("-- Wireconf pipe=%d: #%d BCAST from #%d [%s] \
                                (time=%.2f s): bandwidth=%.2fbit/s lossrate=%.4f delay=%.4f ms", 
                                MIN_PIPE_ID_IN_BCAST + node_i, my_id, node_i, 
                                ipaddrs_c + (node_i - FIRST_NODE_ID) * IP_ADDR_SIZE, 
                                crt_record_time, bandwidth, lossrate, delay);
                        }
                        //bandwidth = 0.0;
                        //delay = 0.0;
                        //lossrate = 0.0;
                    }
    
                    if(configure_rule(dsock, taddr, MIN_PIPE_ID_IN_BCAST + node_i,
                        bandwidth, delay, lossrate) == ERROR) {
                        WARNING("Error configuring BCAST pipe %d.", MIN_PIPE_ID_IN_BCAST + node_i);
                        exit (1);
                    }
                }
            }

#ifdef __FreeBSD
//{
            if(do_adjust_deltaQ == TRUE) {
                rule_cnt = MAX_RULE_COUNT;

                if(get_rules(dummynet_socket_id, &rule_data, &rule_data_alloc_size, rules, &rule_cnt) == SUCCESS) {
                    int i;
                    int32_t node_i;
                    uint32_t delta_pkt_cnter, delta_byte_counter;
                    float adjusted_delta_pkt_cnter;
                    struct stats_class stats;

                    my_total_channel_utilization = 0;
                    my_total_transmission_probability = 0;
                    wireconf.total_self_number_packets = 0;
                    node_i = 0;

                    for(i = 0; i < rule_cnt; i++) {
                        if(rules[i]->rulenum >= MIN_PIPE_ID_OUT && 
                            rules[i]->rulenum < (MIN_PIPE_ID_OUT + bin_hdr.if_num)) {
                            if(node_i == my_id) {
                                wireconf.self_channel_utilizations[node_i] = 0;
                                wireconf.self_transmission_probabilities[node_i] = 0;
                                node_i++;
                            }
        
                            DEBUG("my_id=%d node_i=%d Rule #%d:\t%6" PRIu64 " pkts \t%9" PRIu64 " bytes", 
                                my_id, node_i, (rules[i])->rulenum, (rules[i])->pcnt, (rules[i])->bcnt);
        
                            delta_pkt_cnter = rules[i]->pcnt - last_pkt_cnt[node_i];
                            delta_byte_cnter = rules[i]->bcnt - last_byte_cnt[node_i];
        
                            last_pkt_cnt[node_i] = rules[i]->pcnt;
                            last_byte_cnt[node_i] = rules[i]->bcnt;
        
                            if(delta_pkt_cnter > 0) {
                                avg_frame_sizes[node_i] = (float)delta_byte_cnter / (float)delta_pkt_counter;
                            }
        
                            wireconf.self_channel_utilizations[node_i] = 
                                compute_channel_utilization(&(adjusted_recs_ucast[next_hop_ids[node_i]]),
                                delta_pkt_cnter, delta_byte_counter, 0.5);
        
                            wireconf.self_transmission_probabilities[node_i] =
                                compute_tx_prob(&(adjusted_recs_ucast[next_hop_ids[node_i]]),
                                delta_pkt_cnter, delta_byte_counter, 0.5, &adjusted_delta_pkt_counter);
        
                            wireconf.total_self_number_packets += adjusted_delta_pkt_cnter;
        
                            DEBUG("my_total_channel_utilization=%f", my_total_channel_utilization);
        
                            my_total_channel_utilization += wireconf.self_channel_utilizations[node_i];
                            my_total_transmission_probability += wireconf.self_transmission_probabilities[node_i];
        
                            node_i++;
                        }
                    }
                    DEBUG("my_total_channel_utilization=%f my_total_transmission_probability=%f", 
                        my_total_channel_utilization, my_total_transmission_probability);
    
                    if(my_total_channel_utilization > 1.0) {
                        WARNING("my_total_channel_utilization exceeds 1.0");
                    }
                    if(my_total_transmission_probability > 1.0) {
                        WARNING("my_total_transmission_probability exceeds 1.0");
                    }
    
                    stats.channel_utilization = my_total_channel_utilization;
                    stats.transmission_probability = my_total_transmission_probability;
    
                    if(do_adjust_deltaQ == TRUE && local_experiment == FALSE) {
                        wireconf_deliver_stats (&wireconf, &stats);
                    }
                }
                else {
                    WARNING("Cannot obtain rule information");
                }
            }
//}
#endif
            loop_cnt++;
        }
        if(loop == TRUE) {
            fseek(qomet_fd, 0L, SEEK_SET);
            goto emulation_start;
        }
    }
    else if(sc_type == TXT_SC) {
        while(fgets(buf, BUFSIZ, qomet_fd) != NULL) {
            if(sscanf(buf, "%f %d %f %f %f %d %f %f %f %f %f %f " "%f %f %f %f %f %f %f", \
                &time, &from, &dummy[0],
                &dummy[1], &dummy[2], &to, &dummy[3], &dummy[4],
                &dummy[5],  &dummy[6], &dummy[7], &dummy[8],
                &dummy[9], &dummy[10], &dummy[11], &bandwidth, 
                &lossrate, &delay, &dummy[12]) != PARAMETERS_TOTAL) {
                INFO("Skipped non-parametric line");
                continue;
            }
            if(usage_type == 1) {
                if((from == fid) && (to == next_hop_id)) {
                    INFO("* Wireconf configuration (time=%.2f s): bandwidth=%.2fbit/s lossrate=%.4f delay=%.4f ms", \
                        time, bandwidth, lossrate, delay);
    
                    if(time == 0.0) {
                        //timer_reset(timer, crt_record_time);
                    }
                    else {
                        uint64_t time_usec = time * 1000000;
                        if(timer_wait_rdtsc(timer, time_usec) < 0) {
                            fprintf(stderr, "Timer deadline missed at time=%.2f s", time);
                        }
                    }

                    bandwidth = (int)round(bandwidth);
                    delay = (int)round(delay);
                    lossrate = (int)rint(lossrate * 0x7fffffff);
                    //lossrate = 0;
    
    				//TCHK_START(time);
                    configure_rule(dsock, taddr, pipe_nr, bandwidth, delay, lossrate);
    				//TCHK_END(time);
                    loop_cnt++;
                }
            }
            else { 
                if(time == next_time) {
                    if(from == my_id || direction == DIRECTION_BR) {
                        param_table[rule_cnt].time = time;
                        param_table[rule_cnt].next_hop_id = to;
                        param_table[rule_cnt].bandwidth = bandwidth;
                        param_table[rule_cnt].delay = delay;
                        param_table[rule_cnt].lossrate = lossrate;
                        rule_cnt++;
                    }
                    continue;
                }
                else {
                    if(from == my_id || direction == DIRECTION_BR) {
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
                        timer_reset(timer, crt_record_time);
                    }
                    else {
                        if(timer_wait_rdtsc(timer, time * 1000000) < 0) {
                            WARNING("Timer deadline missed at time=%.2f s", time);
                        }
                    }
    
                    for(i = FIRST_NODE_ID; i < (node_number + FIRST_NODE_ID); i++) {
                        if(i == my_id || direction != DIRECTION_BR) {
                            continue;
                        }
    
                        if((next_hop_id = get_next_hop_id(ipaddrs, ipaddrs_c, i, DIRECTION_OUT)) == ERROR) {
                            WARNING("Could not locate the next hop for destination node %i", i);
                            next_hop_id = 1;
                            //exit(1);
                        }
    
                        offset_num = i;
                        if((rule_num = lookup_param(next_hop_id, param_table, rule_cnt)) == -1) {
                            WARNING("Could not locate the rule number for next hop = %i", 
                                    next_hop_id);
                            dump_param_table(param_table, rule_cnt);
                            exit(1);
                        }
    
                        time = param_table[rule_num].time;
                        bandwidth = param_table[rule_num].bandwidth;
                        delay = param_table[rule_num].delay;
                        lossrate = param_table[rule_num].lossrate;
    
                        INFO("* Wireconf: #%d UCAST to #%d [%s] (time=%.2f s): bandwidth=%.2fbit/s \
                                lossrate=%.4f delay=%.4f ms, offset=%d", \
                                my_id, i, ipaddrs_c + (i - FIRST_NODE_ID) * IP_ADDR_SIZE, 
                                time, bandwidth, lossrate, delay, offset_num);
    
#ifdef __FreeBSD__
                        bandwidth = (int)round(bandwidth);// * 2.56);
                        delay = (int) round(delay);//(delay / 2);
                        lossrate = (int)round(lossrate * 0x7fffffff);
#elif __linux
                        bandwidth = (int)rint(bandwidth);// * 2.56);
                        delay = (int) rint(delay);//(delay / 2);
                        lossrate = (int)rint(lossrate * 0x7fffffff);
#endif
                        configure_rule(dsock, taddr, MIN_PIPE_ID_OUT + offset_num, bandwidth, delay, lossrate);
                        loop_cnt++;
                    }
    
                    for(i = FIRST_NODE_ID; i < (node_number + FIRST_NODE_ID); i++) {
                        if(i == my_id || direction != DIRECTION_BR) {
                            continue;
                        }
    
                        offset_num = i;
                        if((rule_num = lookup_param(i, param_table, rule_cnt)) == -1) {
                            WARNING("Could not locate the rule number for next hop = %d with rule cnt = %d", 
                                    i, rule_cnt);
                            dump_param_table(param_table, rule_cnt);
                            exit(1);
                        }
    
                        time = param_table[rule_num].time;
                        bandwidth = param_table[rule_num].bandwidth;
                        delay = param_table[rule_num].delay;
                        lossrate = param_table[rule_num].lossrate;
    
                        INFO("* Wireconf: #%d BCAST from #%d [%s] (time=%.2f s): bandwidth=%.2fbit/s \
                                lossrate=%.4f delay=%.4f ms, offset=%d", my_id, i, \
                                ipaddrs_c + (i - FIRST_NODE_ID) * IP_ADDR_SIZE, 
                                time, bandwidth, lossrate, delay, offset_num);
#ifdef __FreeBSD__
                        bandwidth = (int)round(bandwidth);// * 2.56);
                        delay = (int)round(delay);//(delay / 2);
                        lossrate = (int)round(lossrate * 0x7fffffff);
#elif __linux
                        bandwidth = (int)rint(bandwidth);// * 2.56);
                        delay = (int)rint(delay);//(delay / 2);
                        lossrate = (int)rint(lossrate * 0x7fffffff);
#endif
                        configure_rule(dsock, taddr, MIN_PIPE_ID_IN_BCAST + offset_num, bandwidth, delay, lossrate);
                        loop_cnt++;
                    }
    
                    memset(param_table, 0, sizeof(qomet_param) * MAX_RULE_NUM);
                    rule_cnt = 0;
                    next_time += time_period;
                    INFO("New timer deadline=%f", next_time);

                    if(over_read == 1) {
                        param_table[0].time = param_over_read.time;
                        param_table[0].next_hop_id = param_over_read.next_hop_id;
                        param_table[0].bandwidth = param_over_read.bandwidth;
                        param_table[0].delay = param_over_read.delay;
                        param_table[0].lossrate = param_over_read.lossrate;
                        rule_cnt++;
                    }
                    memset(&param_over_read, 0, sizeof(qomet_param));
                    over_read = 0;
                }
            }
        } 
    }
    if(loop == TRUE) {
        fseek(qomet_fd, 0L, SEEK_SET);
        goto emulation_start;
    }

    if(loop_cnt == 0) {
        if(usage_type == 1) {
            WARNING("The specified pair from_node_id=%d & to_node_id=%d could not be found", fid, tid);
        }
        else {
            WARNING("No valid line was found for the node %d", my_id); 
        }
    }
//    timer_free(timer);

    delete_rule(dsock, taddr, rule_num);

    gettimeofday(&tp_end, NULL);
    INFO("Experiment execution time=%.4f s", 
        (tp_end.tv_sec+tp_end.tv_usec / 1.0e6) - (tp_begin.tv_sec + tp_begin.tv_usec / 1.0e6));
    DEBUG("Closing socket...");

    close_socket(dsock);
//    fclose(qomet_fd);

    return 0;
}

