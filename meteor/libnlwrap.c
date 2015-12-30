#include <netlink/route/tc.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/qdisc/netem.h>
#include <netlink/route/qdisc/htb.h>
#include <netlink/route/action.h>
#include <netlink/route/act/mirred.h>
#include <netlink/route/classifier.h>
#include <netlink/route/cls/u32.h>
#include <netlink/route/act/skbedit.h>
#include <linux/if_ether.h>
#include <stdio.h>

int
get_ifindex(struct nl_cache *cache, char *ifname)
{
    int if_index;
    struct rtnl_link *link;

    link = rtnl_link_get_by_name(cache, ifname);
    if_index = rtnl_link_get_ifindex(link);

    rtnl_link_put(link);
    return if_index;
}

int
add_ingress_qdisc(struct nl_sock *sock, int if_index)
{
    int err;
    struct rtnl_qdisc *qdisc;
    qdisc = rtnl_qdisc_alloc();

    rtnl_tc_set_ifindex(TC_CAST(qdisc), if_index);
    rtnl_tc_set_parent(TC_CAST(qdisc), TC_H_INGRESS);
    rtnl_tc_set_kind(TC_CAST(qdisc), "ingress");

    if ((err = rtnl_qdisc_add(sock, qdisc, NLM_F_CREATE)) < 0) {
        printf("Can not add ingress qdisc: %s\n", nl_geterror(err));
        return -1;
    }

    return 0;
}

int
add_mirred_filter(struct nl_sock *sock, int src_if, int dst_if)
{
    struct rtnl_cls *cls;

    cls = rtnl_cls_alloc();
    rtnl_tc_set_ifindex(TC_CAST(cls), src_if);
    rtnl_tc_set_parent(TC_CAST(cls), TC_HANDLE(0xffff, 0));
    rtnl_cls_set_protocol(cls, ETH_P_ALL);
    rtnl_tc_set_kind(TC_CAST(cls), "u32");

    uint32_t keyval = 0x00000000; // 0.0.0.0
    uint32_t keymask = 0x00000000; // /0
    int keyoff = 0;
    int keyoffmask = 0;
    rtnl_u32_add_key_uint32(cls, keyval, keymask, keyoff, keyoffmask);

/*
    struct rtnl_act *skbedit;
    skbedit = rtnl_act_alloc();
    if (!skbedit) {
            printf("rtnl_act_alloc() returns %p\n", skbedit);
            return -1;
    }
    rtnl_tc_set_kind(TC_CAST(skbedit), "skbedit");
    rtnl_skbedit_set_queue_mapping(skbedit, 4);
    rtnl_skbedit_set_action(skbedit, TC_ACT_PIPE);
    rtnl_u32_add_action(cls, skbedit);
*/

    // action mirred egress redirect dev ifb0
    struct rtnl_act *act;
    act = rtnl_act_alloc();
    rtnl_tc_set_kind(TC_CAST(act), "mirred");
    rtnl_mirred_set_action(act, TCA_EGRESS_REDIR);
    rtnl_mirred_set_policy(act, TC_ACT_STOLEN);
    rtnl_mirred_set_ifindex(act, dst_if);
    rtnl_u32_add_action(cls, act);

    rtnl_u32_set_cls_terminal(cls);

    int err;
    if ((err = rtnl_cls_add(sock, cls, NLM_F_CREATE)) < 0) {
        printf("Can not add classifier: %s\n", nl_geterror(err));
        return -1;
    }

    rtnl_cls_put(cls);
    rtnl_act_put(act);

    return 0;
}

int
add_class_ipv4filter(struct nl_sock *sock, int if_index, uint32_t parent, uint32_t handle, 
        uint32_t src_addr, int src_prefix, uint32_t dst_addr, int dst_prefix)
{
    int offset;
    uint32_t src_mask, dst_mask;
    struct rtnl_cls *cls;

    offset = 32 - src_prefix;
    src_mask = 0xffffffff >> offset << offset;
    offset = 32 - dst_prefix;
    dst_mask = 0xffffffff >> offset << offset;

    cls = rtnl_cls_alloc();

    rtnl_tc_set_ifindex(TC_CAST(cls), if_index);
    rtnl_tc_set_parent(TC_CAST(cls), parent);
    rtnl_cls_set_protocol(cls, ETH_P_IP);
    rtnl_tc_set_kind(TC_CAST(cls), "u32");

    if (src_addr || src_prefix) {
        rtnl_u32_add_key_uint32(cls, htonl(src_addr), src_mask, 12, 0);
    }
    if (dst_addr || dst_prefix) {
        rtnl_u32_add_key_uint32(cls, htonl(dst_addr), dst_mask, 16, 0);
    }

    rtnl_u32_set_classid(cls, handle);
    rtnl_u32_set_cls_terminal(cls);

    int err;
    if ((err = rtnl_cls_add(sock, cls, NLM_F_CREATE)) < 0) {
        printf("Can not add IPv4 address filter: %s\n", nl_geterror(err));
        return -1;
    }

    rtnl_cls_put(cls);

    return 0;
}

