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

static int
htb_class_opt(n, bandwidth)
struct nlmsghdr *n;
char *bandwidth;
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

	memset(&opt, 0, sizeof(opt));
	mtu = 1600;

	get_rate(&opt.rate.rate, bandwidth);

    if (!opt.ceil.rate) opt.ceil = opt.rate;
    if (!buffer) buffer = opt.rate.rate / get_hz() + mtu;
    if (!cbuffer) cbuffer = opt.ceil.rate / get_hz() + mtu;

    opt.ceil.overhead = overhead;
    opt.rate.overhead = overhead;

    opt.ceil.mpu = mpu;
    opt.rate.mpu = mpu;

    if(tc_calc_rtable(&opt.rate, rtab, cell_log, mtu, mpu) < 0) {
        fprintf(stderr, "htb: failed to calculate rate table.\n");
        return -1;
    }
    opt.buffer = tc_calc_xmittime(opt.rate.rate, buffer);
	dprintf(("[htb_class_opt] buffer = %u\n", buffer));
	dprintf(("[htb_class_opt] opt.rate.rate = %u\n", opt.rate.rate));
	dprintf(("[htb_class_opt] opt.buffer = %u\n", opt.buffer));

    if(tc_calc_rtable(&opt.ceil, ctab, ccell_log, mtu, mpu) < 0) {
        fprintf(stderr, "htb: failed to calculate ceil rate table.\n");
        return -1;
    }
    opt.cbuffer = tc_calc_xmittime(opt.ceil.rate, cbuffer);
	dprintf(("[htb_class_opt] cbuffer = %u\n", cbuffer));
	dprintf(("[htb_class_opt] opt.ceil.rate = %u\n", opt.ceil.rate));
	dprintf(("[htb_class_opt] opt.cbuffer = %u\n", opt.cbuffer));

    tail = NLMSG_TAIL(n);
    addattr_l(n, 1024, TCA_OPTIONS, NULL, 0);
    addattr_l(n, 2024, TCA_HTB_PARMS, &opt, sizeof(opt));
    addattr_l(n, 3024, TCA_HTB_RTAB, rtab, 1024);
    addattr_l(n, 4024, TCA_HTB_CTAB, ctab, 1024);
    tail->rta_len = (void *) NLMSG_TAIL(n) - (void *) tail;

	return 0;
}

