#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <linux/if.h>

#include "utils.h"
#include "tc_util.h"

int
get_u32_handle(handle, str)
uint32_t* handle;
char* str;
{
    char *tmp;
	uint32_t htid;
	uint32_t hash;
	uint32_t nodeid;
    
    tmp = strchr(str, ':');
    htid = 0;
    hash = 0;
    nodeid = 0;

	if(tmp == NULL) {
		if(memcmp("0x", str, 2) == 0) {
			return get_u32(handle, str, 16);
        }
		return -1;
	}
	htid = strtoul(str, &tmp, 16);
	if(tmp == str && *str != ':' && *str != 0) {
		return -1;
    }
	if(htid >= 0x1000) {
		return -1;
    }
	if(*tmp) {
		str = tmp + 1;
		hash = strtoul(str, &tmp, 16);
		if(tmp == str && *str != ':' && *str != 0) {
			return -1;
        }
		if(hash >= 0x100) {
			return -1;
        }
		if(*tmp) {
			str = tmp + 1;
			nodeid = strtoul(str, &tmp, 16);
			if(tmp == str && *str != 0) {
				return -1;
            }
			if(nodeid >= 0x1000) {
				return -1;
            }
		}
	}
	*handle = (htid << 20) | (hash << 12) | nodeid;
	return 0;
}

static int
pack_key(sel, key, mask, off, offmask)
struct tc_u32_sel *sel;
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

			if((key ^ sel->keys[i].val) & intersect) {
				return -1;
            }
			sel->keys[i].val |= key;
			sel->keys[i].mask |= mask;

			return 0;
		}
	}

	if(hwm >= 128) {
		return -1;
    }
	if(off % 4) {
		return -1;
    }
	sel->keys[hwm].val = key;
	sel->keys[hwm].mask = mask;
	sel->keys[hwm].off = off;
	sel->keys[hwm].offmask = offmask;
	sel->nkeys++;

	return 0;
}

static int
pack_key32(sel, key, mask, off, offmask)
struct tc_u32_sel *sel;
uint32_t key;
uint32_t mask;
int off;
int offmask;
{
	key = htonl(key);
	mask = htonl(mask);

	return pack_key(sel, key, mask, off, offmask);
}

static int
pack_key16(sel, key, mask, off, offmask)
struct tc_u32_sel *sel;
uint32_t key;
uint32_t mask;
int off;
int offmask;
{
	if(key > 0xFFFF || mask > 0xFFFF) {
		return -1;
    }

	if((off & 3) == 0) {
		key <<= 16;
		mask <<= 16;
	}
	off &= ~3;

	key = htonl(key);
	mask = htonl(mask);

	return pack_key(sel, key, mask, off, offmask);
}

static int
pack_key8(sel, key, mask, off, offmask)
struct tc_u32_sel *sel;
uint32_t key;
uint32_t mask;
int off;
int offmask;
{
	if(key > 0xFF || mask > 0xFF) {
		return -1;
    }

	if((off & 3) == 0) {
		key <<= 24;
		mask <<= 24;
	}
    else if((off & 3) == 1) {
		key <<= 16;
		mask <<= 16;
	}
    else if((off & 3) == 2) {
		key <<= 8;
		mask <<= 8;
	}
	off &= ~3;

	key = htonl(key);
	mask = htonl(mask);

	return pack_key(sel, key, mask, off, offmask);
}

/*
int
parse_at()
int *off;
int *offmask;
{
	char **argv = *argv_p;
	char *p = *argv;

	if(strlen(p) > strlen("nexthdr+") && memcmp(p, "nexthdr+", strlen("nexthdr+")) == 0) {
		*offmask = -1;
		p += strlen("nexthdr+");
	}
    else if(matches(*argv, "nexthdr+") == 0) {
		NEXT_ARG();
		*offmask = -1;
		p = *argv;
	}

	if(get_integer(off, p, 0)) {
		return -1;
    }

	return 0;
}
*/

