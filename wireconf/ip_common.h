#define IF_UP 0
#define IF_DOWN 1

typedef struct if_list {
    struct nlmsghdr n;
    ssize_t len;
};

extern struct rtnl_handle rth;
extern int print_linkinfo(const struct sockaddr_nl *who, struct nlmsghdr *n, void *arg);
extern int print_addrinfo(const struct sockaddr_nl *who, struct nlmsghdr *n, void *arg);
extern int print_neigh(const struct sockaddr_nl *who, struct nlmsghdr *n, void *arg);
extern int print_ntable(const struct sockaddr_nl *who, struct nlmsghdr *n, void *arg);
extern int ipaddr_list(int argc, char **argv);
extern int ipaddr_list_link(int argc, char **argv);
extern int iproute_monitor(int argc, char **argv);
extern void iplink_usage(void) __attribute__((noreturn));
extern void iproute_reset_filter(void);
extern void ipaddr_reset_filter(int);
extern void ipneigh_reset_filter(void);
extern void ipntable_reset_filter(void);
extern int print_route(const struct sockaddr_nl *who, struct nlmsghdr *n, void *arg);
extern int print_prefix(const struct sockaddr_nl *who, struct nlmsghdr *n, void *arg);
extern int set_ifb(char* devname, int cmd); 
extern int get_if_list(struct if_list *iflist);
