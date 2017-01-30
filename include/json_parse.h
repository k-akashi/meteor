struct link_params {
    struct in_addr addr;
    uint16_t id;
    double delay;
    double rate;
    double loss;
    double fer;
    struct link_params *next;
};

struct node_array {
    uint16_t size;
    uint16_t *id;
};

struct meteor_params {
    struct link_params *add_params;
    struct link_params *update_params;
    struct node_array *delete_params;
};

struct meteor_params * parse_json(char *req);
void dump_meteor_params(struct meteor_params *mp);
void dump_add_params(struct link_params *link_head);
void dump_update_params(struct link_params *link_head);
void clear_meteor_params(struct meteor_params *mp);
