/*
 * q_u32.c		U32 filter.
 *
 *		This program is free software; you can u32istribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *		Match mark added by Catalin(ux aka Dino) BOIE <catab at umbrella.ro> [5 nov 2004]
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
#include <linux/if.h>

#include "utils.h"
#include "tc_util.h"

#define usage() return(-1)

int get_u32_handle(__u32* handle, char* str)
{
	__u32 htid = 0;
	__u32 hash = 0;
	__u32 nodeid = 0;
	dprintf(("get_u32_handle id = %s\n", str));

	char* tmp = strchr(str, ':');

	if(tmp == NULL) {
		if(memcmp("0x", str, 2) == 0)
			return get_u32(handle, str, 16);
		return -1;
	}
	htid = strtoul(str, &tmp, 16);
	if(tmp == str && *str != ':' && *str != 0)
		return -1;
	if(htid >= 0x1000)
		return -1;
	if(*tmp) {
		str = tmp+1;
		hash = strtoul(str, &tmp, 16);
		if(tmp == str && *str != ':' && *str != 0)
			return -1;
		if(hash >= 0x100)
			return -1;
		if(*tmp) {
			str = tmp+1;
			nodeid = strtoul(str, &tmp, 16);
			if(tmp == str && *str != 0)
				return -1;
			if(nodeid >= 0x1000)
				return -1;
		}
	}
	*handle = (htid << 20) | (hash << 12) | nodeid;
	return 0;
}

char*
sprint_u32_handle(__u32 handle, char *buf)
{
	int bsize = SPRINT_BSIZE-1;
	__u32 htid = TC_U32_HTID(handle);
	__u32 hash = TC_U32_HASH(handle);
	__u32 nodeid = TC_U32_NODE(handle);
	char *b = buf;

	if(handle == 0) {
		snprintf(b, bsize, "none");
		return b;
	}
	if(htid) {
		int l = snprintf(b, bsize, "%x:", htid>>20);
		bsize -= l;
		b += l;
	}
	if(nodeid|hash) {
		if(hash) {
			int l = snprintf(b, bsize, "%x", hash);
			bsize -= l;
			b += l;
		}
		if(nodeid) {
			int l = snprintf(b, bsize, ":%x", nodeid);
			bsize -= l;
			b += l;
		}
	}
	if(show_raw)
		snprintf(b, bsize, "[%08x] ", handle);
	return buf;
}

static int
pack_key(sel, key, mask, off, offmask)
struct tc_u32_sel *sel;
__u32 key;
__u32 mask;
int off;
int offmask;
{
	int i;
	int hwm = sel->nkeys;

	key &= mask;

	for(i = 0; i < hwm; i++) {
		if(sel->keys[i].off == off && sel->keys[i].offmask == offmask) {
			__u32 intersect = mask&sel->keys[i].mask;

			if((key^sel->keys[i].val) & intersect)
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
pack_key32(sel, key, mask, off, offmask)
struct tc_u32_sel *sel;
__u32 key;
__u32 mask;
int off;
int offmask;
{
	key = htonl(key);
	mask = htonl(mask);

	return pack_key(sel, key, mask, off, offmask);
}

/*
static int pack_key16(struct tc_u32_sel *sel, __u32 key, __u32 mask, int off, int offmask)
{
	if (key > 0xFFFF || mask > 0xFFFF)
		return -1;

	if ((off & 3) == 0) {
		key <<= 16;
		mask <<= 16;
	}
	off &= ~3;
	key = htonl(key);
	mask = htonl(mask);

	return pack_key(sel, key, mask, off, offmask);
}

static int pack_key8(struct tc_u32_sel *sel, __u32 key, __u32 mask, int off, int offmask)
{
	if (key > 0xFF || mask > 0xFF)
		return -1;

	if ((off & 3) == 0) {
		key <<= 24;
		mask <<= 24;
	} else if ((off & 3) == 1) {
		key <<= 16;
		mask <<= 16;
	} else if ((off & 3) == 2) {
		key <<= 8;
		mask <<= 8;
	}
	off &= ~3;
	key = htonl(key);
	mask = htonl(mask);

	return pack_key(sel, key, mask, off, offmask);
}


int parse_at(int *argc_p, char ***argv_p, int *off, int *offmask)
{
	int argc = *argc_p;
	char **argv = *argv_p;
	char *p = *argv;

	if (argc <= 0)
		return -1;

	if (strlen(p) > strlen("nexthdr+") &&
	    memcmp(p, "nexthdr+", strlen("nexthdr+")) == 0) {
		*offmask = -1;
		p += strlen("nexthdr+");
	} else if (matches(*argv, "nexthdr+") == 0) {
		NEXT_ARG();
		*offmask = -1;
		p = *argv;
	}

	if (get_integer(off, p, 0))
		return -1;
	argc--; argv++;

	*argc_p = argc;
	*argv_p = argv;
	return 0;
}
*/

