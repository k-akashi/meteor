#define MAX_NAME_LEN 20
#define MAX_IFNAME_LEN 16
#define MACADDR_SIZE 6

struct node_data {
    char name[MAX_NAME_LEN];
    char ifname[MAX_IFNAME_LEN];
    int id;
    struct in_addr ipv4addr;
    int ipv4prefix;
    uint8_t mac[MACADDR_SIZE];
    uint8_t padding[2];
};

extern int add_node_param(struct node_data *node, const char *key, const char *val);
extern int get_node_cnt(char *conf_file);
extern struct node_data * create_node_list(char *conf_file, int node_cnt);
extern void dump_node_list(struct node_data *node_list, int node_cnt);
