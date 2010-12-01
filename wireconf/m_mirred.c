/*
 * m_egress.c		ingress/egress packet mirror/redir actions module 
 *
 *		This program is free software; you can distribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:  J Hadi Salim (hadi@cyberus.ca) 
 * 
 * TODO: Add Ingress support
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
#include "tc_common.h"
#include <linux/tc_act/tc_mirred.h>

int mirred_d = 1;

#define usage() return(-1)

char *mirred_n2a(int action)
{
	switch (action) {
	case TCA_EGRESS_REDIR:
		return "Egress Redirect";
	case TCA_INGRESS_REDIR:
		return "Ingress Redirect";
	case TCA_EGRESS_MIRROR:
		return "Egress Mirror";
	case TCA_INGRESS_MIRROR:
		return "Ingress Mirror";
	default:
		return "unknown";
	}
}

int
parse_egress(a, egress, tca_id, n, dev)
struct action_util *a;
char* egress;
int tca_id;
struct nlmsghdr *n;
char* dev;
{

	int mirror = 0;
	int redir = 0;
	struct tc_mirred p;
	struct rtattr *tail;
	char d[16];

	memset(d, 0, sizeof(d) - 1);
	memset(&p, 0 ,sizeof(struct tc_mirred));

	/* not implememted
	// index
	if (get_u32(&p.index, *argv, 10)) {
		fprintf(stderr, "Illegal \"index\"\n");
		return -1;
	}
	*/

	if(strcmp("mirror", egress) == 0) {
		// mirror
		mirror=1;
		if (redir) {
			fprintf(stderr, "Cant have both mirror and redir\n");
			return -1;
		}
		p.eaction = TCA_EGRESS_MIRROR;
		p.action = TC_ACT_PIPE;
	}
	else if(strcmp("redirect", egress) == 0) {
		// redirect
		redir=1;
		if(mirror) {
			fprintf(stderr, "Cant have both mirror and redir\n");
			return -1;
		}
		p.eaction = TCA_EGRESS_REDIR;
		p.action = TC_ACT_STOLEN;
	}

	// device (redirect || mirror)
	if(mirror || redir) {
		strncpy(d, dev, sizeof(d)-1);
	}

	if (d[0])  {
		int idx;
		ll_init_map(&rth);

		if ((idx = ll_name_to_index(d)) == 0) {
			fprintf(stderr, "Cannot find device \"%s\"\n", d);
			return -1;
		}

		p.ifindex = idx;
	}


/*
	if (argc && p.eaction == TCA_EGRESS_MIRROR) {

		if (matches(*argv, "reclassify") == 0) {
			p.action = TC_POLICE_RECLASSIFY;
			NEXT_ARG();
		} else if (matches(*argv, "pipe") == 0) {
			p.action = TC_POLICE_PIPE;
			NEXT_ARG();
		} else if (matches(*argv, "drop") == 0 ||
			   matches(*argv, "shot") == 0) {
			p.action = TC_POLICE_SHOT;
			NEXT_ARG();
		} else if (matches(*argv, "continue") == 0) {
			p.action = TC_POLICE_UNSPEC;
			NEXT_ARG();
		} else if (matches(*argv, "pass") == 0) {
			p.action = TC_POLICE_OK;
			NEXT_ARG();
		}

	}
	if (argc) {
		if (iok && matches(*argv, "index") == 0) {
			fprintf(stderr, "mirred: Illegal double index\n");
			return -1;
		} else {
			if (matches(*argv, "index") == 0) {
				NEXT_ARG();
				if (get_u32(&p.index, *argv, 10)) {
					fprintf(stderr, "mirred: Illegal \"index\"\n");
					return -1;
				}
				argc--;
				argv++;
			}
		}
	}
*/

	if (mirred_d)
		fprintf(stdout, "Action %d device %s ifindex %d\n",p.action, d,p.ifindex);

	tail = NLMSG_TAIL(n);
	addattr_l(n, MAX_MSG, tca_id, NULL, 0);
	addattr_l(n, MAX_MSG, TCA_MIRRED_PARMS, &p, sizeof (p));
	tail->rta_len = (void *) NLMSG_TAIL(n) - (void *) tail;

	return 0;
}


int
parse_mirred(a, egress, tca_id, n, dev)
struct action_util *a;
char* egress;
int tca_id;
struct nlmsghdr *n;
char* dev;
{
	int ret;

	ret = parse_egress(a, egress, tca_id, n, dev);
	if(ret == 0) {
		return 0;
	}

	return -1;
}

int
print_mirred(struct action_util *au,FILE * f, struct rtattr *arg)
{
	struct tc_mirred *p;
	struct rtattr *tb[TCA_MIRRED_MAX + 1];
	const char *dev;
	SPRINT_BUF(b1);

	if (arg == NULL)
		return -1;

	parse_rtattr_nested(tb, TCA_MIRRED_MAX, arg);

	if (tb[TCA_MIRRED_PARMS] == NULL) {
		fprintf(f, "[NULL mirred parameters]");
		return -1;
	}
	p = RTA_DATA(tb[TCA_MIRRED_PARMS]);

	/*
	ll_init_map(&rth);
	*/


	if ((dev = ll_index_to_name(p->ifindex)) == 0) {
		fprintf(stderr, "Cannot find device %d\n", p->ifindex);
		return -1;
	}

	fprintf(f, "mirred (%s to device %s) %s", mirred_n2a(p->eaction), dev,action_n2a(p->action, b1, sizeof (b1)));

	fprintf(f, "\n ");
	fprintf(f, "\tindex %d ref %d bind %d",p->index,p->refcnt,p->bindcnt);

	if (show_stats) {
		if (tb[TCA_MIRRED_TM]) {
			struct tcf_t *tm = RTA_DATA(tb[TCA_MIRRED_TM]);
			print_tm(f,tm);
		}
	}
	fprintf(f, "\n ");
	return 0;
}

struct action_util mirred_action_util = {
	.id = "mirred",
	.parse_aopt = parse_mirred,
	.print_aopt = print_mirred,
};