static int parse_u32(sel, off, offmask)
struct tc_u32_sel *sel;
int off; 
int offmask;
{
	int res = -1;
	__u32 key;
	__u32 mask;

	if(get_u32(&key, "0", 0))
		return -1;

	if(get_u32(&mask, "0", 16))
		return -1;

// at
/*
	if(parse_at(&argc, &argv, &off, &offmask))
		return -1;
	}
*/

	res = pack_key32(sel, key, mask, off, offmask);

	return res;
}

/*
static int parse_u16(int *argc_p, char ***argv_p, struct tc_u32_sel *sel, int off, int offmask)
{
	int res = -1;
	int argc = *argc_p;
	char **argv = *argv_p;
	__u32 key;
	__u32 mask;

	if (argc < 2)
		return -1;

	if (get_u32(&key, *argv, 0))
		return -1;
	argc--; argv++;

	if (get_u32(&mask, *argv, 16))
		return -1;
	argc--; argv++;

	if (argc > 0 && strcmp(argv[0], "at") == 0) {
		NEXT_ARG();
		if (parse_at(&argc, &argv, &off, &offmask))
			return -1;
	}
	res = pack_key16(sel, key, mask, off, offmask);
	*argc_p = argc;
	*argv_p = argv;
	return res;
}

static int parse_u8(int *argc_p, char ***argv_p, struct tc_u32_sel *sel, int off, int offmask)
{
	int res = -1;
	int argc = *argc_p;
	char **argv = *argv_p;
	__u32 key;
	__u32 mask;

	if (argc < 2)
		return -1;

	if (get_u32(&key, *argv, 0))
		return -1;
	argc--; argv++;

	if (get_u32(&mask, *argv, 16))
		return -1;
	argc--; argv++;

	if (key > 0xFF || mask > 0xFF)
		return -1;

	if (argc > 0 && strcmp(argv[0], "at") == 0) {
		NEXT_ARG();
		if (parse_at(&argc, &argv, &off, &offmask))
			return -1;
	}

	res = pack_key8(sel, key, mask, off, offmask);
	*argc_p = argc;
	*argv_p = argv;
	return res;
}
*/

static int
parse_ip_addr(match, sel, off)
struct filter_match match;
struct tc_u32_sel* sel;
int off;
{
	int res = -1;
	inet_prefix addr;
	__u32 mask;
	int offmask = 0;

	if(get_prefix_1(&addr, match.arg, AF_INET))
		return -1;

/*
	if(argc > 0 && strcmp(argv[0], "at") == 0) {
		NEXT_ARG();
		if (parse_at(&argc, &argv, &off, &offmask))
			return -1;
	}
*/

	mask = 0;
	if(addr.bitlen)
		mask = htonl(0xFFFFFFFF << (32 - addr.bitlen));
	if(pack_key(sel, addr.data[0], mask, off, offmask) < 0)
		return -1;
	res = 0;

	return res;
}

