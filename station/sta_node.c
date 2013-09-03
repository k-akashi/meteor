
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
 * File name: sta_node.c
 * Function: Implements a ST node using the station library
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 143 $
 *   $LastChangedDate: 2009-03-31 11:30:13 +0900 (Tue, 31 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include "station.h"
#include "station_global.h"
#include "station_message.h"

//unsigned ID_AP=0;
unsigned ID_REGULAR = 1;
//char IP_ADDRESS_AP[]="192.168.4.1";
char IP_ADDRESS_REGULAR[] = "192.168.4.2";

float EXPECTED_ASSOCIATION_TIME = 0.10;
float EXPECTED_DISASSOCIATION_TIME = 3.20;

int
main ()
{
  // regular station
  struct station_class station_regular;

  struct in_addr ip_addresses[2];

  unsigned experiment_wait;


  //////////////////////////////////////////////////////////////////
  // Initial operations

  INFO ("STA NODE STARTED.");

  // convert IP address to internal representation
  if (inet_aton (IP_ADDRESS_REGULAR, &(ip_addresses[ID_REGULAR])) == 0)
    {
      WARNING ("Address '%s' does not appear to be valid.",
	       IP_ADDRESS_REGULAR);
      return ERROR;
    }

  // make wait period long enough to observe association timeout
  experiment_wait = 10;


  //////////////////////////////////////////////////////////////////
  // Regular station initialization

  INFO ("Initializing regular station '%u'...", ID_REGULAR);

  // create regular station
  sta_init (&station_regular, FALSE, ID_REGULAR, ip_addresses,
	    BEACON_INTERVAL);

#ifdef MESSAGE_DEBUG

  // display parameters
  sta_print (&station_regular);

#endif


  //////////////////////////////////////////////////////////////////
  // Wait for association to take place
  INFO ("Running for %u s...", experiment_wait);
  sleep (experiment_wait);


  //////////////////////////////////////////////////////////////////
  // Regular station finalization

  INFO ("Finalizing regular station '%u'...", station_regular.id);

  // finalize regular station
  sta_finalize (&station_regular);

  return SUCCESS;
}
