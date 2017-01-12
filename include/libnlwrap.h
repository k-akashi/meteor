#ifndef __LIBNLWRAP_H_
#define __LIBNLWRAP_H_
#include <netlink/route/link.h>
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

// Qdisc module
#define NL_INGRESS "ingress"

// Filter module
#define NL_U32 "u32"

int get_ifindex(struct nl_cache *cache, char *ifname);
int add_ingress_qdisc(struct nl_sock *sock, int if_index);
int add_mirred_filter(struct nl_sock *sock, int src_if, int dst_if);
int add_class_ipv4filter(struct nl_sock *sock, int if_index, uint32_t parent, uint32_t handle, uint32_t src_addr, int src_prefix, uint32_t dst_addr, int dst_prefix);
int add_class_macfilter(struct nl_sock *sock, int if_index, uint32_t parent, uint32_t handle, uint8_t src_addr[6], uint8_t dst_addr[6]);
int add_htb_qdisc(struct nl_sock *sock, int if_index, uint32_t parent, uint32_t handle, uint32_t defcls);
int add_htb_class(struct nl_sock *sock, int if_index, uint32_t parent, uint32_t handle, uint32_t clsid, uint32_t rate);
int change_htb_class(struct nl_sock *sock, int if_index, uint32_t parent, uint32_t handle, uint32_t clsid, uint32_t rate);
int add_netem_qdisc(struct nl_sock *sock, int if_index, uint32_t parent, uint32_t handle, int delay, int jitter, int loss, int limit);
int change_netem_qdisc(struct nl_sock *sock, int if_index, uint32_t parent, uint32_t handle, int delay, int jitter, int loss, int limit);
int delete_qdisc(struct nl_sock *sock, int if_index, uint32_t parent, uint32_t handle);
#endif