static int parse_u32(sel, off, offmask)
struct tc_u32_sel *sel;
int off; 
int offmask;
{
	int res = -1;
	uint32_t key;
	uint32_t mask;

	if(get_u32(&key, "0", 0)) {
		return -1;
    }
	if(get_u32(&mask, "0", 16)) {
		return -1;
    }

/* at
	if(parse_at(&argc, &argv, &off, &offmask)) {
		return -1;
	}
*/

	res = pack_key32(sel, key, mask, off, offmask);

	return res;
}

static int
parse_u16(sel, off, offmask)
struct tc_u32_sel *sel;
int off;
int offmask;
{
	int res = -1;
	uint32_t key;
	uint32_t mask;

	if(get_u32(&key, "0", 0)) {
		return -1;
    }
	if(get_u32(&mask, "0", 16)) {
		return -1;
    }

/*
    if(parse_at(&argc, &argv, &off, &offmask)) {
        return -1;
	}
*/

	res = pack_key16(sel, key, mask, off, offmask);

	return res;
}

static int
parse_u8(sel, off, offmask)
struct tc_u32_sel *sel;
int off;
int offmask;
{
	int res = -1;
	uint32_t key;
	uint32_t mask;

	if(get_u32(&key, "0", 0)) {
		return -1;
    }
	if(get_u32(&mask, "0", 16)) {
		return -1;
    }

/*
    if(parse_at(&argc, &argv, &off, &offmask)) {
        return -1;
	}
*/

	res = pack_key8(sel, key, mask, off, offmask);

	return res;
}

static int
parse_ip_addr(match, sel, off)
struct filter_match match;
struct tc_u32_sel* sel;
int off;
{
	int res = -1;
	inet_prefix addr;
	uint32_t mask;
	int offmask = 0;

	if(get_prefix_1(&addr, match.arg, AF_INET)) {
		return -1;
    }

/* at
    if(parse_at(match.at, &off, &offmask)) {
			return -1;
	}
*/

	mask = 0;
	if(addr.bitlen) {
		mask = htonl(0xFFFFFFFF << (32 - addr.bitlen));
    }
	if(pack_key(sel, addr.data[0], mask, off, offmask) < 0) {
		return -1;
    }
	res = 0;

	return res;
}

static int
parse_ip6_addr(match, sel, off)
struct filter_match match;
struct tc_u32_sel *sel;
int off;
{
	int i;
	int res;
	int plen;
	inet_prefix addr;
	int offmask;

    res = -1;
    plen = 128;
    offmask = 0;

	if(get_prefix_1(&addr, match.arg, AF_INET6)) {
		return -1;
    }

/*
    if(parse_at(match.at, &off, &offmask)) {
        return -1;
	}
*/

	plen = addr.bitlen;

	for(i = 0; i < plen; i += 32) {
//		if(((i + 31) &~ 0x1F) <= plen) {
		if(((i + 31)) <= plen) {
			if((res = pack_key(sel, addr.data[i / 32], 0xFFFFFFFF, off + 4 * (i/32), offmask)) < 0) {
				return -1;
            }
		}
        else if(i < plen) {
			uint32_t mask = htonl(0xFFFFFFFF << (32 - (plen - i)));
			if((res = pack_key(sel, addr.data[i /32], mask, off + 4 * (i / 32), offmask)) < 0) {
				return -1;
            }
		}
	}
	res = 0;

	return res;
}

static int
parse_ip(match, sel)
struct filter_match match;
struct tc_u32_sel *sel;
{
	int res = -1;

