#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "utils.h"
#include "tc_util.h"
#include "tc_common.h"

#define TCHK_START(name)           \
struct timeval name##_prev;        \
struct timeval name##_current;     \
gettimeofday(&name##_prev, NULL)

#define TCHK_END(name)                                                             \
    gettimeofday(&name##_current, NULL);                                           \
time_t name##_sec;                                                                 \
suseconds_t name##_usec;                                                           \
if (name##_current.tv_sec == name##_prev.tv_sec) {                                  \
    name##_sec = name##_current.tv_sec - name##_prev.tv_sec;                       \
    name##_usec = name##_current.tv_usec - name##_prev.tv_usec;                    \
}                                                                                  \
else if (name ##_current.tv_sec != name##_prev.tv_sec) {                            \
    int name##_carry = 1000000;                                                    \
    name##_sec = name##_current.tv_sec - name##_prev.tv_sec;                       \
    if (name##_prev.tv_usec > name##_current.tv_usec) {                             \
        name##_usec = name##_carry - name##_prev.tv_usec + name##_current.tv_usec; \
        name##_sec--;                                                              \
        if (name##_usec > name##_carry) {                                           \
            name##_usec = name##_usec - name##_carry;                              \
            name##_sec++;                                                          \
        }                                                                          \
    }                                                                              \
    else {                                                                         \
        name##_usec = name##_current.tv_usec - name##_prev.tv_usec;                \
    }                                                                              \
}                                                                                  \
printf("%s: sec:%lu usec:%06ld\n", #name, name##_sec, name##_usec);

static int
htb_class_opt(n, bandwidth)
struct nlmsghdr *n;
uint32_t bandwidth;
{
	struct tc_htb_opt opt;
	struct rtattr* tail;
	int cell_log = -1;
	int ccell_log = -1;
	uint32_t rtab[256];
	uint32_t ctab[256];
	uint32_t cbuffer = 0;
	uint32_t buffer = 0;
	uint32_t mtu;
	uint32_t mpu = 0;
	uint16_t overhead = 0;

	memset(&opt, 0, sizeof (opt));
	mtu = 1600;

//	get_rate(&opt.rate.rate, bandwidth);
    opt.ceil.rate = bandwidth;
    opt.rate.rate = bandwidth;

    if (!opt.ceil.rate) {
        opt.ceil = opt.rate;
    }
    if (!buffer) {
        buffer = opt.rate.rate / get_hz() + mtu;
    }
    if (!cbuffer) {
        cbuffer = opt.ceil.rate / get_hz() + mtu;
    }

    opt.ceil.overhead = overhead;
    opt.rate.overhead = overhead;

    opt.ceil.mpu = mpu;
    opt.rate.mpu = mpu;

    if (tc_calc_rtable(&opt.rate, rtab, cell_log, mtu, mpu) < 0) {
        fprintf(stderr, "htb: failed to calculate rate table.\n");
        return -1;
    }
    opt.buffer = tc_calc_xmittime(opt.rate.rate, buffer);
	dprintf(("[htb_class_opt] buffer = %u\n", buffer));
	dprintf(("[htb_class_opt] opt.rate.rate = %u\n", opt.rate.rate));
	dprintf(("[htb_class_opt] opt.buffer = %u\n", opt.buffer));

    if (tc_calc_rtable(&opt.ceil, ctab, ccell_log, mtu, mpu) < 0) {
        fprintf(stderr, "htb: failed to calculate ceil rate table.\n");
        return -1;
    }
    opt.cbuffer = tc_calc_xmittime(opt.ceil.rate, cbuffer);
	dprintf(("[htb_class_opt] cbuffer = %u\n", cbuffer));
	dprintf(("[htb_class_opt] opt.ceil.rate = %u\n", opt.ceil.rate));
	dprintf(("[htb_class_opt] opt.cbuffer = %u\n", opt.cbuffer));

    //opt.quantum = opt.rate.rate * 8 / 1500;
    opt.quantum = 1500;
    dprintf(("[htb_class_opt] opt.quantum = %u\n", opt.quantum));

    tail = NLMSG_TAIL(n);
    addattr_l(n, 1024, TCA_OPTIONS, NULL, 0);
    addattr_l(n, 2024, TCA_HTB_PARMS, &opt, sizeof (opt));
    addattr_l(n, 3024, TCA_HTB_RTAB, rtab, 1024);
    addattr_l(n, 4024, TCA_HTB_CTAB, ctab, 1024);
    tail->rta_len = (void *) NLMSG_TAIL(n) - (void *) tail;

	return 0;
}

int
add_htb_class(dev, id, bandwidth)
char* dev;
uint32_t id[4];
uint32_t bandwidth;
{
	char device[16];
    char class_kind[16] = "htb";
	uint32_t flags;
	struct {
		struct nlmsghdr n;
		struct tcmsg t;
		char buf[4096];
	} req;
	memset(&req, 0, sizeof (req));
	strncpy(device, dev, sizeof (device) - 1);
	flags = NLM_F_EXCL|NLM_F_CREATE;

    if (tc_core_init() < 0) {
        fprintf(stderr, "Missing tc core init\n");
    }

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof (struct tcmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST|flags;
    req.n.nlmsg_type = RTM_NEWTCLASS;
    req.t.tcm_family = AF_UNSPEC;


    if (id[0] == TC_H_ROOT) {
        req.t.tcm_parent = TC_H_ROOT;
    }
    else {
        req.t.tcm_parent = TC_HANDLE(id[0], id[1]);
    }
    req.t.tcm_handle = TC_HANDLE(id[2], id[3]);
    dprintf(("[add_htb_class] parent id = %d\n", req.t.tcm_parent));
    dprintf(("[add_htb_class] handle id = %d\n", req.t.tcm_handle));

    addattr_l(&req.n, sizeof (req), TCA_KIND, class_kind, strlen(class_kind) + 1);

	htb_class_opt(&req.n, bandwidth);

    if (device[0]) {
		int idx;

//        ll_init_map(&rth);
        if ((idx = ll_name_to_index(device)) == 0) {
            fprintf(stderr, "Cannot find device \"%s\"\n", device);
            return 1;
        }  
		req.t.tcm_ifindex = idx;
		dprintf(("[add_htb_class] HTB ifindex : %d\n", idx));
    }

    if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0) {
        return 2;
    }

	return 0;
}

int
change_htb_class(dev, id, bandwidth)
char* dev;
uint32_t id[4];
uint32_t bandwidth;
{
	char device[16];
    char class_kind[16] = "htb";
	uint32_t flags = 0;
	struct {
		struct nlmsghdr n;
		struct tcmsg t;
		char buf[4096];
	} req;
	memset(&req, 0, sizeof (req));
	strncpy(device, dev, sizeof (device) - 1);

    if (tc_core_init() < 0) {
        fprintf(stderr, "Missing tc core init\n");
    }

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof (struct tcmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST|flags;
    req.n.nlmsg_type = RTM_NEWTCLASS;
    req.t.tcm_family = AF_UNSPEC;


    if (id[0] == TC_H_ROOT) {
        req.t.tcm_parent = TC_H_ROOT;
    }
    else {
        req.t.tcm_parent = TC_HANDLE(id[0], id[1]);
    }
    req.t.tcm_handle = TC_HANDLE(id[2], id[3]);
    dprintf(("[change_htb_class] parent id = %d\n", req.t.tcm_parent));
    dprintf(("[change_htb_class] handle id = %d\n", req.t.tcm_handle));

    addattr_l(&req.n, sizeof (req), TCA_KIND, class_kind, strlen(class_kind) + 1);

	htb_class_opt(&req.n, bandwidth);

    if (device[0]) {
		int idx;

//        ll_init_map(&rth);
        if ((idx = ll_name_to_index(device)) == 0) {
            fprintf(stderr, "Cannot find device \"%s\"\n", device);
            return 1;
        }  
		req.t.tcm_ifindex = idx;
		dprintf(("[change_htb_class] HTB ifindex : %d\n", idx));
    }

    if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0) {
        return -1;
    }

	return 0;
}

static int
htb_qdisc_opt(n, version, r2q, defcls)
struct nlmsghdr* n;
uint32_t version;
uint32_t r2q;
uint32_t defcls;
{
	struct tc_htb_glob opt;
	struct rtattr*     tail;

	memset(&opt, 0, sizeof (opt));

	opt.version = version;
	opt.rate2quantum = r2q;
	opt.defcls = defcls;

	tail = NLMSG_TAIL(n);
	addattr_l(n, 1024, TCA_OPTIONS, NULL, 0);
	addattr_l(n, 2024, TCA_HTB_INIT, &opt, NLMSG_ALIGN(sizeof (opt)));

	tail->rta_len = (void*)NLMSG_TAIL(n) - (void*)tail;

	return 0;
}

int
add_htb_qdisc(dev, id, version, r2q, defcls)
char* dev;
uint32_t id[4];
uint32_t version;
uint32_t r2q;
uint32_t defcls;
{
	char device[16];
	char qdisc_kind[16] = "htb";
	uint32_t flags;
	//struct qdisc_util *q = NULL;

	struct {
		struct nlmsghdr n;
		struct tcmsg    t;
		char            buf[TCA_BUF_MAX];
	} req;
	memset(&req, 0, sizeof (req));

	flags = NLM_F_EXCL|NLM_F_CREATE;
	strncpy(device, dev, sizeof (device) - 1);

    if (tc_core_init() < 0) {
        fprintf(stderr, "Missing tc core init\n");
    }

	req.n.nlmsg_len   = NLMSG_LENGTH(sizeof (struct tcmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST|flags;
	req.n.nlmsg_type  = RTM_NEWQDISC;
	req.t.tcm_family  = AF_UNSPEC;

    if (id[0] == TC_H_ROOT) {
        req.t.tcm_parent = TC_H_ROOT;
    }
    else {
        req.t.tcm_parent = TC_HANDLE(id[0], id[1]);
    }
    req.t.tcm_handle = TC_HANDLE(id[2], id[3]);
    dprintf(("[add_htb_qdisc] parent id = %d\n", req.t.tcm_parent));
    dprintf(("[add_htb_qdisc] handle id = %d\n", req.t.tcm_handle));

	addattr_l(&req.n, sizeof (req), TCA_KIND, qdisc_kind, strlen(qdisc_kind) + 1);

	htb_qdisc_opt(&req.n, version, r2q, defcls);

    if (device[0]) {
        int idx;

//        ll_init_map(&rth);
        if ((idx = ll_name_to_index(device)) == 0) {
            fprintf(stderr, "Cannot find device \"%s\"\n", device);
            return 1;
        }
        req.t.tcm_ifindex = idx;
		dprintf(("[add_htb_qdisc] HTB ifindex : %d\n", idx));
    }   
            
    if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0) {
        return 2;
    }

	return 0;
}
