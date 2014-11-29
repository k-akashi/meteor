
/*
 * Copyright (c) 2006-2013 The StarBED Project  All rights reserved.
 *
 * See the file 'LICENSE' for licensing information.
 *
 */

/************************************************************************
 *
 * QOMET Emulator Implementation
 *
 * File name: global.h
 * Function:  Header file with global defines
 *
 * Author: Razvan Beuran
 *
 * $Id: global.h 145 2013-06-07 01:11:09Z razvan $
 *
 ***********************************************************************/


#ifndef __GLOBAL_H
#define __GLOBAL_H


/////////////////////////////////////////////
// Basic constants
/////////////////////////////////////////////

// BOOLEAN
#define TRUE                            1
#define FALSE                           0

#define SUCCESS                         0
#define ERROR                           -1

//#ifndef MAX_STRING
#define MAX_STRING                      256
//#endif
#define MAX_STRING_STR                  "255"

// this is long enough to hold IPv4 addresses
// (4x3 digits + 3x1 dots + '\0')?
// it should be increased to 40(?!) if IPv6 addresses are used
#define IP_ADDR_SIZE                    16

// this is long enough to hold Ethernet MAC addresses
// (2x6 digits + 5x1 colon + '\0')?
#define MAC_ADDR_SIZE                   18

#ifndef ETH_SIZE
#define ETH_SIZE                        6
#endif

#define FIRST_NODE_ID                   0

// maximum data sizes constants
#define MAX_NODES                       30000
#define MAX_OBJECTS                     10000   // 500
#define MAX_ENVIRONMENTS                10000   // MAX_NODES*MAX_NODES
#define MAX_MOTIONS                     500
#define MAX_CONNECTIONS                 10000   // MAX_NODES*MAX_NODES

#define TCHK_START(name)           \
struct timeval name##_prev;        \
struct timeval name##_current;     \
gettimeofday(&name##_prev, NULL)

#define TCHK_END(name)                                                             \
    gettimeofday(&name##_current, NULL);                                           \
time_t name##_sec;                                                                 \
suseconds_t name##_usec;                                                           \
if(name##_current.tv_sec == name##_prev.tv_sec) {                                  \
    name##_sec = name##_current.tv_sec - name##_prev.tv_sec;                       \
    name##_usec = name##_current.tv_usec - name##_prev.tv_usec;                    \
}                                                                                  \
else if(name ##_current.tv_sec != name##_prev.tv_sec) {                            \
    int name##_carry = 1000000;                                                    \
    name##_sec = name##_current.tv_sec - name##_prev.tv_sec;                       \
    if(name##_prev.tv_usec > name##_current.tv_usec) {                             \
        name##_usec = name##_carry - name##_prev.tv_usec + name##_current.tv_usec; \
        name##_sec--;                                                              \
        if(name##_usec > name##_carry) {                                           \
            name##_usec = name##_usec - name##_carry;                              \
            name##_sec++;                                                          \
        }                                                                          \
    }                                                                              \
    else {                                                                         \
        name##_usec = name##_current.tv_usec - name##_prev.tv_usec;                \
    }                                                                              \
}                                                                                  \
printf("%s: sec:%lu usec:%06ld\n", #name, name##_sec, name##_usec);


/////////////////////////////////////////////
// Definitions of the main structures
// (used to avoid circular references)
/////////////////////////////////////////////

struct node_class;
struct interface_class;
struct object_class;
struct environment_class;
struct motion_class;
struct connection_class;
struct scenario_class;
struct xml_scenario_class;
struct xml_jpgis_class;

#endif