/*
static int parse_ip6_addr(int *argc_p, char ***argv_p, struct tc_u32_sel *sel, int off)
{
	int res = -1;
	int argc = *argc_p;
	char **argv = *argv_p;
	int plen = 128;
	int i;
	inet_prefix addr;
	int offmask = 0;

	if (argc < 1)
		return -1;

	if (get_prefix_1(&addr, *argv, AF_INET6))
		return -1;
	argc--; argv++;

	if (argc > 0 && strcmp(argv[0], "at") == 0) {
		NEXT_ARG();
		if (parse_at(&argc, &argv, &off, &offmask))
			return -1;
	}

	plen = addr.bitlen;
	for (i=0; i<plen; i+=32) {
//		if (((i+31)&~0x1F)<=plen) {
		if (((i+31))<=plen) {
			if ((res = pack_key(sel, addr.data[i/32], 0xFFFFFFFF, off+4*(i/32), offmask)) < 0)
				return -1;
		} else if (i<plen) {
			__u32 mask = htonl(0xFFFFFFFF<<(32-(plen-i)));
			if ((res = pack_key(sel, addr.data[i/32], mask, off+4*(i/32), offmask)) < 0)
				return -1;
		}
	}
	res = 0;

	*argc_p = argc;
	*argv_p = argv;
	return res;
}
*/

static int
parse_ip(match, sel)
struct filter_match match;
struct tc_u32_sel *sel;
{
	int res = -1;

	if(strcmp(match.filter, "src") == 0) {
		res = parse_ip_addr(match, sel, 12);
		goto done;
	}
	if(strcmp(match.filter, "dst") == 0) {
		res = parse_ip_addr(match, sel, 16);
		goto done;
	}
/*
	if (strcmp(*argv, "tos") == 0 ||
	    matches(*argv, "dsfield") == 0) {
		NEXT_ARG();
		res = parse_u8(&argc, &argv, sel, 1, 0);
		goto done;
	}
	if (strcmp(*argv, "ihl") == 0) {
		NEXT_ARG();
		res = parse_u8(&argc, &argv, sel, 0, 0);
		goto done;
	}
	if (strcmp(*argv, "protocol") == 0) {
		NEXT_ARG();
		res = parse_u8(&argc, &argv, sel, 9, 0);
		goto done;
	}
	if (matches(*argv, "precedence") == 0) {
		NEXT_ARG();
		res = parse_u8(&argc, &argv, sel, 1, 0);
		goto done;
	}
	if (strcmp(*argv, "nofrag") == 0) {
		argc--; argv++;
		res = pack_key16(sel, 0, 0x3FFF, 6, 0);
		goto done;
	}
	if (strcmp(*argv, "firstfrag") == 0) {
		argc--; argv++;
		res = pack_key16(sel, 0, 0x1FFF, 6, 0);
		goto done;
	}
	if (strcmp(*argv, "df") == 0) {
		argc--; argv++;
		res = pack_key16(sel, 0x4000, 0x4000, 6, 0);
		goto done;
	}
	if (strcmp(*argv, "mf") == 0) {
		argc--; argv++;
		res = pack_key16(sel, 0x2000, 0x2000, 6, 0);
		goto done;
	}
	if (strcmp(*argv, "dport") == 0) {
		NEXT_ARG();
		res = parse_u16(&argc, &argv, sel, 22, 0);
		goto done;
	}
	if (strcmp(*argv, "sport") == 0) {
		NEXT_ARG();
		res = parse_u16(&argc, &argv, sel, 20, 0);
		goto done;
	}
	if (strcmp(*argv, "icmp_type") == 0) {
		NEXT_ARG();
		res = parse_u8(&argc, &argv, sel, 20, 0);
		goto done;
	}
	if (strcmp(*argv, "icmp_code") == 0) {
		NEXT_ARG();
		res = parse_u8(&argc, &argv, sel, 20, 1);
		goto done;
	}
*/
	return -1;

done:
	return res;
}