int
add_htb_class(dev, parent_id, class_id, bandwidth)
char* dev;
char* parent_id;
char* class_id;
char* bandwidth;
{
    uint32_t handle;
	struct {
		struct nlmsghdr n;
		struct tcmsg t;
		char buf[4096];
	} req;

	uint32_t flags;
	flags = NLM_F_EXCL|NLM_F_CREATE;

	memset(&req, 0, sizeof(req));

	tc_core_init();

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct tcmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST|flags;
    req.n.nlmsg_type = RTM_NEWTCLASS;
    req.t.tcm_family = AF_UNSPEC;

    char class_kind[16] = "htb";

	char device[16];
	strncpy(device, dev, sizeof(device) - 1);

    if(strcmp(parent_id, "root")) {
        req.t.tcm_parent = TC_H_ROOT;
    }
    else {
        if(get_tc_classid(&handle, parent_id))
            dprintf(("[add_htb_class] Invalid classid : %s\n", parent_id));
        req.t.tcm_parent = handle;
    }

    if(get_tc_classid(&handle, class_id))
        dprintf(("[add_htb_class] Invalid handle id : %s\n", class_id));
    req.t.tcm_handle = handle;

    addattr_l(&req.n, sizeof(req), TCA_KIND, class_kind, strlen(class_kind) + 1);

	htb_class_opt(&req.n, bandwidth);

    if(device[0]) {
		int idx;

        ll_init_map(&rth);

        if((idx = ll_name_to_index(device)) == 0) {
            fprintf(stderr, "Cannot find device \"%s\"\n", device);
            return 1;
        }  
		req.t.tcm_ifindex = idx;
		dprintf(("[add_htb_class] HTB ifindex : %d\n", idx));
    }

    if(rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
        return 2;

	return 0;
}

int
change_htb_class(dev, parent_id, class_id, bandwidth)
char* dev;
char* parent_id;
char* class_id;
char* bandwidth;
{
    uint32_t handle;
	struct {
		struct nlmsghdr n;
		struct tcmsg t;
		char buf[4096];
	} req;

	uint32_t flags;
	flags = 0;

	memset(&req, 0, sizeof(req));

	tc_core_init();

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct tcmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST|flags;
    req.n.nlmsg_type = RTM_NEWTCLASS;
    req.t.tcm_family = AF_UNSPEC;

    char class_kind[16] = "htb";

	char device[16];
	strncpy(device, dev, sizeof(device) - 1);

    if(strcmp(parent_id, "root")) {
        req.t.tcm_parent = TC_H_ROOT;
    }
    else {
        if(get_tc_classid(&handle, parent_id))
            dprintf(("[change_htb_class] Invalid classid : %s\n", parent_id));
        req.t.tcm_parent = handle;
    }

    if(get_tc_classid(&handle, class_id))
        dprintf(("[change_htb_class] Invalid handle id : %s\n", class_id));
    req.t.tcm_handle = handle;

    addattr_l(&req.n, sizeof(req), TCA_KIND, class_kind, strlen(class_kind) + 1);

	htb_class_opt(&req.n, bandwidth);

    if(device[0]) {
		int idx;

        ll_init_map(&rth);

        if((idx = ll_name_to_index(device)) == 0) {
            fprintf(stderr, "Cannot find device \"%s\"\n", device);
            return 1;
        }  
		req.t.tcm_ifindex = idx;
		dprintf(("[change_htb_class] HTB ifindex : %d\n", idx));
    }

    if(rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
        return 2;


	return 0;
}

static int
htb_qdisc_opt(n)
struct nlmsghdr* n;
{
	struct tc_htb_glob opt;
	struct rtattr*     tail;

	memset(&opt, 0, sizeof(opt));

	opt.rate2quantum = 10;
	opt.version = 3;

	tail = NLMSG_TAIL(n);
	addattr_l(n, 1024, TCA_OPTIONS, NULL, 0);
	addattr_l(n, 2024, TCA_HTB_INIT, &opt, NLMSG_ALIGN(sizeof(opt)));

	tail->rta_len = (void*)NLMSG_TAIL(n) - (void*)tail;

	return 0;
}

int
add_htb_qdisc(dev, parent_id, handle_id)
char* dev;
char* parent_id;
char* handle_id;
{
	uint32_t handle;
	//struct qdisc_util *q = NULL;

	struct {
		struct nlmsghdr n;
		struct tcmsg    t;
		char            buf[TCA_BUF_MAX];
	} req;

	memset(&req, 0, sizeof(req));

	tc_core_init();

	uint32_t flags;
	flags = NLM_F_EXCL|NLM_F_CREATE;

	req.n.nlmsg_len   = NLMSG_LENGTH(sizeof(struct tcmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST|flags;
	req.n.nlmsg_type  = RTM_NEWQDISC;
	req.t.tcm_family  = AF_UNSPEC;

	char qdisc_kind[16] = "htb";

	char device[16];
	strncpy(device, dev, sizeof(device) - 1);

	if(strcmp(parent_id, "root") == 0) {
		req.t.tcm_parent = TC_H_ROOT;
	}
	else {
		if(get_tc_classid(&handle, parent_id))
			dprintf(("[add_htb_qdisc] Invalid classid : %s\n", parent_id));
		req.t.tcm_parent = handle;
	}

	if(get_qdisc_handle(&handle, handle_id))
		dprintf(("[add_htb_qdisc] Invalid handle id : %s\n", handle_id));
	req.t.tcm_handle = handle;

	addattr_l(&req.n, sizeof(req), TCA_KIND, qdisc_kind, strlen(qdisc_kind) + 1);

	htb_qdisc_opt(&req.n);

    if(device[0]) {
        int idx;

        ll_init_map(&rth);
       
        if((idx = ll_name_to_index(device)) == 0) {
            fprintf(stderr, "Cannot find device \"%s\"\n", device);
            return 1;
        }
        req.t.tcm_ifindex = idx;
		dprintf(("[add_htb_qdisc] HTB ifindex : %d\n", idx));
    }   
            
    if(rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
        return 2;

	return 0;
}
