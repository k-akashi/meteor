/*
 * tc_filter.c		"tc filter".
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
#include <net/if.h>
#include <net/if_arp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <linux/if_ether.h>

#include "rt_names.h"
#include "utils.h"
#include "tc_util.h"
#include "tc_common.h"

#define __PF(f,n) { ETH_P_##f, #n },
static struct {
	int id;
	const char *name;
} llproto_names[] = {
__PF(LOOP,loop)
__PF(PUP,pup)
#ifdef ETH_P_PUPAT
__PF(PUPAT,pupat)
#endif
__PF(IP,ip)
__PF(X25,x25)
__PF(ARP,arp)
__PF(BPQ,bpq)
#ifdef ETH_P_IEEEPUP
__PF(IEEEPUP,ieeepup)
#endif
#ifdef ETH_P_IEEEPUPAT
__PF(IEEEPUPAT,ieeepupat)
#endif
__PF(DEC,dec)
__PF(DNA_DL,dna_dl)
__PF(DNA_RC,dna_rc)
__PF(DNA_RT,dna_rt)
__PF(LAT,lat)
__PF(DIAG,diag)
__PF(CUST,cust)
__PF(SCA,sca)
__PF(RARP,rarp)
__PF(ATALK,atalk)
__PF(AARP,aarp)
__PF(IPX,ipx)
__PF(IPV6,ipv6)
#ifdef ETH_P_PPP_DISC
__PF(PPP_DISC,ppp_disc)
#endif
#ifdef ETH_P_PPP_SES
__PF(PPP_SES,ppp_ses)
#endif
#ifdef ETH_P_ATMMPOA
__PF(ATMMPOA,atmmpoa)
#endif
#ifdef ETH_P_ATMFATE
__PF(ATMFATE,atmfate)
#endif

__PF(802_3,802_3)
__PF(AX25,ax25)
__PF(ALL,all)
__PF(802_2,802_2)
__PF(SNAP,snap)
__PF(DDCMP,ddcmp)
__PF(WAN_PPP,wan_ppp)
__PF(PPP_MP,ppp_mp)
__PF(LOCALTALK,localtalk)
__PF(PPPTALK,ppptalk)
__PF(TR_802_2,tr_802_2)
__PF(MOBITEX,mobitex)
__PF(CONTROL,control)
__PF(IRDA,irda)
#ifdef ETH_P_ECONET
__PF(ECONET,econet)
#endif

{ 0x8100, "802.1Q" },
{ ETH_P_IP, "ipv4" },
};
#undef __PF

const char* ll_proto_n2a(id, buf, len)
unsigned short id;
char *buf;
int len;
{
	int i;

	id = ntohs(id);

	for(i = 0; i < sizeof(llproto_names) / sizeof(llproto_names[0]); i++) {
		if(llproto_names[i].id == id)
			return llproto_names[i].name;
	}
	snprintf(buf, len, "[%d]", id);
	return buf;
}

int
ll_proto_a2n(id, buf)
unsigned short *id;
char* buf;
{
	int i;

	for(i = 0; i < sizeof(llproto_names) / sizeof(llproto_names[0]); i++) {
		if(strcasecmp(llproto_names[i].name, buf) == 0) {
			*id = htons(llproto_names[i].id);
			return 0;
		}
	}

	if(get_u16(id, buf, 0))
		return -1;

	*id = htons(*id);
	return 0;
}

int 
tc_filter_modify(cmd, flags, dev, parentid, handleid, protocolid, type, up)
int cmd;
unsigned int flags;
char* dev;
char* parentid;
char* handleid;
char* protocolid;
char* type;
struct u32_parameter up;
{
	struct {
		struct nlmsghdr 	n;
		struct tcmsg 		t;
		char   			buf[MAX_MSG];
	} req;
	struct filter_util *q = NULL;
	__u32 prio = 0;
	__u32 protocol = 0;
	char *fhandle = NULL;
	char  d[16];
	char  k[16];
	struct tc_estimator est;

	memset(&req, 0, sizeof(req));
	memset(&est, 0, sizeof(est));
	memset(d, 0, sizeof(d));
	memset(k, 0, sizeof(k));
	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct tcmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST|flags;
	req.n.nlmsg_type = cmd;
	req.t.tcm_family = AF_UNSPEC;

	dprintf(("filter set\n"));

	strncpy(d, dev, sizeof(d)-1);

	if(*parentid == '0') {
		if(req.t.tcm_parent) {
			fprintf(stderr, "Error: \"root\" is duplicate parent ID\n");
			return -1;
		}
		req.t.tcm_parent = TC_H_ROOT;
	}
	else {
		__u32 handle;
		if(req.t.tcm_parent)
			duparg("parent", parentid);
		if(get_tc_classid(&handle, parentid))
			invarg(parentid, "Invalid parent ID");
		req.t.tcm_parent = handle;
	}

	if(fhandle)
		duparg("handle", handleid);
	fhandle = handleid;

	// protocol set
	__u16 id;
	if(protocol)
		duparg("protocol", protocolid);
	if(ll_proto_a2n(&id, protocolid))
		invarg(protocolid, "invalid protocol");
	protocol = id;

	strncpy(k, type, sizeof(k)-1);
	q = get_filter_kind(k);

	req.t.tcm_info = TC_H_MAKE(prio << 16, protocol);

	if(k[0])
		addattr_l(&req.n, sizeof(req), TCA_KIND, k, strlen(k)+1);

	if(q) {
		if(q->parse_fopt(q, fhandle, up, &req.n, dev))
			return 1;
	}

	if(est.ewma_log)
		addattr_l(&req.n, sizeof(req), TCA_RATE, &est, sizeof(est));


	if(d[0]) {
 		ll_init_map(&rth);

		if ((req.t.tcm_ifindex = ll_name_to_index(d)) == 0) {
			fprintf(stderr, "Cannot find device \"%s\"\n", d);
			return 1;
		}
	}

 	if(rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0) {
		fprintf(stderr, "We have an error talking to the kernel\n");
		return 2;
	}

	return 0;
}

static __u32 filter_parent;
static int filter_ifindex;
static __u32 filter_prio;
static __u32 filter_protocol;

static int print_filter(const struct sockaddr_nl *who,
			struct nlmsghdr *n, 
			void *arg)
{
	FILE *fp = (FILE*)arg;
	struct tcmsg *t = NLMSG_DATA(n);
	int len = n->nlmsg_len;
	struct rtattr * tb[TCA_MAX+1];
	struct filter_util *q;
	char abuf[256];

	if (n->nlmsg_type != RTM_NEWTFILTER && n->nlmsg_type != RTM_DELTFILTER) {
		fprintf(stderr, "Not a filter\n");
		return 0;
	}
	len -= NLMSG_LENGTH(sizeof(*t));
	if (len < 0) {
		fprintf(stderr, "Wrong len %d\n", len);
		return -1;
	}

	memset(tb, 0, sizeof(tb));
	parse_rtattr(tb, TCA_MAX, TCA_RTA(t), len);

	if (tb[TCA_KIND] == NULL) {
		fprintf(stderr, "print_filter: NULL kind\n");
		return -1;
	}

	if (n->nlmsg_type == RTM_DELTFILTER)
		fprintf(fp, "deleted ");

	fprintf(fp, "filter ");
	if (!filter_ifindex || filter_ifindex != t->tcm_ifindex)
		fprintf(fp, "dev %s ", ll_index_to_name(t->tcm_ifindex));

	if (!filter_parent || filter_parent != t->tcm_parent) {
		if (t->tcm_parent == TC_H_ROOT)
			fprintf(fp, "root ");
		else {
			print_tc_classid(abuf, sizeof(abuf), t->tcm_parent);
			fprintf(fp, "parent %s ", abuf);
		}
	}
	if (t->tcm_info) {
		__u32 protocol = TC_H_MIN(t->tcm_info);
		__u32 prio = TC_H_MAJ(t->tcm_info)>>16;
		if (!filter_protocol || filter_protocol != protocol) {
			if (protocol) {
				SPRINT_BUF(b1);
				fprintf(fp, "protocol %s ",
					ll_proto_n2a(protocol, b1, sizeof(b1)));
			}
		}
		if (!filter_prio || filter_prio != prio) {
			if (prio)
				fprintf(fp, "pref %u ", prio);
		}
	}
	fprintf(fp, "%s ", (char*)RTA_DATA(tb[TCA_KIND]));
	q = get_filter_kind(RTA_DATA(tb[TCA_KIND]));
	if (tb[TCA_OPTIONS]) {
		if (q)
			q->print_fopt(q, fp, tb[TCA_OPTIONS], t->tcm_handle);
		else
			fprintf(fp, "[cannot parse parameters]");
	}
	fprintf(fp, "\n");

	if (show_stats && (tb[TCA_STATS] || tb[TCA_STATS2])) {
		print_tcstats_attr(fp, tb, " ", NULL);
		fprintf(fp, "\n");
	}

	fflush(fp);
	return 0;
}


int tc_filter_list(int argc, char **argv)
{
	struct tcmsg t;
	char d[16];
	__u32 prio = 0;
	__u32 protocol = 0;
	char *fhandle = NULL;

	memset(&t, 0, sizeof(t));
	t.tcm_family = AF_UNSPEC;
	memset(d, 0, sizeof(d));

	while (argc > 0) {
		if (strcmp(*argv, "dev") == 0) {
			NEXT_ARG();
			if (d[0])
				duparg("dev", *argv);
			strncpy(d, *argv, sizeof(d)-1);
		} else if (strcmp(*argv, "root") == 0) {
			if (t.tcm_parent) {
				fprintf(stderr, "Error: \"root\" is duplicate parent ID\n");
				return -1;
			}
			filter_parent = t.tcm_parent = TC_H_ROOT;
		} else if (strcmp(*argv, "parent") == 0) {
			__u32 handle;
			NEXT_ARG();
			if (t.tcm_parent)
				duparg("parent", *argv);
			if (get_tc_classid(&handle, *argv))
				invarg(*argv, "invalid parent ID");
			filter_parent = t.tcm_parent = handle;
		} else if (strcmp(*argv, "handle") == 0) {
			NEXT_ARG();
			if (fhandle)
				duparg("handle", *argv);
			fhandle = *argv;
		} else if (matches(*argv, "preference") == 0 ||
			   matches(*argv, "priority") == 0) {
			NEXT_ARG();
			if (prio)
				duparg("priority", *argv);
			if (get_u32(&prio, *argv, 0))
				invarg(*argv, "invalid preference");
			filter_prio = prio;
		} else if (matches(*argv, "protocol") == 0) {
			__u16 res;
			NEXT_ARG();
			if (protocol)
				duparg("protocol", *argv);
			if (ll_proto_a2n(&res, *argv))
				invarg(*argv, "invalid protocol");
			protocol = res;
			filter_protocol = protocol;
		} else {
			fprintf(stderr, " What is \"%s\"? Try \"tc filter help\"\n", *argv);
			return -1;
		}

		argc--; argv++;
	}

	t.tcm_info = TC_H_MAKE(prio<<16, protocol);

 	ll_init_map(&rth);

	if (d[0]) {
		if ((t.tcm_ifindex = ll_name_to_index(d)) == 0) {
			fprintf(stderr, "Cannot find device \"%s\"\n", d);
			return 1;
		}
		filter_ifindex = t.tcm_ifindex;
	}

 	if (rtnl_dump_request(&rth, RTM_GETTFILTER, &t, sizeof(t)) < 0) {
		perror("Cannot send dump request");
		return 1;
	}

 	if (rtnl_dump_filter(&rth, print_filter, stdout, NULL, NULL) < 0) {
		fprintf(stderr, "Dump terminated\n");
		return 1;
	}

	return 0;
}


/*
//	add filter rule
		return tc_filter_modify(RTM_NEWTFILTER, NLM_F_EXCL|NLM_F_CREATE, argc-1, argv+1);
//	change filter rule
		return tc_filter_modify(RTM_NEWTFILTER, 0, argc-1, argv+1);
//	replace filter rule
		return tc_filter_modify(RTM_NEWTFILTER, NLM_F_CREATE, argc-1, argv+1);
//	delete filter rule
		return tc_filter_modify(RTM_DELTFILTER, 0,  argc-1, argv+1);
*/