/*
static int parse_ip6(int *argc_p, char ***argv_p, struct tc_u32_sel *sel)
{
	int res = -1;
	int argc = *argc_p;
	char **argv = *argv_p;

	if (argc < 2)
		return -1;

	if (strcmp(*argv, "src") == 0) {
		NEXT_ARG();
		res = parse_ip6_addr(&argc, &argv, sel, 8);
		goto done;
	}
	if (strcmp(*argv, "dst") == 0) {
		NEXT_ARG();
		res = parse_ip6_addr(&argc, &argv, sel, 24);
		goto done;
	}
	if (strcmp(*argv, "priority") == 0) {
		NEXT_ARG();
		res = parse_u8(&argc, &argv, sel, 4, 0);
		goto done;
	}
	if (strcmp(*argv, "protocol") == 0) {
		NEXT_ARG();
		res = parse_u8(&argc, &argv, sel, 6, 0);
		goto done;
	}
	if (strcmp(*argv, "flowlabel") == 0) {
		NEXT_ARG();
		res = parse_u32(&argc, &argv, sel, 0, 0);
		goto done;
	}
	if (strcmp(*argv, "dport") == 0) {
		NEXT_ARG();
		res = parse_u16(&argc, &argv, sel, 42, 0);
		goto done;
	}
	if (strcmp(*argv, "sport") == 0) {
		NEXT_ARG();
		res = parse_u16(&argc, &argv, sel, 40, 0);
		goto done;
	}
	if (strcmp(*argv, "icmp_type") == 0) {
		NEXT_ARG();
		res = parse_u8(&argc, &argv, sel, 40, 0);
		goto done;
	}
	if (strcmp(*argv, "icmp_code") == 0) {
		NEXT_ARG();
		res = parse_u8(&argc, &argv, sel, 41, 1);
		goto done;
	}
	return -1;

done:
	*argc_p = argc;
	*argv_p = argv;
	return res;
}

#define parse_tcp parse_udp
static int parse_udp(int *argc_p, char ***argv_p, struct tc_u32_sel *sel)
{
	int res = -1;
	int argc = *argc_p;
	char **argv = *argv_p;

	if (argc < 2)
		return -1;

	if (strcmp(*argv, "src") == 0) {
		NEXT_ARG();
		res = parse_u16(&argc, &argv, sel, 0, -1);
		goto done;
	}
	if (strcmp(*argv, "dst") == 0) {
		NEXT_ARG();
		res = parse_u16(&argc, &argv, sel, 2, -1);
		goto done;
	}
	return -1;

done:
	*argc_p = argc;
	*argv_p = argv;
	return res;
}

static int parse_icmp(int *argc_p, char ***argv_p, struct tc_u32_sel *sel)
{
	int res = -1;
	int argc = *argc_p;
	char **argv = *argv_p;

	if (argc < 2)
		return -1;

	if (strcmp(*argv, "type") == 0) {
		NEXT_ARG();
		res = parse_u8(&argc, &argv, sel, 0, -1);
		goto done;
	}
	if (strcmp(*argv, "code") == 0) {
		NEXT_ARG();
		res = parse_u8(&argc, &argv, sel, 1, -1);
		goto done;
	}
	return -1;

done:
	*argc_p = argc;
	*argv_p = argv;
	return res;
}

static int parse_mark(int *argc_p, char ***argv_p, struct nlmsghdr *n)
{
	int res = -1;
	int argc = *argc_p;
	char **argv = *argv_p;
	struct tc_u32_mark mark;

	if (argc <= 1)
		return -1;

	if (get_u32(&mark.val, *argv, 0)) {
		fprintf(stderr, "Illegal \"mark\" value\n");
		return -1;
	}
	NEXT_ARG();

	if (get_u32(&mark.mask, *argv, 0)) {
		fprintf(stderr, "Illegal \"mark\" mask\n");
		return -1;
	}
	NEXT_ARG();

	if ((mark.val & mark.mask) != mark.val) {
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

	if (matches(match.protocol, "u32") == 0) {
		res = parse_u32(sel, 0, 0);
		goto done;
	}
/* not implemented
	if (matches(*argv, "u16") == 0) {
		NEXT_ARG();
		res = parse_u16(&argc, &argv, sel, 0, 0);
		goto done;
	}
	if (matches(*argv, "u8") == 0) {
		NEXT_ARG();
		res = parse_u8(&argc, &argv, sel, 0, 0);
		goto done;
	}
*/
	if(matches(match.protocol, "ip") == 0) {
		res = parse_ip(match, sel);
		goto done;
	}
