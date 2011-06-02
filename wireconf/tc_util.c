/*
 * tc_util.c		Misc TC utility functions.
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
#include <math.h>

#include "utils.h"
#include "tc_util.h"

int
get_qdisc_handle(h, str)
uint32_t* h;
const char* str;
{
	uint32_t maj;
	char*    p;

	maj = TC_H_UNSPEC;
	if(strcmp(str, "none") == 0)
		goto ok;
	maj = strtoul(str, &p, 16);
	if(p == str)
		return -1;
	maj <<= 16;
	if(*p != ':' && *p!=0)
		return -1;
ok:
	*h = maj;
	return 0;
}

int
get_tc_classid(h, str)
uint32_t* h;
const char* str;
{
	uint32_t maj;
	uint32_t min;
	char*    p;

	dprintf(("Parent ID = %s\n", str));
	maj = TC_H_ROOT;
	if(strcmp(str, "root") == 0)
		goto ok;
	maj = TC_H_UNSPEC;
	if(strcmp(str, "none") == 0)
		goto ok;
	maj = strtoul(str, &p, 16);
	if(p == str) {
		maj = 0;
		if(*p != ':')
			return -1;
	}
	if(*p == ':') {
		dprintf(("Leaf ID = %s\n", p));
		if(maj >= (1<<16))
			return -1;
        maj <<= 16;
        str = p + 1;
        min = strtoul(str, &p, 16);
		if(*p != 0)
			return -1;
		if(min >= (1 << 16))
			return -1;
		maj |= min;
	} else if(*p != 0)
		return -1;

ok:
	*h = maj;
	return 0;
}

static const struct rate_suffix {
	const char *name;
	double scale;
} suffixes[] = {
	{ "bit",	1. },
	{ "Kibit",	1024. },
	{ "kbit",	1000. },
	{ "mibit",	1024.*1024. },
	{ "mbit",	1000000. },
	{ "gibit",	1024.*1024.*1024. },
	{ "gbit",	1000000000. },
	{ "tibit",	1024.*1024.*1024.*1024. },
	{ "tbit",	1000000000000. },
	{ "Bps",	8. },
	{ "KiBps",	8.*1024. },
	{ "KBps",	8000. },
	{ "MiBps",	8.*1024*1024. },
	{ "MBps",	8000000. },
	{ "GiBps",	8.*1024.*1024.*1024. },
	{ "GBps",	8000000000. },
	{ "TiBps",	8.*1024.*1024.*1024.*1024. },
	{ "TBps",	8000000000000. },
	{ NULL }
};


int
get_rate(rate, str)
uint32_t* rate;
const char* str;
{
	char* p;
	double bps = strtod(str, &p);
	const struct rate_suffix* s;

	if(p == str)
		return -1;

	if(*p == '\0') {
		*rate = bps / 8.;	/* assume bytes/sec */
		return 0;
	}

	for (s = suffixes; s->name; ++s) {
		if(strcasecmp(s->name, p) == 0) {
			*rate = (bps * s->scale) / 8.;
			return 0;
		}
	}

	return -1;
}

void
print_rate(buf, len, rate)
char* buf;
int len;
uint32_t rate;
{
	double tmp = (double)rate * 8;
	extern int use_iec;

	if(use_iec) {
		if(tmp >= 1000.0 * 1024.0 * 1024.0)
			snprintf(buf, len, "%.0fMibit", tmp / 1024.0 * 1024.0);
		else if(tmp >= 1000.0 * 1024)
			snprintf(buf, len, "%.0fKibit", tmp / 1024);
		else
			snprintf(buf, len, "%.0fbit", tmp);
	} else {
		if(tmp >= 1000.0 * 1000000.0)
			snprintf(buf, len, "%.0fMbit", tmp / 1000000.0);
		else if(tmp >= 1000.0 * 1000.0)
			snprintf(buf, len, "%.0fKbit", tmp / 1000.0);
		else
			snprintf(buf, len, "%.0fbit",  tmp);
	}
}

char*
sprint_rate(rate, buf)
uint32_t rate;
char* buf;
{
	print_rate(buf, SPRINT_BSIZE - 1, rate);
	return buf;
}

