/*
 * m_action.c		Action Management
 *
 *		This program is free software; you can distribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:  J Hadi Salim (hadi@cyberus.ca) 
 * 
 * TODO:
 * - parse to be passed a filedescriptor for logging purposes
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
#include <dlfcn.h>

#include "utils.h"
#include "tc_common.h"
#include "tc_util.h"

static struct action_util * action_list;
#ifdef CONFIG_GACT
int gact_ld = 0 ; //fuckin backward compatibility
#endif
int batch_c = 0;
int tab_flush = 0;

/*
static int
print_noaopt(au, f, opt)
struct action_util *au;
FILE *f;
struct rtattr *opt;
{
	if (opt && RTA_PAYLOAD(opt)) {
		fprintf(f, "[Unknown action, optlen=%u] ", (unsigned) RTA_PAYLOAD(opt));
    }
	return 0;
}

static int
parse_noaopt(au, argc_p, argv_p, code, n)
struct action_util *au;
int *argc_p;
char ***argv_p;
int code;
struct nlmsghdr *n;
{
	int argc = *argc_p;
	char **argv = *argv_p;

	if (argc) {
		fprintf(stderr, "Unknown action \"%s\", hence option \"%s\" is unparsable\n", au->id, *argv);
	} else {
		fprintf(stderr, "Unknown action \"%s\"\n", au->id);
	}
	return -1;
}
*/

struct action_util *get_action_kind(str)
char *str;
{
	static void *aBODY;
	void *dlh;
	char buf[256];
	struct action_util *a;
#ifdef CONFIG_GACT
	int looked4gact = 0;
restart_s:
#endif
	for(a = action_list; a; a = a->next) {
		if (strcmp(a->id, str) == 0)
			return a;
	}

	snprintf(buf, sizeof(buf), "m_%s.so", str);
	dlh = dlopen(buf, RTLD_LAZY);
	if(dlh == NULL) {
		dlh = aBODY;
		if(dlh == NULL) {
			dlh = aBODY = dlopen(NULL, RTLD_LAZY);
			if(dlh == NULL) {
				goto noexist;
            }
		}
	}

	snprintf(buf, sizeof(buf), "%s_action_util", str);
	a = dlsym(dlh, buf);
	if(a == NULL) {
		goto noexist;
    }

reg:
	a->next = action_list;
	action_list = a;
	return a;

noexist:
#ifdef CONFIG_GACT
	if(!looked4gact) {
		looked4gact = 1;
		strcpy(str,"gact");
		goto restart_s;
	}
#endif
	a = malloc(sizeof(*a));
	if(a) {
		memset(a, 0, sizeof(*a));
		strncpy(a->id, "noact", 15);
//		a->parse_aopt = parse_noaopt;
//		a->print_aopt = print_noaopt;
		goto reg;
	}
	return a;
}

/*
int
new_cmd(argv) 
char **argv
{
	if ((matches(*argv, "change") == 0) ||
		(matches(*argv, "replace") == 0)||
		(matches(*argv, "delete") == 0)||
		(matches(*argv, "add") == 0)) {
			return 1;
    }

	return 0;

}
*/

int
parse_action(action, tca_id, n, dev)
char* action;
int tca_id;
struct nlmsghdr *n;
char* dev;
{
	struct rtattr *tail, *tail2;
	char k[16];

	int ret = 0;
	int prio = 0;
	struct action_util *a = NULL;

	tail = tail2 = NLMSG_TAIL(n);

	addattr_l(n, MAX_MSG, tca_id, NULL, 0);

	memset(k, 0, sizeof (k));
	strncpy(k, action, sizeof (k) - 1);
	a = get_action_kind(k);

	tail = NLMSG_TAIL(n);
	addattr_l(n, MAX_MSG, ++prio, NULL, 0);
	addattr_l(n, MAX_MSG, TCA_ACT_KIND, k, strlen(k) + 1);

	ret = a->parse_aopt(a, "redirect", TCA_ACT_OPTIONS, n, dev);

	tail->rta_len = (void *) NLMSG_TAIL(n) - (void *)tail;

	tail2->rta_len = (void *) NLMSG_TAIL(n) - (void *)tail2;

	return 0;
}

