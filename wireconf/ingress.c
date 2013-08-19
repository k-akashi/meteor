#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <linux/if_ether.h>

#include "rt_names.h"
#include "utils.h"
#include "tc_util.h"
#include "tc_common.h"

int
add_ingress_qdisc(dev)
char* dev;
{
    char device[16];
    char qdisc_kind[16] = "ingress";
    uint32_t flags;

    struct {
        struct nlmsghdr n;
        struct tcmsg    t;
        char            buf[TCA_BUF_MAX];
    } req;
    memset(&req, 0, sizeof(req));

    flags = NLM_F_EXCL|NLM_F_CREATE;
    strncpy(device, dev, sizeof(device) - 1);

    tc_core_init();

    req.n.nlmsg_len   = NLMSG_LENGTH(sizeof(struct tcmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST|flags;
    req.n.nlmsg_type  = RTM_NEWQDISC;
    req.t.tcm_family  = AF_UNSPEC;

    req.t.tcm_parent = TC_H_ROOT;
    req.t.tcm_handle = TC_HANDLE(65535, 0);
    dprintf(("[add_htb_class] parent id = %d\n", req.t.tcm_parent));
    dprintf(("[add_htb_class] handle id = %d\n", req.t.tcm_handle));

    addattr_l(&req.n, sizeof(req), TCA_KIND, qdisc_kind, strlen(qdisc_kind) + 1);

    if(dev) {
        int idx;

        ll_init_map(&rth);

        if((idx = ll_name_to_index(dev)) == 0) {
            fprintf(stderr, "Cannot find device \"%s\"\n", dev);
            return 1;
        }
        req.t.tcm_ifindex = idx;
        dprintf(("[add_htb_qdisc] HTB ifindex : %d\n", idx));
    }

    if(rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
        return 2;

    return 0;
}


static int
pack_key(sel, key, mask, off, offmask)
struct tc_u32_sel *sel;
__u32 key;
__u32 mask;
int off;
int offmask;
{
    int i;
    int hwm = sel->nkeys;

    key &= mask;

    for (i=0; i<hwm; i++) {
        if (sel->keys[i].off == off && sel->keys[i].offmask == offmask) {
            __u32 intersect = mask&sel->keys[i].mask;

            if ((key^sel->keys[i].val) & intersect)
                return -1;
            sel->keys[i].val |= key;
            sel->keys[i].mask |= mask;
            return 0;
        }
    }

    if (hwm >= 128)
        return -1;
    if (off % 4)
        return -1;
    sel->keys[hwm].val = key;
    sel->keys[hwm].mask = mask;
    sel->keys[hwm].off = off;
    sel->keys[hwm].offmask = offmask;
    sel->nkeys++;
    return 0;
}

int
add_ingress_filter(dev, ifb)
char *dev;
char *ifb;
{
    uint16_t protocol_id;
    uint32_t protocol;
    uint32_t prio;
    uint32_t handle;
    struct rtattr *tail;
    struct tc_estimator *est;
    struct tc_mirred p;
    struct {
        struct nlmsghdr n;
        struct tcmsg    t;
        char            buf[MAX_MSG];
    } req;
    struct {
        struct tc_u32_sel sel;
        struct tc_u32_key keys[128];
    } sel;

    dprintf(("[add_ingress_filter] initialize\n"));
    memset(&req, 0, sizeof(req));
    est = malloc(sizeof(struct tc_estimator));
    memset(est, 0, sizeof(struct tc_estimator));
    memset(&p, 0, sizeof(struct tc_mirred));

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct tcmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
    req.n.nlmsg_type = RTM_NEWTFILTER;
    req.t.tcm_family = AF_UNSPEC;

    req.t.tcm_parent = TC_H_INGRESS;

    ll_proto_a2n(&protocol_id, "all");
    protocol = protocol_id;
    req.t.tcm_info = TC_H_MAKE(prio << 16, protocol);
    dprintf(("[tc_filter_modify] protocol = %d\n", protocol));

    addattr_l(&req.n, sizeof(req), TCA_KIND, "ingress", strlen("ingress") + 1);

// u32 filter
    tail = NLMSG_TAIL(&req.n);
    addattr_l(&req.n, MAX_MSG, TCA_OPTIONS, NULL, 0);

// match-
    pack_key(sel, 0, 0, 0, 0);
    addattr_l(&req.n, MAX_MSG, TCA_U32_SEL, &sel, sizeof(sel.sel) + sel.sel.nkeys * sizeof(struct tc_u32_key));
    tail->rta_len = (void *)NLMSG_TAIL(&req.n) - (void *)tail;

    handle = TC_HANDLE(1, 1);
    addattr_l(&req.n, MAX_MSG, TCA_U32_CLASSID, &handle, 4);
    sel.sel.flags |= TC_U32_TERMINAL;
    tail->rta_len = (void *)NLMSG_TAIL(&req.n) - (void *)tail;

// action
    addattr_l(&req.n, MAX_MSG, TCA_U32_ACT, NULL, 0);
    tail = NLMSG_TAIL(&req.n);
    addattr_l(&req.n, MAX_MSG, ++prio, NULL, 0);
    addattr_l(&req.n, MAX_MSG, TCA_ACT_KIND, "mirred", strlen("mirred") + 1);
    tail->rta_len = (void *)NLMSG_TAIL(&req.n) - (void *)tail;

// mirred : parse_egress(a, &argc, &argv, TCA_U32_CLASSID, n);
    tail->rta_len = (void *)NLMSG_TAIL(&req.n) - (void *)tail;
    p.eaction = TCA_EGRESS_REDIR;
    p.action = TC_ACT_STOLEN;

    ll_init_map(&rth);
    p.ifindex = ll_name_to_index("ifb0");

    tail = NLMSG_TAIL(&req.n);
    addattr_l(&req.n, MAX_MSG, TCA_U32_SEL, NULL, 0);
    addattr_l(&req.n, MAX_MSG, TCA_MIRRED_PARMS, &p, sizeof (p));
    tail->rta_len = (void *)NLMSG_TAIL(&req.n) - (void *)tail;
//saigo-

    ll_init_map(&rth);
    if(get_u32(&prio, "16", 0)) {
        invarg("16", "invalid prpriority value");
    }
    req.t.tcm_ifindex = ll_name_to_index(dev);

    rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL);

    return 0;
}
