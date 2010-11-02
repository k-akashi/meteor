#include <sys/socket.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int 
get_ctl_fd(void)
{
    int s_errno;
    int fd;

	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (fd >= 0)
		return fd;

	s_errno = errno;
	fd = socket(PF_PACKET, SOCK_DGRAM, 0);
	if (fd >= 0)
		return fd;

	fd = socket(PF_INET6, SOCK_DGRAM, 0);
	if (fd >= 0)
		return fd;

	errno = s_errno;
	perror("Cannot create control socket");

	return -1;
}

int
set_ifb(device, cmd)
char* device;
char* cmd;
{
	int fd;
	int err;
	char* dev;
	__u32 mask;
	__u32 flags;
	struct ifreq ifr;
	
	dev = NULL;
	mask = 0;
	flags = 0;

	dev = device;
	if( strcmp(cmd, "up") == 0) {
		mask |= IFF_UP;
		flags |= IFF_UP;
	}
	else if(strcmp(cmd, "down") == 0) {
		mask |= IFF_UP;
		flags &= ~IFF_UP;
	}

	if(!dev){
		fprintf(stderr, "devname is required.\n");
		exit(-1);
	}

	strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	fd = get_ctl_fd();
	if(fd < 0)
		return -1;

	err = ioctl(fd, SIOCGIFFLAGS, &ifr);

	if(err) {
		perror("SIOCGIFFLAGS");
		close(fd);
		return -1;
	}

	if((ifr.ifr_flags^flags)&mask) {
		ifr.ifr_flags &= ~mask;
		ifr.ifr_flags |= mask&flags;
		err = ioctl(fd, SIOCSIFFLAGS, &ifr);
		if(err)
			perror("SIOCSIFFLAGS");
	}
	close(fd);

	return 0;
}

/*
// test code
int
main(argc, argv)
int argc;
char** argv;
{
	char* devname;

	devname = "ifb1";

	if(set_ifb(devname, "down") < 0 ) {
		return 1;
	}

	return 0;
}
*/