int
add_class_macfilter(struct nl_sock *sock, int if_index, uint32_t parent, uint32_t handle, 
        uint8_t src_addr[6], uint8_t dst_addr[6])
{
    int err;
    struct rtnl_cls *cls;

    cls = rtnl_cls_alloc();

    rtnl_tc_set_ifindex(TC_CAST(cls), if_index);
    rtnl_tc_set_parent(TC_CAST(cls), parent);
    rtnl_cls_set_protocol(cls, ETH_P_802_3);
    rtnl_tc_set_kind(TC_CAST(cls), "u32");

    if (dst_addr) {
        uint32_t *dst_addr4 = (uint32_t *)dst_addr;
        uint16_t *dst_addr2 = (uint16_t *)dst_addr + 2;
        rtnl_u32_add_key_uint32(cls, htonl(*dst_addr4), 0xffffffff, 0, 0);
        rtnl_u32_add_key_uint16(cls, htons(*dst_addr2), 0xffff, 4, 0);
    }
    if (src_addr) {
        uint32_t *src_addr4 = (uint32_t *)src_addr;
        uint16_t *src_addr2 = (uint16_t *)(src_addr + 4);
        rtnl_u32_add_key_uint32(cls, htons(*src_addr4), 0xffffffff, -8, 0);
        rtnl_u32_add_key_uint16(cls, htonl(*src_addr2), 0xffff, -4, 0);
    }
    rtnl_u32_set_classid(cls, handle);
    rtnl_u32_set_cls_terminal(cls);

    if ((err = rtnl_cls_add(sock, cls, NLM_F_CREATE)) < 0) {
        printf("Can not add mac address filter: %s\n", nl_geterror(err));
        return -1;
    }

    rtnl_cls_put(cls);

    return 0;
}

int
add_htb_qdisc(struct nl_sock *sock, int if_index, 
        uint32_t parent, uint32_t handle, uint32_t defcls)
{
    int err;
    struct rtnl_qdisc *qdisc;
    qdisc = rtnl_qdisc_alloc();

    rtnl_tc_set_ifindex(TC_CAST(qdisc), if_index);
    rtnl_tc_set_parent(TC_CAST(qdisc), parent);
    rtnl_tc_set_handle(TC_CAST(qdisc), handle);
    rtnl_tc_set_kind(TC_CAST(qdisc), "htb");
    rtnl_htb_set_defcls(qdisc, defcls);

    if ((err = rtnl_qdisc_add(sock, qdisc, NLM_F_CREATE)) < 0) {
        printf("Can not add HTB Qdisc: %s\n", nl_geterror(err));
        return -1;
    }

    return 0;
}

int
add_htb_class(struct nl_sock *sock, int if_index, uint32_t parent, uint32_t handle,
        uint32_t clsid, uint32_t rate)
{
    int err;
    struct rtnl_class *class;

    class = rtnl_class_alloc();
    rtnl_tc_set_ifindex(TC_CAST(class), if_index);
    rtnl_tc_set_parent(TC_CAST(class), parent);
    rtnl_tc_set_handle(TC_CAST(class), handle);

    if ((err = rtnl_tc_set_kind(TC_CAST(class), "htb")) < 0) {
        printf("Can not set HTB to class\n");
        return 1;
    }

    rtnl_htb_set_rate(class, rate / 8);
    rtnl_htb_set_ceil(class, rate / 8);

    if ((err = rtnl_class_add(sock, class, NLM_F_CREATE)) < 0) {
        printf("Can not add HTB class: %s\n", nl_geterror(err));
        return -1;
    }

    rtnl_class_put(class);
    return 0;
}

int
change_htb_class(struct nl_sock *sock, int if_index, uint32_t parent, uint32_t handle,
        uint32_t clsid, uint32_t rate)
{
    int err;
    struct rtnl_class *class;

    class = rtnl_class_alloc();
    rtnl_tc_set_ifindex(TC_CAST(class), if_index);
    rtnl_tc_set_parent(TC_CAST(class), parent);
    rtnl_tc_set_handle(TC_CAST(class), handle);

    if ((err = rtnl_tc_set_kind(TC_CAST(class), "htb")) < 0) {
        printf("Can not set HTB to class\n");
        return 1;
    }

    rtnl_htb_set_rate(class, rate / 8);
    rtnl_htb_set_ceil(class, rate / 8);

    if ((err = rtnl_class_add(sock, class, NLM_F_REPLACE)) < 0) {
        printf("Can not change HTB class: %s\n", nl_geterror(err));
        return -1;
    }

    rtnl_class_put(class);
    return 0;
}


