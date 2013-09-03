/*
 * tc_qdisc.c		"tc qdisc".
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *		J Hadi Salim: Extension to ingress
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
#include <math.h>

#include "utils.h"
#include "tc_util.h"
#include "tc_common.h"

int
tc_qdisc_modify(cmd, flags, dev, handle_id, argv)
int cmd;
unsigned flags;
char* dev;
char* handle_id;
char** argv;
{
	struct qdisc_util* q = NULL;
	struct tc_estimator est;
	char  d[16];
	char  k[16];
	struct {
		struct nlmsghdr 	n;
		struct tcmsg 		t;
		char   			buf[TCA_BUF_MAX];
	} req;

	__u32 handle;

	memset(&req, 0, sizeof(req));
	memset(&est, 0, sizeof(est));
	memset(&d, 0, sizeof(d));
	memset(&k, 0, sizeof(k));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct tcmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST|flags;
	req.n.nlmsg_type = cmd;
	req.t.tcm_family = AF_UNSPEC;

	/* device set */
	if(d[0])
		duparg("dev", dev);
	strncpy(d, dev, sizeof(d) - 1);

	/* handle id set */
	if(req.t.tcm_handle)
		duparg("handle", handle_id);
	if(get_qdisc_handle(&handle, handle_id))
		invarg(handle_id, "invalid qdisc ID");
	req.t.tcm_handle = handle;

	/* root id */
	if(req.t.tcm_parent) {
		fprintf(stderr, "Error: \"root\" is duplicate parent ID\n");
		return -1;
	}
	req.t.tcm_parent = TC_H_ROOT;

	/* parent id set */
	if(req.t.tcm_parent)
		duparg("parent", *argv);
	if(get_tc_classid(&handle, *argv))
		invarg(*argv, "invalid parent ID");
	req.t.tcm_parent = handle;

	strncpy(k, *argv, sizeof(k)-1);

	q = get_qdisc_kind(k);

	if(k[0])
		addattr_l(&req.n, sizeof(req), TCA_KIND, k, strlen(k)+1);
	if(est.ewma_log)
		addattr_l(&req.n, sizeof(req), TCA_RATE, &est, sizeof(est));

	if(q) {
		if(!q->parse_qopt) {
			fprintf(stderr, "qdisc '%s' does not support option parsing\n", k);
			return -1;
		}
	}

	if(d[0])  {
		int idx;

 		ll_init_map(&rth);

		if((idx = ll_name_to_index(d)) == 0) {
			fprintf(stderr, "Cannot find device \"%s\"\n", d);
			return 1;
		}
		req.t.tcm_ifindex = idx;
	}

 	if(rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0) 
		return 2;

	return 0;
}

