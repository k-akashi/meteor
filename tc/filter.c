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
		if(llproto_names[i].id == id) {
			return llproto_names[i].name;
        }
	}
	snprintf(buf, len, "[%d]", id);
	return buf;
}

int
ll_proto_a2n(id, buf)
unsigned short *id;
char *buf;
{
	int i;

	for(i = 0; i < sizeof(llproto_names) / sizeof(llproto_names[0]); i++) {
		if(strcasecmp(llproto_names[i].name, buf) == 0) {
			*id = htons(llproto_names[i].id);
			return 0;
		}
	}

	if(get_u16(id, buf, 0)) {
		return -1;
    }
	*id = htons(*id);

	return 0;
}

int 
add_tc_filter(dev, id, protocolid, type, up)
char* dev;
uint32_t id[4];
char* protocolid;
char* type;
struct u32_params* up;
{
	uint16_t protocol_id;
	uint32_t prio = 16;
	uint32_t protocol = 0;
	uint32_t handle = 0;
	struct tc_estimator est;
	struct {
		struct nlmsghdr n;
		struct tcmsg t;
		char buf[MAX_MSG];
	} req;

	memset(&req, 0, sizeof(req));
	memset(&est, 0, sizeof(est));
	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct tcmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
	req.n.nlmsg_type = RTM_NEWTFILTER;
	req.t.tcm_family = AF_UNSPEC;

    if(id[0] == 0) {
        req.t.tcm_parent = TC_H_ROOT;
    }
    else {
        req.t.tcm_parent = TC_HANDLE(id[0], id[1]);
    }
    dprintf(("[add_tc_filter] parent id = %d\n", req.t.tcm_parent));
	handle = TC_HANDLE(id[2], id[3]);

	if(protocol) {
		duparg("protocol", protocolid);
    }
	if(ll_proto_a2n(&protocol_id, protocolid)) {
		invarg(protocolid, "invalid protocol");
    }
	protocol = protocol_id;
    dprintf(("[add_tc_filter] protocol = %d\n", protocol));
    dprintf(("[ad_tc_filter] kind = %s\n", type));
	req.t.tcm_info = TC_H_MAKE(prio << 16, protocol);
    addattr_l(&req.n, sizeof(req), TCA_KIND, type, strlen(type) + 1);

	if(up->rdev) {
		sprintf(dev, "%s", up->rdev);
	}

    u32_filter_parse(handle, *up, &req.n, dev);

    ll_init_map(&rth);
    if((req.t.tcm_ifindex = ll_name_to_index(dev)) == 0) {
        fprintf(stderr, "Cannot find device \"%s\"\n", dev);
        return 1;
	}

 	if(rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0) {
		fprintf(stderr, "We have an error talking to the kernel\n");
		return 2;
	}

	return 0;
}
