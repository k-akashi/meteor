
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
 * File name: test_station.c
 * Function: Tests the functionality of the station library
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

unsigned ID_AP = 0;
unsigned ID_REGULAR = 1;
char IP_ADDRESS_AP[] = "192.168.4.1";
char IP_ADDRESS_REGULAR[] = "192.168.4.1";

float EXPECTED_ASSOCIATION_TIME = 0.10;
float EXPECTED_DISASSOCIATION_TIME = 3.20;

int
main ()
{
  // AP station
  struct station_class station_ap;

  // regular station
  struct station_class station_regular;

  struct in_addr ip_addresses[2];

  unsigned experiment_wait;

  int result = SUCCESS;

  //////////////////////////////////////////////////////////////////
  // Initial operations

  INFO ("TEST STARTED FOR station LIBRARY.");

  // convert IP address to internal representation
  if (inet_aton (IP_ADDRESS_AP, &(ip_addresses[ID_AP])) == 0)
    {
      WARNING ("Address '%s' does not appear to be valid.", IP_ADDRESS_AP);
      return ERROR;
    }

  // convert IP address to internal representation
  if (inet_aton (IP_ADDRESS_REGULAR, &(ip_addresses[ID_REGULAR])) == 0)
    {
      WARNING ("Address '%s' does not appear to be valid.",
	       IP_ADDRESS_REGULAR);
      return ERROR;
    }

  // make wait period long enough to observe association timeout
  experiment_wait =
    (unsigned) (ceil ((ASSOCIATION_TIMEOUT_COUNT * BEACON_INTERVAL) / 1e3) +
		1);

  //////////////////////////////////////////////////////////////////
  // AP station initialization

  INFO ("Initializing AP station '%u'...", ID_AP);

  // create AP station
  sta_init (&station_ap, TRUE, ID_AP, ip_addresses, BEACON_INTERVAL);

#ifdef MESSAGE_DEBUG

  // display parameters
  sta_print (&station_ap);

#endif


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
  INFO ("Waiting %u s to make association possible...", experiment_wait);
  sleep (experiment_wait);


  //////////////////////////////////////////////////////////////////
  // AP station finalization

  INFO ("Finalizing AP station '%u'...", station_ap.id);

  // finalize AP station
  sta_finalize (&station_ap);


  //////////////////////////////////////////////////////////////////
  // Wait for disassociation to take place

  INFO ("Waiting %u s to cause association timeout...", experiment_wait);

  sleep (experiment_wait);



  //////////////////////////////////////////////////////////////////
  // Regular station finalization

  INFO ("Finalizing regular station '%u'...", station_regular.id);

  // finalize regular station
  sta_finalize (&station_regular);

  // wait for threads to complete execution
  sleep (2);

  if (station_regular.last_association_response_time
      > EXPECTED_ASSOCIATION_TIME)
    {
      WARNING ("Association time '%.2f' is larger than the expected one \
(%.2f).", station_regular.last_association_response_time, EXPECTED_ASSOCIATION_TIME);
      result = ERROR;
    }

  if (station_regular.last_disassociation_time !=
      EXPECTED_DISASSOCIATION_TIME)
    {
      WARNING ("Disassociation time '%.2f' differs from the expected one \
(%.2f).", station_regular.last_disassociation_time, EXPECTED_DISASSOCIATION_TIME);
      result = ERROR;
    }

  if (result == ERROR)
    INFO ("TEST RESULT: completed with ERRORS.");
  else
    INFO ("TEST RESULT: completed successfully.");

  return result;
}
