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
#include "routing_info.h"
#include "statistics.h"
#include "timer.h"
#include "tc_util.h"
#include "meteor.h"

int32_t use_mac_addr = FALSE;
int32_t assign_id = FIRST_NODE_ID;
int32_t division = 1;
int32_t re_flag = FALSE;
int32_t verbose_level = 0;
int32_t my_id;
int32_t busy = 0;
int32_t loop = FALSE;
int32_t direction;

char *logfile = NULL;
FILE *logfd;

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
    fprintf(stderr, "    Usage: meteor -q <deltaQ_output_binary_file> -i <own_id> -s <settings_file> -m <in|hypervisor|bridge>\n"
            "\t\t[-M] [-b <baddr>] [-I <Interface Name>] [-a <assign_id>] [-d <division>] [-l] [-d]\n");

    fprintf(stderr, "\t-q, --qomet_scenario: Scenario file.\n");
    fprintf(stderr, "\t-i, --id: Own ID in QOMET scenario\n");
    fprintf(stderr, "\t-s, --settings: Setting file.\n");
    fprintf(stderr, "\t-m, --mode: ingress|hypervisor|bridge\n");
    fprintf(stderr, "\t-M, --use_mac_address: Meteor use MAC Address filtering for network emulation. Default: IP Address\n");
    fprintf(stderr, "\t-I, --interface: Physical interface select for network emulation.\n");
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
	if (busy == 0) {
       		usleep(1000);
	}
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
        fprintf(logfd, "/proc/cpuinfo file cannot open\n");
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
    dprintf(("[timer_reset] next_event : %lu\n", handle->next_event));
    dprintf(("[timer_reset] zero : %lu\n", handle->zero));
}

struct timer_handle *
timer_init_rdtsc(void)
{
    struct timer_handle *handle;
    
    handle = (struct timer_handle *)malloc(sizeof (struct timer_handle));
    if (handle == NULL) {
        WARNING("Could not allocate memory for the timer");
        return NULL;
    }
    handle->cpu_frequency = get_cpu_frequency();
    timer_reset_rdtsc(handle);
    
    return handle;
}

int
read_settings(char *path, in_addr_t *p, int *prefix, int p_size)
{
    char *slash;
    char *ptr;
    char node_name[20];
    char interface[20];
    char node_ip[IP_ADDR_SIZE];
    static char buf[BUFSIZ];
    int32_t i = 0;
    int32_t line_nr = 0;
    int32_t node_id;
    FILE *fd;

    if ((fd = fopen(path, "r")) == NULL) {
        WARNING("Cannot open settings file '%s'", path);
        return -1;
    }

    while (fgets(buf, BUFSIZ, fd) != NULL) {
        line_nr++;

        if (i >= p_size) {
            WARNING("Maximum number of IP addresses (%d) exceeded", p_size);
            fclose(fd);
            return -1;
        }
        else {
            int scaned_items;
            scaned_items = sscanf(buf, "%s %s %d %s",node_name, interface,  &node_id, node_ip);
            if (scaned_items < 2) {
                WARNING("Skipped invalid line #%d in settings file '%s'", line_nr, path);
                continue;
            }
            if (node_id < 0 || node_id < FIRST_NODE_ID || node_id >= MAX_NODES) {
                fprintf(logfd, "Node id %d is not within the permitted range [%d, %d]\n", node_id, FIRST_NODE_ID, MAX_NODES);
                fclose(fd);
                return -1;
            }

            slash = strchr(node_ip, '/');
            if (slash) {
                *slash = 0;
            }
                
            prefix[node_id] = 32;
            if (slash) {
                slash++;
                if (!slash || !*slash) {
                    prefix[node_id] = 32;
                }
                prefix[node_id] = strtoul(slash, &ptr, 0);
                if (!ptr || ptr == slash || *ptr || *prefix > UINT_MAX) {
                    prefix[node_id] = 32;
                }
            }
            if ((p[node_id] = inet_addr(node_ip)) != INADDR_NONE) {
                i++;
            }
        }
    }

    fclose(fd);
    return i;
}

