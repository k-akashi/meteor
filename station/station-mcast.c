
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
 * File name: station.c
 * Function: Implements the core functionality of the station library
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 143 $
 *   $LastChangedDate: 2009-03-31 11:30:13 +0900 (Tue, 31 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <arpa/inet.h>

#include "station.h"
#include "station_global.h"
#include "station_message.h"
#include "../timer/timer.h"

void *
sta_association_timeout_routine (void *arg)
{
  // station_class structure passed as argument
  struct station_class *station = (struct station_class *) arg;

  // timer handle
  struct timer_handle timer;

  // time to send next beacon in s
  double next_time = 0.0;

  // time increment
  double next_time_increment =
    ((double) (station->beacon_interval / 1e3) * ASSOCIATION_TIMEOUT_COUNT);

  /////////////////////////////////////////////////////
  // start execution

  // reset timer
  timer_reset (&timer, next_time);

  // thread starts execution
  DEBUG ("Station '%u': association timeout thread started \
(relative time=%.2f s).", station->id, timer_elapsed_time (&timer));


  /////////////////////////////////////////////////////
  // do timeout measurements until interrupted
  while (station->do_interrupt == FALSE)
    {
      // wait until next timeout check
      next_time += next_time_increment;
      DEBUG ("Waiting for association timeout until time=%.2f s...",
	     next_time);
      timer_wait (&timer, next_time);


      // NOTE: could use pthread_cond_wait to prevent loops here
      // when a node is not associated, and use pthread_cond_signal to 
      // resume looping when a node becomes associated

      // lock mutex
      pthread_mutex_lock (&(station->association_mutex));

      if (station->beacon_count < REQUIRED_BEACON_COUNT)
	{
	  if (station->is_associated == TRUE)
	    {
	      DEBUG ("Station '%u': received beacon count (%u) in the \
interval %.2f-%.2f is less than the required threshold (%u) => considering \
that association has been dropped.", station->id, station->beacon_count, next_time - next_time_increment, next_time, REQUIRED_BEACON_COUNT);

	      station->is_associated = FALSE;
	      station->last_association_request_time = -MAX_DOUBLE;
	      station->last_disassociation_time = next_time;

	      INFO ("Station '%u': association with AP '%s' dropped \
(time=%.2f).", station->id, inet_ntoa (station->associated_ip_address), next_time);
	    }
	  else
	    DEBUG ("Station '%u': received beacon count (%u) in the \
interval %.2f-%.2f is less than the required threshold (%u), but station is \
not associated => doing nothing.", station->id, station->beacon_count, next_time - next_time_increment, next_time, REQUIRED_BEACON_COUNT);
	}
      else
	DEBUG ("Station '%u': received beacon count (%u) in the \
interval %.2f-%.2f is larger or equal to the required threshold (%u) => \
considering that association continues.", station->id, station->beacon_count, next_time - next_time_increment, next_time, REQUIRED_BEACON_COUNT);

      // reset beacon counter
      station->beacon_count = 0;

      // unlock mutex
      pthread_mutex_unlock (&(station->association_mutex));
    }

  // association timeout thread finished execution
  INFO ("Station '%u: association timeout thread finished execution \
(relative time=%.2f s).", station->id, timer_elapsed_time (&timer));

  return NULL;
}

// routine of AP station thread that sends beacons
void *
sta_ap_send_beacon_routine (void *arg)
{
  /////////////////////////////////////////////////////
  // variables related to networking

  // number of bytes sent on multicast socket
  ssize_t sent_byte_count;

  // id and address of the MCAST socket used for sending traffic
  int mcast_out_socket_id = -1;
  struct sockaddr_in mcast_out_socket_addr;

  // address of local interface
  struct in_addr local_interface;

  // message structure
  struct message_class message;


  /////////////////////////////////////////////////////
  // variables related to station behavior

  // station_class structure passed as argument
  struct station_class *station = (struct station_class *) arg;

  // number of beacons sent since execution started
  unsigned long total_beacon_count = 0;

  // timer handle
  struct timer_handle timer;

  // time to send next beacon in s
  double next_time = 0.0;


  /////////////////////////////////////////////////////
  // start execution

  // reset timer
  timer_reset (&timer, next_time);

  // thread starts execution
  DEBUG ("Station '%u': send beacon thread started (relative time=%.2f s).",
	 station->id, timer_elapsed_time (&timer));


  /////////////////////////////////////////////////////
  // create multicast socket

  // create sending socket (SOCK_DGRAM signifies UDP, the only choice
  // available since we use MCAST)
  if ((mcast_out_socket_id = socket (PF_INET, SOCK_DGRAM, 0)) == -1)
    {
      WARNING ("Station '%u': could not open MCAST outbound socket in send \
beacon thread.", station->id);
      return NULL;
    }

  // configure sending socket address
  memset (&mcast_out_socket_addr, 0, sizeof (mcast_out_socket_addr));
  mcast_out_socket_addr.sin_family = AF_INET;
  mcast_out_socket_addr.sin_port = htons (BEACONS_PORT);
  mcast_out_socket_addr.sin_addr.s_addr = inet_addr (BEACONS_ADDRESS);

  // disable loopback so that we don't receive our own traffic
  // NOTE: temporarily not used
  /*
     {
     char loop_char=0;

     if (setsockopt(mcast_socket_id, IPPROTO_IP, IP_MULTICAST_LOOP,
     (char *)&loop_char, sizeof(loop_char)) < 0)
     {
     WARNING("Error when setting socket option IP_MULTICAST_LOOP");
     perror("setsockopt");
     close(mcast_socket_id);
     return NULL;
     }
     }
   */


  /////////////////////////////////////////////////////
  // set local interface for outbound multicast traffic

  // set local interface address
  local_interface.s_addr = station->ip_address.s_addr;

  // associate the interface to the MCAST outbound socket
  if (setsockopt (mcast_out_socket_id, IPPROTO_IP, IP_MULTICAST_IF,
		  (char *) &local_interface, sizeof (local_interface)) < 0)
    {
      WARNING ("Station '%u': error when setting socket option \
IP_MULTICAST_IF", station->id);
      perror ("setsockopt");
      close (mcast_out_socket_id);
      return NULL;
    }

  /////////////////////////////////////////////////////
  // prepare beacon message
  message.type = BEACON;
  memset (&message.dst_addr, 0xFF, sizeof (message.dst_addr));
  message.src_addr = station->ip_address;
  message.bssid[0] = 1;
  message.bssid[1] = 0;
  message.bssid[2] = 0;
  message.bssid[3] = 0;
  message.bssid[4] = 0;
  message.bssid[5] = 0;
  message.timestamp = 0.0;

  /////////////////////////////////////////////////////
  // send beacons until interrupted
  while (station->do_interrupt == FALSE)
    {
      message.timestamp = next_time;

      sent_byte_count =
	sendto (mcast_out_socket_id, &message, sizeof (struct message_class),
		0, (struct sockaddr *) &mcast_out_socket_addr,
		sizeof (mcast_out_socket_addr));

      // check for errors
      if (sent_byte_count == -1)
	{
	  WARNING ("Station '%u': cannot send message to socket",
		   station->id);
	  perror ("sendto");
	  close (mcast_out_socket_id);
	  return NULL;
	}
      else if (sent_byte_count != sizeof (struct message_class))
	{
	  WARNING ("Station '%u': failed to send the entire message to \
socket", station->id);
	  close (mcast_out_socket_id);
	  return NULL;
	}

      // increment beacon counter
      total_beacon_count++;
      DEBUG ("Station '%u': sent beacon until time=%.2f \
(total_beacon_count=%lu).", station->id, next_time, total_beacon_count);

      // wait until time to send next beacon
      next_time += ((double) station->beacon_interval / 1e3);
      timer_wait (&timer, next_time);
    }

  /////////////////////////////////////////////////////
  // do clean up
  if (mcast_out_socket_id != -1)
    close (mcast_out_socket_id);

  // beacon send thread finished execution
  INFO ("Station '%u': send beacon thread finished execution (relative \
time=%.2f s).", station->id, timer_elapsed_time (&timer));

  return NULL;
}

// routine of station thread that listens to messages (both for regular and AP
// stations)
void *
sta_listen_message_routine (void *arg)
{
  // id and address of the socket used for receiving traffic via MCAST
  int mcast_in_socket_id = -1;
  struct sockaddr_in mcast_in_socket_addr;

  // id and address of the socket used for sending traffic
  int mcast_out_socket_id = -1;
  struct sockaddr_in mcast_out_socket_addr;

  // number of bytes sent to multicast outbound socket
  ssize_t sent_byte_count;

  // address of local interface
  struct in_addr local_interface;

  // originating address and address size of the incoming traffic
  struct sockaddr_storage incoming_address;
  socklen_t incoming_address_size;

  // id of the originating address 
  int incoming_id;



  // station structure passed as argument
  struct station_class *station = (struct station_class *) arg;

  // message
  struct message_class message;

  // number of received bytes
  ssize_t recv_byte_count;

  // timer handle
  struct timer_handle timer;

  // MCAST group structure
  struct ip_mreq mcast_group;

  int sockopt_val;

  double next_time = 0.0;


  /////////////////////////////////////////////////////
  // start execution

  // reset timer
  timer_reset (&timer, next_time);

  // listen thread starts execution
  DEBUG
    ("Station '%u': listen message thread started (relative time=%.2f s).",
     station->id, next_time);

  incoming_id = -1;


  /////////////////////////////////////////////////////
  // create multicast outbound socket

  // create sending socket (SOCK_DGRAM signifies UDP, the only choice
  // available since we use MCAST)
  if ((mcast_out_socket_id = socket (PF_INET, SOCK_DGRAM, 0)) == -1)
    {
      WARNING ("Station '%u: could not open MCAST outbound socket in \
listen beacon thread.", station->id);
      return NULL;
    }

  // configure sending socket address
  memset (&mcast_out_socket_addr, 0, sizeof (mcast_out_socket_addr));
  mcast_out_socket_addr.sin_family = AF_INET;
  mcast_out_socket_addr.sin_port = htons (BEACONS_PORT);
  mcast_out_socket_addr.sin_addr.s_addr = inet_addr (BEACONS_ADDRESS);


  /////////////////////////////////////////////////////
  // set local interface for MCAST outbound multicast traffic

  // set local interface address
  local_interface.s_addr = station->ip_address.s_addr;

  // associate the interface to the MCAST outbound socket
  if (setsockopt (mcast_out_socket_id, IPPROTO_IP, IP_MULTICAST_IF,
		  (char *) &local_interface, sizeof (local_interface)) < 0)
    {
      WARNING ("Station '%u': error when setting socket option \
IP_MULTICAST_IF", station->id);
      perror ("setsockopt");
      close (mcast_out_socket_id);
      return NULL;
    }



  /////////////////////////////////////////////////////
  // create multicast inbound socket

  // open listening socket (use SOCK_STREAM for TCP/IP, SOCK_DGRAM for UDP)
  if ((mcast_in_socket_id = socket (PF_INET, SOCK_DGRAM, 0)) == -1)
    {
      WARNING ("Station '%u': could not create MCAST inbound socket in \
listen beacon thread", station->id);
      perror ("socket");
      return NULL;
    }

  sockopt_val = 1;
  // set SO_REUSEADDR on a socket to true (1) so that more threads can listen
  // to the same socket
  if (setsockopt (mcast_in_socket_id, SOL_SOCKET, SO_REUSEADDR, &sockopt_val,
		  sizeof (sockopt_val)) < 0)
    {
      WARNING ("Station '%u': error when setting socket option SO_REUSEADDR",
	       station->id);
      perror ("setsockopt");
      close (mcast_in_socket_id);
      return NULL;
    }

  // configure MCAST socket address
  memset (&mcast_in_socket_addr, 0, sizeof (mcast_in_socket_addr));
  mcast_in_socket_addr.sin_family = AF_INET;
  mcast_in_socket_addr.sin_port = htons (BEACONS_PORT);
  mcast_in_socket_addr.sin_addr.s_addr = INADDR_ANY;
  memset (mcast_in_socket_addr.sin_zero, '\0',
	  sizeof (mcast_in_socket_addr.sin_zero));

  // binding to the appropriate port 
  if (bind (mcast_in_socket_id, (struct sockaddr *) &mcast_in_socket_addr,
	    sizeof (mcast_in_socket_addr)) == -1)
    {
      WARNING ("Station '%u': could not bind MCAST inbound socket in listen \
beacon thread.", station->id);
      perror ("bind");
      close (mcast_in_socket_id);

      return NULL;
    }


  /////////////////////////////////////
  // join the multicast group

  // set MCAST address and interface
  mcast_group.imr_multiaddr.s_addr = inet_addr (BEACONS_ADDRESS);
  mcast_group.imr_interface.s_addr = station->ip_address.s_addr;
  if (setsockopt (mcast_in_socket_id, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		  (char *) &mcast_group, sizeof (mcast_group)) < 0)
    {
      WARNING ("Station '%u': error when setting socket option \
IP_ADD_MEMBERSHIP", station->id);
      perror ("setsockopt");
      close (mcast_in_socket_id);
      return NULL;
    }


  /////////////////////////////////////
  // Start main loop

  // initialize the size of the incoming address
  incoming_address_size = sizeof (incoming_address);

  // execute until interrupted
  while (station->do_interrupt == FALSE)
    {
      // receive from socket
      recv_byte_count =
	recvfrom (mcast_in_socket_id, &message, sizeof (struct message_class),
		  0, (struct sockaddr *) &incoming_address,
		  &incoming_address_size);

      // check for errors
      if (recv_byte_count == -1)
	{
	  WARNING ("Station '%u': could not receive from socket",
		   station->id);
	  perror ("recvfrom");
	  close (mcast_in_socket_id);
	  return NULL;
	}
      // other side was closed (only valid for connection-oriented 
      // sockets)
      else if (recv_byte_count == 0)
	{
	  close (mcast_in_socket_id);
	  break;
	}
      // not all bytes were received
      else if (recv_byte_count != sizeof (struct message_class))
	{
	  WARNING ("Station '%u': failed to receive entire message from \
socket", station->id);
	  perror ("recvfrom");
	}
      else
	{
	  // successful reception => check message type
	  if (message.type == BEACON)
	    {
	      if (station->is_access_point == TRUE)
		{
		  // on access point beacons are ignored
		  DEBUG ("Station '%u': received beacon (time=%.2f) \
from AP '%s' => ignoring message because AP.", station->id, message.timestamp, inet_ntoa (message.src_addr));
		}
	      else
		{
		  DEBUG ("Station '%u': received beacon (time=%.2f) \
from AP '%s'.", station->id, message.timestamp, inet_ntoa (message.src_addr));

#ifdef MESSAGE_DEBUG

		  message_print (&message);

#endif
		  // "refresh" beacon info by updating time
		  //station->last_beacon_time = message.timestamp;

		  // lock mutex
		  pthread_mutex_lock (&(station->association_mutex));

		  // should test inside mutex?!
		  if (station->is_associated == TRUE)
		    {
		      DEBUG ("Station '%u': already associated => \
ignoring beacon.", station->id);

		      // increment number of beacons if received from AP
		      // with which we are associated
		      if (station->associated_ip_address.s_addr ==
			  message.src_addr.s_addr)
			station->beacon_count++;

		      // unlock mutex
		      pthread_mutex_unlock (&(station->association_mutex));
		    }
		  else
		    {
		      // check whether we are during an association process
		      if ((message.timestamp
			   - station->last_association_request_time)
			  < ASSOCIATION_REQUEST_TIMEOUT)
			{
			  // unlock mutex
			  pthread_mutex_unlock (&
						(station->association_mutex));

			  DEBUG ("Station '%u': has started \
association with AP '%s' at time=%.2f => ignoring beacon.", station->id, inet_ntoa (message.src_addr), station->last_association_request_time);
			}
		      else
			{
			  DEBUG ("Station '%u': not associated => \
sending association request to AP '%s'", station->id, inet_ntoa (message.src_addr));
			  station->last_association_request_time
			    = message.timestamp;

			  // unlock mutex
			  pthread_mutex_unlock (&
						(station->association_mutex));



			  message.type = ASSOCIATION_REQUEST;
			  // copy AP address from source field to destination
			  message.dst_addr = message.src_addr;
			  // copy station address to source field
			  message.src_addr = station->ip_address;

#ifdef MESSAGE_DEBUG

			  message_print (&message);

#endif

			  //ucast_socket_addr.sin_addr.s_addr = 
			  //  message.dst_addr.s_addr;

			  sent_byte_count =
			    sendto (mcast_out_socket_id, &message,
				    sizeof (struct message_class), 0,
				    (struct sockaddr *)
				    &mcast_out_socket_addr,
				    sizeof (mcast_out_socket_addr));

			  // check for errors
			  if (sent_byte_count == -1)
			    {
			      WARNING ("Station '%u': cannot send message to \
socket", station->id);
			      perror ("sendto");
			      close (mcast_out_socket_id);
			      return NULL;
			    }
			  else if (sent_byte_count !=
				   sizeof (struct message_class))
			    {
			      WARNING ("Station '%u': failed to send the \
entire message to socket", station->id);
			      close (mcast_out_socket_id);
			      return NULL;
			    }
			}
		    }
		}
	    }
	  else if (message.type == ASSOCIATION_REQUEST)
	    {
	      if (station->is_access_point == TRUE)
		{
		  DEBUG ("Station '%u': received association request \
from station '%s' => sending association response.", station->id, inet_ntoa (message.src_addr));

		  message.type = ASSOCIATION_RESPONSE;
		  // copy AP address from source field to destination
		  message.dst_addr = message.src_addr;
		  // copy station address to source field
		  message.src_addr = station->ip_address;

#ifdef MESSAGE_DEBUG

		  message_print (&message);

#endif

		  sent_byte_count =
		    sendto (mcast_out_socket_id, &message,
			    sizeof (struct message_class), 0,
			    (struct sockaddr *) &mcast_out_socket_addr,
			    sizeof (mcast_out_socket_addr));

		  // check for errors
		  if (sent_byte_count == -1)
		    {
		      WARNING ("Station '%u': cannot send message to \
socket", station->id);
		      perror ("sendto");
		      close (mcast_out_socket_id);
		      return NULL;
		    }
		  else if (sent_byte_count != sizeof (struct message_class))
		    {
		      WARNING ("Station '%u': failed to send the \
entire message to socket", station->id);
		      close (mcast_out_socket_id);
		      return NULL;
		    }
		}
	      else
		{
		  DEBUG ("Station '%u': received association request \
from station '%s' => ignoring message because not AP.", station->id, inet_ntoa (message.src_addr));
		}
	    }
	  else if (message.type == ASSOCIATION_RESPONSE)
	    {
	      if (station->is_access_point == TRUE)
		{
		  DEBUG ("Station '%u': received association response \
from station '%s' => ignoring message because AP.", station->id, inet_ntoa (message.src_addr));
		}
	      else
		{
		  DEBUG ("Station '%u': received association response \
from station '%s' => doing local configuration.", station->id, inet_ntoa (message.src_addr));

		  // lock mutex
		  pthread_mutex_lock (&(station->association_mutex));

		  station->is_associated = TRUE;

		  // unlock mutex
		  pthread_mutex_unlock (&(station->association_mutex));

		  // update local information
		  station->associated_ip_address = message.src_addr;
		  station->last_association_response_time = message.timestamp;
		  INFO ("Station '%u': associated with AP '%s' (time=%.2f).",
			station->id,
			inet_ntoa (station->associated_ip_address),
			station->last_association_response_time);
		  // set up routing tables etc.
		  //.....
		}
	    }
	  else
	    {
	      WARNING ("Station '%u': received unknown message (type=%u)",
		       station->id, message.type);
	    }
	}
      /*
         if (stats.channel_utilization == STATISTICS_END_EXEC)
         wireconf->do_interrupt_listen = TRUE;
       */
    }

  // do cleaning up
  if (mcast_out_socket_id != -1)
    close (mcast_out_socket_id);
  if (mcast_in_socket_id != -1)
    close (mcast_in_socket_id);

  // listen message thread finished execution
  INFO ("Station '%u: listen message thread finished execution \
(relative time=%.2f s).", station->id, timer_elapsed_time (&timer));

  return NULL;
}

int
sta_init (struct station_class *station, int is_access_point,
	  unsigned id, struct in_addr *ip_addresses, unsigned beacon_interval)
{
  // initialize fields of station structure
  station->is_access_point = is_access_point;
  station->id = id;
  station->ip_address = ip_addresses[id];
  station->beacon_interval = beacon_interval;
  station->beacon_count = 0;
  station->is_associated = FALSE;
  station->do_interrupt = FALSE;
  //set to large "negative" time
  station->last_association_request_time = -MAX_DOUBLE;
  pthread_mutex_init (&station->association_mutex, NULL);

  if (station->is_access_point == TRUE)
    {
      // start AP station send beacon thread
      DEBUG ("Station '%u': starting send beacon thread because AP...",
	     station->id);
      if (pthread_create (&(station->sta_ap_send_beacon_thread), NULL,
			  sta_ap_send_beacon_routine, (void *) station) != 0)
	{
	  WARNING ("Station '%u': could not create send beacon thread.",
		   station->id);
	  return ERROR;
	}
    }
  else
    {
      // start regular station association timeout thread
      DEBUG ("Station '%u': starting association timeout thread because \
not AP...", station->id);
      if (pthread_create (&(station->sta_association_timeout_thread), NULL,
			  sta_association_timeout_routine,
			  (void *) station) != 0)
	{
	  WARNING ("Station '%u': could not create association timeout\
 thread.", station->id);
	  return ERROR;
	}
    }

  // start station listen message thread both for regular and AP stations
  DEBUG ("Station '%u': starting listen message thread...", station->id);
  if (pthread_create (&(station->sta_listen_message_thread), NULL,
		      sta_listen_message_routine, (void *) station) != 0)
    {
      WARNING ("Station '%u': could not create listen message \
thread.", station->id);
      return ERROR;
    }

  return SUCCESS;
}

int
sta_finalize (struct station_class *station)
{
  // set interrupt flag to signal end of execution to all threads
  station->do_interrupt = TRUE;

  // wait for threads to react to flag changes
  sleep (2);

  if (station->is_access_point == TRUE)
    {
      // stop AP station send beacon thread
      DEBUG ("Station '%u': stopping send beacon thread because AP...",
	     station->id);

      // stop and wait for statistics send thread to end
      pthread_cancel (station->sta_ap_send_beacon_thread);
      if (pthread_join (station->sta_ap_send_beacon_thread, NULL) != 0)
	{
	  WARNING ("Station '%u': error joining send beacon thread",
		   station->id);
	  return ERROR;
	}
    }
  else
    {
      // stop regular station association timeout thread
      DEBUG ("Station '%u': stopping association timeout thread because \
not AP...", station->id);

      // stop and wait for statistics send thread to end
      pthread_cancel (station->sta_association_timeout_thread);
      if (pthread_join (station->sta_association_timeout_thread, NULL) != 0)
	{
	  WARNING ("Station '%u': error joining association timeout thread",
		   station->id);
	  return ERROR;
	}
    }

  // stop station listen message thread
  DEBUG ("Station '%u': stopping listen beacon thread...", station->id);

  // stop and wait for statistics send thread to end
  pthread_cancel (station->sta_listen_message_thread);
  if (pthread_join (station->sta_listen_message_thread, NULL) != 0)
    {
      WARNING ("Station '%u': error joining listen message thread",
	       station->id);
      return ERROR;
    }

  pthread_mutex_destroy (&station->association_mutex);

  return SUCCESS;
}

void
sta_print (struct station_class *station)
{
  INFO ("Station is %s with parameters:",
	(station->is_access_point == TRUE) ? "access point (AP)" :
	"regular (non-AP)");
  INFO ("\tid=%u  IP address=%s", station->id,
	inet_ntoa (station->ip_address));
  if (station->is_access_point == TRUE)
    INFO ("\tBeacon interval=%u ms", station->beacon_interval);
}

void
message_print (struct message_class *message)
{
  INFO ("Message: type=%u (size=%zu)", message->type,
	sizeof (struct message_class));
  INFO ("\tdst_addr=%s", inet_ntoa (message->dst_addr));
  // if put inet_ntoa calls on same line, then result is incorrect
  // because of buffer used internally by the function
  INFO ("\tsrc_addr=%s", inet_ntoa (message->src_addr));
  INFO ("\tbssid[0]=%u", message->bssid[0]);
  INFO ("\ttimestamp=%.2f", message->timestamp);
}