int
get_usecs(usecs, str)
unsigned* usecs;
const char* str;
{
	double t;
	char* p;

	t = strtod(str, &p);
	if(p == str)
		return -1;

	if(*p) {
		if(strcasecmp(p, "s") == 0 || strcasecmp(p, "sec")==0 || strcasecmp(p, "secs") == 0)
			t *= 1000000;
		else if(strcasecmp(p, "ms") == 0 || strcasecmp(p, "msec")==0 || strcasecmp(p, "msecs") == 0)
			t *= 1000;
		else if(strcasecmp(p, "us") == 0 || strcasecmp(p, "usec")==0 || strcasecmp(p, "usecs") == 0)
			t *= 1;
		else
			return -1;
	}

	*usecs = t;
	return 0;
}

int 
get_size(size, str)
unsigned* size;
char* str;
{
	double sz;
	char *p;

	sz = strtod(str, &p);
	if(p == str)
		return -1;

	if(*p) {
		if(strcasecmp(p, "kb") == 0 || strcasecmp(p, "k")==0)
			sz *= 1024;
		else if(strcasecmp(p, "gb") == 0 || strcasecmp(p, "g")==0)
			sz *= 1024 * 1024 * 1024;
		else if(strcasecmp(p, "gbit") == 0)
			sz *= 1024 * 1024 * 1024 / 8;
		else if(strcasecmp(p, "mb") == 0 || strcasecmp(p, "m")==0)
			sz *= 1024 * 1024;
		else if(strcasecmp(p, "mbit") == 0)
			sz *= 1024 * 1024 / 8;
		else if(strcasecmp(p, "kbit") == 0)
			sz *= 1024 / 8;
		else if(strcasecmp(p, "b") != 0)
			return -1;
	}

	*size = sz;
	return 0;
}

int 
get_size_and_cell(size, cell_log, str)
unsigned* size;
int* cell_log;
char* str;
{
	char* slash = strchr(str, '/');

	if(slash)
		*slash = 0;

	if(get_size(size, str))
		return -1;

	if(slash) {
		int cell;
		int i;

		if(get_integer(&cell, slash + 1, 0))
			return -1;
		*slash = '/';

		for (i = 0; i < 32; i++) {
			if((1 << i) == cell) {
				*cell_log = i;
				return 0;
			}
		}
		return -1;
	}
	return 0;
}

static const double max_percent_value = 0xffffffff;

int get_percent(__u32 *percent, const char *str)
{
	char *p;
	double per = strtod(str, &p) / 100.;

	if(per > 1. || per < 0)
		return -1;
	if(*p && strcmp(p, "%"))
		return -1;

	*percent = (unsigned) rint(per * max_percent_value);
	return 0;
}

char*
action_n2a(action, buf, len)
int action;
char* buf;
int len;
{
	switch (action) {
		case -1:
			return "continue";
			break;
		case TC_ACT_OK:
			return "pass";
			break;
		case TC_ACT_SHOT:
			return "drop";
			break;
		case TC_ACT_RECLASSIFY:
			return "reclassify";
		case TC_ACT_PIPE:
			return "pipe";
		case TC_ACT_STOLEN:
			return "stolen";
		default:
			snprintf(buf, len, "%d", action);
			return buf;
	}
}

int
action_a2n(arg, result)
char* arg;
int* result;
{
	int res;

	if(matches(arg, "continue") == 0)
		res = -1;
	else if(matches(arg, "drop") == 0)
		res = TC_ACT_SHOT;
	else if(matches(arg, "shot") == 0)
		res = TC_ACT_SHOT;
	else if(matches(arg, "pass") == 0)
		res = TC_ACT_OK;
	else if(strcmp(arg, "ok") == 0)
		res = TC_ACT_OK;
	else if(matches(arg, "reclassify") == 0)
		res = TC_ACT_RECLASSIFY;
	else {
		char dummy;
		if(sscanf(arg, "%d%c", &res, &dummy) != 1)
			return -1;
	}
	*result = res;

	return 0;
}

