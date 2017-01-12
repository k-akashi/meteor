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
#include "meteor.h"
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

struct METEOR_CONF *
init_meteor_conf()
{
    struct METEOR_CONF *meteor_conf;

    meteor_conf = malloc(sizeof (struct METEOR_CONF));
    memset(meteor_conf, '0', sizeof (struct METEOR_CONF));
    if (!meteor_conf) {
        fprintf(stderr, "[%s] Cannot allocate memory\n", __func__);
        exit(1);
    }
    meteor_conf->id          = -1;
    meteor_conf->loop        = FALSE;
    meteor_conf->direction   = INGRESS;
    meteor_conf->node_cnt    = -1;
    meteor_conf->verbose     = 0;
    meteor_conf->daemonize   = FALSE;
    meteor_conf->deltaq_fd   = malloc(sizeof(FILE));
    meteor_conf->settings_fd = malloc(sizeof(FILE));

    return meteor_conf;
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
            fprintf(stderr, "[%s] Invalid Parameters", __func__);
            exit(1);
        }

        if (src_id > node_cnt) {
            fprintf(stderr, "[%s] Source node id too big: %d > %d\n",
                            __func__, src_id, node_cnt);
            exit(1);
        }
        else if (dst_id > node_cnt) {
            fprintf(stderr, "[%s] Destination node id too big: %d > %d\n",
                            __func__, dst_id, node_cnt);
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
meteor_loop(struct METEOR_CONF *meteor_conf, struct nl_sock *sock, struct bin_hdr_cls bin_hdr)
{
    int time_i, node_i, ret;
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
    for (node = meteor_conf->node_list_head; node->id < meteor_conf->node_cnt; node++) {
        if (node->id == meteor_conf->id) {
            my_node = node;
        }
    }

    if ((timer = timer_init_rdtsc()) == NULL) {
        fprintf(stderr, "[%s] Could not initialize timer", __func__);
        exit(1);
    }

    if (meteor_conf->node_cnt != bin_hdr.if_num) {
        fprintf(stderr, "Number of nodes according to the settings file (%d) "
                "and number of nodes according to QOMET scenario (%d) differ", 
                meteor_conf->node_cnt, bin_hdr.if_num);
        exit(1);
    }

    bin_recs_max_cnt = bin_hdr.if_num * (bin_hdr.if_num - 1);
    bin_recs_all = (struct bin_rec_cls *)calloc(bin_recs_max_cnt, sizeof (struct bin_rec_cls));
    if (!bin_recs_all) {
        WARNING("[%s] Cannot allocate memory for binary records", __func__);
        exit(1);
    }

    recs_ucast = (struct bin_rec_cls**)calloc(bin_hdr.if_num, sizeof (struct bin_rec_cls*));
    if (!recs_ucast) {
        fprintf(stderr, "[%s] Cannot allocate memory for recs_ucast\n", __func__);
        exit(1);
    }

    for (node_i = 0; node_i < bin_hdr.if_num; node_i++) {
        if (!recs_ucast[node_i]) {
            recs_ucast[node_i] = (struct bin_rec_cls *)calloc(bin_hdr.if_num, sizeof (struct bin_rec_cls));
        }
        if (!recs_ucast[node_i]) {
            fprintf(stderr, "[%s] Cannot allocate memory for recs_ucast[%d]\n",
                            __func__, node_i);
            exit(1);
        }

        int node_j;
        for (node_j = 0; node_j < bin_hdr.if_num; node_j++) {
            recs_ucast[node_i][node_j].bandwidth = UNDEFINED_BANDWIDTH;
        }
    }

    if (meteor_conf->direction == BRIDGE) {
        bin_hdr_if_num = bin_hdr.if_num * bin_hdr.if_num;
    }
    else {
        bin_hdr_if_num = bin_hdr.if_num;
    }
    if (!adjusted_recs_ucast) {
        adjusted_recs_ucast = (struct bin_rec_cls *)calloc(bin_hdr_if_num, sizeof (struct bin_rec_cls));
        if (adjusted_recs_ucast == NULL) {
            fprintf(stderr, "[%s] Cannot allocate memory for adjusted_recs_ucast", __func__);
            exit(1);
        }
    }

    if (!recs_ucast_changed) {
        recs_ucast_changed = (int32_t *)calloc(bin_hdr_if_num, sizeof (int32_t));
        if (recs_ucast_changed == NULL) {
            fprintf(stderr, "Cannot allocate memory for recs_ucast_changed\n");
            exit(1);
        }
    }

emulation_start:
    for (time_i = 0; time_i < bin_hdr.time_rec_num; time_i++) {
        int rec_i;

        if (meteor_conf->verbose >= 2) {
            printf("Reading QOMET data from file... Time : %d/%d\n", time_i, bin_hdr.time_rec_num);
        }

        if (io_binary_read_time_record_from_file(&bin_time_rec, meteor_conf->deltaq_fd) == ERROR) {
            fprintf(stderr, "Aborting on input error (time record)\n");
            exit (1);
        }
        io_binary_print_time_record(&bin_time_rec);
        crt_record_time = bin_time_rec.time;

        if (bin_time_rec.record_number > bin_recs_max_cnt) {
            fprintf(stderr, "The number of records to be read exceeds allocated size (%d)\n", bin_recs_max_cnt);
            exit (1);
        }

        if (io_binary_read_records_from_file(bin_recs_all, bin_time_rec.record_number, meteor_conf->deltaq_fd) == ERROR) {
            fprintf(stderr, "Aborting on input error (records)\n");
            exit (1);
        }

        for (rec_i = 0; rec_i < bin_time_rec.record_number; rec_i++) {
            if (bin_recs_all[rec_i].from_id < FIRST_NODE_ID) {
                INFO("Source with id = %d is smaller first node id : %d", bin_recs_all[rec_i].from_id, assign_id);
                exit(1);
            }
            if (bin_recs_all[rec_i].from_id > bin_hdr.if_num) {
                INFO("Source with id = %d is out of the valid range [%d, %d] rec_i : %d\n", 
                    bin_recs_all[rec_i].from_id, assign_id, bin_hdr.if_num + assign_id - 1, rec_i);
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
                    fseek(meteor_conf->deltaq_fd, 0L, SEEK_SET);
                    if ((timer = timer_init_rdtsc()) == NULL) {
                        fprintf(stderr, "Could not initialize timer\n");
                        exit(1);
                    }
                    io_binary_read_header_from_file(&bin_hdr, meteor_conf->deltaq_fd);
                    if (meteor_conf->verbose >= 1) {
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

                    if (node->id < 0 || (node->id > bin_hdr.if_num - 1)) {
                        WARNING("Next hop with id = %d is out of the valid range [%d, %d]",
                                bin_recs_all[node->id].to_id, 0, bin_hdr.if_num - 1);
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
                    ret = configure_rule(sock, meteor_conf->ifb_index, node->id + 10, node->id + 10, bandwidth, delay, lossrate);
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
        io_binary_read_header_from_file(&bin_hdr, meteor_conf->deltaq_fd);
        if (meteor_conf->verbose >= 1) {
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
    struct nl_sock *sock;
    struct bin_hdr_cls bin_hdr;
    struct METEOR_CONF *meteor_conf;

    FILE *conn_fd;
    int32_t proto = ETH_P_IP;
    struct sigaction sa;

    meteor_conf = init_meteor_conf();
    memset(&sa, 0, sizeof (struct sigaction));
    sa.sa_handler = &restart_scenario;
    sa.sa_flags |= SA_RESTART;

    if (sigaction(SIGUSR1, &sa, NULL) != 0) {
        fprintf(stderr, "Signal Set Error\n");
        exit(1);
    }

    DEBUG("Open control socket...");
    sock = nl_socket_alloc();
    if (!sock) {
        WARNING("Could not open control socket (requires root priviledges)\n");
        exit(1);
    }
    nl_connect(sock, NETLINK_ROUTE);
    struct nl_cache *cache;
    rtnl_link_alloc_cache(sock, AF_UNSPEC, &cache);

    char ch;
    int index;
    while ((ch = getopt_long(argc, argv, "c:dhi:I:lm:Mq:s:v", options, &index)) != -1) {
        switch (ch) {
            case 'c':
                conn_fd = fopen(optarg, "r");
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
                meteor_conf->pif_index = get_ifindex(cache, optarg);
                break;
            case 'l':
                meteor_conf->loop = TRUE;
                break;
            case 'm':
                if ((strcmp(optarg, "ingress") == 0) || strcmp(optarg, "in") == 0) {
                    meteor_conf->direction = INGRESS;
                }
                else if ((strcmp(optarg, "bridge") == 0) ||(strcmp(optarg, "br") == 0)) {
                    meteor_conf->direction = BRIDGE;
                }
                else {
                    WARNING("Invalid direction '%s'", optarg);
                    exit(1);
                }
                break;
            case 'M':
                meteor_conf->filter_mode = ETH_P_ALL;
                break;
            case 'q':
                if ((meteor_conf->deltaq_fd = fopen(optarg, "rb")) == NULL) {
                    WARNING("Could not open QOMET output file '%s'", optarg);
                    exit(1);
                }
                break;
            case 's':
                if ((meteor_conf->node_cnt = get_node_cnt(optarg)) <= 0) {
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

    if (meteor_conf->deltaq_fd == NULL) {
        fprintf(stderr, "No QOMET data file was provided\n");
        usage();
        exit(1);
    }

    if (meteor_conf->node_cnt < meteor_conf->id) {
        fprintf(stderr, "Invalid ID\n");
        exit(1);
    }

    if (meteor_conf->direction == INGRESS) {
        if (meteor_conf->id == -1) {
            fprintf(stderr, "Please specify node id. option: -i ID\n");
            exit(1);
        }
    }
    else if (meteor_conf->direction == BRIDGE) {
        if (!conn_fd) {
            fprintf(stderr, "no connection file");
            usage();
            exit(1);
        }
        meteor_conf->conn_list_head = create_conn_list(conn_fd, meteor_conf->node_cnt);
    }

/*
    DEBUG("Initialize timer...");
    if ((timer = timer_init_rdtsc()) == NULL) {
        fprintf(stderr, "Could not initialize timer");
        exit(1);
    }
*/
    if (meteor_conf->daemonize == TRUE) {
        meteor_conf->loop = TRUE;
        ret = daemon(0, 0);
    }

    meteor_conf->ifb_index = create_ifb(sock, meteor_conf->id);
    init_rule(sock, meteor_conf);

    if (meteor_conf->direction == INGRESS) {
        struct node_data *node;
        struct node_data *my_node;
        for (node = meteor_conf->node_list_head; node->id < meteor_conf->node_cnt; node++) {
            if (node->id == meteor_conf->id) {
                my_node = node;
            }
        }
        int i;
        node = meteor_conf->node_list_head;
        for (i = 0; i < meteor_conf->node_cnt; i++) {
            if (node->id == meteor_conf->id) {
                node++;
                continue;
            }
            if (meteor_conf->verbose >= 1) {
                printf("Node %d: Add rule #%d from source %s\n", meteor_conf->id, node->id, inet_ntoa(node->ipv4addr));
            }

            if (add_rule(sock, meteor_conf->ifb_index, node->id + 10, node->id + 10, proto, node, NULL) < 0) {
                fprintf(stderr, "Node %d: Could not add rule #%d from source %s\n", 
                        meteor_conf->id, node->id, inet_ntoa(node->ipv4addr));
                exit(1);
            }
            node++;
        }
    }
    else if (meteor_conf->direction == BRIDGE) {
    }

    INFO("Reading QOMET data from file...");

    if (io_binary_read_header_from_file(&bin_hdr, meteor_conf->deltaq_fd) == ERROR) {
        WARNING("Aborting on input error (binary header)");
        exit(1);
    }
    if (meteor_conf->verbose >= 1) {
        io_binary_print_header(&bin_hdr);

        switch (meteor_conf->direction) {
            case INGRESS:
                fprintf(stdout, "Direction Mode: In\n");
                break;
            case BRIDGE:
                fprintf(stdout, "Direction Mode: Bridge\n");
                break;
        }
    }

    if (meteor_conf->node_cnt != bin_hdr.if_num) {
        fprintf(stderr, "Number of nodes according to the settings file (%d) "
                "and number of nodes according to QOMET scenario (%d) differ", 
                meteor_conf->node_cnt, bin_hdr.if_num);
        exit(1);
    }

    meteor_loop(meteor_conf, sock, bin_hdr);

    delete_qdisc(sock, meteor_conf->ifb_index, TC_H_ROOT, 0);
    delete_qdisc(sock, meteor_conf->pif_index, TC_H_INGRESS, 0);
    delete_ifb(sock, meteor_conf->ifb_index);

    nl_close(sock);
    return 0;
}
