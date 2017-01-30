#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>
#include <jansson.h>
#include <stdlib.h>
#include <stdio.h>

#include "json_parse.h"

#define HEREDOC(...) #__VA_ARGS__ "\n";

struct link_params *
parse_params(json_t *json_data)
{
    char *ptr;
    const char *node, *key;
    struct json_t *nodeval, *value;
    struct link_params *link_ptr;
    struct link_params *link_head;

    link_head = link_ptr = NULL;

    if (json_is_object(json_data) != 1) {
        return link_head;
    }

    json_object_foreach(json_data, node, nodeval) {
        if (json_is_object(nodeval) != 1) {
            continue;
        }

        if (link_ptr) {
            link_ptr->next = (struct link_params *)malloc(sizeof (struct link_params));
            link_ptr = link_ptr->next;
        }
        else {
            link_ptr = (struct link_params *)malloc(sizeof (struct link_params));
        }
        memset(link_ptr, 0, sizeof (struct link_params));
        link_ptr->id = -1;
        link_ptr->next = NULL;
        if (link_head == NULL) {
            link_head = link_ptr;
        }
        json_object_foreach(nodeval, key, value) {
            if (strncmp(key, "delay", sizeof ("delay")) == 0) {
                link_ptr->delay = strtod(json_string_value(value), &ptr);
            }
            else if (strncmp(key, "bandwidth", sizeof ("bandwidth")) == 0) {
                link_ptr->rate = strtod(json_string_value(value), &ptr);
            }
            else if (strncmp(key, "lossrate", sizeof ("lossrate")) == 0) {
                link_ptr->loss = strtod(json_string_value(value), &ptr);
            }
            else if (strncmp(key, "fer", sizeof ("fer")) == 0) {
                link_ptr->fer = strtod(json_string_value(value), &ptr);
            }
            else if (strncmp(key, "address", sizeof ("address")) == 0) {
                //strncpy(link_ptr->addr, json_string_value(value), 16);
                inet_aton(json_string_value(value), &link_ptr->addr);
            }
        }

        if (link_ptr->delay != 0.0 || link_ptr->rate != 0.0) {
            link_ptr->id = (uint16_t)strtol(node, &ptr, 10);
        }
    }

    return link_head;
}

struct node_array *
parse_array(json_t *json_data)
{
    size_t index, size;
    json_t *value;
    struct node_array *node_ary = NULL;

    size = json_array_size(json_data);
    if (size != 0) {
        node_ary = (struct node_array *)malloc(sizeof (struct node_array));
        memset(node_ary, 0, sizeof (struct node_array));
        node_ary->id = (uint16_t *)malloc(sizeof (uint16_t) * size);
        node_ary->size = size;
    }
    else {
        return node_ary;
    }
    json_array_foreach(json_data, index, value) {
        node_ary->id[index] = (uint16_t)json_integer_value(value);
    }

    return node_ary;
}

struct meteor_params *
parse_json(char *req)
{
    void *nodes;
    const char *opcode, *node, *key;

    struct meteor_params *mp = NULL;
    json_t *opval, *nodeval;
    json_t *retjson;
    json_error_t err;
    
    retjson = json_loads(req, 0, &err);
    if (retjson == NULL) {
        fprintf(stderr, "json_loads: %s\n", err.text);
        return mp;
    }

    mp = (struct meteor_params *)malloc(sizeof (struct meteor_params));
    mp->update_params = NULL;
    mp->add_params = NULL;
    mp->delete_params = NULL;
    json_object_foreach(retjson, opcode, opval) {
        if (strncmp(opcode, "opts", sizeof ("opts")) == 0) {
            if (json_is_object(opval) != 1) {
                continue;
            }
            nodes = json_object_iter(opval);
            while (nodes) {
                node    = (char *)json_object_iter_key(nodes);
                nodeval = json_object_iter_value(nodes);
                printf("--> key: %s, value: %s\n", node, json_string_value(nodeval));

                nodes = json_object_iter_next(opval, nodes);
            }
        }
        else if (strncmp(opcode, "add", sizeof ("add")) == 0) {
            mp->add_params = parse_params(opval);
        }
        else if (strncmp(opcode, "update", sizeof ("update")) == 0) {
            mp->update_params = parse_params(opval);
        }
        else if (strncmp(key, "delete", sizeof ("delete")) == 0) {
            mp->delete_params = parse_array(opval);
        }
    }
    //printf("%s\n", json_dumps(retjson, 0));

    json_decref(retjson);

    return mp;
}

void
dump_add_params(struct link_params *link_head)
{
    struct link_params *link;

    link = link_head;
    printf("add parameter: \n");
    while (link != NULL) {
        printf("  -> id: %hu\n", link->id);
        printf("  --> address: %s\n", inet_ntoa(link->addr));
        printf("  --> delay:   %f\n", link->delay);
        printf("  --> rate:    %f\n", link->rate);
        printf("  --> loss:    %f\n", link->loss);
        link = link->next;
    }
}

void
dump_update_params(struct link_params *link_head)
{
    struct link_params *link;

    link = link_head;
    printf("update parameter: \n");
    while (link != NULL) {
        printf("  -> id: %hu\n", link->id);
        printf("  --> delay:   %f\n", link->delay);
        printf("  --> rate:    %f\n", link->rate);
        printf("  --> loss:    %f\n", link->loss);
        link = link->next;
    }
}

void
dump_meteor_params(struct meteor_params *mp)
{
    dump_add_params(mp->add_params);
    dump_update_params(mp->update_params);
}

void
clear_meteor_params(struct meteor_params *mp)
{
    struct link_params *link, *next;
    struct node_array *ary;
    
    link = mp->add_params;
    if (link) {
        while (link->next) {
            next = link->next;
            free(link);
            link = next;
        }
        free(link);
    }

    link = mp->update_params;
    if (link) {
        while (link->next) {
            next = link->next;
            free(link);
            link = next;
        }
        free(link);
    }
    ary = mp->delete_params;
    if (ary) {
        if (ary->id) {
            free(ary->id);
        }
        free(ary);
    }

    free(mp);
}

#ifdef DEBUG
int
main(int argc, char **argv)
{
    char *json = HEREDOC(
        {
            "opts": {
            },
            "add": {
                "1": { "address": "192.168.0.1", "delay": "101.0", "bandwidth": "8000.0", "lossrate": "0.0", "fer": "1.0" },
                "2": { "address": "192.168.0.2", "delay": "101.0", "bandwidth": "8000.0", "lossrate": "0.0", "fer": "1.0" }
            },
            "update": {
                "1": { "delay": "12", "bandwidth": "8001", "lossrate": "2", "fer": "3" }
            },
            "delete": [ "1" ]
        });
    printf("Data: %s\n", json);

    struct meteor_params *mp;
    struct link_params *link;

    for (int i = 0; i < 100; i++) {
        mp = parse_json(json);
        dump_meteor_params(mp);
        clear_meteor_params(mp);
    }

    return 0;
}
#endif
