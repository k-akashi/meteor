#ifndef _TC_UTIL_H_
#define _TC_UTIL_H_ 1

#define MAX_MSG 16384
#include <linux/pkt_sched.h>
#include <linux/pkt_cls.h>
#include <linux/tc_act/tc_mirred.h>
#include <linux/gen_stats.h>
#include "tc_core.h"
#include "libnetlink.h"

#ifdef TCDEBUG
#define dprintf(x) printf x
#else
#define dprintf(x)
#endif

#define TC_HANDLE(maj, min) (maj << 16 | min)

struct rtnl_handle rth;

struct qdisc_parameter
{
	char* limit;
	char* delay;
	char* jitter;
	char* delay_corr;
	char* loss;
	char* loss_corr;
	char* reorder_prob;
	char* reorder_corr;
	char* rate;
	char* buffer;
};

struct filter_match {
	char* type;
	char* protocol;
	char* filter;
	char* arg;
};


struct u32_parameter
{
	struct filter_match match;
	char* offset;
	char* hashkey;
	uint32_t classid[2];
	char* divisor;
	char* order;
	char* link;
	char* ht;
	char* indev;
	char* action;
	char* police;
	char* rdev; //redirect device
};

struct qdisc_util
{
	struct  qdisc_util* next;
	const char* id;
	int	(*parse_qopt)(struct qdisc_util* qu, struct qdisc_parameter* qp, struct nlmsghdr* n);
	int	(*print_qopt)(struct qdisc_util* qu, FILE* f, struct rtattr* opt);
	int (*print_xstats)(struct qdisc_util* qu, FILE* f, struct rtattr* xstats);

	int	(*parse_copt)(struct qdisc_util* qu, int argc, char** argv, struct nlmsghdr* n);
	int	(*print_copt)(struct qdisc_util* qu, FILE* f, struct rtattr* opt);
};

struct filter_util
{
	struct filter_util *next;
	char	id[16];
	int	(*parse_fopt)(struct filter_util* qu, char* fhandle, struct u32_parameter, struct nlmsghdr* n, char* dev);
	int	(*print_fopt)(struct filter_util* qu, FILE* f, struct rtattr* opt, __u32 fhandle);
};

struct action_util
{
	struct  action_util* next;
	char    id[16];
	int     (*parse_aopt)(struct action_util *a, char* action, 
			      int code, struct nlmsghdr *n, char* dev);
	int     (*print_aopt)(struct action_util *au, FILE *f, struct rtattr *opt);
	int     (*print_xstats)(struct action_util *au, FILE *f, struct rtattr *xstats);
};

extern struct qdisc_util* get_qdisc_kind(const char *str);
extern struct filter_util* get_filter_kind(const char *str);

extern int get_qdisc_handle(__u32 *h, const char *str);
extern int get_rate(unsigned *rate, const char *str);
extern int get_percent(unsigned *percent, const char *str);
extern int get_size(unsigned *size, char *str);
extern int get_size_and_cell(unsigned* size, int* cell_log, char* str);
extern int get_usecs(unsigned *usecs, const char *str);

extern int get_tc_classid(__u32 *h, const char *str);

extern int parse_action(char *, int, struct nlmsghdr *, char* dev);

extern int tc_cmd(int cmd, int flags, char* dev, char* handleid, char* root, struct qdisc_parameter qp, char* type);

extern int add_netem_qdisc(char* device, uint32_t id[4], struct qdisc_parameter qp);
extern int change_netem_qdisc(char* device, uint32_t id[4], struct qdisc_parameter qp);
extern int delete_netem_qdisc(char* device, int ingress);
extern int add_htb_qdisc(char* device, uint32_t id[4]);
extern int add_htb_class(char* device, uint32_t id[4], char* bnadwidth);
extern int change_htb_class(char* device, uint32_t id[4], char* bnadwidth);
extern int add_tbf_qdisc(char* device, uint32_t id[4], struct qdisc_parameter qp);
extern int change_tbf_qdisc(char* device, uint32_t id[4], struct qdisc_parameter qp);

extern char* get_route_info(char *info, char *addr);

extern int tc_filter_modify(int cmd, unsigned int flags, char* dev, uint32_t id[4], char* protocolid, char* type, struct u32_parameter* up);
extern int u32_filter_parse(struct filter_util *qu, uint32_t handle, struct u32_parameter up, struct nlmsghdr *n, char* dev);
extern int add_ingress_qdisc(char *dev);
extern int add_ingress_filter(char *dev, char *ifb_dev);

#endif
