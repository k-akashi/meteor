/*
 * q_fifo.c		FIFO.
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

#define usage() return(-1)

static int
fifo_parse_opt(qu, qp, n)
struct qdisc_util* qu;
struct qdisc_parameter* qp;
struct nlmsghdr* n;
{
//	int ok=0;
	struct tc_fifo_qopt opt;
	memset(&opt, 0, sizeof (opt));

	get_size(&opt.limit, "1000");

/*
	while (argc > 0) {
		if (strcmp(*argv, "limit") == 0) {
			NEXT_ARG();
			if (get_size(&opt.limit, *argv)) {
				fprintf(stderr, "Illegal \"limit\"\n");
				return -1;
			}
			ok++;
		} else if (strcmp(*argv, "help") == 0) {
			explain();
			return -1;
		} else {
			fprintf(stderr, "What is \"%s\"?\n", *argv);
			explain();
			return -1;
		}
		argc--; argv++;
	}
*/
	addattr_l(n, 1024, TCA_OPTIONS, &opt, sizeof (opt));

	return 0;
}

struct qdisc_util bfifo_qdisc_util = {
	.id = "bfifo",
	.parse_qopt = fifo_parse_opt,
	.print_qopt = NULL,
};

struct qdisc_util pfifo_qdisc_util = {
	.id = "pfifo",
	.parse_qopt = fifo_parse_opt,
	.print_qopt = NULL,
};

struct qdisc_util pfifo_fast_qdisc_util = {
	.id = "pfifo_fast",
	.print_qopt = NULL,
};
