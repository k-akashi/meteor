#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include "utils.h"
#include "tc_util.h"
#include "tc_common.h"

#define MAXDIST 65536
#define NETEM_LIMIT 10000

#define NEXT_IS_NUMBER() (NEXT_ARG_OK() && isnumber(argv[1]))

static int
get_ticks(ticks, str)
uint32_t *ticks;
const char* str;
{
    unsigned t;

    if (get_usecs(&t, str)) {
        return -1;
    }

    fprintf(stderr, "t : %d\n", t);

    if (tc_core_time2big(t)) {
        fprintf(stderr, "Illegal %u time (too large)\n", t);
        return -1;
    } 

    //*ticks = tc_core_usec2tick(t);
    *ticks = tc_core_time2tick(t);

    fprintf(stderr, "tics : %d\n", *ticks);
    return 0;
}

static int
netem_opt(qp, n)
struct qdisc_params* qp;
struct nlmsghdr *n;
{
    uint32_t latency;
    size_t dist_size = 0;
    struct rtattr* tail;
    struct tc_netem_qopt opt;
    struct tc_netem_corr cor;
    struct tc_netem_reorder reorder;
    struct tc_netem_corrupt corrupt;
    __s16* dist_data = NULL;
    int present[__TCA_NETEM_MAX];

    memset(&opt, 0, sizeof (opt));
    opt.limit = NETEM_LIMIT;
    memset(&cor, 0, sizeof (cor));
    memset(&reorder, 0, sizeof (reorder));
    memset(&corrupt, 0, sizeof (corrupt));
    memset(present, 0, sizeof (present));

    if (qp->limit) {
        opt.limit = qp->limit;
    }
    if (qp->delay) {
        latency = qp->delay * 1000;
        opt.latency = tc_core_time2tick(latency);
    }
    if (qp->jitter) {
        qp->jitter = 0;
/*
        if (get_ticks(&opt.jitter, qp->jitter)) {
            return -1;
        }
*/
    }
    if (qp->delay_corr) {
        ++present[TCA_NETEM_CORR];
        cor.delay_corr = qp->delay_corr;
    }
    if (qp->loss) {
        opt.loss = qp->loss;
    }
    if (qp->loss_corr) {
        ++present[TCA_NETEM_CORR];
        cor.loss_corr = qp->loss_corr;
    }
    if (qp->reorder_prob) {
        present[TCA_NETEM_REORDER] = 1;
        reorder.probability = qp->reorder_prob;
    }
    if (qp->reorder_corr) {
        ++present[TCA_NETEM_CORR];
        reorder.correlation = qp->reorder_corr;
    }

    tail = NLMSG_TAIL(n);

    if (reorder.probability) {
        if (opt.latency == 0) {
            fprintf(stderr, "reordering not possible without specifying some delay\n");
        }
        if (opt.gap == 0) {
            opt.gap = 1;
        }
    } else if (opt.gap > 0) {
        fprintf(stderr, "gap specified without reorder probability\n");
        return -1;
    }
    if (dist_data && (opt.latency == 0 || opt.jitter == 0)) {
        fprintf(stderr, "distribution specified but no latency and jitter values\n");
        return -1;
    }
    if (addattr_l(n, TCA_BUF_MAX, TCA_OPTIONS, &opt, sizeof (opt)) < 0) {
        return -1;
    }
    if (cor.delay_corr || cor.loss_corr || cor.dup_corr) {
        if (present[TCA_NETEM_CORR] && addattr_l(n, TCA_BUF_MAX, TCA_NETEM_CORR, &cor, sizeof (cor)) < 0) {
            return -1;
        }
    }
    if (present[TCA_NETEM_REORDER] && addattr_l(n, TCA_BUF_MAX, TCA_NETEM_REORDER, &reorder, sizeof (reorder)) < 0) {
        return -1;
    }
    if (corrupt.probability) {
        if (present[TCA_NETEM_CORRUPT] && addattr_l(n, TCA_BUF_MAX, TCA_NETEM_CORRUPT, &corrupt, sizeof (corrupt)) < 0) {
            return -1;
        }
    }
    if (dist_data) {
        if (addattr_l(n, 32768, TCA_NETEM_DELAY_DIST, dist_data, dist_size * sizeof (dist_data[0])) < 0) {
            return -1;
        }
    }
    tail->rta_len = (void*)NLMSG_TAIL(n) - (void*)tail;

    return 0;
}

int
add_netem_qdisc(dev, id, qp)
char* dev;
uint32_t id[4];
struct qdisc_params qp;
{
    char device[16];
    char qdisc_kind[16] = "netem";
    uint32_t flags;
    struct {
        struct nlmsghdr n;
        struct tcmsg t;
        char buf[TCA_BUF_MAX];
    } req;
    memset(&req, 0, sizeof (req));
    strncpy(device, dev, sizeof (device) - 1);

