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
 * Meteor Emulator Implementation
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
#include <assert.h>
#include <signal.h>
#include <getopt.h>
#include <sched.h>

#include "global.h"
#include "wireconf.h"
#include "message.h"
#include "statistics.h"
#include "timer.h"
//#include "tc_util.h"
#include "meteor.h"
#include "config.hpp"
#include "libnlwrap.h"

int32_t use_mac_addr = FALSE;
int32_t re_flag = FALSE;
int32_t verbose_level = 0;
int32_t my_id;
int32_t loop = FALSE;
int32_t direction;

#ifdef __amd64__
#define rdtsc(t)                                                              \
    __asm__ __volatile__ ("rdtsc; movq %%rdx, %0; salq $32, %0;orq %%rax, %0" \
    : "=r"(t) : : "%rax", "%rdx"); 
#else
#define rdtsc(t) asm volatile("rdtsc" : "=A" (t))
#endif

void
usage()
{
    fprintf(stderr, "Meteor. Drive network emulation using QOMET scenario.\n\n");
    fprintf(stderr, "\tUsage: meteor -q <deltaQ_binary_file> -i <own_id> -s <settings_file> -m <in|br>\n"
            "\t\t[-M] [-I <Interface Name>] [-a <assign_id>] [-l] [-d]\n");

    fprintf(stderr, "\t-q, --qomet_scenario: Scenario file.\n");
    fprintf(stderr, "\t-i, --id: Own ID in QOMET scenario\n");
    fprintf(stderr, "\t-s, --settings: Setting file.\n");
    fprintf(stderr, "\t-m, --mode: ingress|hypervisor|bridge\n");
    fprintf(stderr, "\t-M, --use_mac_address: Meteor use MAC Address filtering. Default: IP Address\n");
    fprintf(stderr, "\t-I, --interface: Select physical interface.\n");
    fprintf(stderr, "\t-d, --daemon: Daemon mode.\n");
}

void
restart_scenario()
{
    re_flag = TRUE;
}

int
timer_wait_rdtsc(struct timer_handle* handle, uint64_t time_in_us)
{
    uint64_t crt_time;

    handle->next_event = handle->zero + (handle->cpu_frequency * time_in_us) / 1000000;
    rdtsc(crt_time);

    if (handle->next_event < crt_time) {
        return ERROR;
    }

    do {
        if (re_flag == TRUE) {
            return 2;
        }
        usleep(1000);
        rdtsc(crt_time);
    } while (handle->next_event >= crt_time);

    return SUCCESS;
}

uint32_t
get_cpu_frequency(void)
{
    char str[256];
    char buff[256];
    uint32_t hz = 0;
    double tmp_hz;
    FILE *cpuinfo;

    if ((cpuinfo = fopen("/proc/cpuinfo", "r")) == NULL) {
        fprintf(stderr, "/proc/cpuinfo file cannot open\n");
        exit(EXIT_FAILURE);
    }

    while (fgets(str, sizeof (str), cpuinfo) != NULL) {
        if (strstr(str, "cpu MHz") != NULL) {
            memset(buff, 0, sizeof (buff));
            strncpy(buff, str + 11, strlen(str + 11));
            tmp_hz = atof(buff);
            hz = tmp_hz * 1000 * 1000;
            break;
        }
    }
    fclose(cpuinfo);

    return hz;
}

void
timer_free(struct timer_handle *handle)
{
    free(handle);
}

void
timer_reset_rdtsc(struct timer_handle *handle)
{
    rdtsc(handle->next_event);
    rdtsc(handle->zero);
    //dprintf(("[timer_reset] next_event : %lu\n", handle->next_event));
    //dprintf(("[timer_reset] zero : %lu\n", handle->zero));
}

struct timer_handle *
timer_init_rdtsc(void)
{
    struct timer_handle *handle;
    
    handle = (struct timer_handle *)malloc(sizeof (struct timer_handle));
    if (!handle) {
        WARNING("Could not allocate memory for the timer");
        return NULL;
    }
    handle->cpu_frequency = get_cpu_frequency();
    timer_reset_rdtsc(handle);
    
    return handle;
}