/*
	if (matches(*argv, "ip6") == 0) {
		NEXT_ARG();
		res = parse_ip6(&argc, &argv, sel);
		goto done;
	}
	if (matches(*argv, "udp") == 0) {
		NEXT_ARG();
		res = parse_udp(&argc, &argv, sel);
		goto done;
	}
	if (matches(*argv, "tcp") == 0) {
		NEXT_ARG();
		res = parse_tcp(&argc, &argv, sel);
		goto done;
	}
	if (matches(*argv, "icmp") == 0) {
		NEXT_ARG();
		res = parse_icmp(&argc, &argv, sel);
		goto done;
	}
	if (matches(*argv, "mark") == 0) {
		NEXT_ARG();
		res = parse_mark(&argc, &argv, n);
		goto done;
	}
*/

	return -1;

done:
	return res;
}

/*
static int parse_offset(int *argc_p, char ***argv_p, struct tc_u32_sel *sel)
{
	int argc = *argc_p;
	char **argv = *argv_p;

	while (argc > 0) {
		if (matches(*argv, "plus") == 0) {
			int off;
			NEXT_ARG();
			if (get_integer(&off, *argv, 0))
				return -1;
			sel->off = off;
			sel->flags |= TC_U32_OFFSET;
		} else if (matches(*argv, "at") == 0) {
			int off;
			NEXT_ARG();
			if (get_integer(&off, *argv, 0))
				return -1;
			sel->offoff = off;
			if (off%2) {
				fprintf(stderr, "offset \"at\" must be even\n");
				return -1;
			}
			sel->flags |= TC_U32_VAROFFSET;
		} else if (matches(*argv, "mask") == 0) {
			__u16 mask;
			NEXT_ARG();
			if (get_u16(&mask, *argv, 16))
				return -1;
			sel->offmask = htons(mask);
			sel->flags |= TC_U32_VAROFFSET;
		} else if (matches(*argv, "shift") == 0) {
			int shift;
			NEXT_ARG();
			if (get_integer(&shift, *argv, 0))
				return -1;
			sel->offshift = shift;
			sel->flags |= TC_U32_VAROFFSET;
		} else if (matches(*argv, "eat") == 0) {
			sel->flags |= TC_U32_EAT;
		} else {
			break;
		}
		argc--; argv++;
	}

	*argc_p = argc;
	*argv_p = argv;
	return 0;
}

static int parse_hashkey(int *argc_p, char ***argv_p, struct tc_u32_sel *sel)
{
	int argc = *argc_p;
	char **argv = *argv_p;

	while (argc > 0) {
		if (matches(*argv, "mask") == 0) {
			__u32 mask;
			NEXT_ARG();
			if (get_u32(&mask, *argv, 16))
				return -1;
			sel->hmask = htonl(mask);
		} else if (matches(*argv, "at") == 0) {
			int num;
			NEXT_ARG();
			if (get_integer(&num, *argv, 0))
				return -1;
			if (num%4)
				return -1;
			sel->hoff = num;
		} else {
			break;
		}
		argc--; argv++;
	}

	*argc_p = argc;
	*argv_p = argv;
	return 0;
}
*/

