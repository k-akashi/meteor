/*
 * q_prio.c		PRIO.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *
 * Changes:
 *
 * Ole Husgaard <sparre@login.dknet.dk>: 990513: prio2band map was always reset.
 * J Hadi Salim <hadi@cyberus.ca>: 990609: priomap fix.
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

#define usage() return(-1)

static int
prio_parse_opt(qu, qp, n)
struct qdisc_util* qu;
struct qdisc_parameter* qp;
struct nlmsghdr* n;
{
	struct tc_prio_qopt opt={16,{ 1, 2, 2, 2, 1, 2, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 }};

//	get_integer(&opt.bands, "16", 10);
/*
	if (pmap_mode) {
		for (; idx < TC_PRIO_MAX; idx++)
			opt.priomap[idx] = opt.priomap[TC_PRIO_BESTEFFORT];
	}
*/
	addattr_l(n, 1024, TCA_OPTIONS, &opt, sizeof (opt));
	return 0;
}

int
prio_print_opt(qu, f, opt)
struct qdisc_util* qu;
FILE* f;
struct rtattr* opt;
{
	int i;
	struct tc_prio_qopt* qopt;

	if (opt == NULL)
		return 0;

	if (RTA_PAYLOAD(opt) < sizeof (*qopt))
		return -1;
	qopt = RTA_DATA(opt);
	fprintf(f, "bands %u priomap ", qopt->bands);
	for (i = 0; i <= TC_PRIO_MAX; i++)
		fprintf(f, " %d", qopt->priomap[i]);

	return 0;
}

struct qdisc_util prio_qdisc_util = {
	.id	 	= "prio",
	.parse_qopt	= prio_parse_opt,
	.print_qopt	= prio_print_opt,
};

