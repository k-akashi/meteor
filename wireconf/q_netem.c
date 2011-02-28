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

static void
explain(void)
{
	fprintf(stderr, "Unknown Error\n");
}

static void
explain1(const char *arg)
{
	fprintf(stderr, "%s", arg);
}
/*
static int get_distribution(const char *type, __s16 *data)
{
	FILE *f;
	int n;
	long x;
	size_t len;
	char *line = NULL;
	char name[128];

	snprintf(name, sizeof(name), "/usr/lib/tc/%s.dist", type);
	if ((f = fopen(name, "r")) == NULL) {
		fprintf(stderr, "No distribution data for %s (%s: %s)\n", 
			type, name, strerror(errno));
		return -1;
	}
	
	n = 0;
	while (getline(&line, &len, f) != -1) {
		char *p, *endp;
		if (*line == '\n' || *line == '#')
			continue;

		for (p = line; ; p = endp) {
			x = strtol(p, &endp, 0);
			if (endp == p) 
				break;

			if (n >= MAXDIST) {
				fprintf(stderr, "%s: too much data\n",
					name);
				n = -1;
				goto error;
			}
			data[n++] = x;
		}
	}
 error:
	free(line);
	fclose(f);
	return n;
}
*/
/*
static int isnumber(const char *arg) 
{
	char *p;
	(void) strtod(arg, &p);
	return (p != arg);
}
*/
#define NEXT_IS_NUMBER() (NEXT_ARG_OK() && isnumber(argv[1]))

/* Adjust for the fact that psched_ticks aren't always usecs 
   (based on kernel PSCHED_CLOCK configuration */
static int
get_ticks(ticks, str)
__u32 *ticks;
const char* str;
{
	unsigned t;

	dprintf(("delay str = %s\n", str));
	if(get_usecs(&t, str))
		return -1;
	dprintf(("delay pointer = %d\n", t));

	if(tc_core_time2big(t)) {
		fprintf(stderr, "Illegal %u time (too large)\n", t);
		return -1;
	} 
	dprintf(("delay pointer time2big = %d\n", t));

	dprintf(("ticks = %d\n", *ticks));

	//*ticks = tc_core_usec2tick(t);
	*ticks = tc_core_time2tick(t);
	dprintf(("ticks = %d\n", *ticks));

	return 0;
}