	if(strcmp(match.filter, "src") == 0) {
		res = parse_ip_addr(match, sel, 12);
        return res;
	}
	if(strcmp(match.filter, "dst") == 0) {
		res = parse_ip_addr(match, sel, 16);
        return res;
	}
	if(strcmp(match.filter, "tos") == 0 || matches(match.filter, "dsfield") == 0) {
		res = parse_u8(match, sel, 1, 0);
        return res;
	}
	if(strcmp(match.filter, "ihl") == 0) {
		res = parse_u8(match, sel, 0, 0);
        return res;
	}
	if(strcmp(match.filter, "protocol") == 0) {
		res = parse_u8(match, sel, 9, 0);
        return res;
	}
	if(matches(match.filter, "precedence") == 0) {
		res = parse_u8(match, sel, 1, 0);
		return res;
	}
	if(strcmp(match.filter, "nofrag") == 0) {
		res = pack_key16(sel, 0, 0x3FFF, 6, 0);
		return res;
	}
	if(strcmp(match.filter, "firstfrag") == 0) {
		res = pack_key16(sel, 0, 0x1FFF, 6, 0);
		return res;
	}
	if(strcmp(match.filter, "df") == 0) {
		res = pack_key16(sel, 0x4000, 0x4000, 6, 0);
		return res;
	}
	if(strcmp(match.filter, "mf") == 0) {
		res = pack_key16(sel, 0x2000, 0x2000, 6, 0);
		return res;
	}
	if(strcmp(match.filter, "dport") == 0) {
		res = parse_u16(match, sel, 22, 0);
		return res;
	}
	if(strcmp(match.filter, "sport") == 0) {
		res = parse_u16(match, sel, 20, 0);
		return res;
	}
	if(strcmp(match.filter, "icmp_type") == 0) {
		res = parse_u8(match, sel, 20, 0);
		return res;
	}
	if(strcmp(match.filter, "icmp_code") == 0) {
		res = parse_u8(match, sel, 20, 1);
		return res;
	}

	return -1;
}

static int
parse_ip6(match, sel)
struct filter_match match;
struct tc_u32_sel *sel;
{
	int res = -1;

	if(strcmp(match.filter, "src") == 0) {
		res = parse_ip6_addr(match, sel, 8);
		return res;
	}
	if(strcmp(match.filter, "dst") == 0) {
		res = parse_ip6_addr(match, sel, 24);
		return res;
	}
	if(strcmp(match.filter, "priority") == 0) {
		res = parse_u8(match, sel, 4, 0);
		return res;
	}
	if(strcmp(match.filter, "protocol") == 0) {
		res = parse_u8(match, sel, 6, 0);
		return res;
	}
	if(strcmp(match.filter, "flowlabel") == 0) {
		res = parse_u32(match, sel, 0, 0);
		return res;
	}
	if(strcmp(match.filter, "dport") == 0) {
		res = parse_u16(match, sel, 42, 0);
		return res;
	}
	if(strcmp(match.filter, "sport") == 0) {
		res = parse_u16(match, sel, 40, 0);
		return res;
	}
	if(strcmp(match.filter, "icmp_type") == 0) {
		res = parse_u8(match, sel, 40, 0);
		return res;
	}
	if(strcmp(match.filter, "icmp_code") == 0) {
		res = parse_u8(match, sel, 41, 1);
		return res;
	}

	return -1;
}

#define parse_tcp parse_udp
static int
parse_udp(match, sel)
struct filter_match match;
struct tc_u32_sel *sel;
{
	int res = -1;

	if(strcmp(match.filter, "src") == 0) {
		res = parse_u16(match, sel, 0, -1);
        return res;
	}
	if(strcmp(match.filter, "dst") == 0) {
		res = parse_u16(match, sel, 2, -1);
	    return res;
	}

	return -1;
}