static int 
u32_parse_opt(qu, handle, up, n, dev)
struct filter_util *qu;
char *handle;
struct u32_parameter up;
struct nlmsghdr *n;
char* dev;
{
	struct {
		struct tc_u32_sel sel;
		struct tc_u32_key keys[128];
	} sel;
	struct tcmsg *t = NLMSG_DATA(n);
	struct rtattr *tail;
	int sel_ok = 0;
	int sample_ok = 0;
	__u32 htid = 0;
	__u32 order = 0;

	dprintf(("u32_parse_opt\n"));
	memset(&sel, 0, sizeof(sel));

	if(handle) {
		dprintf(("handle : %s\n", handle));
		if(get_u32_handle(&t->tcm_handle, handle)) {
			fprintf(stderr, "Illegal filter ID\n");
			return -1;
		}
	}

	tail = NLMSG_TAIL(n);
	addattr_l(n, MAX_MSG, TCA_OPTIONS, NULL, 0);

	// match
	if(up.match.type) {
		if(parse_selector(up.match, &sel.sel, n)) {
			fprintf(stderr, "Illegal \"match\"\n");
			return -1;
		}
		sel_ok++;
	}

/* not implemented
	// offset
	if(parse_offset(&argc, up.offset, &sel.sel)) {
		fprintf(stderr, "Illegal \"offset\"\n");
		return -1;
	}

	// hashkey
	if(parse_hashkey(&argc, up.hashkey, &sel.sel)) {
		fprintf(stderr, "Illegal \"hashkey\"\n");
		return -1;
	}
*/

	// classid and flowid
	if(up.classid) {
		unsigned classid;
		if(get_tc_classid(&classid, up.classid)) {
			fprintf(stderr, "Illegal \"classid\"\n");
			return -1;
		}
		addattr_l(n, MAX_MSG, TCA_U32_CLASSID, &classid, 4);
		sel.sel.flags |= TC_U32_TERMINAL;
	}

	// divisor
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

	// order
	if(up.order) {
		if(get_u32(&order, up.order, 0)) {
			fprintf(stderr, "Illegal \"order\"\n");
			return -1;
		}
	}

	// link
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

	// ht
	if(up.ht) {
		unsigned ht;
		if(get_u32_handle(&ht, up.ht)) {
			fprintf(stderr, "Illegal \"ht\"\n");
			return -1;
		}
		if (ht && TC_U32_NODE(ht)) {
			fprintf(stderr, "\"ht\" must be a hash table.\n");
			return -1;
		}
		if (sample_ok)
			htid = (htid & 0xFF000) | (ht & 0xFFF00000);
		else
			htid = (ht & 0xFFFFF000);
	}

	// action
	if(up.action) {
		if(parse_action(up.action, TCA_U32_ACT, n, dev)) {
			fprintf(stderr, "Illegal \"action\"\n");
			return -1;
		}
	}

/* not implemented
	// police
	if(parse_police(&argc, up.police, TCA_U32_POLICE, n)) {
		fprintf(stderr, "Illegal \"police\"\n");
		return -1;
	}
*/

	if(order) {
		if(TC_U32_NODE(t->tcm_handle) && order != TC_U32_NODE(t->tcm_handle)) {
			fprintf(stderr, "\"order\" contradicts \"handle\"\n");
			return -1;
		}
		t->tcm_handle |= order;
	}

	if(htid)
		addattr_l(n, MAX_MSG, TCA_U32_HASH, &htid, 4);
	if(sel_ok)
		addattr_l(n, MAX_MSG, TCA_U32_SEL, &sel, sizeof(sel.sel)+sel.sel.nkeys*sizeof(struct tc_u32_key));

	tail->rta_len = (void *) NLMSG_TAIL(n) - (void *) tail;

	return 0;
}

struct filter_util u32_filter_util = {
	.id = "u32",
	.parse_fopt = u32_parse_opt,
	//.print_fopt = u32_print_opt,
};
