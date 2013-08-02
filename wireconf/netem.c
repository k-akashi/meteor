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
__u32 *ticks;
const char* str;
{
    unsigned t;

    if(get_usecs(&t, str))
        return -1;

    if(tc_core_time2big(t)) {
        fprintf(stderr, "Illegal %u time (too large)\n", t);
        return -1;
    } 

    //*ticks = tc_core_usec2tick(t);
    *ticks = tc_core_time2tick(t);

    return 0;
}

static int
netem_opt(qp, n)
struct qdisc_parameter* qp;
struct nlmsghdr *n;
{
    size_t dist_size = 0;
    struct rtattr* tail;
    struct tc_netem_qopt opt;
    struct tc_netem_corr cor;
    struct tc_netem_reorder reorder;
    struct tc_netem_corrupt corrupt;
    __s16* dist_data = NULL;
    int present[__TCA_NETEM_MAX];

    memset(&opt, 0, sizeof(opt));
    opt.limit = NETEM_LIMIT;
    memset(&cor, 0, sizeof(cor));
    memset(&reorder, 0, sizeof(reorder));
    memset(&corrupt, 0, sizeof(corrupt));
    memset(present, 0, sizeof(present));

    if(qp->limit) {
        if(get_size(&opt.limit, qp->limit))
            return -1;
    }
    if(qp->delay) {
        if(get_ticks(&opt.latency, qp->delay))
            return -1;
    }
    if(qp->jitter) {
        if(get_ticks(&opt.jitter, qp->jitter))
            return -1;
    }
    if(qp->delay_corr) {
        ++present[TCA_NETEM_CORR];
        if(get_percent(&cor.delay_corr, qp->delay_corr))
            return -1;
    }
    if(qp->loss) {
        if(get_percent(&opt.loss, qp->loss))
            return -1;
    }
    if(qp->loss_corr) {
        ++present[TCA_NETEM_CORR];
        if(get_percent(&cor.loss_corr, qp->loss_corr))
            return -1;
    }
    if(qp->reorder_prob) {
        present[TCA_NETEM_REORDER] = 1;
        if(get_percent(&reorder.probability, qp->reorder_prob))
            return -1;
    }
    if(qp->reorder_corr) {
        ++present[TCA_NETEM_CORR];
        if(get_percent(&reorder.correlation, qp->reorder_corr))
            return -1;
    }

    tail = NLMSG_TAIL(n);

    if(reorder.probability) {
        if(opt.latency == 0) {
            fprintf(stderr, "reordering not possible without specifying some delay\n");
        }
        if(opt.gap == 0)
            opt.gap = 1;
    } else if(opt.gap > 0) {
        fprintf(stderr, "gap specified without reorder probability\n");
        return -1;
    }

    if(dist_data && (opt.latency == 0 || opt.jitter == 0)) {
        fprintf(stderr, "distribution specified but no latency and jitter values\n");
        return -1;
    }

    if(addattr_l(n, TCA_BUF_MAX, TCA_OPTIONS, &opt, sizeof(opt)) < 0)
        return -1;

    if(cor.delay_corr || cor.loss_corr || cor.dup_corr) {
        if (present[TCA_NETEM_CORR] && addattr_l(n, TCA_BUF_MAX, TCA_NETEM_CORR, &cor, sizeof(cor)) < 0)
            return -1;
    }

    if(present[TCA_NETEM_REORDER] && addattr_l(n, TCA_BUF_MAX, TCA_NETEM_REORDER, &reorder, sizeof(reorder)) < 0)
        return -1;

    if(corrupt.probability) {
        if (present[TCA_NETEM_CORRUPT] && addattr_l(n, TCA_BUF_MAX, TCA_NETEM_CORRUPT, &corrupt, sizeof(corrupt)) < 0)
            return -1;
    }

    if(dist_data) {
        if(addattr_l(n, 32768, TCA_NETEM_DELAY_DIST, dist_data, dist_size * sizeof(dist_data[0])) < 0)
            return -1;
    }
    tail->rta_len = (void*)NLMSG_TAIL(n) - (void*)tail;

    return 0;
}

int
add_netem_qdisc(dev, parent_id, handle_id, qp)
char* dev;
char* parent_id;
char* handle_id;
struct qdisc_parameter qp;
{
    uint32_t handle;
    struct {
        struct nlmsghdr n;
        struct tcmsg t;
        char buf[TCA_BUF_MAX];
    } req;
    
    memset(&req, 0, sizeof(req));

    tc_core_init();

