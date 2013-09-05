#ifndef _TC_CORE_H_
#define _TC_CORE_H_ 1

#include <asm/types.h>
#include <linux/pkt_sched.h>

enum link_layer {
    LINKLAYER_UNSPEC,
    LINKLAYER_ETHERNET,
    LINKLAYER_ATM,
}; 

unsigned tc_core_time2tick(unsigned time);
long tc_core_usec2tick(long usec);
long tc_core_tick2usec(long tick);
int tc_core_time2big(unsigned time);
unsigned tc_calc_xmittime(unsigned rate, unsigned size);
int tc_calc_rtable(struct tc_ratespec* r, uint32_t *rtab, int cell_log, unsigned mtu, enum link_layer linklayer);

int tc_setup_estimator(unsigned A, unsigned time_const, struct tc_estimator *est);

int tc_core_init(void);

extern struct rtnl_handle g_rth;
extern int is_batch_mode;

#endif
