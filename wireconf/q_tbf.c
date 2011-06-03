/*
 * q_tbf.c		TBF.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *
 */

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

/*
static void explain(void)
{
	fprintf(stderr, "Usage: ... tbf limit BYTES burst BYTES[/BYTES] rate KBPS [ mtu BYTES[/BYTES] ]\n");
	fprintf(stderr, "               [ peakrate KBPS ] [ latency TIME ]\n");
}

static void explain1(char *arg)
{
	fprintf(stderr, "Illegal \"%s\"\n", arg);
}
*/

#define usage() return(-1)

static int
tbf_parse_opt(qu, qp, n)
struct qdisc_util *qu;
struct qdisc_parameter* qp;
struct nlmsghdr *n;
{
//	int ok=0;
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

struct qdisc_util tbf_qdisc_util = {
	.id		= "tbf",
	.parse_qopt	= tbf_parse_opt,
	.print_qopt	= NULL,
};