struct connection_list *
add_conn_list(struct connection_list *conn_list, int32_t src_id, int32_t dst_id)
{
    uint16_t rec_i = 1;

    if (!conn_list) {
        if (!(conn_list = malloc(sizeof (struct connection_list)))) {
            perror("malloc");
            exit(1);
        }
    }
    else {
        rec_i = conn_list->rec_i + 1;
        if (!(conn_list->next_ptr = malloc(sizeof (struct connection_list)))) {
            perror("malloc");
            exit(1);
        }
        conn_list = conn_list->next_ptr;
    }
    conn_list->next_ptr = NULL;
    conn_list->src_id = src_id;
    conn_list->dst_id = dst_id;
    conn_list->rec_i = rec_i;

    return conn_list;
}

struct connection_list *
create_conn_list(FILE *conn_fd, int32_t node_cnt)
{
    char buf[BUFSIZ];
    int32_t src_id;
    int32_t dst_id;
    struct connection_list *conn_list;
    struct connection_list *conn_list_head;

    conn_list = NULL;
    conn_list_head = NULL;
    while (fgets(buf, BUFSIZ, conn_fd) != NULL) {
        if (sscanf(buf, "%d %d", &src_id, &dst_id) != 2) {
            fprintf(stderr, "Invalid Parameters");
            exit(1);
        }

        if (src_id > node_cnt) {
            fprintf(stderr, "Source node id too big: %d > %d\n", src_id, node_cnt);
            exit(1);
        }
        else if (dst_id > node_cnt) {
            fprintf(stderr, "Destination node id too big: %d > %d\n", dst_id, node_cnt);
            exit(1);
        }

        conn_list = add_conn_list(conn_list, src_id, dst_id);
        if (conn_list_head == NULL) {
            conn_list_head = conn_list;
        }
    }

    return conn_list_head;
}

int
meteor_loop(FILE *qomet_fd, struct nl_sock *sock, int ifb_index, 
        struct bin_hdr_cls bin_hdr, int32_t node_cnt, 
        struct node_data *node_list_head, int direction, 
        struct connection_list *conn_list_head)
{
    int time_i, node_i, ret;
    int32_t bin_recs_max_cnt;
    uint32_t bin_hdr_if_num;
    float crt_record_time = 0.0;
    double bandwidth, delay, lossrate;
    struct bin_time_rec_cls bin_time_rec;
    struct bin_rec_cls *bin_recs = NULL;
    struct bin_rec_cls **my_recs_ucast = NULL;
    struct bin_rec_cls *adjusted_recs_ucast = NULL;
    struct timer_handle *timer;
    int *my_recs_ucast_changed = NULL;
    struct connection_list *conn_list = NULL;
 
    struct node_data *node;
    struct node_data *my_node;
    for (node = node_list_head; node->id < node_cnt; node++) {
        if (node->id == my_id) {
            my_node = node;
        }
    }

    if ((timer = timer_init_rdtsc()) == NULL) {
        fprintf(stderr, "Could not initialize timer");
        exit(1);
    }

    if (node_cnt != bin_hdr.if_num) {
        fprintf(stderr, "Number of nodes according to the settings file (%d) "
                "and number of nodes according to QOMET scenario (%d) differ", 
                node_cnt, bin_hdr.if_num);
        exit(1);
    }

    bin_recs_max_cnt = bin_hdr.if_num * (bin_hdr.if_num - 1);
    if (!bin_recs) {
        bin_recs = (struct bin_rec_cls *)calloc(bin_recs_max_cnt, sizeof (struct bin_rec_cls));
    }
    if (!bin_recs) {
        WARNING("Cannot allocate memory for binary records");
        exit(1);
    }

    if (!my_recs_ucast) {
        my_recs_ucast = (struct bin_rec_cls**)calloc(bin_hdr.if_num, sizeof (struct bin_rec_cls*));
    }
    if (!my_recs_ucast) {
        fprintf(stderr, "Cannot allocate memory for my_recs_ucast\n");
        exit(1);
    }

    for (node_i = 0; node_i < bin_hdr.if_num; node_i++) {
        if (!my_recs_ucast[node_i]) {
            my_recs_ucast[node_i] = (struct bin_rec_cls *)calloc(bin_hdr.if_num, sizeof (struct bin_rec_cls));
        }
        if (!my_recs_ucast[node_i]) {
            fprintf(stderr, "Cannot allocate memory for my_recs_ucast[%d]\n", node_i);
            exit(1);
        }

        int node_j;
        for (node_j = 0; node_j < bin_hdr.if_num; node_j++) {
            my_recs_ucast[node_i][node_j].bandwidth = UNDEFINED_BANDWIDTH;
        }
    }

