/************************************************************************
 *
 * Meteor Emulator Implementation
 *
 * Authors : Kunio AKASHI
 *
 ***********************************************************************/

#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ev.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "message.h"
#include "statistics.h"
#include "timer.h"
#include "meteor.h"
#include "json_parse.h"
#include "utils.h"
#include "libnlwrap.h"

#define MAX_BUF 65535

#ifdef __amd64__
#define rdtsc(t)                                                              \
    __asm__ __volatile__ ("rdtsc; movq %%rdx, %0; salq $32, %0;orq %%rax, %0" \
    : "=r"(t) : : "%rax", "%rdx"); 
#else
#define rdtsc(t) asm volatile("rdtsc" : "=A" (t))
#endif

int session = 0;
struct meteor_config *mc;

void
usage()
{
    fprintf(stderr, "meteord. Wireless network emulator daemon.\n\n");
    fprintf(stderr, "\tUsage: meteord -c <CONFIG_FILE>\n");
}

struct meteor_config *
init_meteor_conf()
{
    struct meteor_config *mc;

    mc = malloc(sizeof (struct meteor_config));
    memset(mc, '0', sizeof (struct meteor_config));
    if (!mc) {
        fprintf(stderr, "[%s] Cannot allocate memory\n", __func__);
        exit(1);
    }
    mc->id          = -1;
    mc->loop        = FALSE;
    mc->direction   = INGRESS;
    mc->node_cnt    = -1;
    mc->verbose     = 0;
    mc->filter_mode = ETH_P_IP;
    mc->daemonize   = FALSE;
    mc->deltaq_fd   = NULL;
    mc->settings_fd = NULL;
    mc->logfd       = NULL;

    mc->bin_hdr  = malloc(sizeof (struct bin_hdr_cls));

    return mc;
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

int32_t
create_ifb(struct meteor_config *mc)
{
    char *ifbdevname;
    int err;
    int if_index;
    struct nl_cache *cache;
    struct rtnl_link *link;

    ifbdevname = (char *)malloc(15);
    sprintf(ifbdevname, "ifb%d", mc->id);
    rtnl_link_alloc_cache(mc->nlsock, AF_UNSPEC, &cache);
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
        if ((err = rtnl_link_add(mc->nlsock, link, NLM_F_CREATE)) < 0) {
            nl_perror(err, "Unable to add link");
            return err;
        }
        rtnl_link_alloc_cache(mc->nlsock, AF_UNSPEC, &cache);
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
        if ((err = rtnl_link_add(mc->nlsock, link, RTM_SETLINK)) < 0) {
            nl_perror(err, "Unable to modify link");
            return err;
        }
    }

    free(ifbdevname);
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
init_rule(struct meteor_config *mc)
{
    struct nl_sock *sock = mc->nlsock;
    int pif_index = mc->pif_index;
    int ifb_index = mc->ifb_index;

    delete_qdisc(sock, pif_index, TC_H_INGRESS, 0);
    add_ingress_qdisc(sock, pif_index);
    add_mirred_filter(sock, pif_index, ifb_index);

    delete_qdisc(sock, ifb_index, TC_H_ROOT, 0);

    add_htb_qdisc(sock, ifb_index, TC_H_ROOT, TC_HANDLE(1, 0), 65535);
    add_htb_class(sock, ifb_index, TC_HANDLE(1, 0), TC_HANDLE(1, 1), 1, DEF_BW);

    add_htb_class(sock, ifb_index, TC_HANDLE(1, 0), TC_HANDLE(1, 1), 1, DEF_BW);
    add_htb_class(sock, ifb_index, TC_HANDLE(1, 0), TC_HANDLE(1, 65535), 1, DEF_BW);

    int delay     = DEF_DELAY; // us
    int jitter    = DEF_DELAY; // us
    uint32_t loss = DEF_LOSS;  // %
    add_netem_qdisc(sock, mc->ifb_index, 
            TC_HANDLE(1, 65535), TC_HANDLE(65535, 0),
            delay, jitter, loss, 1000);

    return 0;
}

int
add_rule(struct nl_sock *sock, int ifb_index, uint16_t parent, uint16_t handle, 
        int32_t proto, struct in_addr *src, struct in_addr *dst)
{
    add_htb_class(sock, ifb_index, TC_HANDLE(1, 0),
            TC_HANDLE(1, parent), 1, DEF_BW);

    //if (proto == ETH_P_ALL) {
    //    if (!dst) {
    //        add_class_macfilter(sock, ifb_index,
    //                TC_HANDLE(1, 0), TC_HANDLE(1, parent), 
    //                src, NULL);
    //    }
    //    else {
    //        add_class_macfilter(sock, ifb_index, TC_HANDLE(1, 0),
    //                TC_HANDLE(1, parent), 
    //                src, dst);
    //    }
    //}
    if (proto == ETH_P_IP) {
        if (!dst) {
            add_class_ipv4filter(sock, ifb_index,
                    TC_HANDLE(1, 0), TC_HANDLE(1, parent), 
                    src->s_addr, 32, 0, 0);
        }
        else {
            add_class_ipv4filter(sock, ifb_index,
                    TC_HANDLE(1, 0), TC_HANDLE(1, parent), 
                    src->s_addr, 32,
                    dst->s_addr, 30);
        }
    }

    int delay = 0 ;
    int jitter = 0;
    uint32_t loss = 100; // %
    add_netem_qdisc(sock, ifb_index,
            TC_HANDLE(1, parent), TC_HANDLE(parent, 0),
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
delete_rule(struct nl_sock *sock, int ifb_index, uint16_t parent, uint16_t handle)
{
    int ret;
    ret = delete_qdisc(sock, ifb_index, TC_HANDLE(1, parent), TC_HANDLE(parent, 0));
    ret = delete_ipv4filter(sock, ifb_index, TC_H_ROOT, TC_HANDLE(1, parent));
    ret = delete_class(sock, ifb_index, TC_H_ROOT, TC_HANDLE(1, parent));

    return;
}

void
command_cb(EV_P_ ev_io *client_watch, int revents)
{
    char *c, *buf_ptr; 
    ssize_t size;
    char buf[MAX_BUF];
    struct meteor_params *mp = NULL;
    struct link_params *lp;

    if (EV_ERROR & revents) {
        perror("everror");
        return;
    }

    size = recv(client_watch->fd, buf, 1500, 0);
    if (size <= 0) {
        close(client_watch->fd);
        ev_io_stop(EV_A_ client_watch);
        free(client_watch);
        return;
    }
    if (size < 5) {
        close(client_watch->fd);
        ev_io_stop(EV_A_ client_watch);
        free(client_watch);
        return;
    }

    buf_ptr = c = buf;
    for (int buf_i = 0; buf_i < size; buf_i++) {
        if (strncmp(c, "\n", 1) == 0) {
            *c = '\0';
        }
        else {
            c++;
            continue;
        }

        debug("buf => %s\n", buf_ptr);
        mp = parse_json(buf_ptr);
        if (mp == NULL) {
            fprintf(mc->logfd, "invalid parameter => %s\n", buf);
            memset(buf, 0, MAX_BUF);
            return;
        }
    
        lp = mp->add_params;
        while (lp) {
            add_rule(mc->nlsock, mc->ifb_index, lp->id + 10, lp->id + 10, ETH_P_IP, 
                    &lp->addr, NULL);
            configure_rule(mc->nlsock, mc->ifb_index, lp->id + 10, lp->id + 10,
                    lp->rate, lp->delay, lp->loss);
            lp = lp->next;
        }
    
        lp = mp->update_params;
        while (lp) {
            configure_rule(mc->nlsock, mc->ifb_index, lp->id + 10, lp->id + 10,
                    lp->rate, lp->delay, lp->loss);
            lp = lp->next;
        }
        if (mp->delete_params != NULL) {
            struct node_array *dp;
            dp = mp->delete_params;
            if (dp->size != 0) {
                for (int i = 0; i < dp->size; i++) {
                    delete_rule(mc->nlsock, mc->ifb_index, dp->id[i] + 10, dp->id[i] + 10);
                }
            }
        }

        c++;
        buf_ptr = c;
        clear_meteor_params(mp);
    }

    memset(buf, 0, MAX_BUF);
}

void
meteor_cb(EV_P_ ev_io *meteor_watch, int revents)
{
    int client_fd;
    struct sockaddr_in caddr;
    socklen_t caddrlen = sizeof (caddr);
    struct ev_loop *client_loop;
    ev_io *client_watch;

    if (EV_ERROR & revents) {
        perror("everror");
        return;
    }

    if ((client_fd = accept(meteor_watch->fd, (struct sockaddr *)&caddr, &caddrlen)) < 0) {
        perror("accept");
        return;
    }

    client_watch = (ev_io *)malloc(sizeof (ev_io));
    memset(client_watch, 0, sizeof (ev_io));
    client_loop = meteor_watch->data;

    ev_io_init(client_watch, command_cb, client_fd, EV_READ);
    ev_io_start(client_loop, client_watch);
}

int
init_meteor(struct meteor_config *mc)
{
    int sock;
    int reuse = 1;
    struct sockaddr_in saddr;
    struct ev_loop *meteor_loop = ev_default_loop(0); //ev_loop_new(ev_recommended_backends());
    ev_io meteor_watch;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    memset(&saddr, 0, sizeof (saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(10000);
    saddr.sin_addr.s_addr = INADDR_ANY;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));

    if (bind(sock, (struct sockaddr *)&saddr, sizeof (struct sockaddr_in)) != 0) {
        perror("bind");
        return -1;
    }
    if (listen(sock, 2) < 0) {
        perror("listen");
        return -1;
    }

    meteor_watch.data = meteor_loop;
    ev_io_init(&meteor_watch, meteor_cb, sock, EV_READ);
    ev_io_start(meteor_loop, &meteor_watch);

    ev_loop(meteor_loop, 0);

    close(sock);

    return 0;
}

void
mode_select(struct meteor_config *mc, char *mode)
{
    if ((strcmp(mode, "ingress") == 0) || strcmp(mode, "in") == 0) {
        mc->direction = INGRESS;
    }
    else if ((strcmp(mode, "bridge") == 0) ||(strcmp(mode, "br") == 0)) {
        mc->direction = BRIDGE;
    }
    else {
        WARNING("Invalid direction '%s'", mode);
        exit(1);
    }
}


struct option options[] = 
{
    {"config", required_argument, NULL, 'c'},
    {0, 0, 0, 0}
};

int
main(int argc, char **argv)
{
    FILE *conf;

    char ch;
    int index;
    while ((ch = getopt_long(argc, argv, "c:", options, &index)) != -1) {
        switch (ch) {
            case 'c':
                conf = fopen(optarg, "r");
                break;
            default:
                usage();
                exit(1);
        }
    }

    mc = init_meteor_conf();

    mc->nlsock = nl_socket_alloc();
    if (!mc->nlsock) {
        perror("nl_socket_alloc");
        exit(1);
    }
    if (nl_connect(mc->nlsock, NETLINK_ROUTE) != 0) {
        perror("nl_connect");
        exit(1);
    }
    if (rtnl_link_alloc_cache(mc->nlsock, AF_UNSPEC, &mc->cache) != 0) {
        perror("rtnl_link_alloc_cache");
        exit(1);
    }

    mc->id = 0;
    mc->pif_index = get_ifindex(mc->cache, "eth2");
    mc->ifb_index = create_ifb(mc);


    if (!mc->logfd) {
        mc->logfd = stdout;
    }

    if (mc->id == -1) {
        fprintf(mc->logfd, "Please specify node id.\n");
        exit(1);
    }

    init_rule(mc);
    init_meteor(mc);

    delete_qdisc(mc->nlsock, mc->ifb_index, TC_H_ROOT, 0);
    delete_qdisc(mc->nlsock, mc->pif_index, TC_H_INGRESS, 0);
    delete_ifb(mc->nlsock, mc->ifb_index);

    nl_close(mc->nlsock);

    return 0;
}
