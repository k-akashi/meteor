#include <sys/socket.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "tc_util.h"
#include "ip_common.h"

int
get_if_list(iflist)
struct if_list *iflist;
{ 
    ssize_t ret;
    struct {
        struct nlmsghdr n;
        struct ifinfomsg ifinfo;
    } req;
    struct {
        struct nlmsghdr n;
        uint8_t payload[1024];
    } res;

    memset(&req, 0, sizeof(req));

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.ifinfo));
    req.n.nlmsg_type = RTM_GETLINK;
    req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
    req.ifinfo.ifi_family = AF_UNSPEC;

    if(send(rth.fd, &req, sizeof(req), 0) == -1) {
        perror("send");
        return 1;
    }

    ret = recv(rth.fd, &res, sizeof(res), 0);
    if(ret == -1) {
        perror("recv");
        return 1;
    }

    iflist->n = res.n;
    iflist->len = ret;

    return 0;
}

static int 
get_ctl_fd(void)
{
    int s_errno;
    int fd;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd >= 0) {
		return fd;
    }

	s_errno = errno;
	fd = socket(AF_PACKET, SOCK_DGRAM, 0);
	if (fd >= 0) {
		return fd;
    }

	fd = socket(AF_INET6, SOCK_DGRAM, 0);
	if (fd >= 0) {
		return fd;
    }

	errno = s_errno;
	perror("Cannot create control socket");

	return -1;
}

int
set_ifb(dev, cmd)
char* dev;
int cmd;
{
	int fd;
	int err;
	uint32_t mask;
	uint32_t flags;
	struct ifreq ifr;
	
	mask = 0;
	flags = 0;
    memset(&ifr, 0, sizeof(struct ifreq));

	if(cmd == IF_UP) {
		mask |= IFF_UP;
		flags |= IFF_UP;
	}
	else if(cmd  == IF_DOWN) {
		mask |= IFF_UP;
		flags &= ~IFF_UP;
	}

	strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	fd = get_ctl_fd();
	if(fd < 0) {
		return -1;
    }

	err = ioctl(fd, SIOCGIFFLAGS, &ifr);
	if(err) {
		perror("SIOCGIFFLAGS");
		close(fd);
		return -1;
	}
	if((ifr.ifr_flags ^ flags) & mask) {
		ifr.ifr_flags &= ~mask;
		ifr.ifr_flags |= mask&flags;
		err = ioctl(fd, SIOCSIFFLAGS, &ifr);
		if(err) {
			perror("SIOCSIFFLAGS");
        }
	}
	close(fd);

	return 0;
}

/* test code
int
main(argc, argv)
int argc;
char** argv;
{
	char* devname;

	devname = "ifb1";
	if(set_ifb(devname, IF_DOWN) < 0 ) {
		return 1;
	}

	return 0;
}
*/
