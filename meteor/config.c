#include <jansson.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.hpp"
#include "global.h"

int
parse_ipaddr(const char *val, struct in_addr *addr, int *prefix)
{
    int ret;
    char *s, *p;
    const char *delm = "/";

    s = strtok_r((char *)val, delm, &p);
    if (!s) {
        return 1;
    }

    ret = inet_aton(s, addr);
    if (ret == 0) {
        return 1;
    }
    s = strtok_r(NULL, delm, &p);
    if (s) {
        *prefix = strtol(s, &p, 10);
    }
    else {
        *prefix= 32;
    }

    return 0;
}

int
parse_macaddr(const char *val, uint8_t *mac)
{
    int i = 0;
    char *s, *p;
    char *endptr;
    const char *delm = ":";

    s = strtok_r((char *)val, delm, &p);
    if (!s) {
        return 1;
    }
    mac[i] = strtol(s, &endptr, 16);
    for (i = 1; i < 6; i++) {
        s = strtok_r(NULL, delm, &p);
        if (!s) {
            return 1;
        }
        mac[i] = strtol(s, &endptr, 16);
    }

    return 0;
}

int
add_node_param(struct node_data *node, const char *key, const char *val)
{
    int ret;
    char *ptr;
    //"node0" : { "interface": "mesh0", "id": 0, "ipaddr": "172.16.0.1/32", "macaddr": "00:00:00:00:00:01" }

    if (strncmp(key, "node", sizeof ("nod")) == 0) {
        strncpy(node->name, key, MAX_NAME_LEN);
    }
    else if (strncmp(key, "interface", sizeof ("interface")) == 0) {
        strncpy(node->ifname, val, MAX_IFNAME_LEN);
    }
    else if (strncmp(key, "id", sizeof ("id")) == 0) {
        node->id = strtol(val, &ptr, 10);
    }
    else if (strncmp(key, "ipaddr", sizeof ("ipaddr")) == 0) {
        ret = parse_ipaddr(val, &(node->ipv4addr), &(node->ipv4prefix));
        if (ret) {
            fprintf(stderr, "Invalid IPv4 address.\n");
            exit(1);
        }
    }
    else if (strncmp(key, "macaddr", sizeof ("macaddr")) == 0) {
        ret = parse_macaddr(val, node->mac);
        if (ret) {
            fprintf(stderr, "Invalid MAC address.\n");
            exit(1);
        }
    }

    return 0;
}

int
get_node_cnt(char *conf_file)
{
    const char *key;
    int node_cnt = 0;
    json_t *fjson, *val;
    json_error_t err;

    fjson = json_load_file(conf_file, 0, &err);

    json_object_foreach(fjson, key, val) {
        if (strncmp(key, "node", 4) == 0) {
            node_cnt++;
        }
    }

    return node_cnt;
}

struct node_data *
create_node_list(char *conf_file, int node_cnt)
{
    const char *key, *node_key;
    json_t *fjson, *val, *node_val;
    json_error_t err;
    struct node_data *node_list, *node_ptr;
    struct node_data *node;

    fjson = json_load_file(conf_file, 0, &err);

    node_ptr = node_list = (struct node_data *)malloc(sizeof (struct node_data) * node_cnt);
    node = (struct node_data *)malloc(sizeof (struct node_data));

    json_object_foreach(fjson, key, val) {
        if (strncmp(key, "node", 4) == 0) {
            memset(node, 0, sizeof (struct node_data));
            add_node_param(node_ptr, key, json_string_value(val));
            json_object_foreach(val, node_key, node_val) {
                add_node_param(node_ptr, node_key, json_string_value(node_val));
            }
        }
        
        node_ptr++;
    }

    return node_list;
}

void
dump_node_list(struct node_data *node_list, int node_cnt)
{
    int i;
    struct node_data *node_ptr = node_list;

    //"node0" : { "interface": "mesh0", "id": 0, "ipaddr": "172.16.0.1/32", "macaddr": "00:00:00:00:00:01" }
    for (i = 0; i < node_cnt; i++) {
        printf("node info:\n");
        printf("\tnode name:           %s\n", node_ptr->name);
        printf("\tnode interface name: %s\n", node_ptr->ifname);
        printf("\tnode id:             %d\n", node_ptr->id);
        printf("\tnode IPv4 address:   %s\n", inet_ntoa((struct in_addr)(node_ptr->ipv4addr)));
        printf("\tnode IPv4 Prefix:    %d\n", node_ptr->ipv4prefix);
        printf("\tnode MAC address:    %02x:%02x:%02x:%02x:%02x:%02x\n",
                node_ptr->mac[0], node_ptr->mac[1], node_ptr->mac[2],
                node_ptr->mac[3], node_ptr->mac[4], node_ptr->mac[5]);

        node_ptr++;
    }
}

#ifdef DEBUG
int
main(int argc, char **argv)
{
    int cnt;
    struct node_data *nlist;

    cnt = get_node_cnt(argv[1]);
    printf("%d\n", cnt);
    nlist = create_node_list(argv[1], cnt);
    dump_node_list(nlist, cnt);

    return 0;
}
#endif