    if (direction == DIRECTION_BR) {
        bin_hdr_if_num = bin_hdr.if_num * bin_hdr.if_num;
    }
    else {
        bin_hdr_if_num = bin_hdr.if_num;
    }
    if (!adjusted_recs_ucast) {
        adjusted_recs_ucast = (struct bin_rec_cls *)calloc(bin_hdr_if_num, sizeof (struct bin_rec_cls));
        if (adjusted_recs_ucast == NULL) {
            fprintf(stderr, "Cannot allocate memory for adjusted_recs_ucast");
            exit(1);
        }
    }

    if (!my_recs_ucast_changed) {
        my_recs_ucast_changed = (int32_t *)calloc(bin_hdr_if_num, sizeof (int32_t));
        if (my_recs_ucast_changed == NULL) {
            fprintf(stderr, "Cannot allocate memory for my_recs_ucast_changed\n");
            exit(1);
        }
    }

emulation_start:
    for (time_i = 0; time_i < bin_hdr.time_rec_num; time_i++) {
        int rec_i;

        if (verbose_level >= 2) {
            printf("Reading QOMET data from file... Time : %d/%d\n", time_i, bin_hdr.time_rec_num);
        }

        if (io_binary_read_time_record_from_file(&bin_time_rec, qomet_fd) == ERROR) {
            fprintf(stderr, "Aborting on input error (time record)\n");
            exit (1);
        }
        io_binary_print_time_record(&bin_time_rec);
        crt_record_time = bin_time_rec.time;

        if (bin_time_rec.record_number > bin_recs_max_cnt) {
            fprintf(stderr, "The number of records to be read exceeds allocated size (%d)\n", bin_recs_max_cnt);
            exit (1);
        }

        if (io_binary_read_records_from_file(bin_recs, bin_time_rec.record_number, qomet_fd) == ERROR) {
            fprintf(stderr, "Aborting on input error (records)\n");
            exit (1);
        }

        for (rec_i = 0; rec_i < bin_time_rec.record_number; rec_i++) {
            if (bin_recs[rec_i].from_id < FIRST_NODE_ID) {
                INFO("Source with id = %d is smaller first node id : %d", bin_recs[rec_i].from_id, assign_id);
                exit(1);
            }
            if (bin_recs[rec_i].from_id > bin_hdr.if_num) {
                INFO("Source with id = %d is out of the valid range [%d, %d] rec_i : %d\n", 
                    bin_recs[rec_i].from_id, assign_id, bin_hdr.if_num + assign_id - 1, rec_i);
                exit(1);
            }

            int32_t src_id;
            int32_t dst_id;
            if (direction == DIRECTION_BR) {
            }
            else if (direction == DIRECTION_IN && bin_recs[rec_i].to_id == my_id) {
                src_id = bin_recs[rec_i].to_id;
                dst_id = bin_recs[rec_i].from_id;

                io_bin_cp_rec(&(my_recs_ucast[src_id][dst_id]), &bin_recs[rec_i]);
                my_recs_ucast_changed[bin_recs[rec_i].to_id] = TRUE;

                if(verbose_level >= 3) {
                    io_binary_print_record(&(my_recs_ucast[src_id][dst_id]));
                }
            }
        }

        if (time_i == 0 && re_flag == -1) {
            if (direction == DIRECTION_BR) {
                uint32_t rec_index;
                int32_t src_id, dst_id;
                conn_list = conn_list_head;
                while (conn_list) {
                    src_id = conn_list->src_id;
                    dst_id = conn_list->dst_id;
                    rec_index = conn_list->rec_i;
                    io_bin_cp_rec(&(adjusted_recs_ucast[rec_index]), &(my_recs_ucast[src_id][dst_id]));
                    DEBUG("Copied my_recs_ucast to adjusted_recs_ucast (index is rec_i=%d).", rec_index);

                    conn_list = conn_list->next_ptr;
                }
            }
            else {
                int i;
                node = node_list_head;
                for (i = 1; i < node_cnt; i++) {
                     if (node->id != my_id) {
                        io_bin_cp_rec(&(adjusted_recs_ucast[node->id]), &(my_recs_ucast[my_id][node->id]));
                        DEBUG("Copied my_recs_ucast to adjusted_recs_ucast (index is rec_i=%d).", node->id);
                    }
                    node++;
                }
            }
        }
        else {
            INFO("Adjustment of deltaQ is disabled.");
            if (direction == DIRECTION_BR) {
                //uint32_t rec_index;
            }
            else {
                int i;
                node = node_list_head;
                for (i = 1 ; i < node_cnt; i++) {
                    if (node->id != my_id) {
                        io_bin_cp_rec(&(adjusted_recs_ucast[node->id]), &(my_recs_ucast[my_id][node->id]));
                        DEBUG("Copied my_recs_ucast to adjusted_recs_ucast (index is rec_i=%d).", node->id);
                    }
                    node++;
                }
            }
        }

        if (time_i == 0) {
            timer_reset(timer, crt_record_time);
        }
        else {
            if (SCALING_FACTOR == 10.0) {
                INFO("Waiting to reach time %.6f s...", crt_record_time);
            }
            else {
                INFO("Waiting to reach real time %.6f s (scenario time %.6f)...\n", crt_record_time * SCALING_FACTOR, crt_record_time);

                if ((ret = timer_wait_rdtsc(timer, crt_record_time * 1000000)) < 0) {
                    fprintf(stderr, "Timer deadline missed at time=%.6f s ", crt_record_time);
                    fprintf(stderr, "This rule is skip.\n");
                    continue;
                }
                if (ret == 2) {
                    fseek(qomet_fd, 0L, SEEK_SET);
                    if ((timer = timer_init_rdtsc()) == NULL) {
                        fprintf(stderr, "Could not initialize timer\n");
                        exit(1);
                    }
                    io_binary_read_header_from_file(&bin_hdr, qomet_fd);
                    if (verbose_level >= 1) {
                        io_binary_print_header(&bin_hdr);
                    }
                    re_flag = FALSE;
                    goto emulation_start;
                }
/*
                if (timer_wait(timer, crt_record_time * SCALING_FACTOR) != 0) {
                    fprintf(stderr, "Timer deadline missed at time=%.2f s\n", crt_record_time);
                }
*/
            }

            if (direction == DIRECTION_BR) {
            }
            else {
                int i;
                node = node_list_head;
                for (i = 0; i < node_cnt - 1; i++) {
                    if (node->id == my_id) {
                        continue;
                    }

                    if (node->id < 0 || (node->id > bin_hdr.if_num - 1)) {
                        WARNING("Next hop with id = %d is out of the valid range [%d, %d]",
                                bin_recs[node->id].to_id, 0, bin_hdr.if_num - 1);
                        exit(1);
                    }

                    bandwidth = adjusted_recs_ucast[node->id].bandwidth;
                    delay = adjusted_recs_ucast[node->id].delay * 1000;
                    lossrate = adjusted_recs_ucast[node->id].loss_rate * 100;

                    if (bandwidth != UNDEFINED_BANDWIDTH) {
                        INFO("-- Meteor id = %d #%d to me (time=%.2f s): bandwidth=%.2fbit/s lossrate=%.4f delay=%.4f ms",
                            my_id, node->id, crt_record_time, bandwidth, lossrate, delay);
                    }
                    else {
                        INFO("-- Meteor id = %d #%d to me (time=%.2f s): no valid record could be found => configure with no degradation", 
                            my_id, node->id, crt_record_time);
                    }
                    ret = configure_rule(sock, ifb_index, node->id + 10, node->id + 10, bandwidth, delay, lossrate);
                    if (ret != SUCCESS) {
                        WARNING("Error configuring Meteor rule %d.", node->id);
                        exit (1);
                    }
                    node++;
                }
            }
        }
    }

