#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "utils.h"
#include "tc_util.h"
#include "tc_common.h"

static int
tbf_opt(qp, n)
struct qdisc_params* qp;
struct nlmsghdr *n;
{
	struct tc_tbf_qopt opt;
	uint32_t rtab[256];
//	__u32 ptab[256];
	unsigned buffer  = 0;
	unsigned mtu     = 0; 
	unsigned mpu     = 0;
	unsigned latency = 0;
	int Rcell_log    = -1;
//	int Pcell_log    = -1; 
	struct rtattr *tail;

	memset(&opt, 0, sizeof(opt));

	get_rate(&opt.rate.rate, qp->rate);
	get_usecs(&latency, "1s");
	get_size_and_cell(&buffer, &Rcell_log, qp->buffer);

/*
	while (argc > 0) {
		if (matches(*argv, "limit") == 0) {
			NEXT_ARG();
			if (opt.limit || latency) {
				fprintf(stderr, "Double \"limit/latency\" spec\n");
				return -1;
			}
			if (get_size(&opt.limit, *argv)) {
				explain1("limit");
				return -1;
			}
			ok++;
		} else if (matches(*argv, "latency") == 0) {
			NEXT_ARG();
			if (opt.limit || latency) {
				fprintf(stderr, "Double \"limit/latency\" spec\n");
				return -1;
			}
			if (get_usecs(&latency, "1s")) {
				explain1("latency");
				return -1;
			}
			ok++;
		} else if (matches(*argv, "burst") == 0 ||
			strcmp(*argv, "buffer") == 0 ||
			strcmp(*argv, "maxburst") == 0) {
			NEXT_ARG();
			if (buffer) {
				fprintf(stderr, "Double \"buffer/burst\" spec\n");
				return -1;
			}
			if (get_size_and_cell(&buffer, &Rcell_log, *argv) < 0) {
				explain1("buffer");
				return -1;
			}
			ok++;
		} else if (strcmp(*argv, "mtu") == 0 ||
			   strcmp(*argv, "minburst") == 0) {
			NEXT_ARG();
			if (mtu) {
				fprintf(stderr, "Double \"mtu/minburst\" spec\n");
				return -1;
			}
			if (get_size_and_cell(&mtu, &Pcell_log, *argv) < 0) {
				explain1("mtu");
				return -1;
			}
			ok++;
		} else if (strcmp(*argv, "mpu") == 0) {
			NEXT_ARG();
			if (mpu) {
				fprintf(stderr, "Double \"mpu\" spec\n");
				return -1;
			}
			if (get_size(&mpu, *argv)) {
				explain1("mpu");
				return -1;
			}
			ok++;
		} else if (strcmp(*argv, "rate") == 0) {
			NEXT_ARG();
			if (opt.rate.rate) {
				fprintf(stderr, "Double \"rate\" spec\n");
				return -1;
			}
			if (get_rate(&opt.rate.rate, *argv)) {
				explain1("rate");
				return -1;
			}
			ok++;
		}
		argc--; argv++;
	}

	if (!ok)
		return 0;
*/

	if(opt.rate.rate == 0 || !buffer) {
		fprintf(stderr, "Both \"rate\" and \"burst\" are required.\n");
		return -1;
	}

	if(opt.limit == 0 && latency == 0) {
		fprintf(stderr, "Either \"limit\" or \"latency\" are required.\n");
		return -1;
	}

	if(opt.limit == 0) {
		double lim = opt.rate.rate * (double)latency / 1000000 + buffer;
		if(opt.peakrate.rate) {
			double lim2 = opt.peakrate.rate * (double)latency / 1000000 + mtu;
			if(lim2 < lim)
				lim = lim2;
		}
		opt.limit = lim;
	}

	if((Rcell_log = tc_calc_rtable(&opt.rate, rtab, Rcell_log, mtu, mpu)) < 0) {
		fprintf(stderr, "TBF: failed to calculate rate table.\n");
		return -1;
	}
	opt.buffer = tc_calc_xmittime(opt.rate.rate, buffer);
	opt.rate.cell_log = Rcell_log;
	opt.rate.mpu = mpu;

	tail = NLMSG_TAIL(n);
	addattr_l(n, 1024, TCA_OPTIONS, NULL, 0);
	addattr_l(n, 2024, TCA_TBF_PARMS, &opt, sizeof(opt));
	addattr_l(n, 3024, TCA_TBF_RTAB, rtab, 1024);

	tail->rta_len = (void *)NLMSG_TAIL(n) - (void *)tail;

	return 0;
}

int
add_tbf_qdisc(dev, id, qp)
char* dev;
uint32_t id[4];
struct qdisc_params qp;
{
	char device[16];
	char qdisc_kind[16] = "tbf";
	uint32_t flags;
	struct {
		struct nlmsghdr n;
		struct tcmsg t;
		char buf[TCA_BUF_MAX];
	} req;
	memset(&req, 0, sizeof(req));
	flags = NLM_F_EXCL|NLM_F_CREATE;
	strncpy(device, dev, sizeof(device) - 1);

	tc_core_init();

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct tcmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST|flags;
	req.n.nlmsg_type = RTM_NEWQDISC;
	req.t.tcm_family = AF_UNSPEC;

    if(id[0] == TC_H_ROOT) {
        req.t.tcm_parent = TC_H_ROOT;
    }
    else {
        req.t.tcm_parent = TC_HANDLE(id[0], id[1]);
    }
    req.t.tcm_handle = TC_HANDLE(id[2], id[3]);

	addattr_l(&req.n, sizeof(req), TCA_KIND, qdisc_kind, strlen(qdisc_kind) + 1);

	tbf_opt(&qp, &req.n);

	if(device[0]) {
		int idx;

		ll_init_map(&rth);

		if((idx = ll_name_to_index(device)) == 0) {
			fprintf(stderr, "Cannot find device \"%s\"\n", device);
			return 1;
		}
		req.t.tcm_ifindex = idx;
		dprintf(("[add_tbf_qdisc] TBF ifindex = %d\n", idx));
	}

	if(rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
		return -1;

	return 0;
}

int
change_tbf_qdisc(dev, id, qp)
char* dev;
uint32_t id[4];
struct qdisc_params qp;
{
	char device[16];
	char qdisc_kind[16] = "tbf";
	uint32_t flags;
	struct {
		struct nlmsghdr n;
		struct tcmsg t;
		char buf[TCA_BUF_MAX];
	} req;
	memset(&req, 0, sizeof(req));
	strncpy(device, dev, sizeof(device) - 1);

	tc_core_init();

	flags = 0;

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct tcmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST|flags;
	req.n.nlmsg_type = RTM_NEWQDISC;
	req.t.tcm_family = AF_UNSPEC;

    if(id[0] == TC_H_ROOT) {
        req.t.tcm_parent = TC_H_ROOT;
    }
    else {
        req.t.tcm_parent = TC_HANDLE(id[0], id[1]);
    }
    req.t.tcm_handle = TC_HANDLE(id[2], id[3]);

	addattr_l(&req.n, sizeof(req), TCA_KIND, qdisc_kind, strlen(qdisc_kind) + 1);

	tbf_opt(&qp, &req.n);

	if(device[0]) {
		int idx;

		ll_init_map(&rth);

		if((idx = ll_name_to_index(device)) == 0) {
			fprintf(stderr, "Cannot find device \"%s\"\n", device);
			return 1;
		}
		req.t.tcm_ifindex = idx;
		dprintf(("[change_tbf_qdisc] TBF ifindex = %d\n", idx));
	}

	if(rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
		return -1;

	return 0;
}