    if (tc_core_init() < 0) {
        fprintf(stderr, "Missing tc core init\n");
    }

    flags = NLM_F_EXCL|NLM_F_CREATE;
    req.n.nlmsg_len = NLMSG_LENGTH(sizeof (struct tcmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST|flags;
    req.n.nlmsg_type = RTM_NEWQDISC;
    req.t.tcm_family = AF_UNSPEC;

    if (id[0] == 0) {
        req.t.tcm_parent = TC_H_ROOT;
    }
    else {
        req.t.tcm_parent = TC_HANDLE(id[0], id[1]);
    }
    req.t.tcm_handle = TC_HANDLE(id[2], id[3]);

    addattr_l(&req.n, sizeof (req), TCA_KIND, qdisc_kind, strlen(qdisc_kind) + 1);
    dprintf(("[add_netem_qdisc] parent id = %u\n", req.t.tcm_parent));
    dprintf(("[add_netem_qdisc] handle id = %u\n", req.t.tcm_handle));
    dprintf(("[add_netem_qdisc] delay = %f\n", qp.delay));
    dprintf(("[add_netem_qdisc] loss = %f\n", qp.loss));

    netem_opt(&qp, &req.n);

    if (device[0]) {
        int idx;

//        ll_init_map(&rth);
        if ((idx = ll_name_to_index(device)) == 0) {
            fprintf(stderr, "Cannot find device \"%s\"\n", device);
            return 1;
        }
        req.t.tcm_ifindex = idx;
        dprintf(("[add_netem_qdisc] netem ifindex = %d\n", idx));
    }

    if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0) {
        return -1;
    }

    return 0;
}

int
change_netem_qdisc(dev, id, qp)
char* dev;
uint32_t id[4];
struct qdisc_params qp;
{
    uint32_t flags;
    char device[16];
    char qdisc_kind[16] = "netem";
    struct {
        struct nlmsghdr n;
        struct tcmsg t;
        char buf[TCA_BUF_MAX];
    } req;
    memset(&req, 0, sizeof (req));
    strncpy(device, dev, sizeof (device) - 1);

    if (tc_core_init() < 0) {
        fprintf(stderr, "Missing tc core init\n");
    }

    flags = 0;

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof (struct tcmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST|flags;
    req.n.nlmsg_type = RTM_NEWQDISC;
    req.t.tcm_family = AF_UNSPEC;

    if (id[0] == 0) {
        req.t.tcm_parent = TC_H_ROOT;
    }
    else {
        req.t.tcm_parent = TC_HANDLE(id[0], id[1]);
    }
    req.t.tcm_handle = TC_HANDLE(id[2], id[3]);
    dprintf(("[change_netem_qdisc] parent id = %d\n", req.t.tcm_parent));
    dprintf(("[change_netem_qdisc] handle id = %d\n", req.t.tcm_handle));
    dprintf(("[change_netem_qdisc] delay = %f\n", qp.delay));
    dprintf(("[change_netem_qdisc] loss = %f\n", qp.loss));

    addattr_l(&req.n, sizeof (req), TCA_KIND, qdisc_kind, strlen(qdisc_kind) + 1);

    netem_opt(&qp, &req.n);

    if (device[0]) {
        int idx;

//        ll_init_map(&rth);
        if ((idx = ll_name_to_index(device)) == 0) {
            fprintf(stderr, "Cannot find device \"%s\"\n", device);
            return 1;
        }
        req.t.tcm_ifindex = idx;
        dprintf(("[change_netem_qdisc] netem ifindex : %d\n", idx));
    }

    if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0) {
        return -1;
    }

    return 0;
}

int
delete_netem_qdisc(dev, ingress)
char* dev;
int ingress;
{
    char device[16];
    uint32_t flags;
	struct {
		struct nlmsghdr n;
		struct tcmsg t;
		char buf[TCA_BUF_MAX];
	} req;

    flags = 0;
    memset(&req, 0, sizeof (req));

    if (tc_core_init() < 0) {
        fprintf(stderr, "Missing tc core init\n");
    }

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof (struct tcmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST|flags;
    req.n.nlmsg_type = RTM_DELQDISC;
    req.t.tcm_family = AF_UNSPEC;

    strncpy(device, dev, sizeof (device) - 1);

    if (ingress == 1) {
	    req.t.tcm_parent = 0xffff0000;
    }
    else {
	    req.t.tcm_parent = TC_H_ROOT;
    }
	
    if (device[0]) {
        int idx;

//       ll_init_map(&rth);
        if ((idx = ll_name_to_index(device)) == 0) {
            fprintf(stderr, "Cannot find device \"%s\"\n", device);
            return 1;
        }
        req.t.tcm_ifindex = idx;
        dprintf(("[delete_netem_qdisc] netem ifindex : %d\n", idx));
    }

    if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0) {
        return -1;
    }

	return 0;
}
