/************************************************************************
 *
 * Meteor Emulator Implementation
 *
 * Authors : Kunio AKASHI
 *
 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <signal.h>
#include <getopt.h>
#include <sched.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>

#include "global.h"
//#include "wireconf.h"
#include "message.h"
#include "statistics.h"
#include "timer.h"
#include "meteor.h"
#include "utils.h"
#include "config.hpp"
#include "libnlwrap.h"

int32_t re_flag = FALSE;

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
    fprintf(stderr, "Meteor. Wireless network emulator.\n\n");
    fprintf(stderr, "\tUsage: meteor -q <deltaQ_binary_file>"
            " -i <node_id> -s <settings_file>\n"
            "\t\t[-m <in|br>] [-M] [-I <Interface Name>] "
            "[-a <assign_id>] [-l] [-d] [-v]\n");

    fprintf(stderr, "\t-q, --qomet_scenario: Scenario file.\n");
    fprintf(stderr, "\t-i, --id: Own ID in QOMET scenario\n");
    fprintf(stderr, "\t-s, --settings: Setting file.\n");
    fprintf(stderr, "\t-m, --mode: ingress|hypervisor|bridge\n");
    fprintf(stderr, "\t-M, --use_mac_address: Use MAC Address filtering.\n");
    fprintf(stderr, "\t-I, --interface: Select physical interface.\n");
    fprintf(stderr, "\t-l, --loop: Scenario loop mode.\n");
    fprintf(stderr, "\t-d, --daemon: Daemon mode.\n");
    fprintf(stderr, "\t-v, --verbose: Verbose mode.\n");
}

void
restart_scenario()
{
    re_flag = TRUE;
}

struct meteor_config *
init_meteor_conf()
{
    struct meteor_config *meteor_conf;

    meteor_conf = malloc(sizeof (struct meteor_config));
    memset(meteor_conf, '0', sizeof (struct meteor_config));
    if (!meteor_conf) {
        fprintf(stderr, "[%s] Cannot allocate memory\n", __func__);
        exit(1);
    }
    meteor_conf->id          = -1;
    meteor_conf->loop        = FALSE;
    meteor_conf->direction   = INGRESS;
    meteor_conf->node_cnt    = -1;
    meteor_conf->verbose     = 0;
    meteor_conf->filter_mode = ETH_P_IP;
    meteor_conf->daemonize   = FALSE;
    meteor_conf->deltaq_fd   = NULL;
    meteor_conf->settings_fd = NULL;
    meteor_conf->logfd       = NULL;

    meteor_conf->bin_hdr  = malloc(sizeof (struct bin_hdr_cls));

    return meteor_conf;
}

int
timer_wait_rdtsc(struct timer_handle *handle, uint64_t time_in_us)
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

        usleep(100);
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
        fprintf(stderr, "[%s] /proc/cpuinfo file cannot open\n", __func__);
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
        WARNING("[%s] Could not allocate memory for the timer", __func__);
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
create_conn_list(struct meteor_config *meteor_conf)
{
    char buf[BUFSIZ];
    int32_t src_id;
    int32_t dst_id;
    struct connection_list *conn_list;
    struct connection_list *conn_list_head;

    conn_list = NULL;
    conn_list_head = NULL;
    while (fgets(buf, BUFSIZ, meteor_conf->connection_fd) != NULL) {
        if (sscanf(buf, "%d %d", &src_id, &dst_id) != 2) {
            fprintf(meteor_conf->logfd, "Invalid Parameters");
            exit(1);
        }

        if (src_id > meteor_conf->node_cnt) {
            fprintf(meteor_conf->logfd, "Source id too big: %d > %d\n",
                            src_id, meteor_conf->node_cnt);
            exit(1);
        }
        else if (dst_id > meteor_conf->node_cnt) {
            fprintf(meteor_conf->logfd, "Destination id too big: %d > %d\n",
                            dst_id, meteor_conf->node_cnt);
            exit(1);
        }

        conn_list = add_conn_list(conn_list, src_id, dst_id);
        if (!conn_list_head) {
            conn_list_head = conn_list;
        }
    }

    return conn_list_head;
}

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
init_rule(struct meteor_config *meteor_conf)
{
    struct nl_sock *sock = meteor_conf->nlsock;
    int pif_index = meteor_conf->pif_index;
    int ifb_index = meteor_conf->ifb_index;

    delete_qdisc(sock, pif_index, TC_H_INGRESS, 0);
    add_ingress_qdisc(sock, pif_index);
    add_mirred_filter(sock, pif_index, ifb_index);

    delete_qdisc(sock, ifb_index, TC_H_ROOT, 0);

    add_htb_qdisc(sock, ifb_index, TC_H_ROOT, TC_HANDLE(1, 0), 65535);
    add_htb_class(sock, ifb_index, TC_HANDLE(1, 0), TC_HANDLE(1, 1), 1, DEF_BW);

    add_htb_class(sock, ifb_index, TC_HANDLE(1, 0), 
            TC_HANDLE(1, 1), 1, DEF_BW);
    add_htb_class(sock, ifb_index, TC_HANDLE(1, 0), 
            TC_HANDLE(1, 65535), 1, DEF_BW);

    int delay     = DEF_DELAY; // us
    int jitter    = DEF_DELAY; // us
    uint32_t loss = DEF_LOSS;  // %
    add_netem_qdisc(sock, meteor_conf->ifb_index, 
            TC_HANDLE(1, 65535), TC_HANDLE(65535, 0),
            delay, jitter, loss, 1000);

    return 0;
}

int
add_rule(struct meteor_config *meteor_conf, uint16_t parent, uint16_t handle, 
        struct node_data *src, struct node_data *dst)
{
    struct nl_sock *sock = meteor_conf->nlsock;
    int idx = meteor_conf->ifb_index;

    add_htb_class(sock, idx, TC_HANDLE(1, 0), TC_HANDLE(1, parent), 1, DEF_BW);

    if (meteor_conf->filter_mode == ETH_P_ALL) {
        if (!dst) {
            add_class_macfilter(sock, idx,
                    TC_HANDLE(1, 0), TC_HANDLE(1, parent), 
                    src->mac, NULL);
        }
        else {
            add_class_macfilter(sock, idx, TC_HANDLE(1, 0), TC_HANDLE(1, parent), 
                    src->mac, dst->mac);
        }
    }
    else if (meteor_conf->filter_mode == ETH_P_IP) {
        if (!dst) {
            add_class_ipv4filter(sock, idx,
                    TC_HANDLE(1, 0), TC_HANDLE(1, parent), 
                    src->ipv4addr.s_addr, src->ipv4prefix, 0, 0);
        }
        else {
            add_class_ipv4filter(sock, idx,
                    TC_HANDLE(1, 0), TC_HANDLE(1, parent), 
                    src->ipv4addr.s_addr, src->ipv4prefix,
                    dst->ipv4addr.s_addr, dst->ipv4prefix);
        }
    }

    int delay = 0 ;
    int jitter = 0;
    uint32_t loss = 100; // %
    add_netem_qdisc(sock, idx, TC_HANDLE(1, parent), TC_HANDLE(parent, 0),
            delay, jitter, loss, 1000);

    return 0;
}

int32_t
configure_rule(struct nl_sock *sock, int ifb_index,
        uint16_t parent, uint16_t handle, 
        int32_t bandwidth, double delay, double loss)
{
    change_htb_class(sock, ifb_index,
            TC_HANDLE(1, 0), TC_HANDLE(1, parent), 1, bandwidth);
    change_netem_qdisc(sock, ifb_index,
            TC_HANDLE(1, parent), TC_HANDLE(parent, 0), delay, 0, loss, 1000);
 
    return 0;
}

void
finalize_rule(struct meteor_config *meteor_conf)
{
    delete_qdisc(meteor_conf->nlsock, meteor_conf->ifb_index, TC_H_ROOT, 0);
    delete_qdisc(meteor_conf->nlsock, meteor_conf->pif_index, TC_H_INGRESS, 0);
    delete_ifb(meteor_conf->nlsock, meteor_conf->ifb_index);

    nl_close(meteor_conf->nlsock);
}

int
meteor_loop(struct meteor_config *meteor_conf)
{
    int node_i, ret;
    int32_t bin_recs_max_cnt;
    uint32_t bin_hdr_if_num;
    float crt_record_time = 0.0;
    double bandwidth, delay, lossrate;
    struct bin_time_rec_cls bin_time_rec;
    struct bin_rec_cls *bin_recs_all = NULL;
    struct bin_rec_cls **recs_ucast = NULL;
    struct bin_rec_cls *adjusted_recs_ucast = NULL;
    struct timer_handle *timer;
    int *recs_ucast_changed = NULL;
    struct connection_list *conn_list = NULL;
 
    struct node_data *node;
    struct node_data *my_node;
    struct bin_hdr_cls *bin_hdr = meteor_conf->bin_hdr;

    for (node = meteor_conf->node_list_head; node->id < meteor_conf->node_cnt; node++) {
        if (node->id == meteor_conf->id) {
            my_node = node;
        }
    }

    if (!(timer = timer_init_rdtsc())) {
        fprintf(meteor_conf->logfd, "[%s] Could not initialize timer", __func__);
        exit(1);
    }

    bin_recs_max_cnt = bin_hdr->if_num * (bin_hdr->if_num - 1);
    bin_recs_all = (struct bin_rec_cls *)calloc(bin_recs_max_cnt, sizeof (struct bin_rec_cls));
    if (!bin_recs_all) {
        WARNING("[%s] Cannot allocate memory for binary records", __func__);
        exit(1);
    }

    recs_ucast = (struct bin_rec_cls**)calloc(bin_hdr->if_num, sizeof (struct bin_rec_cls*));
    if (!recs_ucast) {
        fprintf(meteor_conf->logfd, "[%s] Cannot allocate memory for recs_ucast\n", __func__);
        exit(1);
    }

    for (node_i = 0; node_i < bin_hdr->if_num; node_i++) {
        if (!recs_ucast[node_i]) {
            recs_ucast[node_i] = (struct bin_rec_cls *)calloc(bin_hdr->if_num, sizeof (struct bin_rec_cls));
        }
        if (!recs_ucast[node_i]) {
            fprintf(meteor_conf->logfd, "[%s] Cannot allocate memory for recs_ucast[%d]\n",
                            __func__, node_i);
            exit(1);
        }

        int node_j;
        for (node_j = 0; node_j < bin_hdr->if_num; node_j++) {
            recs_ucast[node_i][node_j].bandwidth = UNDEFINED_BANDWIDTH;
        }
    }

    if (meteor_conf->direction == BRIDGE) {
        bin_hdr_if_num = bin_hdr->if_num * bin_hdr->if_num;
    }
    else {
        bin_hdr_if_num = bin_hdr->if_num;
    }

    if (!adjusted_recs_ucast) {
        adjusted_recs_ucast = (struct bin_rec_cls *)calloc(bin_hdr_if_num, sizeof (struct bin_rec_cls));
        if (adjusted_recs_ucast == NULL) {
            fprintf(meteor_conf->logfd, "[%s] Cannot allocate memory for adjusted_recs_ucast", __func__);
            exit(1);
        }
    }

    if (!recs_ucast_changed) {
        recs_ucast_changed = (int32_t *)calloc(bin_hdr_if_num, sizeof (int32_t));
        if (recs_ucast_changed == NULL) {
            fprintf(meteor_conf->logfd, "Cannot allocate memory for recs_ucast_changed\n");
            exit(1);
        }
    }

emulation_start:
    for (int time_i = 0; time_i < bin_hdr->time_rec_num; time_i++) {
        int rec_i;

        if (meteor_conf->verbose >= 2) {
            printf("Reading QOMET data from file... Time : %d/%d\n",
                    time_i, bin_hdr->time_rec_num);
        }

        if (io_binary_read_time_record_from_file(&bin_time_rec, meteor_conf->deltaq_fd) == ERROR) {
            fprintf(meteor_conf->logfd, "Aborting on input error (time record)\n");
            exit (1);
        }
        io_binary_print_time_record(&bin_time_rec);
        crt_record_time = bin_time_rec.time;

        if (bin_time_rec.record_number > bin_recs_max_cnt) {
            fprintf(meteor_conf->logfd, "The number of records to be read exceeds allocated size (%d)\n", bin_recs_max_cnt);
            exit (1);
        }

        if (io_binary_read_records_from_file(bin_recs_all, bin_time_rec.record_number, meteor_conf->deltaq_fd) == ERROR) {
            fprintf(meteor_conf->logfd, "Aborting on input error (records)\n");
            exit (1);
        }

        for (rec_i = 0; rec_i < bin_time_rec.record_number; rec_i++) {
            if (bin_recs_all[rec_i].from_id < FIRST_NODE_ID) {
                INFO("Source with id = %d is smaller first node id : %d", bin_recs_all[rec_i].from_id, assign_id);
                exit(1);
            }
            if (bin_recs_all[rec_i].from_id > bin_hdr->if_num) {
                INFO("Source with id = %d is out of the valid range [%d, %d] rec_i : %d\n", 
                    bin_recs_all[rec_i].from_id, assign_id, bin_hdr->if_num + assign_id - 1, rec_i);
                exit(1);
            }

            int32_t src_id;
            int32_t dst_id;
            if (meteor_conf->direction == INGRESS) {
                if (bin_recs_all[rec_i].to_id == meteor_conf->id || bin_recs_all[rec_i].from_id == meteor_conf->id) {
                    src_id = bin_recs_all[rec_i].to_id;
                    dst_id = bin_recs_all[rec_i].from_id;

                    io_bin_cp_rec(&(recs_ucast[src_id][dst_id]), &bin_recs_all[rec_i]);
                    io_bin_cp_rec(&(recs_ucast[dst_id][src_id]), &bin_recs_all[rec_i]);
                    recs_ucast_changed[bin_recs_all[rec_i].to_id] = TRUE;

                    if(meteor_conf->verbose >= 3) {
                        io_binary_print_record(&(recs_ucast[src_id][dst_id]));
                    }
                }
            }
            else if (meteor_conf->direction == BRIDGE) {
            }
        }

        if (time_i == 0 && re_flag == -1) {
            if (meteor_conf->direction == BRIDGE) {
                uint32_t rec_index;
                int32_t src_id, dst_id;
                conn_list = meteor_conf->conn_list_head;
                while (conn_list) {
                    src_id = conn_list->src_id;
                    dst_id = conn_list->dst_id;
                    rec_index = conn_list->rec_i;
                    io_bin_cp_rec(&(adjusted_recs_ucast[rec_index]), &(recs_ucast[src_id][dst_id]));
                    DEBUG("Copied recs_ucast to adjusted_recs_ucast (index is rec_i=%d).", rec_index);

                    conn_list = conn_list->next_ptr;
                }
            }
            else {
                int i;
                node = meteor_conf->node_list_head;
                for (i = 1; i < meteor_conf->node_cnt; i++) {
                     if (node->id != meteor_conf->id) {
                        printf("node->id => %d\n", node->id);
                        io_bin_cp_rec(&(adjusted_recs_ucast[node->id]), &(recs_ucast[meteor_conf->id][node->id]));
                        DEBUG("Copied recs_ucast to adjusted_recs_ucast (index is rec_i=%d).", node->id);
                    }
                    node++;
                }
            }
        }
        else {
            INFO("Adjustment of deltaQ is disabled.");
            if (meteor_conf->direction == BRIDGE) {
                //uint32_t rec_index;
            }
            else {
                int i;
                node = meteor_conf->node_list_head;
                for (i = 0 ; i < meteor_conf->node_cnt; i++) {
                    if (node->id != meteor_conf->id) {
                        io_bin_cp_rec(&(adjusted_recs_ucast[node->id]), &(recs_ucast[meteor_conf->id][node->id]));
                        DEBUG("Copied recs_ucast to adjusted_recs_ucast (index is rec_i=%d).", node->id);
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
                INFO("Waiting to reach time %.6fs...", crt_record_time);
            }
            else {
                INFO("Waiting to reach real time %.6fs (scenario time %.6f)\n",
                        crt_record_time * SCALING_FACTOR, crt_record_time);

                if ((ret = timer_wait_rdtsc(timer, crt_record_time * 1000000)) < 0) {
                    fprintf(meteor_conf->logfd, 
                            "Timer deadline missed at time=%.6f s ",
                            crt_record_time);
                    fprintf(meteor_conf->logfd, "This rule is skip.\n");
                    continue;
                }
                if (ret == 2) {
                    fseek(meteor_conf->deltaq_fd, 0L, SEEK_SET);
                    if ((timer = timer_init_rdtsc()) == NULL) {
                        fprintf(meteor_conf->logfd, "Could not initialize timer\n");
                        exit(1);
                    }
                    io_binary_read_header_from_file(bin_hdr, meteor_conf->deltaq_fd);
                    if (meteor_conf->verbose >= 1) {
                        io_binary_print_header(bin_hdr);
                    }
                    re_flag = FALSE;
                    goto emulation_start;
                }
/*
                if (timer_wait(timer, crt_record_time * SCALING_FACTOR) != 0) {
                    fprintf(meteor_conf->logfd, "Timer deadline missed at time=%.2f s\n", crt_record_time);
                }
*/
            }

            if (meteor_conf->direction == BRIDGE) {
            }
            else {
                int i;
                node = meteor_conf->node_list_head;
                for (i = 0; i < meteor_conf->node_cnt; i++) {
                    if (node->id == meteor_conf->id) {
                        node++;
                        continue;
                    }

                    if (node->id < 0 || (node->id > bin_hdr->if_num - 1)) {
                        WARNING("Next hop with id = %d is out of the valid range [%d, %d]",
                                bin_recs_all[node->id].to_id, 0, bin_hdr->if_num - 1);
                        exit(1);
                    }

                    bandwidth = adjusted_recs_ucast[node->id].bandwidth;
                    delay = adjusted_recs_ucast[node->id].delay * 1000;
                    lossrate = adjusted_recs_ucast[node->id].loss_rate * 100;

                    if (bandwidth != UNDEFINED_BANDWIDTH) {
                        INFO("-- Meteor id = %d #%d to me (time=%.2f s): bandwidth=%.2fbit/s lossrate=%.4f delay=%.4f ms",
                            meteor_conf->id, node->id, crt_record_time, bandwidth, lossrate, delay);
                    }
                    else {
                        INFO("-- Meteor id = %d #%d to me (time=%.2f s): no valid record could be found => configure with no degradation", 
                            meteor_conf->id, node->id, crt_record_time);
                    }
                    ret = configure_rule(meteor_conf->nlsock, meteor_conf->ifb_index, node->id + 10, node->id + 10, bandwidth, delay, lossrate);
                    if (ret != SUCCESS) {
                        WARNING("Error configuring Meteor rule %d.", node->id);
                        exit (1);
                    }
                    node++;
                }
            }
        }
    }

    if (meteor_conf->loop == TRUE) {
        re_flag = FALSE;
        fseek(meteor_conf->deltaq_fd, 0L, SEEK_SET);
        if ((timer = timer_init_rdtsc()) == NULL) {
            WARNING("Could not initialize timer");
            exit(1);
        }
        io_binary_read_header_from_file(bin_hdr, meteor_conf->deltaq_fd);
        if (meteor_conf->verbose >= 1) {
            io_binary_print_header(bin_hdr);
        }
        goto emulation_start;
    }

    return 0;
}

