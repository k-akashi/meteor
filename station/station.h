
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
 * File name: station.h
 * Function: Header file of the station library
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 143 $
 *   $LastChangedDate: 2009-03-31 11:30:13 +0900 (Tue, 31 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#ifndef	__STATION_H
#define	__STATION_H


#include <arpa/inet.h>
#include <pthread.h>


/////////////////////////////////////////////
// Basic constants
/////////////////////////////////////////////

#define MAX_DOUBLE             1e100

#define BEACONS_PORT           54322
#define BEACONS_END_EXEC       -1
#define BEACONS_ADDRESS        "192.168.4.255"

// according to a document from Cisco, their clients disassociate or
// roam to another AP when they miss eight consecutive beacons
// (https://supportforums.cisco.com/docs/DOC-3803); this is equivalent to 
// having less than 1 beacon in an interval equal to the duration
// of 8 beacons
#define REQUIRED_BEACON_COUNT      1
#define ASSOCIATION_TIMEOUT_COUNT  8

#define BEACON_INTERVAL        100

/////////////////////////////////////////////
// Message constants
/////////////////////////////////////////////

// based on 802.11 standard, page 36
#define ASSOCIATION_REQUEST     0
#define ASSOCIATION_RESPONSE    1
#define BEACON                  8
#define DISASSOCIATION         10

// timeout for association request in s
#define ASSOCIATION_REQUEST_TIMEOUT      1

struct station_class
{
  // flag indicating whether the current station is access point or not
  int is_access_point;

  // id of station
  unsigned id;

  // IP address of station
  struct in_addr ip_address;

  // IP address of the AP with which this station is associated
  struct in_addr associated_ip_address;

  // beacon interval in ms (for AP stations only);
  // default value use in real devices is 100
  unsigned beacon_interval;

  // beacon count during a predefined interval
  // equal to beacon_interval * ASSOCIATION_TIMEOUT_COUNT
  unsigned beacon_count;

  // flag indicating whether the current station is associated to an AP or not
  // (for regular stations only)
  int is_associated;

  // flag indicating that execution is to be interrupted
  int do_interrupt;

  // send beacon thread structure (for AP stations only)
  pthread_t sta_ap_send_beacon_thread;

  pthread_t sta_association_timeout_thread;

  // listen message thread structure (for regular stations only)
  pthread_t sta_listen_message_thread;

  pthread_mutex_t association_mutex;

  // time when the last association request was sent (for regular stations only)
  float last_association_request_time;

  // time when the last association response was received 
  // (for regular stations only)
  float last_association_response_time;

  // time when the last disassociation took place (for regular stations only)
  float last_disassociation_time;
};

struct message_class
{
  // message type (see 802.11 standard, page 36)
  unsigned char type;

  struct in_addr dst_addr;

  struct in_addr src_addr;

  // bssid of current BSS (see 802.11 standard, page 45)
  unsigned char bssid[6];

  float timestamp;
};

// routine of STA AP thread that sends beacons
void *sta_ap_send_beacon_routine (void *arg);

// routine of station thread that listens to messages
void *sta_listen_message_routine (void *arg);

int sta_init (struct station_class *station, int is_access_point, unsigned id,
	      struct in_addr *ip_addresses, unsigned beacon_interval);

int sta_finalize (struct station_class *station);

void sta_print (struct station_class *station);

void message_print (struct message_class *message);


#endif