    if (loop == TRUE) {
        re_flag = FALSE;
        fseek(qomet_fd, 0L, SEEK_SET);
        if ((timer = timer_init_rdtsc()) == NULL) {
            WARNING("Could not initialize timer");
            exit(1);
        }
        io_binary_read_header_from_file(&bin_hdr, qomet_fd);
        if (verbose_level >= 1) {
            io_binary_print_header(&bin_hdr);
        }
        goto emulation_start;
    }

    return 0;
}

struct option options[] = 
{
    {"connection", required_argument, NULL, 'c'},
    {"mode", required_argument, NULL, 'm'},
    {"daemon", no_argument, NULL, 'd'},
    {"help", no_argument, NULL, 'h'},
    {"id", required_argument, NULL, 'i'},
    {"interface", required_argument, NULL, 'I'},
    {"loop", no_argument, NULL, 'l'},
    {"use_mac_address", no_argument, NULL, 'M'},
    {"qomet_scenario", required_argument, NULL, 'q'},
    {"settings", required_argument, NULL, 's'},
    {"verbose", no_argument, NULL, 'v'},
    {0, 0, 0, 0}
};

int
main(int argc, char **argv)
{
    int32_t ret;
    int32_t node_cnt;
    struct nl_sock *sock;
    int daemon_flag = FALSE;
    struct bin_hdr_cls bin_hdr;

    FILE *qomet_fd;
    FILE *conn_fd;

    int32_t proto = ETH_P_IP;

    struct sigaction sa;
    struct connection_list *conn_list = NULL;
    struct connection_list *conn_list_head = NULL;

    memset(&device_list, 0, sizeof (struct DEVICE_LIST) * MAX_IFS);
    memset(&sa, 0, sizeof (struct sigaction));
    sa.sa_handler = &restart_scenario;
    sa.sa_flags |= SA_RESTART;

    if (sigaction(SIGUSR1, &sa, NULL) != 0) {
        fprintf(stderr, "Signal Set Error\n");
        exit(1);
    }

    if_num = 0;
    qomet_fd = NULL;
    direction = DIRECTION_IN;

    //my_id = -1;
    node_cnt = -1;

    char ch;
    int index;
    struct node_data *node_list_head;
    while ((ch = getopt_long(argc, argv, "c:dhi:I:lm:Mq:s:v", options, &index)) != -1) {
        switch (ch) {
            case 'c':
                conn_fd = fopen(optarg, "r");
                break;
            case 'd':
                daemon_flag = TRUE;
                break;
            case 'h':
                usage();
                exit(0);
            case 'i':
                my_id = strtol(optarg, NULL, 10);
                break;
            case 'I':
                strcpy(device_list[if_num].dev_name, optarg);
                if_num++;
                break;
            case 'l':
                loop = TRUE;
                break;
            case 'm':
                if ((strcmp(optarg, "ingress") == 0) || strcmp(optarg, "in") == 0) {
                    direction = DIRECTION_IN;
                }
                else if ((strcmp(optarg, "bridge") == 0) ||(strcmp(optarg, "br") == 0)) {
                    direction = DIRECTION_BR;
                }
                else {
                    WARNING("Invalid direction '%s'", optarg);
                    exit(1);
                }
                break;
            case 'M':
                use_mac_addr = TRUE;
                proto = ETH_P_ALL;
                break;
            case 'q':
                if ((qomet_fd = fopen(optarg, "rb")) == NULL) {
                    WARNING("Could not open QOMET output file '%s'", optarg);
                    exit(1);
                }
                break;
            case 's':
                if ((node_cnt = get_node_cnt(optarg)) <= 0) {
                    fprintf(stderr, "Settings file '%s' is invalid", optarg);
                    exit(1);
                }
                node_list_head = create_node_list(optarg, node_cnt);
                break;
            case 'v':
                verbose_level += 1;
                break;
            default:
                usage();
                exit(1);
        }
    }

    if (qomet_fd == NULL) {
        fprintf(stderr, "No QOMET data file was provided\n");
        usage();
        exit(1);
    }

    if (node_cnt < my_id) {
        fprintf(stderr, "Invalid ID\n");
        exit(1);
    }

    if (direction == DIRECTION_IN) {
        if (my_id == -1) {
            fprintf(stderr, "Please specify node id. option: -i ID\n");
            exit(1);
        }
    }
    if (!conn_fd && direction == DIRECTION_BR) {
        fprintf(stderr, "no connection file");
        usage();
        exit(1);
    }
    else if (direction == DIRECTION_BR) {
        conn_list_head = create_conn_list(conn_fd, node_cnt);
    }

/*
    DEBUG("Initialize timer...");
    if ((timer = timer_init_rdtsc()) == NULL) {
        fprintf(stderr, "Could not initialize timer");
        exit(1);
    }
*/
    if (daemon_flag == TRUE) {
        loop = TRUE;
        ret = daemon(0, 0);
    }

    DEBUG("Open control socket...");
    sock = nl_socket_alloc();
    if (!sock) {
        WARNING("Could not open control socket (requires root priviledges)\n");
        exit(1);
    }
    nl_connect(sock, NETLINK_ROUTE);

    int if_index, ifb_index;
    struct nl_cache *cache;
    rtnl_link_alloc_cache(sock, AF_UNSPEC, &cache);
    if_index = get_ifindex(cache, device_list[0].dev_name);
    ifb_index = create_ifb(sock, my_id);
    init_rule(sock, if_index, ifb_index, proto);

/*
    // add default rule
    uint32_t src_id;
    uint32_t dst_id;
    if (direction == DIRECTION_BR) {
        conn_list = conn_list_head;
        while (conn_list != NULL) {
            src_id = conn_list->src_id;
            dst_id = conn_list->dst_id;

            offset_num = conn_list->rec_i;
            rule_num = MIN_PIPE_ID_BR + offset_num;
            if (protocol == ETH) {
                strcpy(saddr, mac_char_addresses[src_id]);
                strcpy(daddr, mac_char_addresses[dst_id]);
            }
            else if (protocol == IP) {
                strcpy(saddr, ipaddrs_c + src_id * IP_ADDR_SIZE);
                sprintf(saddr, "%s/%d", saddr, ipprefix[src_id]);
                strcpy(daddr, ipaddrs_c + dst_id * IP_ADDR_SIZE);
                sprintf(daddr, "%s/%d", daddr, ipprefix[dst_id]);
            }

            ret = add_rule(dsock, rule_num, rule_num, protocol, saddr, daddr, direction);
            if (ret != SUCCESS) {
                fprintf(stderr, "Node %d: Could not add rule #%d", src_id, rule_num);
                exit(1);
            }

            conn_list = conn_list->next_ptr;
        }
    }
*/
    if (direction == DIRECTION_BR) {
    }
    if (direction == DIRECTION_IN) {
        struct node_data *node;
        struct node_data *my_node;
        for (node = node_list_head; node->id < node_cnt; node++) {
            if (node->id == my_id) {
                my_node = node;
            }
        }
        int i;
        node = node_list_head;
        for (i = 0; i < node_cnt; i++) {
            if (node->id == my_id) {
                continue;
            }
            printf("Node %d: Add rule #%d from source %s\n", my_id, node->id, inet_ntoa(node->ipv4addr));

            if (add_rule(sock, ifb_index, node->id + 10, node->id + 10, proto, node, my_node) < 0) {
                fprintf(stderr, "Node %d: Could not add rule #%d from source %s\n", 
                        my_id, node->id, inet_ntoa(node->ipv4addr));
                exit(1);
            }
            node++;
        }
    }

    INFO("Reading QOMET data from file...");

    if (io_binary_read_header_from_file(&bin_hdr, qomet_fd) == ERROR) {
        WARNING("Aborting on input error (binary header)");
        exit(1);
    }
    if (verbose_level >= 1) {
        io_binary_print_header(&bin_hdr);

        switch (direction) {
            case DIRECTION_BR:
                fprintf(stdout, "Direction Mode: Bridge\n");
                break;
            case DIRECTION_IN:
                fprintf(stdout, "Direction Mode: In\n");
                break;
        }
    }

    if (node_cnt != bin_hdr.if_num) {
        fprintf(stderr, "Number of nodes according to the settings file (%d) "
                "and number of nodes according to QOMET scenario (%d) differ", 
                node_cnt, bin_hdr.if_num);
        exit(1);
    }

    meteor_loop(qomet_fd, sock, ifb_index, bin_hdr, node_cnt, node_list_head, direction, conn_list_head);

    delete_qdisc(sock, ifb_index, TC_H_ROOT, 0);
    delete_qdisc(sock, if_index, TC_H_INGRESS, 0);
    delete_ifb(sock, ifb_index);

    nl_close(sock);
    return 0;
}
