
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
 * File name: stationd.c
 * Function: Daemon that behaves as a wireless station (AP or STA)
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 143 $
 *   $LastChangedDate: 2009-03-31 11:30:13 +0900 (Tue, 31 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "station.h"
#include "station_global.h"
#include "station_message.h"

//unsigned ID_AP=0;
//unsigned ID_REGULAR=1;
//char IP_ADDRESS_AP[]="192.168.4.1";
//char IP_ADDRESS_REGULAR[]="192.168.4.2";

//float EXPECTED_ASSOCIATION_TIME = 0.10;
//float EXPECTED_DISASSOCIATION_TIME = 3.20;

int
main (int argc, char *argv[])
{
  struct station_class station;

  int is_ap;
  unsigned station_id;
  char station_ip_char[20];
  struct in_addr station_ip;

  unsigned execution_duration;

  //////////////////////////////////////////////////////////////////
  // Initial operations

  if (argc < 3)
    {
      WARNING ("You must provide the node type, id and IP address:");
      WARNING ("stationd AP|STA id ip_address");
      return ERROR;
    }

  if (strncmp (argv[1], "AP", 2) == 0)
    is_ap = TRUE;
  else if (strncmp (argv[1], "STA", 3) == 0)
    is_ap = FALSE;
  else
    {
      WARNING ("Node type '%s' is unrecognized", argv[1]);
      return ERROR;
    }

  station_id = atoi (argv[2]);

  strncpy (station_ip_char, argv[3], 20);

  INFO ("Station daemon started: type=%s id=%d IP=%s",
	argv[1], station_id, argv[3]);

  // convert IP address to internal representation
  if (inet_aton (station_ip_char, &(station_ip)) == 0)
    {
      WARNING ("Address '%s' does not appear to be valid.", station_ip_char);
      return ERROR;
    }

  execution_duration = 10;


  //////////////////////////////////////////////////////////////////
  // Station daemon initialization

  INFO ("Initializing station daemon '%u'...", station_id);

  // create station
  sta_init (&station, is_ap, station_id, &(station_ip), BEACON_INTERVAL);

#ifdef MESSAGE_DEBUG

  // display parameters
  sta_print (&station);

#endif


  //////////////////////////////////////////////////////////////////
  // Wait for association to take place
  INFO ("Executing daemon for %u s...", execution_duration);
  sleep (execution_duration);


  //////////////////////////////////////////////////////////////////
  // Station finalization

  INFO ("Shutting down station daemon '%u'...", station.id);
  // finalize station
  sta_finalize (&station);

  return SUCCESS;
}