static char*
sprint_ticks(ticks, buf)
__u32 ticks;
char* buf;
{
	return sprint_usecs(tc_core_tick2usec(ticks), buf);
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

	dprintf(("debug message : netem_parse_opt\n"));
	if(qp->limit) {
		if(get_size(&opt.limit, qp->limit)){
			explain1("limit");
			return -1;
		}
	}
	if(qp->delay) {
		if(get_ticks(&opt.latency, qp->delay)){
			explain1("latency");
			return -1;
		}
	}
	if(qp->jitter) {
		if(get_ticks(&opt.jitter, qp->jitter)){
			explain1("latency");
			return -1;
		}
	}
	if(qp->delay_corr) {
		++present[TCA_NETEM_CORR];
		if(get_percent(&cor.delay_corr, qp->delay_corr)){
			explain1("latency");
			return -1;
		}
	}
	if(qp->loss) {
		if(get_percent(&opt.loss, qp->loss)){
			explain1("loss");
			return -1;
		}
	}
	if(qp->loss_corr) {
		++present[TCA_NETEM_CORR];
		if(get_percent(&cor.loss_corr, qp->loss_corr)){
			explain1("loss");
			return -1;
		}
	}
	if(qp->reorder_prob) {
		present[TCA_NETEM_REORDER] = 1;
		if(get_percent(&reorder.probability, qp->reorder_prob)){
			explain1("reorder");
			return -1;
		}
	}
	if(qp->reorder_corr) {
		++present[TCA_NETEM_CORR];
		if(get_percent(&reorder.correlation, qp->reorder_corr)){
			explain1("reorder");
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
		explain();
		return -1;
	}

	if(dist_data && (opt.latency == 0 || opt.jitter == 0)) {
		fprintf(stderr, "distribution specified but no latency and jitter values\n");
		explain();
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
		dprintf(("dist_data\n\n"));
		if(addattr_l(n, 32768, TCA_NETEM_DELAY_DIST,
			      dist_data, dist_size * sizeof(dist_data[0])) < 0)
			return -1;
	}
	tail->rta_len = (void*)NLMSG_TAIL(n) - (void*)tail;
	return 0;
}

static int
netem_print_opt(qu, f, opt)
struct qdisc_util* qu;
FILE* f;
struct rtattr* opt;
{
	const struct tc_netem_corr* cor = NULL;
	const struct tc_netem_reorder* reorder = NULL;
	const struct tc_netem_corrupt* corrupt = NULL;
	struct tc_netem_qopt qopt;
	int len = RTA_PAYLOAD(opt) - sizeof(qopt);
	SPRINT_BUF(b1);

	if(opt == NULL)
		return 0;

	if(len < 0) {
		fprintf(stderr, "options size error\n");
		return -1;
	}
	memcpy(&qopt, RTA_DATA(opt), sizeof(qopt));

	if(len > 0) {
		struct rtattr* tb[TCA_NETEM_MAX + 1];
		parse_rtattr(tb, TCA_NETEM_MAX, RTA_DATA(opt) + sizeof(qopt), len);
		
		if(tb[TCA_NETEM_CORR]) {
			if(RTA_PAYLOAD(tb[TCA_NETEM_CORR]) < sizeof(*cor))
				return -1;
			cor = RTA_DATA(tb[TCA_NETEM_CORR]);
		}
		if(tb[TCA_NETEM_REORDER]) {
			if(RTA_PAYLOAD(tb[TCA_NETEM_REORDER]) < sizeof(*reorder))
				return -1;
			reorder = RTA_DATA(tb[TCA_NETEM_REORDER]);
		}
		if(tb[TCA_NETEM_CORRUPT]) {
			if(RTA_PAYLOAD(tb[TCA_NETEM_CORRUPT]) < sizeof(*corrupt))
				return -1;
			corrupt = RTA_DATA(tb[TCA_NETEM_CORRUPT]);
		}
	}

	fprintf(f, "limit %d", qopt.limit);

	if(qopt.latency) {
		fprintf(f, " delay %s", sprint_ticks(qopt.latency, b1));

		if(qopt.jitter) {
			fprintf(f, "  %s", sprint_ticks(qopt.jitter, b1));
			if(cor && cor->delay_corr)
				fprintf(f, " %s", sprint_percent(cor->delay_corr, b1));
		}
	}

	if(qopt.loss) {
		fprintf(f, " loss %s", sprint_percent(qopt.loss, b1));
		if(cor && cor->loss_corr)
			fprintf(f, " %s", sprint_percent(cor->loss_corr, b1));
	}

	if(qopt.duplicate) {
		fprintf(f, " duplicate %s",
			sprint_percent(qopt.duplicate, b1));
		if(cor && cor->dup_corr)
			fprintf(f, " %s", sprint_percent(cor->dup_corr, b1));
	}
			
	if(reorder && reorder->probability) {
		fprintf(f, " reorder %s", 
			sprint_percent(reorder->probability, b1));
		if(reorder->correlation)
			fprintf(f, " %s", 
				sprint_percent(reorder->correlation, b1));
	}

	if(corrupt && corrupt->probability) {
		fprintf(f, " corrupt %s", 
			sprint_percent(corrupt->probability, b1));
		if(corrupt->correlation)
			fprintf(f, " %s", 
				sprint_percent(corrupt->correlation, b1));
	}

	if(qopt.gap)
		fprintf(f, " gap %lu", (unsigned long)qopt.gap);

	return 0;
}

struct qdisc_util netem_qdisc_util = {
	.id	   	= "netem",
	.parse_qopt	= netem_parse_opt,
	.print_qopt	= netem_print_opt,
};