int
add_netem_qdisc(struct nl_sock *sock, int if_index, 
        uint32_t parent, uint32_t handle, int delay, int jitter, int loss, int limit)
{
    int err;
    struct rtnl_qdisc *qdisc;
    qdisc = rtnl_qdisc_alloc();

    rtnl_tc_set_ifindex(TC_CAST(qdisc), if_index);
    rtnl_tc_set_parent(TC_CAST(qdisc), parent);
    rtnl_tc_set_handle(TC_CAST(qdisc), handle);
    rtnl_tc_set_kind(TC_CAST(qdisc), "netem");
    rtnl_netem_set_delay(qdisc, delay);
    rtnl_netem_set_jitter(qdisc, jitter);
    rtnl_netem_set_loss(qdisc, 0xffffffff / 100 * loss);

    if ((err = rtnl_qdisc_add(sock, qdisc, NLM_F_CREATE)) < 0) {
        printf("Can not add Netem. parent: %u. handle: %u. error: %s\n", parent, handle, nl_geterror(err));
        return -1;
    }

    rtnl_qdisc_put(qdisc);
    return 0;
}

int
change_netem_qdisc(struct nl_sock *sock, int if_index, 
        uint32_t parent, uint32_t handle, int delay, int jitter, int loss, int limit)
{
    int err;
    struct rtnl_qdisc *qdisc;
    qdisc = rtnl_qdisc_alloc();

    rtnl_tc_set_ifindex(TC_CAST(qdisc), if_index);
    rtnl_tc_set_parent(TC_CAST(qdisc), parent);
    rtnl_tc_set_handle(TC_CAST(qdisc), handle);
    rtnl_tc_set_kind(TC_CAST(qdisc), "netem");
    rtnl_netem_set_delay(qdisc, delay);
    rtnl_netem_set_jitter(qdisc, jitter);
    rtnl_netem_set_loss(qdisc, 0xffffffff / 100 * loss);

    if ((err = rtnl_qdisc_add(sock, qdisc, NLM_F_REPLACE)) < 0) {
        printf("Can not add netem: %s\n", nl_geterror(err));
        return -1;
    }

    rtnl_qdisc_put(qdisc);
    return 0;
}

int
delete_qdisc(struct nl_sock *sock, int if_index, uint32_t parent, uint32_t handle)
{
    struct rtnl_qdisc *qdisc;

    qdisc = rtnl_qdisc_alloc();
    rtnl_tc_set_ifindex(TC_CAST(qdisc), if_index);
    rtnl_tc_set_parent(TC_CAST(qdisc), parent);
    if (handle) {
        rtnl_tc_set_handle(TC_CAST(qdisc), handle);
    }

    rtnl_qdisc_delete(sock, qdisc);

    rtnl_qdisc_put(qdisc);

    return 0;
}

#if 0
int
main(int argc, char **argv)
{
    struct nl_sock *sock;
    struct nl_cache *cache;
    struct rtnl_link *link;
    int if_index;

    sock = nl_socket_alloc();

    nl_connect(sock, NETLINK_ROUTE);
    rtnl_link_alloc_cache(sock, AF_UNSPEC, &cache);
    link = rtnl_link_get_by_name(cache, "eth0");
    if_index = rtnl_link_get_ifindex(link);

    delete_qdisc(sock, if_index, TC_H_INGRESS, 0);

    add_ingress_qdisc(sock, if_index);

    int ifb_index;
    struct rtnl_link *ifb_link;
    ifb_link = rtnl_link_get_by_name(cache, "ifb0");
    ifb_index = rtnl_link_get_ifindex(ifb_link);

    add_mirred_filter(sock, if_index, ifb_index);

    delete_qdisc(sock, ifb_index, TC_H_ROOT, 0);

    add_htb_qdisc(sock, ifb_index, TC_H_ROOT, TC_HANDLE(1, 0), 1);
    add_htb_class(sock, ifb_index, TC_HANDLE(1, 0), TC_HANDLE(1, 1), 1, 1000000000);
    add_htb_class(sock, ifb_index, TC_HANDLE(1, 1), TC_HANDLE(1, 10), 1, 1000000);
    add_class_ipv4filter(sock, ifb_index, TC_HANDLE(1, 0), TC_HANDLE(1, 10), 0x0a000000, 8, 0x0b000000, 8);
    //uint8_t src[6] = { 1, 2, 3, 4, 5, 6 };
    //uint8_t dst[6] = { 10, 11, 12, 13, 14, 15 };
    //add_class_macfilter(sock, ifb_index, TC_HANDLE(1, 0), TC_HANDLE(1, 10), src, dst);
    
    int delay = 10 * 1000; // us
    int jitter = 0; // us
    uint32_t loss = 10; // %
    add_netem_qdisc(sock, ifb_index, TC_HANDLE(1, 10), TC_HANDLE(10, 0), delay, jitter, loss, 1000);

    nl_socket_free(sock);
    rtnl_link_put(link);
    nl_cache_put(cache);

    return 0;
}
#endif
