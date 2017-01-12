#ifndef METEOR_H
#define METEOR_H

#define PARAMETERS_TOTAL        19
#define PARAMETERS_UNUSED       13

#define MIN_PIPE_ID_HV          10
#define MIN_PIPE_ID_BR          10
#define MIN_PIPE_ID_IN          10000
#define MIN_PIPE_ID_IN_BCAST	20000
#define MIN_PIPE_ID_OUT         30000

#define MAX_RULE_NUM            100
#define MAX_RULE_COUNT          500

#define UNDEFINED_SIGNED        -1
#define UNDEFINED_UNSIGNED      65535
#define UNDEFINED_BANDWIDTH     -1.0
#define SCALING_FACTOR          1.0

#define DEF_DELAY               0
#define DEF_BW                  1000000000
#define DEF_LOSS                100

typedef struct {
    float time;
    int32_t next_hop_id;
    double bandwidth;
    double delay;
    double lossrate;
} qomet_param;

struct METEOR_CONF {
    int32_t pif_index;
    int32_t ifb_index;

    int32_t  node_cnt;
    uint32_t id;
    uint32_t loop;
    uint32_t direction;
    uint32_t filter_mode;
    int32_t  daemonize;
    int32_t  verbose;

    FILE *deltaq_fd;
    FILE *settings_fd;

    struct bin_hdr_cls *bin_hdr;
    struct node_data *node_list_head;
    struct connection_list *conn_list_head;
};

#endif