/*
static
int parse_icmp(match, sel)
struct filter_match match;
struct tc_u32_sel *sel;
{
	int res = -1;

	if(strcmp(match.filter, "type") == 0) {
		res = parse_u8(match, sel, 0, -1);
        return res;
	}
	if(strcmp(match.filter, "code") == 0) {
		res = parse_u8(match, sel, 1, -1);
        return res;
	}

	return -1;
}

static int parse_mark(int *argc_p, char ***argv_p, struct nlmsghdr *n)
{
	int res = -1;
	int argc = *argc_p;
	char **argv = *argv_p;
	struct tc_u32_mark mark;

	if(argc <= 1)
		return -1;

	if(get_u32(&mark.val, *argv, 0)) {
		fprintf(stderr, "Illegal \"mark\" value\n");
		return -1;
	}
	NEXT_ARG();

	if(get_u32(&mark.mask, *argv, 0)) {
		fprintf(stderr, "Illegal \"mark\" mask\n");
		return -1;
	}
	NEXT_ARG();

	if((mark.val & mark.mask) != mark.val) {
		fprintf(stderr, "Illegal \"mark\" (impossible combination)\n");
		return -1;
	}

	addattr_l(n, MAX_MSG, TCA_U32_MARK, &mark, sizeof(mark));
	res = 0;

	*argc_p = argc;
	*argv_p = argv;
	return res;
}
*/

static int
parse_selector(match, sel, n)
struct filter_match match;
struct tc_u32_sel *sel;
struct nlmsghdr *n;
{
	int res = -1;

    dprintf(("[parse_selector] match.protocol : %s\n", match.protocol));
	if(matches(match.protocol, "u32") == 0) {
		res = parse_u32(sel, 0, 0);
	    return res;
	}
	if(matches(match.protocol, "u16") == 0) {
		res = parse_u16(match, sel, 0, 0);
	    return res;
	}
	if(matches(match.protocol, "u8") == 0) {
		res = parse_u8(match, sel, 0, 0);
	    return res;
	}
	if(matches(match.protocol, "ip") == 0) {
		res = parse_ip(match, sel);
	    return res;
	}
	if(matches(match.protocol, "ip6") == 0) {
		res = parse_ip6(match, sel);
	    return res;
	}
/*
	if(matches(*argv, "udp") == 0) {
		NEXT_ARG();
		res = parse_udp(&argc, &argv, sel);
	    return res;
	}
	if(matches(*argv, "tcp") == 0) {
		NEXT_ARG();
		res = parse_tcp(&argc, &argv, sel);
	    return res;
	}
	if(matches(*argv, "icmp") == 0) {
		NEXT_ARG();
		res = parse_icmp(&argc, &argv, sel);
	    return res;
	}
	if(matches(*argv, "mark") == 0) {
		NEXT_ARG();
		res = parse_mark(&argc, &argv, n);
	    return res;
	}
*/

	return -1;

}

static int
parse_offset(match, sel)
struct filter_match match;
struct tc_u32_sel *sel;
{
    if(matches(match.filter, "plus") == 0) {
        int off;
        if(get_integer(&off, match.arg, 0)) {
            return -1;
        }
        sel->off = off;
        sel->flags |= TC_U32_OFFSET;
    }
    else if(matches(match.filter, "at") == 0) {
        int off;
        if(get_integer(&off, match.arg, 0)) {
            return -1;
        }
        sel->offoff = off;
        if(off % 2) {
            return -1;
        }
        sel->flags |= TC_U32_VAROFFSET;
    }
    else if(matches(match.filter, "mask") == 0) {
		uint16_t mask;
		if(get_u16(&mask, match.arg, 16)) {
			return -1;
        }
		sel->offmask = htons(mask);
		sel->flags |= TC_U32_VAROFFSET;
	}
    else if(matches(match.filter, "shift") == 0) {
		int shift;
		if(get_integer(&shift, match.arg, 0)) {
			return -1;
        }
		sel->offshift = shift;
		sel->flags |= TC_U32_VAROFFSET;
	}
    else if(matches(match.filter, "eat") == 0) {
		sel->flags |= TC_U32_EAT;
	}

	return 0;
}

static int
parse_hashkey(match, sel)
struct filter_match match;
struct tc_u32_sel *sel;
{
	if(matches(match.filter, "mask") == 0) {
		uint32_t mask;
		if(get_u32(&mask, match.arg, 16)) {
			return -1;
        }
		sel->hmask = htonl(mask);
	}
    else if(matches(match.filter, "at") == 0) {
		int num;
		if(get_integer(&num, match.arg, 0)) {
			return -1;
        }
		if(num % 4) {
			return -1;
        }
		sel->hoff = num;
	}

