/*
 * tc_core.c		TC core library.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <math.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "tc_core.h"

/*
static __u32 t2us = 1;
static __u32 us2t = 1;
*/
static double tick_in_usec = 1;
// for 2.6.34
static double clock_factor = 1;
//

unsigned
tc_core_time2tick(time)
unsigned time;
{   
    return time * tick_in_usec;
}

long
tc_core_usec2tick(usec)
long usec;
{
    return usec * tick_in_usec;
}

int
tc_core_time2big(time)
unsigned time;
{   
    __u64 t = time;

    t *= tick_in_usec;
    return (t >> 32) != 0;
}

long
tc_core_tick2usec(tick)
long tick;
{
    return tick / tick_in_usec;
}

unsigned
tc_calc_xmittime(rate, size)
unsigned rate;
unsigned size;
{
    return tc_core_usec2tick(1000000 * ((double)size / rate));
}

/*
rtab[pkt_len>>cell_log] = pkt_xmit_time
*/

unsigned 
tc_adjust_size(sz, mpu, linklayer)
unsigned sz;
unsigned mpu;
enum link_layer linklayer;
{
    if (sz < mpu)
        sz = mpu;

	return sz;
} 

int
tc_calc_rtable(r, rtab, cell_log, mtu, linklayer)
struct tc_ratespec* r;
uint32_t* rtab;
int cell_log;
unsigned mtu;
enum link_layer linklayer;
{
    int i;
    unsigned sz;
    unsigned bps = r->rate; 
    unsigned mpu = r->mpu;

    if (mtu == 0)
        mtu = 2047;

    if (cell_log < 0) {
        cell_log = 0;
        while ((mtu >> cell_log) > 255)
            cell_log++; 
    } 

    for (i = 0; i < 256; i++) {
        sz = tc_adjust_size((i + 1) << cell_log, mpu, linklayer);
        rtab[i] = tc_calc_xmittime(bps, sz);
    }

    r->cell_align = -1;
    r->cell_log=cell_log;

    return cell_log;
}

int
tc_core_init()
{
    FILE *fp = fopen("/proc/net/psched", "r");
    uint32_t clock_res;
    uint32_t t2us;
    uint32_t us2t;

    if (fp == NULL)
        return -1;

    if (fscanf(fp, "%08x%08x%08x", &t2us, &us2t, &clock_res) != 3) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

	// for 2.6.18
    //tick_in_usec = (double)t2us / us2t;
    // for 2.6.34
    tick_in_usec = (double)t2us / us2t * clock_factor;
    return 0;
}
