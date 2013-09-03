
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
 * File name: station_message.h
 * Function:  Header file for messages in the station library
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 93 $
 *   $LastChangedDate: 2008-09-12 11:16:33 +0900 (Fri, 12 Sep 2008) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#ifndef __STATION_MESSAGE_H
#define __STATION_MESSAGE_H


/////////////////////////////////////////////
// Special functions for messages 
/////////////////////////////////////////////

// display a warning/error message
#ifdef MESSAGE_WARNING
#define WARNING(message...) do {                                         \
  fprintf(stdout, "station WARNING: %s, line %d: ", __FILE__, __LINE__); \
  fprintf(stdout, message); fprintf(stdout,"\n");                        \
} while(0)
#else
#define WARNING(message...)	/* message */
#endif

// display a debugging message
#ifdef MESSAGE_DEBUG
#define DEBUG(message...) do {                                         \
  fprintf(stdout, "station DEBUG: %s, line %d: ", __FILE__, __LINE__); \
  fprintf(stdout, message); fprintf(stdout,"\n");                      \
} while(0)
#else
#define DEBUG(message...)	/* message */
#endif

// display an informational message
#ifdef MESSAGE_INFO
#define INFO(message...) do {                                         \
  fprintf(stdout, message); fprintf(stdout,"\n");                     \
} while(0)
#else
#define INFO(message...)	/* message */
#endif


#endif
