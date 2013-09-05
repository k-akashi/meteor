
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
 * File name: message.h
 * Function:  Header file for messages in the deltaQ computation library
 *
 * Author: Junya Nakata & Razvan Beuran
 *
 * $Id: message.h 145 2013-06-07 01:11:09Z razvan $
 *
 ***********************************************************************/


#ifndef __MESSAGE_H
#define __MESSAGE_H


/////////////////////////////////////////////
// Control the types of displayed messages
/////////////////////////////////////////////

//#define MESSAGE_WARNING
//#define MESSAGE_DEBUG
//#define MESSAGE_INFO


/////////////////////////////////////////////
// Special functions for messages 
/////////////////////////////////////////////

// display a warning/error message
#ifdef MESSAGE_WARNING
#define WARNING(message...) do {                                        \
  fprintf(stdout, "deltaQ WARNING: %s, line %d: ", __FILE__, __LINE__); \
  fprintf(stdout, message); fprintf(stdout,"\n");                       \
} while(0)
#else
#define WARNING(message...)	/* message */
#endif

// display a debugging message
#ifdef MESSAGE_DEBUG
#define DEBUG(message...) do {                                        \
  fprintf(stdout, "deltaQ DEBUG: %s, line %d: ", __FILE__, __LINE__); \
  fprintf(stdout, message); fprintf(stdout,"\n");                     \
} while(0)
#else
#define DEBUG(message...)	/* message */
#endif

// display an informational message
#ifdef MESSAGE_INFO
#define INFO(message...) do {                                        \
  fprintf(stdout, message); fprintf(stdout,"\n");                    \
} while(0)
#else
#define INFO(message...)	/* message */
#endif


#endif
