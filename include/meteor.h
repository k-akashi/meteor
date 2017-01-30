#ifndef METEOR_H
#define METEOR_H

#define FRAME_LENGTH            1522
#define INGRESS                 1
#define DEV_NAME                256
#define OFFSET_RULE             32767

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

#define DIRECTION_BOTH          0
#define INGRESS                 1
#define BRIDGE                  4

#define QLEN                    100000

#ifdef TCDEBUG
#define debug(...) { \
    printf("%s in %s:%d. ", __func__, __FILE__, __LINE__); \
    printf(__VA_ARGS__); \
}
#else
#define debug(...) 
#endif

typedef struct {
    float time;
    int32_t next_hop_id;
    double bandwidth;
    double delay;
    double lossrate;
} qomet_param;

struct connection_list {
    struct connection_list *next_ptr;
    int32_t src_id;
    int32_t dst_id;
    uint16_t rec_i;
};

struct meteor_config {
    struct nl_sock *nlsock;
    struct nl_cache *cache;

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
    FILE *connection_fd;
    FILE *logfd;

    struct bin_hdr_cls *bin_hdr;
    struct node_data *node_list_head;
    struct connection_list *conn_list_head;
};

#endif