	return 0;
}

int
u32_filter_parse(handle, up, n, dev)
uint32_t handle;
struct u32_parameter up;
struct nlmsghdr *n;
char* dev;
{
    int i;
	int sel_ok = 0;
	uint32_t htid = 0;
	uint32_t order = 0;
	struct rtattr *tail;
	struct tcmsg *t = NLMSG_DATA(n);
	struct {
		struct tc_u32_sel sel;
		struct tc_u32_key keys[128];
	} sel;

	memset(&sel, 0, sizeof(sel));

    dprintf(("[u32_filter_parse] initalize\n"));
    dprintf(("[u32_filter_parse] handle : %d\n", handle));
    t->tcm_handle =  handle;

	tail = NLMSG_TAIL(n);
	addattr_l(n, MAX_MSG, TCA_OPTIONS, NULL, 0);

    for(i = 0; i < FILTER_MAX; i++) {
    	if(up.match[i].type) {
            dprintf(("FILTER Type ; %d\n", i));
    		if(parse_selector(up.match[i], &sel.sel, n)) {
    			fprintf(stderr, "Illegal \"match\"\n");
    			return -1;
    		}
    		sel_ok++;
    	}
    }
	if(up.classid) {
		unsigned classid;
        classid = TC_HANDLE(up.classid[0], up.classid[1]);
		addattr_l(n, MAX_MSG, TCA_U32_CLASSID, &classid, 4);
		sel.sel.flags |= TC_U32_TERMINAL;
        dprintf(("[u32_filter_parse] classid : %d\n", classid));
	}
	if(up.divisor) {
		unsigned divisor;
		if(get_unsigned(&divisor, up.divisor, 0) || 
		    divisor == 0 ||
		    divisor > 0x100 || ((divisor - 1) & divisor)) {
			fprintf(stderr, "Illegal \"divisor\"\n");
			return -1;
		}
		addattr_l(n, MAX_MSG, TCA_U32_DIVISOR, &divisor, 4);
	}
	if(up.order) {
		if(get_u32(&order, up.order, 0)) {
			fprintf(stderr, "Illegal \"order\"\n");
			return -1;
		}
	}
	if(up.link) {
		unsigned link;
		if(get_u32_handle(&link, up.link)) {
			fprintf(stderr, "Illegal \"link\"\n");
			return -1;
		}
		if(link && TC_U32_NODE(link)) {
			fprintf(stderr, "\"link\" must be a hash table.\n");
			return -1;
		}
		addattr_l(n, MAX_MSG, TCA_U32_LINK, &link, 4);
	}
	if(up.ht) {
		unsigned ht;
		if(get_u32_handle(&ht, up.ht)) {
			fprintf(stderr, "Illegal \"ht\"\n");
			return -1;
		}
		if(ht && TC_U32_NODE(ht)) {
			fprintf(stderr, "\"ht\" must be a hash table.\n");
			return -1;
		}
        htid = (ht & 0xFFFFF000);
	}
	if(up.action) {
		if(parse_action(up.action, TCA_U32_ACT, n, dev)) {
			fprintf(stderr, "Illegal \"action\"\n");
			return -1;
		}
	}
	if(order) {
		if(TC_U32_NODE(t->tcm_handle) && order != TC_U32_NODE(t->tcm_handle)) {
			fprintf(stderr, "\"order\" contradicts \"handle\"\n");
			return -1;
		}
		t->tcm_handle |= order;
	}

	if(htid) {
		addattr_l(n, MAX_MSG, TCA_U32_HASH, &htid, 4);
    }
	if(sel_ok) {
		addattr_l(n, MAX_MSG, TCA_U32_SEL, &sel, sizeof(sel.sel)+sel.sel.nkeys*sizeof(struct tc_u32_key));
    }

	tail->rta_len = (void *) NLMSG_TAIL(n) - (void *) tail;

	return 0;
}
