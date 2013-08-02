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

const char* 
ll_proto_n2a(id, buf, len)
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

static int
pack_key(sel, key, mask, off, offmask)
struct tc_u32_sel* sel;
uint32_t key;
uint32_t mask;
int off;
int offmask;
{
	int i;
	int hwm = sel->nkeys;

	key &= mask;

	for(i = 0; i < hwm; i++) {
		if(sel->keys[i].off == off && sel->keys[i].offmask == offmask) {
			uint32_t intersect = mask&sel->keys[i].mask;

			if((key ^ sel->keys[i].val) & intersect)
				return -1;
			sel->keys[i].val |= key;
			sel->keys[i].mask |= mask;
			return 0;
		}
	}

	if(hwm >= 128)
		return -1;
	if(off % 4)
		return -1;
	sel->keys[hwm].val = key;
	sel->keys[hwm].mask = mask;
	sel->keys[hwm].off = off;
	sel->keys[hwm].offmask = offmask;
	sel->nkeys++;

	return 0;
}

static int
parse_ipv4_dstaddr(match, sel)
struct filter_match match;
struct tc_u32_sel* sel;
{
    inet_prefix addr;
    uint32_t mask;

    if(get_prefix_1(&addr, match.org, AF_INET))
        return -1;

    mask = 0;
    if(addr.bitlen)
        mask = hton(0xFFFFFFFF << (24 - addr.bitlen));
    if(pack_key(sel, addr.data[0], mask, 12, 0))
        return -1;

    return 0;
    
}

static int
parse_u32(sel)
struct tc_u32_sel* sel;
{
    int res = -1;
    uint32_t key;
    uint32_t mask;

    get_u32(&key, '0', 0)
    get_u32(&mask, '0', 0)

    res = pack_key32(sel, key, mask, off, offmask);

    return res;
}

static int
u32_opt(dev, handle)
char* dev;
char* handle;
{
    struct {
        struct tc_u32_sel sel;
        struct tc_u32_key key[128];
    } sel;
    struct tcmsg* t = NLMSG_DATA(n);
    struct rtattr* tail;
    int protocol;

    if(handle) {
        dprintf(("[u32_opt] handle : %s\n", handle));
        if(get_u32_handle(&t->tcm_handle, handle)) {
            fprintf(stderr, "Illegal Filter ID\n");
            return -1;
        }
    }

    tail = NLMSG_TAIL(n);
    addattr_l(n, MAX_MSG, TCA_OPTIONS, NULL, 0);

    parse_u32(sel, );
    
//    if(ipv4) {
    parse_ipv4_dstaddr(match, sel);
//    }

    // flow id
    uint32_t class_id;
    get_tc_classid(&class_id, up.classid);
    addattr_l(n, MAX_MSG, TCA_U32_CLASSID, &class_id, 4);
    sel.sel.flags |= TC_U32_TERMINAL;

    // order
    get_u32(&order, up.order, 0);

    // link
    get_u32_handle(&link, up.link);
    addattr_l(n, MAX_MSG, TCA_U32_LINK, &link, 4);

    // ht
    uint32_t ht;
    get_u32_handle(&ht, up.ht);
    htid = (ht & 0xFFFFF000);

    tail->rta_len = (void*)NLMSG_TAIL(n) - (void*)tail;

    return 0;
}

int 
add_netem_filter(dev, parentid, handleid, protocolid, up)
char* dev;
char* parentid;
char* handleid;
char* protocolid;
struct u32_parameter* up;
{
    int flags = NLM_F_EXCL|NLM_F_CREATE;
	struct {
		struct nlmsghdr 	n;
		struct tcmsg 		t;
		char                buf[MAX_MSG];
	} req;
	struct filter_util* q = NULL;
	__u32 prio = 0;
	__u32 protocol = 0;
	char* fhandle = NULL;
	char  device[16];
	char  filter_kind[16];
	struct tc_estimator est;

	memset(&req, 0, sizeof(req));
	memset(&est, 0, sizeof(est));
	memset(d, 0, sizeof(d));
	memset(k, 0, sizeof(k));
	memset(&req, 0, sizeof(req));


	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct tcmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST|flags;
	req.n.nlmsg_type = RTM_NEWTFILTER;
	req.t.tcm_family = AF_UNSPEC;

	dprintf(("\n[add_netem_filter] filter function\n"));

	strncpy(device, dev, sizeof(device)-1);

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

	// protocol set
	__u16 id;
	if(protocol)
		duparg("protocol", protocolid);
	if(ll_proto_a2n(&id, protocolid))
		invarg(protocolid, "invalid protocol");
	protocol = id;

    if (get_u32(&prio, "16", 0))
        invarg("16", "invalid prpriority value");

	strncpy(filter_kind, "u32", sizeof(filter_kind) - 1);
	q = get_filter_kind(k);

	req.t.tcm_info = TC_H_MAKE(prio << 16, protocol);

	if(filter_kind[0])
		addattr_l(&req.n, sizeof(req), TCA_KIND, k, strlen(k) + 1);

	if(up->rdev) {
		sprintf(dev, "%s", up->rdev);
	}

    u32_opt();

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

int 
tc_filter_modify(cmd, flags, dev, parentid, handleid, protocolid, type, up)
int cmd;
unsigned int flags;
char* dev;
char* parentid;
char* handleid;
char* protocolid;
char* type;
struct u32_parameter* up;
{
	struct {
		struct nlmsghdr 	n;
		struct tcmsg 		t;
		char   			buf[MAX_MSG];
	} req;
	struct filter_util* q = NULL;
	__u32 prio = 0;
	__u32 protocol = 0;
	char* fhandle = NULL;
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

	dprintf(("\nfilter function\n"));

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

/*
	if(fhandle)
		duparg("handle", handleid);
	fhandle = handleid;
*/

	// protocol set
	__u16 id;
	if(protocol)
		duparg("protocol", protocolid);
	if(ll_proto_a2n(&id, protocolid))
		invarg(protocolid, "invalid protocol");
	protocol = id;

    if (get_u32(&prio, "16", 0))
        invarg("16", "invalid prpriority value");

	strncpy(k, type, sizeof(k) - 1);
	q = get_filter_kind(k);

	req.t.tcm_info = TC_H_MAKE(prio << 16, protocol);

	if(k[0])
		addattr_l(&req.n, sizeof(req), TCA_KIND, k, strlen(k)+1);

	if(up->rdev) {
		sprintf(dev, "%s", up->rdev);
	}
	if(q) {
		if(q->parse_fopt(q, fhandle, *up, &req.n, dev))
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


/*
// filter_modify function uasage
// add filter rule
		return tc_filter_modify(RTM_NEWTFILTER, NLM_F_EXCL|NLM_F_CREATE, argc-1, argv+1);
// change filter rule
		return tc_filter_modify(RTM_NEWTFILTER, 0, argc-1, argv+1);
// replace filter rule
		return tc_filter_modify(RTM_NEWTFILTER, NLM_F_CREATE, argc-1, argv+1);
// delete filter rule
		return tc_filter_modify(RTM_DELTFILTER, 0,  argc-1, argv+1);
*/
