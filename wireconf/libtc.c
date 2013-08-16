#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>

#include "utils.h"
#include "ip_common.h"
#include "iproute.h"
#include "tc_common.h"
#include "tc_util.h"

#define TC_H_UNSPEC	(0U)
#define TC_H_ROOT   (0xFFFFFFFFU)

int preferred_family = AF_UNSPEC;
int oneline = 0;
char * _SL_ = NULL;
int open_flag = 1;

char* 
get_route_info(info, addr)
char *info;
char *addr;
{
    REQ req;
    struct rtattr* tb[RTA_MAX+1];
    inet_prefix dst;
    char abuf[256];
    _SL_ = oneline ? "\\" : "\n" ;

    if(open_flag) {
        if(rtnl_open(&rth, 0) < 0)
            exit(1);
        open_flag = 0;
    }

    req = iproute_get(1, addr);
    struct rtmsg *r = NLMSG_DATA(&req.n);
    int len = req.n.nlmsg_len;

    len -= NLMSG_LENGTH(sizeof(*r));
    parse_rtattr(tb, RTA_MAX, RTM_RTA(r), len);
    memset(&dst, 0, sizeof(dst));
    dst.family = r->rtm_family;
    if(filter.oifmask) {
        int oif = 0;
        if(tb[RTA_OIF])
            oif = *(int*)RTA_DATA(tb[RTA_OIF]);
        if((oif^filter.oif)&filter.oifmask) {
            return 0;
        }
    }

    if(strcmp(info, "dev") == 0) {
        return (char* )ll_index_to_name(*(int* )RTA_DATA(tb[RTA_OIF]));
    } else if(strcmp(info, "next") == 0){
        if((char* )inet_ntop(r->rtm_family,
                    RTA_DATA(tb[RTA_GATEWAY]),
                    abuf, sizeof(abuf)) == NULL) {
            return addr;
        }
        else {
            dprintf(("[get_route_info] %s\n",  (char* )inet_ntop(r->rtm_family,
                            RTA_DATA(tb[RTA_GATEWAY]),
                            abuf, sizeof(abuf))));
            return (char* )inet_ntop(r->rtm_family,
                    RTA_DATA(tb[RTA_GATEWAY]),
                    abuf, sizeof(abuf));
        }
    }
    else {
        return "null";
    }

    return 0;
}

int
tc_cmd(cmd, flags, dev, handleid, root, qp, type)
int cmd;
int flags;
char *dev;
char *handleid;
char *root;
struct qdisc_parameter qp;
char *type;
{
    struct qdisc_util *q = NULL;
    char d[16];
    char k[16];
    __u32 handle;
    struct {
        struct nlmsghdr n;
        struct tcmsg t;
        char buf[TCA_BUF_MAX];
    } req;

    tc_core_init();
    dprintf(("[tc_cmd] Start tc_cmd function type = %s\n", type));

    memset(&req, 0, sizeof(req));
    memset(&d, 0, sizeof(d));
    memset(&k, 0, sizeof(k));

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct tcmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST|flags;
    req.n.nlmsg_type = cmd;
    req.t.tcm_family = AF_UNSPEC;

    strncpy(d, dev, sizeof(d) - 1);
    if(cmd != RTM_DELQDISC) {
        get_qdisc_handle(&handle, handleid);
        req.t.tcm_handle = handle;
        dprintf(("[tc_cmd] req.t.tcm.handle = %d\n", req.t.tcm_handle));
    }
    if(strcmp(root, "root") == 0)
        req.t.tcm_parent = TC_H_ROOT;
    else if(strcmp(root, "ingress") == 0) {
        req.t.tcm_parent = TC_H_INGRESS;
        memset(&qp, 0, sizeof(struct qdisc_parameter));
        // lookup device table and set device 
        //if(set_ifb(ifb_device_name, "up") < 0) {
        //	printf("cannot create ifb device\n");
        //	return 1;
        //}
        if(cmd == RTM_DELQDISC) {
            strncpy(k, type, sizeof(k) - 1);
            q = get_qdisc_kind(k);
        }
        req.t.tcm_handle = 0xffff0000;
    }
    else {
        if(get_tc_classid(&handle, root)) {
            invarg(handleid, "invalid parent ID");
        }
        req.t.tcm_parent = handle;
        dprintf(("[tc_cmd] req.t.tcm.parent = %d\n", req.t.tcm_parent));
    }

    /*
       int idx;

       ll_init_map(&rth);

       if((idx = ll_name_to_index(d)) == 0) {
       fprintf(stderr, "Cannot find device \"%s\"\n", d);
       return 1;
       }
       req.t.tcm_ifindex = idx;
    */

    if(cmd != RTM_DELQDISC) {
        strncpy(k, type, sizeof(k) - 1);
        q = get_qdisc_kind(k);
    }

    if(k[0]) {
        addattr_l(&req.n, sizeof(req), TCA_KIND, k, strlen(k) + 1);
    }

    if(q) {
        if(q->parse_qopt(q, &qp, &req.n)) {
            return 1;
        }
    }
    if(d[0]) {
        int idx;

        ll_init_map(&rth);

        if((idx = ll_name_to_index(d)) == 0) {
            fprintf(stderr, "Cannot find device \"%s\"\n", d);
            return 1;
        }
        req.t.tcm_ifindex = idx;
        dprintf(("[tc_cmd] tc ifindex = %d\n", idx));
    }

    if(rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0) {
        return -1;
    }

    return 0;
}

/* test code
int
main(argc, argv)
int argc;
char **argv;
{
    char* ret;
    struct qdisc_parameter qp =  {"1000", "10ms", "10", "0", "0.01", "0", "0", "0"};
    
    ret = (char*)get_route_info(argv[1], argv[2]);
    printf("result = %s\n", ret);
    
    if(strcmp("add", argv[3]) == 0) {
        if(tc_cmd(RTM_NEWQDISC, NLM_F_EXCL|NLM_F_CREATE, (char* )get_route_info("dev", argv[2]), "1", "1", qp, "netem") < 0)
        printf("missing tc\n");
    }
    else if(strcmp("change", argv[3]) == 0) {
        if(tc_cmd(RTM_NEWQDISC, 0, (char* )get_route_info("dev", argv[2]), "1", "1", qp, "netem") < 0)
        printf("missing tc\n");
    }
    else if(strcmp("del", argv[3]) == 0) {
        if(tc_cmd(RTM_DELQDISC, 0, (char* )get_route_info("dev", argv[2]), "1", "1", qp, "netem") < 0)
        printf("missing tc\n");
    }
    else {
        printf("implememtation only add, del\n");
    }
    
    return 0;
}
*/
