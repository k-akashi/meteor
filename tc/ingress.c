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
    dprintf(("[add_htb_qdisc] Ingress Interface : %s\n", dev));
    strncpy(device, dev, sizeof(device) - 1);

    tc_core_init();

    req.n.nlmsg_len   = NLMSG_LENGTH(sizeof(struct tcmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST|flags;
    req.n.nlmsg_type  = RTM_NEWQDISC;
    req.t.tcm_family  = AF_UNSPEC;

    req.t.tcm_parent = TC_H_INGRESS;
    req.t.tcm_handle = 0xffff0000;
    dprintf(("[add_ingress_qdisc] parent id = %d\n", req.t.tcm_parent));
    dprintf(("[add_ingress_qdisc] handle id = %d\n", req.t.tcm_handle));
    addattr_l(&req.n, sizeof(req), TCA_KIND, qdisc_kind, strlen(qdisc_kind) + 1);

    if(dev) {
        int idx;

        ll_init_map(&rth);

        if((idx = ll_name_to_index(dev)) == 0) {
            fprintf(stderr, "Cannot find device \"%s\"\n", dev);
            return 1;
        }
        req.t.tcm_ifindex = idx;
        dprintf(("[add_htb_qdisc] Ingress ifindex : %d\n", idx));
    }

    if(rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0) {
        return 2;
    }

    return 0;
}


static int
pack_key(sel, key, mask, off, offmask)
struct tc_u32_sel *sel;
uint32_t key;
uint32_t mask;
int off;
int offmask;
{
    int i;
    int hwm = sel->nkeys;

    key &= mask;

    for(i = 0; i < hwm; i++) {
        if(sel->keys[i].off == off && sel->keys[i].offmask == offmask) {
            uint32_t intersect = mask&sel->keys[i].mask;

            if((key ^ sel->keys[i].val) & intersect) {
                return -1;
            }
            sel->keys[i].val |= key;
            sel->keys[i].mask |= mask;

            return 0;
        }
    }

    if(hwm >= 128) {
        return -1;
    }
    if(off % 4) {
        return -1;
    }
    sel->keys[hwm].val = key;
    sel->keys[hwm].mask = mask;
    sel->keys[hwm].off = off;
    sel->keys[hwm].offmask = offmask;
    sel->nkeys++;

    return 0;
}

int
mirred_ingress_filter(n, ifb)
struct nlmsghdr *n;
char *ifb;
{
    struct tc_mirred p;
    struct rtattr *tail;

    memset(&p, 0, sizeof(struct tc_mirred));

    p.eaction = TCA_EGRESS_REDIR;
    p.action = TC_ACT_STOLEN;

    ll_init_map(&rth);

    p.ifindex = ll_name_to_index(ifb);
    dprintf(("[add_ingress_filter] redirect interface = %d\n", p.ifindex));

    tail = NLMSG_TAIL(n);
    addattr_l(n, MAX_MSG, TCA_ACT_OPTIONS, NULL, 0);
    addattr_l(n, MAX_MSG, TCA_MIRRED_PARMS, &p, sizeof(p));
    tail->rta_len = (void *)NLMSG_TAIL(n) - (void *)tail;

    return 0;
}

int
action_ingress_filter(n, ifb)
struct nlmsghdr *n;
char *ifb;
{
    int prio = 0;
    struct rtattr *tail;
    struct rtattr *tail2;

    tail = tail2 = NLMSG_TAIL(n);
    addattr_l(n, MAX_MSG, TCA_U32_ACT, NULL, 0);

    tail = NLMSG_TAIL(n);
    addattr_l(n, MAX_MSG, ++prio, NULL, 0);
    addattr_l(n, MAX_MSG, TCA_ACT_KIND, "mirred", strlen("mirred") + 1);

    mirred_ingress_filter(n, ifb);
    tail->rta_len = (void *)NLMSG_TAIL(n) - (void *)tail;
    tail2->rta_len = (void *)NLMSG_TAIL(n) - (void *)tail2;

    return 0;
}

int
u32_ingress_filter(n, ifb)
struct nlmsghdr *n;
char* ifb;
{
    uint32_t key = 0;
    uint32_t mask = 0;
    int off = 0;
    int offmask = 0;
    unsigned handle;
    struct rtattr *tail;
    struct {
        struct tc_u32_sel sel;
        struct tc_u32_key keys[128];
    } sel;
    memset(&sel, 0, sizeof(sel));

    tail = NLMSG_TAIL(n);
    addattr_l(n, MAX_MSG, TCA_OPTIONS, NULL, 0);

    pack_key(sel, htonl(key), htonl(mask), off, offmask);

    handle = TC_HANDLE(1, 1);
    dprintf(("[u32_ingress_filter] handle id %d\n", handle));
    addattr_l(n, MAX_MSG, TCA_U32_CLASSID, &handle, 4);
    sel.sel.flags |= TC_U32_TERMINAL;

    action_ingress_filter(n, ifb);

    addattr_l(n, MAX_MSG, TCA_U32_SEL, &sel, sizeof(sel.sel) + sel.sel.nkeys * sizeof(struct tc_u32_key));
    tail->rta_len = (void *)NLMSG_TAIL(n) - (void *)tail;

    return 0;
}

int
add_ingress_filter(dev, ifb)
char *dev;
char *ifb;
{
    char filter_kind[16] = "u32";
    uint16_t protocol_id;
    uint32_t protocol;
    uint32_t prio;
    struct tc_estimator *est;
    struct {
        struct nlmsghdr n;
        struct tcmsg    t;
        char            buf[MAX_MSG];
    } req;

    dprintf(("[add_ingress_filter] initialize\n"));
    memset(&req, 0, sizeof(req));
    est = malloc(sizeof(struct tc_estimator));
    memset(est, 0, sizeof(struct tc_estimator));

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct tcmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
    req.n.nlmsg_type = RTM_NEWTFILTER;
    req.t.tcm_family = AF_UNSPEC;

    req.t.tcm_parent = 0xffff0000;

    dprintf(("[add_ingress_filter] parent id = %d\n", req.t.tcm_parent));

    protocol = htons(ETH_P_ALL);
    ll_proto_a2n(&protocol_id, "all");
    protocol = protocol_id;

    prio = 10;
    req.t.tcm_info = TC_H_MAKE(prio << 16, protocol);
    dprintf(("[add_ingress_filter] prio = %d\n", prio));
    dprintf(("[add_ingress_filter] protocol = %d\n", protocol));
    dprintf(("[add_ingress_filter] tcm_info = %d\n", req.t.tcm_info));

    addattr_l(&req.n, sizeof(req), TCA_KIND, filter_kind, strlen(filter_kind) + 1);

    u32_ingress_filter(&req.n, ifb);

    ll_init_map(&rth);
    req.t.tcm_ifindex = ll_name_to_index(dev);
    dprintf(("[add_ingress_filter] filter ifindex = %d\n", req.t.tcm_ifindex));

    rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL);

    return 0;
}