    uint32_t flags;
    flags = NLM_F_EXCL|NLM_F_CREATE;

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct tcmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST|flags;
    req.n.nlmsg_type = RTM_NEWQDISC;
    req.t.tcm_family = AF_UNSPEC;

    // Qdisc kind
    char qdisc_kind[16] = "netem";

    // device name
    char device[16];
    strncpy(device, dev, sizeof(device) - 1);

    // set root or parent-id
    if(strcmp(parent_id, "root") == 0) {
        req.t.tcm_parent = TC_H_ROOT;
    }
    else {
        if(get_tc_classid(&handle, parent_id))
            dprintf(("[add_netem_qdisc] Invalid parent id : %s\n", parent_id));
        req.t.tcm_parent = handle;
    }

    // set handle-id
    if(get_qdisc_handle(&handle, handle_id))
        dprintf(("[add_netem_qdisc] Invalid handle id : %s\n", handle_id));
    req.t.tcm_handle = handle;

    addattr_l(&req.n, sizeof(req), TCA_KIND, qdisc_kind, strlen(qdisc_kind) + 1);

    netem_opt(&qp, &req.n);

    if(device[0]) {
        int idx;

        ll_init_map(&rth);

        if((idx = ll_name_to_index(device)) == 0) {
            fprintf(stderr, "Cannot find device \"%s\"\n", device);
            return 1;
        }
		idx = 1;
        req.t.tcm_ifindex = idx;
        dprintf(("[add_netem_qdisc] netem ifindex = %d\n", idx));
    }

    if(rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
        return -1;

    return 0;
}

int
change_netem_qdisc(dev, parent_id, handle_id, qp)
char* dev;
char* parent_id;
char* handle_id;
struct qdisc_parameter qp;
{
    uint32_t handle;
    struct {
        struct nlmsghdr n;
        struct tcmsg t;
        char buf[TCA_BUF_MAX];
    } req;
    
    memset(&req, 0, sizeof(req));

    tc_core_init();

    uint32_t flags;
    flags = 0;

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct tcmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST|flags;
    req.n.nlmsg_type = RTM_NEWQDISC;
    req.t.tcm_family = AF_UNSPEC;

    // Qdisc kind
    char qdisc_kind[16] = "netem";

    // device name
    char device[16];
    strncpy(device, dev, sizeof(device) - 1);

    // set root or parent-id
    if(strcmp(parent_id, "root") == 0) {
        req.t.tcm_parent = TC_H_ROOT;
    }
    else {
        if(get_tc_classid(&handle, parent_id))
            dprintf(("[change_netem_qdisc] Invalid parent id : %s\n", parent_id));
        req.t.tcm_parent = handle;
    }

    // set handle-id
    if(get_qdisc_handle(&handle, handle_id))
        dprintf(("[change_netem_qdisc] Invalid handle id : %s\n", handle_id));
    req.t.tcm_handle = handle;

    addattr_l(&req.n, sizeof(req), TCA_KIND, qdisc_kind, strlen(qdisc_kind) + 1);

    netem_opt(&qp, &req.n);
	dprintf(("[change_netem_qdisc] req.n.nlmsglen : %d\n", req.n.nlmsg_len));

    if(device[0]) {
        int idx;

        ll_init_map(&rth);

        if((idx = ll_name_to_index(device)) == 0) {
            fprintf(stderr, "Cannot find device \"%s\"\n", device);
            return 1;
        }
        req.t.tcm_ifindex = idx;
        dprintf(("[change_netem_qdisc] netem ifindex : %d\n", idx));
    }

    if(rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
        return -1;

    return 0;
}

int
delete_netem_qdisc(dev)
char* dev;
{
	struct {
		struct nlmsghdr n;
		struct tcmsg t;
		char buf[TCA_BUF_MAX];
	} req;


    memset(&req, 0, sizeof(req));

    tc_core_init();

    uint32_t flags;
    flags = 0;

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct tcmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST|flags;
    req.n.nlmsg_type = RTM_DELQDISC;
    req.t.tcm_family = AF_UNSPEC;

    // device name
    char device[16];
    strncpy(device, dev, sizeof(device) - 1);

	req.t.tcm_parent = TC_H_ROOT;
	
    if(device[0]) {
        int idx;

        ll_init_map(&rth);

        if((idx = ll_name_to_index(device)) == 0) {
            fprintf(stderr, "Cannot find device \"%s\"\n", device);
            return 1;
        }
        req.t.tcm_ifindex = idx;
        dprintf(("[delete_netem_qdisc] netem ifindex : %d\n", idx));
    }

    if(rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
        return -1;

	return 0;
}