void
mode_select(struct meteor_config *meteor_conf, char *mode)
{
    if ((strcmp(mode, "ingress") == 0) || strcmp(mode, "in") == 0) {
        meteor_conf->direction = INGRESS;
    }
    else if ((strcmp(mode, "bridge") == 0) ||(strcmp(mode, "br") == 0)) {
        meteor_conf->direction = BRIDGE;
    }
    else {
        WARNING("Invalid direction '%s'", mode);
        exit(1);
    }
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
    {"Log", required_argument, NULL, 'L'},
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
    struct meteor_config *meteor_conf;

    struct sigaction sa;
    memset(&sa, 0, sizeof (struct sigaction));

    sa.sa_handler = &restart_scenario;
    sa.sa_flags |= SA_RESTART;
    if (sigaction(SIGUSR1, &sa, NULL) != 0) {
        fprintf(stderr, "Cannot set signal.\n");
        exit(1);
    }

    meteor_conf = init_meteor_conf();
    meteor_conf->nlsock = nl_socket_alloc();
    if (!meteor_conf->nlsock) {
        perror("nl_socket_alloc");
        exit(1);
    }
    if (nl_connect(meteor_conf->nlsock, NETLINK_ROUTE) != 0) {
        perror("nl_connect");
        exit(1);
    }
    if (rtnl_link_alloc_cache(meteor_conf->nlsock, AF_UNSPEC, &(meteor_conf->cache)) != 0) {
        perror("rtnl_link_alloc_cache");
        exit(1);
    }

    char ch;
    int index;
    while ((ch = getopt_long(argc, argv, "c:dhi:I:lL:m:Mq:s:v", options, &index)) != -1) {
        switch (ch) {
            case 'c':
                meteor_conf->connection_fd = fopen(optarg, "r");
                break;
            case 'd':
                meteor_conf->daemonize = TRUE;
                break;
            case 'h':
                usage();
                exit(0);
            case 'i':
                meteor_conf->id = strtol(optarg, NULL, 10);
                break;
            case 'I':
                meteor_conf->pif_index = get_ifindex(meteor_conf->cache, optarg);
                break;
            case 'l':
                meteor_conf->loop = TRUE;
                break;
            case 'L':
                meteor_conf->logfd = fopen(optarg, "w");
                break;
            case 'm':
                mode_select(meteor_conf, optarg);
                break;
            case 'M':
                meteor_conf->filter_mode = ETH_P_ALL;
                break;
            case 'q':
                if (!(meteor_conf->deltaq_fd = fopen(optarg, "rb"))) {
                    WARNING("Could not open QOMET output file '%s'", optarg);
                    exit(1);
                }
                break;
            case 's':
                if (!(meteor_conf->node_cnt = get_node_cnt(optarg))) {
                    fprintf(stderr, "Settings file '%s' is invalid", optarg);
                    exit(1);
                }
                meteor_conf->node_list_head = create_node_list(optarg, meteor_conf->node_cnt);
                break;
            case 'v':
                meteor_conf->verbose += 1;
                break;
            default:
                usage();
                exit(1);
        }
    }

    if (!meteor_conf->logfd) {
        meteor_conf->logfd = stdout;
    }

    if (meteor_conf->node_cnt < meteor_conf->id) {
        fprintf(meteor_conf->logfd, "Invalid Node ID: %d\n", meteor_conf->id);
        exit(1);
    }
    else if (meteor_conf->id == -1) {
        fprintf(meteor_conf->logfd, "Please specify node id. option: -i ID\n");
        exit(1);
    }

    if (meteor_conf->direction == BRIDGE) {
        if (!meteor_conf->connection_fd) {
            fprintf(meteor_conf->logfd, "no connection file\n");
            usage();
            exit(1);
        }
        meteor_conf->conn_list_head = create_conn_list(meteor_conf);
    }

/*
    DEBUG("Initialize timer...");
    if ((timer = timer_init_rdtsc()) == NULL) {
        fprintf(stderr, "Could not initialize timer");
        exit(1);
    }
*/
    if (meteor_conf->daemonize == TRUE) {
        ret = daemon(0, 0);
    }

    meteor_conf->ifb_index = create_ifb(meteor_conf->nlsock, meteor_conf->id);
    init_rule(meteor_conf);

    if (meteor_conf->direction == INGRESS) {
        struct node_data *node = meteor_conf->node_list_head;

        for (int i = 0; i < meteor_conf->node_cnt; i++) {
            if (node->id == meteor_conf->id) {
                node++;
                continue;
            }
            if (meteor_conf->verbose >= 1) {
                fprintf(meteor_conf->logfd, "Add rule %d from source %s\n",
                        node->id, inet_ntoa(node->ipv4addr));
            }

            if (add_rule(meteor_conf, node->id + 10, node->id + 10, node, NULL) < 0) {
                fprintf(meteor_conf->logfd, "Could not add rule %d from source %s\n", 
                        node->id, inet_ntoa(node->ipv4addr));
                exit(1);
            }
            node++;
        }
    }
    else if (meteor_conf->direction == BRIDGE) {
        // not implemented
    }

    fprintf(meteor_conf->logfd, "Reading QOMET data from file...");

    if (io_binary_read_header_from_file(meteor_conf->bin_hdr, meteor_conf->deltaq_fd) == ERROR) {
        WARNING("Aborting on input error (binary header)");
        exit(1);
    }
    if (meteor_conf->verbose >= 1) {
        io_binary_print_header(meteor_conf->bin_hdr);

        switch (meteor_conf->direction) {
            case INGRESS:
                fprintf(meteor_conf->logfd, "Direction Mode: Ingress\n");
                break;
            case BRIDGE:
                fprintf(meteor_conf->logfd, "Direction Mode: Bridge\n");
                break;
        }
    }

    if (meteor_conf->node_cnt != meteor_conf->bin_hdr->if_num) {
        fprintf(meteor_conf->logfd,
                "Number of nodes is different in "
                "settings file (%d) and QOMET scenario (%d).", 
                meteor_conf->node_cnt, meteor_conf->bin_hdr->if_num);
        exit(1);
    }

    meteor_loop(meteor_conf);

    finalize_rule(meteor_conf);

    return 0;
}