struct connection_list *
add_conn_list(struct connection_list *conn_list, int32_t src_id, int32_t dst_id)
{
    uint16_t rec_i = 1;

    if (conn_list == NULL) {
        conn_list = malloc(sizeof (struct connection_list));
        if (conn_list == NULL) {
            perror("malloc");
            exit(1);
        }
    }
    else {
        rec_i = conn_list->rec_i + 1;
        if ((conn_list->next_ptr = malloc(sizeof (struct connection_list))) == NULL) {
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
create_conn_list(FILE *conn_fd, int32_t all_node_cnt)
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
            fprintf(logfd, "Invalid Parameters");
            exit(1);
        }

        if (src_id > all_node_cnt) {
            fprintf(logfd, "Source node id too big: %d > %d\n", src_id, all_node_cnt);
            exit(1);
        }
        else if (dst_id > all_node_cnt) {
            fprintf(logfd, "Destination node id too big: %d > %d\n", dst_id, all_node_cnt);
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
meteor_loop(FILE *qomet_fd, int dsock, bin_hdr_cls bin_hdr, bin_rec_cls *bin_recs, int32_t all_node_cnt, int32_t node_cnt, int direction, struct connection_list *conn_list_head)
{
    int time_i, node_i, ret;
    int32_t next_hop_id;
    int32_t bin_recs_max_cnt;
    uint32_t bin_hdr_if_num;
    float crt_record_time = 0.0;
    double bandwidth, delay, lossrate;
    bin_time_rec_cls bin_time_rec;
    bin_rec_cls **my_recs_ucast = NULL;
    bin_rec_cls *adjusted_recs_ucast = NULL;
    struct timer_handle *timer;
    int *my_recs_ucast_changed = NULL;
    bin_rec_cls *my_recs_bcast = NULL;
    int *my_recs_bcast_changed = NULL;
    struct connection_list *conn_list = NULL;
 
    if ((timer = timer_init_rdtsc()) == NULL) {
        fprintf(logfd, "Could not initialize timer");
        exit(1);
    }

    if (all_node_cnt != bin_hdr.interface_number) {
        fprintf(logfd, "Number of nodes according to the settings file (%d) "
                "and number of nodes according to QOMET scenario (%d) differ", 
                all_node_cnt, bin_hdr.interface_number);
        exit(1);
    }

    bin_recs_max_cnt = bin_hdr.interface_number * (bin_hdr.interface_number - 1);
    if (bin_recs == NULL) {
        bin_recs = (bin_rec_cls *)calloc(bin_recs_max_cnt, sizeof (bin_rec_cls));
    }
    if (bin_recs == NULL) {
        WARNING("Cannot allocate memory for binary records");
        exit(1);
    }

    if (my_recs_ucast == NULL) {
        my_recs_ucast = (bin_rec_cls**)calloc(bin_hdr.interface_number, sizeof (bin_rec_cls *));
    }
    if (my_recs_ucast == NULL) {
        fprintf(logfd, "Cannot allocate memory for my_recs_ucast\n");
        exit(1);
    }

    for (node_i = 0; node_i < bin_hdr.interface_number; node_i++) {
        if (my_recs_ucast[node_i] == NULL) {
            my_recs_ucast[node_i] = (bin_rec_cls *)calloc(bin_hdr.interface_number, sizeof (bin_rec_cls));
        }
        if (my_recs_ucast[node_i] == NULL) {
            fprintf(logfd, "Cannot allocate memory for my_recs_ucast[%d]\n", node_i);
            exit(1);
        }

        int node_j;
        for (node_j = 0; node_j < bin_hdr.interface_number; node_j++) {
            my_recs_ucast[node_i][node_j].bandwidth = UNDEFINED_BANDWIDTH;
        }
    }

    if (direction == DIRECTION_HV || direction == DIRECTION_BR) {
        bin_hdr_if_num = bin_hdr.interface_number * bin_hdr.interface_number;
    }
    else {
        bin_hdr_if_num = bin_hdr.interface_number;
    }
    if (adjusted_recs_ucast == NULL) {
        adjusted_recs_ucast = (bin_rec_cls *)calloc(bin_hdr_if_num, sizeof (bin_rec_cls));
        if (adjusted_recs_ucast == NULL) {
            fprintf(logfd, "Cannot allocate memory for adjusted_recs_ucast");
            exit(1);
        }
    }

    if (my_recs_ucast_changed == NULL) {
        my_recs_ucast_changed = (int32_t *)calloc(bin_hdr_if_num, sizeof (int32_t));
        if (my_recs_ucast_changed == NULL) {
            fprintf(logfd, "Cannot allocate memory for my_recs_ucast_changed\n");
            exit(1);
        }
    }

    if (my_recs_bcast == NULL && use_mac_addr == FALSE) {
        my_recs_bcast = (bin_rec_cls *)calloc(bin_hdr_if_num, sizeof (bin_rec_cls));
        if (my_recs_bcast == NULL) {
            fprintf(logfd, "Cannot allocate memory for my_recs_bcast\n");
            exit(1);
        }
        for (node_i = 0; node_i < bin_hdr.interface_number; node_i++) {
            my_recs_bcast[node_i].bandwidth = UNDEFINED_BANDWIDTH;
        }
        my_recs_bcast_changed = (int32_t *)calloc(bin_hdr_if_num, sizeof (int32_t));
        if (my_recs_bcast_changed == NULL) {
            fprintf(logfd, "Cannot allocate memory for my_recs_bcast_changed\n");
            exit(1);
        }
    }

emulation_start:
    for (time_i = 0; time_i < bin_hdr.time_record_number; time_i++) {
        int rec_i;

        if (verbose_level >= 2) {
            fprintf(logfd, "Reading QOMET data from file... Time : %d/%d\n", time_i, bin_hdr.time_record_number);
        }

        if (io_binary_read_time_record_from_file(&bin_time_rec, qomet_fd) == ERROR) {
            fprintf(logfd, "Aborting on input error (time record)\n");
            exit (1);
        }
        //io_binary_print_time_record(&bin_time_rec, logfd);
        crt_record_time = bin_time_rec.time;

        if (bin_time_rec.record_number > bin_recs_max_cnt) {
            fprintf(logfd, "The number of records to be read exceeds allocated size (%d)\n", bin_recs_max_cnt);
            exit (1);
        }

        if (io_binary_read_records_from_file(bin_recs, bin_time_rec.record_number, qomet_fd) == ERROR) {
            fprintf(logfd, "Aborting on input error (records)\n");
            exit (1);
        }

        for (rec_i = 0; rec_i < bin_time_rec.record_number; rec_i++) {
            if (bin_recs[rec_i].from_id < FIRST_NODE_ID) {
                INFO("Source with id = %d is smaller first node id : %d", bin_recs[rec_i].from_id, assign_id);
                exit(1);
            }
            if (bin_recs[rec_i].from_id > bin_hdr.interface_number) {
                INFO("Source with id = %d is out of the valid range [%d, %d] rec_i : %d\n", 
                    bin_recs[rec_i].from_id, assign_id, bin_hdr.interface_number + assign_id - 1, rec_i);
                exit(1);
            }

            int32_t src_id;
            int32_t dst_id;
            int32_t next_hop_id;

            if (direction == DIRECTION_HV || direction == DIRECTION_BR) {
                src_id = bin_recs[rec_i].from_id;
                dst_id = bin_recs[rec_i].to_id;
                next_hop_id = src_id * all_node_cnt + dst_id;

                io_binary_copy_record(&(my_recs_ucast[src_id][dst_id]), &bin_recs[rec_i]);
                my_recs_ucast_changed[next_hop_id] = TRUE;

                if (verbose_level >= 3) {
                    io_binary_print_record(&(my_recs_ucast[src_id][dst_id]), logfd);
                }
            }
            else if (direction == DIRECTION_IN && bin_recs[rec_i].to_id == my_id) {
                src_id = bin_recs[rec_i].to_id;
                dst_id = bin_recs[rec_i].from_id;
                next_hop_id = src_id * all_node_cnt + dst_id;

                io_binary_copy_record(&(my_recs_ucast[src_id][dst_id]), &bin_recs[rec_i]);
                my_recs_ucast_changed[bin_recs[rec_i].to_id] = TRUE;

                if(verbose_level >= 3) {
                    io_binary_print_record(&(my_recs_ucast[src_id][dst_id]), logfd);
                }
            }

            if (use_mac_addr == FALSE && (bin_recs[rec_i].to_id == my_id || direction == DIRECTION_HV || direction == DIRECTION_BR)) {
                io_binary_copy_record(&(my_recs_bcast[bin_recs[rec_i].from_id]), &bin_recs[rec_i]);
                my_recs_bcast_changed[bin_recs[rec_i].from_id] = TRUE;
                //io_binary_print_record (&(my_recs_bcast[bin_recs[rec_i].from_node]), logfd);
            }
        }

        if (time_i == 0 && re_flag == -1) {
            uint32_t rec_index;
            if (direction == DIRECTION_BR) {
                int32_t src_id, dst_id;
                conn_list = conn_list_head;
                while (conn_list != NULL) {
                    src_id = conn_list->src_id;
                    dst_id = conn_list->dst_id;
                    rec_index = conn_list->rec_i;
                    io_binary_copy_record(&(adjusted_recs_ucast[rec_index]), &(my_recs_ucast[src_id][dst_id]));
                    DEBUG("Copied my_recs_ucast to adjusted_recs_ucast (index is rec_i=%d).", rec_index);

                    conn_list = conn_list->next_ptr;
                }
            }
            else  if (direction == DIRECTION_HV) {
                int32_t src_id, dst_id;
                for (src_id = assign_id; src_id < all_node_cnt; src_id += division) {
                    for (dst_id = 0; dst_id < all_node_cnt; dst_id++) {
                        rec_index = src_id * all_node_cnt + dst_id;
                        io_binary_copy_record(&(adjusted_recs_ucast[rec_index]), &(my_recs_ucast[src_id][dst_id]));
                        DEBUG("Copied my_recs_ucast to adjusted_recs_ucast (index is rec_i=%d).", rec_index);
                    }
                }
            }
            else {
                for (rec_i = FIRST_NODE_ID; rec_i < all_node_cnt; rec_i++) {
                     if (rec_i != my_id) {
                        io_binary_copy_record(&(adjusted_recs_ucast[rec_i]), &(my_recs_ucast[my_id][rec_i]));
                        DEBUG("Copied my_recs_ucast to adjusted_recs_ucast (index is rec_i=%d).", rec_i);
                    }
                }
            }
        }
        else {
            uint32_t rec_index;
            INFO("Adjustment of deltaQ is disabled.");
            if (direction == DIRECTION_BR) {
                int32_t src_id, dst_id;
                conn_list = conn_list_head;
                while (conn_list != NULL) {
                    src_id = conn_list->src_id;
                    dst_id = conn_list->dst_id;
                    rec_index = conn_list->rec_i;
                    INFO("index: %d src: %d dst: %d delay: %f\n", rec_index, src_id, dst_id, my_recs_ucast[src_id][dst_id].delay);
                    io_binary_copy_record(&(adjusted_recs_ucast[rec_index]), &(my_recs_ucast[src_id][dst_id]));
                    DEBUG("Copied my_recs_ucast to adjusted_recs_ucast (index is rec_i=%d).", rec_index);

                    conn_list = conn_list->next_ptr;
                }
            }
            else if (direction == DIRECTION_HV) {
                int32_t src_id, dst_id;
                for (src_id = assign_id; src_id < all_node_cnt; src_id += division) {
                    for (dst_id = 0; dst_id < all_node_cnt; dst_id++) {
                        if (src_id <= dst_id) {
                            break;
                        }
                        rec_index = src_id * all_node_cnt + dst_id;
                        io_binary_copy_record(&(adjusted_recs_ucast[rec_index]), &(my_recs_ucast[src_id][dst_id]));
                        INFO("index: %d src: %d dst: %d delay: %f\n", rec_index, src_id, dst_id, my_recs_ucast[src_id][dst_id].delay);
                        DEBUG("Copied my_recs_ucast to adjusted_recs_ucast (index is rec_i=%d).\n", rec_index);
                    }
                }
            }
            else {
                int32_t dst_id;
                for (dst_id = FIRST_NODE_ID; dst_id < all_node_cnt; dst_id++) {
                    if (dst_id != my_id) {
                        io_binary_copy_record(&(adjusted_recs_ucast[dst_id]), &(my_recs_ucast[my_id][dst_id]));
                        DEBUG("Copied my_recs_ucast to adjusted_recs_ucast (index is rec_i=%d).", dst_id);
                    }
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
                    WARNING("Timer deadline missed at time=%.6f s", crt_record_time);
                    WARNING("This rule is skip.\n");
                    continue;
                }
                if (ret == 2) {
                    fseek(qomet_fd, 0L, SEEK_SET);
                    if ((timer = timer_init_rdtsc()) == NULL) {
                        fprintf(logfd, "Could not initialize timer\n");
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
                    fprintf(logfd, "Timer deadline missed at time=%.2f s\n", crt_record_time);
                }
*/
            }

            int32_t src_id, dst_id;
            uint32_t rec_index;
            int32_t conf_rule_num;
            if (direction == DIRECTION_BR) {
                conn_list = conn_list_head;
                while (conn_list != NULL) {
                    src_id = conn_list->src_id;
                    dst_id = conn_list->dst_id;
                    rec_index = src_id * all_node_cnt + dst_id;
                    
                    if (my_recs_ucast_changed[rec_index] == FALSE) {
                        conn_list = conn_list->next_ptr;
                        continue;
                    }

                    next_hop_id = conn_list->rec_i;
                    conf_rule_num = next_hop_id + MIN_PIPE_ID_BR;

                    bandwidth = adjusted_recs_ucast[next_hop_id].bandwidth / 8;
                    delay = adjusted_recs_ucast[next_hop_id].delay;
                    lossrate = adjusted_recs_ucast[next_hop_id].loss_rate;

                    ret = configure_rule(dsock, conf_rule_num, bandwidth, delay, lossrate);
                    if (ret != SUCCESS) {
                        fprintf(logfd, "Error: UCAST rule %d. Error Code %d\n", conf_rule_num, ret);
                        exit(1);
                    }
                    my_recs_ucast_changed[next_hop_id] = FALSE;
                    if (re_flag == TRUE) {
                        fseek(qomet_fd, 0L, SEEK_SET);
                        if ((timer = timer_init_rdtsc()) == NULL) {
                            WARNING("Could not initialize timer");
                            exit(1);
                        }
                        io_binary_read_header_from_file(&bin_hdr, qomet_fd);
                        if (verbose_level >= 1) {
                            io_binary_print_header(&bin_hdr);
                        }
                        re_flag = FALSE;
                        goto emulation_start;
                    }
                    conn_list = conn_list->next_ptr;
                }
            }
            else if (direction == DIRECTION_HV) {
                for (src_id = assign_id; src_id < all_node_cnt; src_id += division) {
                    for (dst_id = 0; dst_id < all_node_cnt; dst_id++) {
                         if (src_id <= dst_id) {
                            break;
                        }
                        next_hop_id = src_id * all_node_cnt + dst_id;
                        conf_rule_num = (src_id - assign_id) * all_node_cnt + dst_id + MIN_PIPE_ID_OUT;

                        if (my_recs_ucast_changed[next_hop_id] == FALSE) {
                            continue;
                        }

                        bandwidth = adjusted_recs_ucast[next_hop_id].bandwidth;
                        delay = adjusted_recs_ucast[next_hop_id].delay;
                        lossrate = adjusted_recs_ucast[next_hop_id].loss_rate;

                        ret = configure_rule(dsock, conf_rule_num, bandwidth, delay, lossrate);
                        if (ret != SUCCESS) {
                            fprintf(logfd, "Error: rule %d. Error Code %d\n", conf_rule_num, ret);
                            exit(1);
                        }
                        my_recs_ucast_changed[next_hop_id] = FALSE;
                        if (re_flag == TRUE) {
                            fseek(qomet_fd, 0L, SEEK_SET);
                            if ((timer = timer_init_rdtsc()) == NULL) {
                                WARNING("Could not initialize timer");
                                exit(1);
                            }
                            io_binary_read_header_from_file(&bin_hdr, qomet_fd);
                            if (verbose_level >= 1) {
                                io_binary_print_header(&bin_hdr);
                            }
                            re_flag = FALSE;
                            goto emulation_start;
                        }
                    }
                }
            }
            else {
                for (src_id = FIRST_NODE_ID; src_id < all_node_cnt; src_id++) {
                    if (src_id == my_id) {
                        continue;
                    }

                    if (src_id < 0 || (src_id > bin_hdr.interface_number - 1)) {
                        WARNING("Next hop with id = %d is out of the valid range [%d, %d]", bin_recs[rec_i].to_id, 0, bin_hdr.interface_number - 1);
                        exit(1);
                    }

                    bandwidth = adjusted_recs_ucast[src_id].bandwidth;
                    delay = adjusted_recs_ucast[src_id].delay;
                    lossrate = adjusted_recs_ucast[src_id].loss_rate;

                    if (bandwidth != UNDEFINED_BANDWIDTH) {
                        INFO ("-- Meteor id = %d #%d to me (time=%.2f s): bandwidth=%.2fbit/s lossrate=%.4f delay=%.4f ms",
                            MIN_PIPE_ID_OUT + src_id, src_id, crt_record_time, bandwidth, lossrate, delay);
                    }
                    else {
                        INFO ("-- Meteor id = %d #%d to me (time=%.2f s): no valid record could be found => configure with no degradation", 
                            MIN_PIPE_ID_OUT + src_id, src_id, crt_record_time);
                    }
                    ret = configure_rule(dsock, MIN_PIPE_ID_OUT + src_id, bandwidth, delay, lossrate);
                    if (ret != SUCCESS) {
                        WARNING("Error configuring Meteor rule %d.", MIN_PIPE_ID_OUT + src_id);
                        exit (1);
                    }
                }
            }
            if (direction != DIRECTION_HV && direction != DIRECTION_BR && direction != DIRECTION_IN) {
                for (node_i = assign_id; node_i < (node_cnt + assign_id); node_i++) {
                    if (node_i == my_id) {
                        continue;
                    }
                        
                    if (use_mac_addr == TRUE) {
                        continue;
                    }
    
                    bandwidth = my_recs_bcast[node_i].bandwidth;
                    delay = my_recs_bcast[node_i].delay;
                    lossrate = my_recs_bcast[node_i].loss_rate;
    
                    DEBUG("Used contents of my_recs_bcast for deltaQ parameters (index is node_i=%d).", node_i);
                    //io_binary_print_record (&(my_recs_bcast[node_i]));
                }
    
                if (configure_rule(dsock, MIN_PIPE_ID_IN_BCAST + node_i, bandwidth, delay, lossrate) == ERROR) {
                    WARNING("Error configuring BCAST pipe %d.", MIN_PIPE_ID_IN_BCAST + node_i);
                    exit (1);
                }
            }
        }
        fflush(logfd);
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
    {"assign_id", required_argument, NULL, 'a'},
    {"broadcast", required_argument, NULL, 'b'},
    {"connection", required_argument, NULL, 'c'},
    {"mode", required_argument, NULL, 'm'},
    {"daemon", no_argument, NULL, 'd'},
    {"division", required_argument, NULL, 'D'},
    {"exec", required_argument, NULL, 'e'},
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
    char *p;
    char *saddr, *daddr;
    char settings_file_name[BUFSIZ];
    char *exec_file;
    int32_t ret;
    int32_t node_i;
    int32_t dsock;
    int32_t node_cnt = 0;
    int32_t all_node_cnt;
    int daemon_flag = FALSE;
    bin_hdr_cls bin_hdr;
    bin_rec_cls *bin_recs = NULL;

  
    FILE *qomet_fd;
    FILE *conn_fd;

    int32_t i, j;
    in_addr_t ipaddrs[MAX_NODES];
    int ipprefix[MAX_NODES];
    char ipaddrs_c[MAX_NODES * IP_ADDR_SIZE];

    int32_t offset_num;
    int32_t rule_cnt;
    int32_t next_hop_id, rule_num;
    int32_t protocol = IP;
    float next_time;
    char baddr[IP_ADDR_SIZE];

    struct sigaction sa;
    struct connection_list *conn_list = NULL;
    struct connection_list *conn_list_head = NULL;

    memset(&device_list, 0, sizeof (struct DEVICE_LIST) * MAX_IFS);
    memset(&exec_file, 0, sizeof (char) * 255);

    memset(&sa, 0, sizeof (struct sigaction));
    sa.sa_handler = &restart_scenario;
    sa.sa_flags |= SA_RESTART;

    if (sigaction(SIGUSR1, &sa, NULL) != 0) {
        fprintf(logfd, "Signal Set Error\n");
        exit(1);
    }

    if_num = 0;
    rule_cnt = 0;
    next_time = 0.0;
    next_hop_id = 0;
    rule_num = -1;
    qomet_fd = NULL;
    saddr = daddr = NULL;
    direction = DIRECTION_IN;

    //my_id = -1;
    all_node_cnt = -1;
    strncpy(baddr, "255.255.255.255", IP_ADDR_SIZE);

    i = 0;
    char ch;
    int index;
    while ((ch = getopt_long(argc, argv, "a:b:Bc:dD:e:hi:I:lL:m:Mq:s:v", options, &index)) != -1) {
        switch (ch) {
            case 'a':
                assign_id = strtol(optarg, &p, 10);
                break;
            case 'b':
                strncpy(baddr, optarg, IP_ADDR_SIZE);
                break;
            case 'B':
		busy = 1;
                break;
            case 'c':
                conn_fd = fopen(optarg, "r");
                break;
            case 'd':
                daemon_flag = TRUE;
                break;
            case 'D':
                division = strtol(optarg, &p, 10);
                break;
            case 'e':
                exec_file = optarg;
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
            case 'L':
                logfile = optarg;
                logfd = fopen(logfile, "w");
                break;
            case 'm':
                if ((strcmp(optarg, "ingress") == 0) || strcmp(optarg, "in") == 0) {
                    direction = DIRECTION_IN;
                }
                else if (strcmp(optarg, "out") == 0) {
                    direction = DIRECTION_OUT;
                }
                else if ((strcmp(optarg, "hypervisor") == 0) ||(strcmp(optarg, "hv") == 0)) {
                    direction = DIRECTION_HV;
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
                protocol = ETH;
                break;
            case 'q':
                if ((qomet_fd = fopen(optarg, "rb")) == NULL) {
                    WARNING("Could not open QOMET output file '%s'", optarg);
                    exit(1);
                }
                break;
            case 's':
                strncpy(settings_file_name, optarg, MAX_STRING - 1);
                if ((all_node_cnt = read_settings(optarg, ipaddrs, ipprefix, MAX_NODES)) < 1) {
                    fprintf(stderr, "Settings file '%s' is invalid", optarg);
                    exit(1);
                }
                for (i = 0; i < all_node_cnt; i++) {
                    snprintf(ipaddrs_c + i * IP_ADDR_SIZE, IP_ADDR_SIZE, "%d.%d.%d.%d",
                        *(((uint8_t *)&ipaddrs[i]) + 0), *(((uint8_t *)&ipaddrs[i]) + 1),
                        *(((uint8_t *)&ipaddrs[i]) + 2), *(((uint8_t *)&ipaddrs[i]) + 3));
                }
                break;
            case 'v':
                verbose_level += 1;
                break;
            default:
                usage();
                exit(1);
        }
    }

    if (logfd == NULL) {
        logfd = stderr;
    }

    if (use_mac_addr == TRUE) {
        saddr = (char*)calloc(1, MAC_ADDR_SIZE);
        daddr = (char*)calloc(1, MAC_ADDR_SIZE);
    }
    else {
        saddr = (char*)calloc(1, IP_ADDR_SIZE);
        daddr = (char*)calloc(1, IP_ADDR_SIZE);
    }

    if (qomet_fd == NULL) {
        fprintf(logfd, "No QOMET data file was provided\n");
        usage();
        exit(1);
    }

    if (all_node_cnt - 1 < my_id) {
        fprintf(stderr, "Invalid ID\n");
        exit(1);
    }

    if (conn_fd != NULL && direction == DIRECTION_BR) {
        conn_list_head = create_conn_list(conn_fd, all_node_cnt);
    }
    else if (direction == DIRECTION_BR) {
        fprintf(stderr, "no connection file");
        usage();
        exit(1);
    }

    if (direction == DIRECTION_IN || direction == DIRECTION_OUT) {
        if (my_id == -1) {
            fprintf(stderr, "Please specify node id. option: -i ID\n");
            exit(1);
        }
    }

    unsigned char mac_addresses[MAX_NODES][ETH_SIZE];
    char mac_char_addresses[MAX_NODES][MAC_ADDR_SIZE];
    if (use_mac_addr == TRUE) {
        if ((node_cnt = io_read_settings_file_mac(settings_file_name, ipaddrs, ipaddrs_c, mac_addresses, mac_char_addresses, MAX_NODES)) < 1) {
            fprintf(stderr, "Invalid MAC address settings file: '%s'\n", settings_file_name);
            exit(1);
        }
        for (node_i = 0; node_i < MAX_NODES; node_i++) {
            ipaddrs[node_i] = node_i;
            sprintf(ipaddrs_c + (node_i * IP_ADDR_SIZE), "0.0.0.%d", node_i);
        }
    }
    else {
        if ((node_cnt = io_read_settings_file(settings_file_name, ipaddrs, ipaddrs_c, MAX_NODES)) < 1) {
            fprintf(stderr, "Invalid IP address settings file: '%s'\n", settings_file_name);
            exit(1);
        }
    }

    if (division > 0) {
        node_cnt = all_node_cnt / division;
    }
    else {
        fprintf(stderr, "division is not lower 0\n");
        exit(1);
    }

/*
    DEBUG("Initialize timer...");
    if ((timer = timer_init_rdtsc()) == NULL) {
        fprintf(stderr, "Could not initialize timer");
        exit(1);
    }
*/
    DEBUG("Open control socket...");
    if ((dsock = get_socket()) < 0) {
        WARNING("Could not open control socket (requires root priviledges)\n");
        exit(1);
    }

    if (daemon_flag == TRUE) {
        ret = daemon(0, 0);
    }

    create_ifb(my_id);
    init_rule(daddr, protocol, direction);

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
    else if (direction == DIRECTION_HV) {
        for (src_id = assign_id; src_id < all_node_cnt; src_id += division) {
            if (assign_id != src_id % division) {
                continue;
            }
            for (dst_id = 0; dst_id < all_node_cnt; dst_id++) {
                if (src_id <= dst_id && all_node_cnt == node_cnt) {
                    break;
                }
                if (src_id <= dst_id) {
                    break;
                }

                offset_num = (src_id - assign_id) * all_node_cnt + dst_id;
                rule_num = MIN_PIPE_ID_OUT + offset_num;
                strcpy(saddr, ipaddrs_c + src_id  * IP_ADDR_SIZE);
                strcpy(daddr, ipaddrs_c + dst_id * IP_ADDR_SIZE);

                ret = add_rule(dsock, rule_num, rule_num, protocol, saddr, daddr, direction);
                if (ret != SUCCESS) {
                    fprintf(stderr, "Node %d: Could not add rule #%d", src_id, rule_num);
                    exit(1);
                }
            }
        }
        free(saddr);
        free(daddr);
    }
    else {
        for (src_id = FIRST_NODE_ID; src_id < all_node_cnt; src_id++) {
            if (src_id == my_id) {
                continue;
            }
            offset_num = src_id;

            if (protocol == ETH) {
                strcpy(daddr, mac_char_addresses[src_id]);
            }
            else if (protocol == IP) {
                strcpy(daddr, ipaddrs_c + (src_id - assign_id) * IP_ADDR_SIZE);
                sprintf(daddr, "%s/%d", daddr, ipprefix[src_id]);
            }

            INFO("Node %d: Add rule #%d with pipe #%d to destination %s", 
                    my_id, MIN_PIPE_ID_OUT + offset_num, MIN_PIPE_ID_OUT + offset_num, daddr);

            if (add_rule(dsock, MIN_PIPE_ID_OUT + offset_num, MIN_PIPE_ID_OUT + offset_num, protocol, "any", daddr, direction) < 0) {
                fprintf(stderr, "Node %d: Could not add rule #%d with pipe #%d to \
                        destination %s\n", my_id, MIN_PIPE_ID_OUT + offset_num, 
                        MIN_PIPE_ID_OUT + offset_num, daddr);
                exit(1);
            }
        }
    }
    if (direction != DIRECTION_HV && direction != DIRECTION_BR) {
        int rulenum;
        for (j = assign_id; j < node_cnt + assign_id; j++) {
            if (j == my_id || direction != DIRECTION_HV) {
                continue;
            }
            offset_num = j; 
            rulenum = MIN_PIPE_ID_IN_BCAST + offset_num;

            INFO("Node %d: Add rule #%d to destination %s", my_id, rulenum, baddr);
            if (add_rule(dsock, rulenum, rulenum, protocol, ipaddrs_c + (j - assign_id) * IP_ADDR_SIZE, baddr, direction) < 0)
            {
                WARNING("Node %d: Could not add rule #%d from %s to destination %s", my_id, rule_num, ipaddrs_c + (j - assign_id) * IP_ADDR_SIZE, baddr);
                exit(1);
            }
            if (add_rule(dsock, rulenum, rulenum + 1, protocol, ipaddrs_c+(j-assign_id)*IP_ADDR_SIZE, baddr, direction) < 0)
            {
                WARNING("Node %d: Could not add rule #%d with pipe #%d from %s to destination %s", 
                        my_id, rulenum, rulenum + 1, ipaddrs_c + (j - assign_id) * IP_ADDR_SIZE, baddr);
                exit(1);
            }
        }
    }

    if (exec_file) {
        ret = system(exec_file);
        if (ret != 0) {
            fprintf(stderr, "failed system: %s\n", exec_file);
            exit(1);
        }
    }

    INFO("Reading QOMET data from file...");
    next_time = 0;

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
            case DIRECTION_HV:
                fprintf(stdout, "Direction Mode: HyperVisor\n");
                break;
            case DIRECTION_IN:
                fprintf(stdout, "Direction Mode: In\n");
                break;
            case DIRECTION_OUT:
                fprintf(stdout, "Direction Mode: Out\n");
                break;
        }
    }

    if (all_node_cnt != bin_hdr.interface_number) {
        fprintf(logfd, "Number of nodes according to the settings file (%d) "
                "and number of nodes according to QOMET scenario (%d) differ", 
                all_node_cnt, bin_hdr.interface_number);
        exit(1);
    }

    meteor_loop(qomet_fd, dsock, bin_hdr, bin_recs, all_node_cnt, node_cnt, direction, conn_list_head);

    if (logfd) {
        fprintf(logfd, "Emulation finished.\n");
        fclose(logfd);
    }

    delete_rule(dsock, daddr, rule_num);
    delete_ifb();
    close_socket(dsock);

    return 0;
}
