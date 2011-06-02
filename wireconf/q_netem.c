/*
 * q_netem.c		NETEM.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Stephen Hemminger <shemminger@osdl.org>
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
#include <errno.h>

#include "utils.h"
#include "tc_util.h"
#include "tc_common.h"

/*
 * Simplistic file parser for distrbution data.
 * Format is:
 *	# comment line(s)
 *	data0 data1
 */
#define MAXDIST	65536

#define NEXT_IS_NUMBER() (NEXT_ARG_OK() && isnumber(argv[1]))

/* Adjust for the fact that psched_ticks aren't always usecs 
   (based on kernel PSCHED_CLOCK configuration */
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
netem_parse_opt(qu, qp, n)
struct qdisc_util* qu;
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
	opt.limit = 100000;
	memset(&cor, 0, sizeof(cor));
	memset(&reorder, 0, sizeof(reorder));
	memset(&corrupt, 0, sizeof(corrupt));
	memset(present, 0, sizeof(present));

	if(qp->limit) {
		if(get_size(&opt.limit, qp->limit)){
			return -1;
		}
	}
	if(qp->delay) {
		if(get_ticks(&opt.latency, qp->delay)){
			return -1;
		}
	}
	if(qp->jitter) {
		if(get_ticks(&opt.jitter, qp->jitter)){
			return -1;
		}
	}
	if(qp->delay_corr) {
		++present[TCA_NETEM_CORR];
		if(get_percent(&cor.delay_corr, qp->delay_corr)){
			return -1;
		}
	}
	if(qp->loss) {
		if(get_percent(&opt.loss, qp->loss)){
			return -1;
		}
	}
	if(qp->loss_corr) {
		++present[TCA_NETEM_CORR];
		if(get_percent(&cor.loss_corr, qp->loss_corr)){
			return -1;
		}
	}
	if(qp->reorder_prob) {
		present[TCA_NETEM_REORDER] = 1;
		if(get_percent(&reorder.probability, qp->reorder_prob)){
			return -1;
		}
	}
	if(qp->reorder_corr) {
		++present[TCA_NETEM_CORR];
		if(get_percent(&reorder.correlation, qp->reorder_corr)){
			return -1;
		}
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
		if(addattr_l(n, 32768, TCA_NETEM_DELAY_DIST,
			      dist_data, dist_size * sizeof(dist_data[0])) < 0)
			return -1;
	}
	tail->rta_len = (void*)NLMSG_TAIL(n) - (void*)tail;

	return 0;
}

struct qdisc_util netem_qdisc_util = {
	.id	   	= "netem",
	.parse_qopt	= netem_parse_opt,
	.print_qopt	= NULL,
};

