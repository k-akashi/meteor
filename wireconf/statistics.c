/*
 * Copyright (c) 2006-2013 The StarBED Project  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/************************************************************************
 *
 * QOMET Emulator Implementation
 *
 * File name: statistics.c
 * Function: Real-time statistics computation and management module for
 *           the wireconf library
 *
 * Author: Razvan Beuran
 *
 ***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>

#include "wlan.h"
#include "wireconf.h"
//#include "management.h"
#include "statistics.h"

#define LEARNING_RATE             0.5
#define ACK_COLLISION_SUPPORT     0

// Lan manually define the packet size in slot for multihop experiment !!!!!!!!
// which is used for computing collison proability
//#define PKT_SIZE_IN_SLOT       60     // 1024 bytes packet including headers
//#define PKT_SIZE_IN_SLOT       37     // 512 bytes packet including headers

// statistics communication thread that sends traffic statistics
// to other wireconf instances
void *
stats_send_thread (void *arg)
{
  // channel utilization statistics provided to this thread 
  // for being distributed
  //float channel_utilization;

  struct stats_class *stats = NULL;

  // number of bytes read from the pipe on which byte statistics
  // provided to this thread for being distributed
  ssize_t read_byte_count;

  // number of bytes sent on multicast socket
  ssize_t sent_byte_count;

  // id of the MCAST socket
  int mcast_socket_id = -1;

  // address of the MCAST socket
  struct sockaddr_in mcast_socket_address;

  // wireconf structure passed as argument
  struct wireconf_class *wireconf = (struct wireconf_class *) arg;

  // thread starts execution
  // NOTE: to be replaced by INFO
  //INFO ("Statistics send thread started (relative time=%.2f s).", get_elapsed_time(wireconf));

  stats = calloc (1, sizeof (struct stats_class));
  if (stats == NULL)
    {
      //WARNING("Could not open allocate statistics structure.");
      return NULL;
    }

  // create sending socket (SOCK_DGRAM signifies UDP, the only choice
  // available since we use MCAST)
  if ((mcast_socket_id = socket (PF_INET, SOCK_DGRAM, 0)) == -1)
    {
      //WARNING ("Could not open MCAST socket in statistics send thread.");
      return NULL;
    }

  // configure sending socket address
  memset (&mcast_socket_address, 0, sizeof (mcast_socket_address));
  mcast_socket_address.sin_family = AF_INET;
  mcast_socket_address.sin_port = htons (STATISTICS_PORT);
  mcast_socket_address.sin_addr.s_addr = inet_addr (STATISTICS_ADDRESS);

  // disable loopback so that we don't receive our own traffic
  // NOTE: temporarily not used
  /*
     {
     char loop_char=0;

     if (setsockopt(mcast_socket_id, IPPROTO_IP, IP_MULTICAST_LOOP,
     (char *)&loop_char, sizeof(loop_char)) < 0)
     {
     //WARNING("Error when setting socket option IP_MULTICAST_LOOP");
     perror("setsockopt");
     close(mcast_socket_id);
     return NULL;
     }
     }
   */

  // set local interface for outbound multicast traffic
  {
    // address of local interface
    struct in_addr local_interface;

    // set local interface address
    local_interface.s_addr = (wireconf->IP_addresses[wireconf->my_id]).s_addr;

    // associate the interface to the MCAST socket
    if (setsockopt (mcast_socket_id, IPPROTO_IP, IP_MULTICAST_IF,
            (char *) &local_interface, sizeof (local_interface)) < 0)
      {
    //WARNING ("Error when setting socket option IP_MULTICAST_IF");
    perror ("setsockopt");
    close (mcast_socket_id);
    return NULL;
      }
  }

  // execute until interrupted
  while (1)
    {
      // read statistics provided to this thread by means of a pipe
      read_byte_count = read (wireconf->stats_provide_pipe[0],
                  stats, sizeof (struct stats_class));

      // check for errors
      if (read_byte_count == -1)
    {
      //WARNING ("Error reading from pipe in statistics send thread");
      perror ("read");
      return NULL;
    }
      // the other end was closed => end execution
      else if (read_byte_count == 0 || wireconf->do_interrupt_send == TRUE)
    {
      // instead of breaking execution directly, we send one more
      // packet with contents STATISTICS_END_EXEC, thus signaling
      // to the listening thread that it has to terminate
      // execution as well
      stats->channel_utilization = STATISTICS_END_EXEC;
    }

      // print provided statistics (unless execution must be terminated)
      //if (stats->channel_utilization != STATISTICS_END_EXEC)
    //INFO ("Statistics send thread (time=%.2f s): channel_utilization=%f SENT to %s:%d", 
        //get_elapsed_time (wireconf), stats->channel_utilization, inet_ntoa (mcast_socket_address.sin_addr), ntohs (mcast_socket_address.sin_port));

      // send statistics data to multicast socket
      sent_byte_count = sendto (mcast_socket_id,
                stats,
                sizeof (struct stats_class), 0,
                (struct sockaddr *) &mcast_socket_address,
                sizeof (mcast_socket_address));

      // check for errors
      if (sent_byte_count == -1)
    {
      //WARNING ("Cannot send statistics to socket");
      perror ("sendto");
      close (mcast_socket_id);
      return NULL;
    }
      else if (sent_byte_count != sizeof (struct stats_class))
    {
      //WARNING ("Failed to send all statistics to socket");
    }

      // check if we need to end execution
      if (stats->channel_utilization == STATISTICS_END_EXEC)
    break;
    }

  // do clean up
  if (mcast_socket_id != -1)
    close (mcast_socket_id);

  if (stats != NULL)
    free (stats);

  // thread finished execution
  // NOTE: to be replaced by INFO
  //INFO ("Statistics send thread finished execution (relative time=%.2f s).", get_elapsed_time (wireconf));

  return NULL;
}

// statistics communication thread that listens for 
// traffic statistics sent by other wireconf instances
void *
stats_listen_thread (void *arg)
{
  // id and address of the socket used for receiving traffic
  // statistics via MCAST
  int mcast_socket_id = -1;
  struct sockaddr_in mcast_socket_address;

  // originating address and address size of the incoming traffic
  struct sockaddr_storage incoming_address;
  socklen_t incoming_address_size;

  // id of the originating address 
  int incoming_id;
  //int ch_util_i;

  // received channel utilization statistics (one item)
  //float channel_utilization;

  struct stats_class stats;

  // number of received bytes
  ssize_t recv_byte_count;

  // wireconf structure passed as argument
  struct wireconf_class *wireconf = (struct wireconf_class *) arg;

  // listen thread starts execution
  // NOTE: to be replaced by INFO
  //INFO ("Statistics listen thread started (relative time=%.2f s).", get_elapsed_time (wireconf));

  incoming_id = -1;

  // open listening socket (use SOCK_STREAM for TCP/IP, SOCK_DGRAM for UDP)
  if ((mcast_socket_id = socket (PF_INET, SOCK_DGRAM, 0)) == -1)
    {
      //WARNING ("Cannot create MCAST socket in statistics listen thread");
      perror ("socket");
      return NULL;
    }

  // configure MCAST socket address
  memset (&mcast_socket_address, 0, sizeof (mcast_socket_address));
  mcast_socket_address.sin_family = AF_INET;
  mcast_socket_address.sin_port = htons (STATISTICS_PORT);
  mcast_socket_address.sin_addr.s_addr = INADDR_ANY;
  memset (mcast_socket_address.sin_zero, '\0',
      sizeof (mcast_socket_address.sin_zero));

  // binding to the appropriate port 
  if (bind (mcast_socket_id, (struct sockaddr *) &mcast_socket_address,
        sizeof (mcast_socket_address)) == -1)
    {
      //WARNING ("Cannot bind socket with id %d in statistics listen thread", mcast_socket_id);
      perror ("bind");
      close (mcast_socket_id);

      return NULL;
    }

  // join the multicast group
  {
    // MCAST group structure
    struct ip_mreq mcast_group;

    // set MCAST address and interface
    mcast_group.imr_multiaddr.s_addr = inet_addr (STATISTICS_ADDRESS);
    mcast_group.imr_interface.s_addr =
      (wireconf->IP_addresses[wireconf->my_id]).s_addr;
    if (setsockopt (mcast_socket_id, IPPROTO_IP, IP_ADD_MEMBERSHIP,
            (char *) &mcast_group, sizeof (mcast_group)) < 0)
      {
    //WARNING ("Error when setting socket option IP_ADD_MEMBERSHIP");
    perror ("setsockopt");
    close (mcast_socket_id);
    return NULL;
      }
  }

  // initialize the size of the incoming address
  incoming_address_size = sizeof (incoming_address);

  // execute until interrupted
  while (wireconf->do_interrupt_listen == FALSE)
    {
      // receive from socket
      recv_byte_count = recvfrom (mcast_socket_id, &stats,
                  sizeof (struct stats_class), 0,
                  (struct sockaddr *) &incoming_address,
                  &incoming_address_size);

      // check for errors
      if (recv_byte_count == -1)
    {
      //WARNING ("Cannot receive from socket");
      perror ("recvfrom");
      close (mcast_socket_id);
      return NULL;
    }
      // other side was closed (only valid for connection-oriented 
      // sockets)
      else if (recv_byte_count == 0)
    {
      close (mcast_socket_id);
      break;
    }

      // not all bytes were received
      else if (recv_byte_count != sizeof (struct stats_class))
    {
      //WARNING ("Failed to receive all data from socket");
      perror ("recvfrom");
    }
      else          // succesfull reception
    {
      // print received statistics (unless execution must end)
      if (stats.channel_utilization != STATISTICS_END_EXEC)
        {
          //incoming_id = get_id_from_ip_in_addr_t (wireconf->IP_addresses, ((struct sockaddr_in *)(&incoming_address))-> sin_addr.s_addr);
          incoming_id = 1;

          // CHECK range!!!!!!!!!!!!!!!!!!!!!!!!!!11111

          if (incoming_id < 0 || incoming_id > wireconf->node_count - 1) {
        //WARNING ("Id %d is out of range [%d, %d]. Ignoring...", incoming_id, 0, wireconf->node_count - 1);
            printf("");
            }
          else
        {
          wireconf->total_channel_utilizations[incoming_id]
            = stats.channel_utilization;
          wireconf->total_transmission_probabilities[incoming_id]
            = stats.transmission_probability;

          //Lan : channel utilization per source
          /*
             for (ch_util_i = 0; ch_util_i < wireconf->node_count;
             ch_util_i++)
             {
             wireconf->
             channel_utilization_per_source[incoming_id][ch_util_i]
             = stats.channel_utilization_per_source[ch_util_i];
             }
           */
        }

          //INFO ("Statistics listen thread (time=%.2f s): \
channel_utilization=\t%f RECEIVED from %s:%d\t => incoming_id=%d", get_elapsed_time (wireconf), stats.channel_utilization, inet_ntoa (((struct sockaddr_in *) (&incoming_address))->sin_addr), ntohs (((struct sockaddr_in *) (&incoming_address))->sin_port), incoming_id);
        }
    }

      // end execution if signaled by sender
      if (stats.channel_utilization == STATISTICS_END_EXEC)
    wireconf->do_interrupt_listen = TRUE;
    }

  // do cleaning up
  if (mcast_socket_id != -1)
    close (mcast_socket_id);

  // listen thread finished execution
  //INFO ("Statistics listen thread finished execution (relative time=%.2f s).", get_elapsed_time (wireconf));

  return NULL;
}

float
compute_channel_utilization (struct bin_rec_cls *binary_record,
                 long long unsigned int delta_pkt_counter,
                 long long unsigned int delta_byte_counter,
                 float time_interval)
{
  float avg_frame_size;
  float channel_utilization;
  float alpha;

  float float_delta_pkt_counter, float_delta_byte_counter;

  struct connection_class connection;
  double ppdu_duration, frame_duration;
  int slot_time;

  //DEBUG ("Computing statistics...");
  //DEBUG ("delta_pkt_counter=%llu delta_byte_counter=%llu", delta_pkt_counter, delta_byte_counter);

  // adjust counters according to duration of measurements
  float_delta_pkt_counter = delta_pkt_counter / time_interval;
  float_delta_byte_counter = delta_byte_counter / time_interval;

  // check whether packets were transmitted
  if (float_delta_pkt_counter != 0)
    {
      avg_frame_size = float_delta_byte_counter / float_delta_pkt_counter;

      connection_init (&connection, "UNKNOWN", "UNKNOWN", "UNKNOWN",
               (int) avg_frame_size, binary_record->standard,
               1, 2347, FALSE);

      //DEBUG ("Standard: %d  Operating rate: %f", binary_record->standard, binary_record->operating_rate);

      connection.operating_rate =
    wlan_operating_rate_index
    (&connection, binary_record->operating_rate);

      if (connection.operating_rate == -1)
    {
      //DEBUG ("Unknown operating rate for WLAN: %f, assuming non-WLAN standard and returning 0.0 channel utilization.", binary_record->operating_rate);

      return 0.0;
    }

      wlan_ppdu_duration (&connection, &ppdu_duration, &slot_time);

#ifdef ZERO
      frame_duration = 192 /*PHY_Overhead */  +
    ceil ((224 /*MAC_Overhead */  + avg_frame_size * 8 /*Frame_Body */ ) *
          1e6 / binary_record->operating_rate);
#endif

      frame_duration = ceil (avg_frame_size * 8 * 1e6
                 / binary_record->operating_rate);

      alpha = ppdu_duration / frame_duration;   //!!!!!!!!!!!!!!!!!!

      //DEBUG ("avg_frame_size=%.2f PPDU duration=%.2f frame duration=%.2f alpha=%f", avg_frame_size, ppdu_duration, frame_duration, alpha);

      // special adjust delta_byte_counter because of bandwidth limitation
      if ((float_delta_byte_counter * 8) > binary_record->bandwidth)
    float_delta_byte_counter = binary_record->bandwidth / 8;

      // NOTE: COMMENTED OUT SINCE THE LOSS IN WIRELESS MEDIA DOESN'T
      // HAPPEN AT SENDER, BUT IN THE MEDIA, THEREFORE CHANNEL IS USED
      // EVEN FOR LOST PACKETS => LOSS RATE SHOULD ONLY BE CONSIDERED
      // WHEN IT IS EQUAL TO 1 (~disconnect)
      // special  adjust delta_byte_counter because of loss rate
      // float_delta_byte_counter *= (1 - binary_record->loss_rate);
      if (binary_record->loss_rate == 1.00)
    float_delta_byte_counter = 0;

      // compute channel utilization
      channel_utilization = (float_delta_byte_counter * 8 * alpha
                 * (binary_record->num_retransmissions + 1))
    / binary_record->operating_rate;

#ifdef DEBUG
      //io_binary_print_record (binary_record);
      //DEBUG ("throughput=%.4f  channel_utilization=%f", float_delta_byte_counter * 8, channel_utilization);
#endif

    }
  // if no packets were transmitted,
  // channel utilization should be 0
  else
    channel_utilization = 0;

  return channel_utilization;
}


float
compute_transmission_probability (struct bin_rec_cls *binary_record,
                  long long unsigned int delta_pkt_counter,
                  long long unsigned int delta_byte_counter,
                  float time_interval,
                  float *adjusted_float_delta_pkt_counter1)
{
  float avg_frame_size;
  float transmission_probability;
  float number_slots;

  float float_delta_pkt_counter, float_delta_byte_counter;
  float adjusted_float_delta_pkt_counter;

  struct connection_class connection;
  double ppdu_duration;
  int slot_time;

  *adjusted_float_delta_pkt_counter1 = 0;

  //DEBUG ("Computing transmission probability...");
  //DEBUG ("delta_pkt_counter=%llu delta_byte_counter=%llu time_interval=%f", delta_pkt_counter, delta_byte_counter, time_interval);

  // adjust counters according to duration of measurements
  float_delta_pkt_counter = delta_pkt_counter / time_interval;
  float_delta_byte_counter = delta_byte_counter / time_interval;

  // check whether packets were transmitted
  if (float_delta_pkt_counter != 0)
    {
      avg_frame_size = float_delta_byte_counter / float_delta_pkt_counter;

      connection_init (&connection, "UNKNOWN", "UNKNOWN", "UNKNOWN",
               (int) avg_frame_size, binary_record->standard,
               1, 2347, FALSE);

      //DEBUG ("Standard: %d  Operating rate: %f", binary_record->standard, binary_record->operating_rate);

      connection.operating_rate =
    wlan_operating_rate_index
    (&connection, binary_record->operating_rate);

      if (connection.operating_rate == -1)
    {
      //DEBUG ("Unknown operating rate for WLAN: %f, assuming non-WLAN standard and returning 0.0 channel utilization.", binary_record->operating_rate);

      return 0.0;
    }

      wlan_ppdu_duration (&connection, &ppdu_duration, &slot_time);

      //DEBUG ("float_delta_byte_counter=%f avg_frame_size=%f slot_time=%d", float_delta_byte_counter, avg_frame_size, slot_time);

      // special adjust delta_byte_counter because of bandwidth limitation
      if (binary_record->bandwidth != -1.0) // not UNDEFINED_BANDWIDTH
    if ((float_delta_byte_counter * 8) > binary_record->bandwidth)
      float_delta_byte_counter = binary_record->bandwidth / 8;

      // special  adjust delta_byte_counter because of loss rate
      float_delta_byte_counter *= (1 - binary_record->loss_rate);

      adjusted_float_delta_pkt_counter =
    float_delta_byte_counter / avg_frame_size;

      *adjusted_float_delta_pkt_counter1 = adjusted_float_delta_pkt_counter;

      number_slots = 1e6 / (float) slot_time;

      //DEBUG ("float_delta_byte_counter=%f avg_frame_size=%f slot_time=%d", float_delta_byte_counter, avg_frame_size, slot_time);
      //DEBUG ("adjusted_float_delta_pkt_counter=%f number_slots=%f", adjusted_float_delta_pkt_counter, number_slots);

      // compute transmission probability
      transmission_probability =
    adjusted_float_delta_pkt_counter / number_slots;

      //DEBUG ("transmission_probability=%f", transmission_probability);
    }
  // if no packets were transmitted,
  // transmission probability should be 0
  else
    transmission_probability = 0;

  return transmission_probability;
}



int
adjust_deltaQ (struct wireconf_class *wireconf,
           struct bin_rec_cls **binary_records_ucast,
           struct bin_rec_cls *adjusted_binary_records_ucast,
           int *binary_records_ucast_changed, float *avg_frame_sizes)
{
  int i;

  //float avg_frame_size;

  float total_channel_utilization_others = 0.0;
  float total_transmission_probability_others = 0.0;

  struct connection_class connection;

  float number_slots_per_packet;
  float number_packets_by_others;

  float total_self_channel_utilization = 0.0;
  float total_self_transmission_probability = 0.0;

  int number_active_nodes = 0;


  float ACTIVE_THRESH = 0.02;

  double fixed_delay = 0.0;

  // need default value !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //avg_frame_size = 1024; //1500; //1024;
  //avg_frame_size = 512;

  // compute first total channel utilization of other nodes
  for (i = 0; i < wireconf->node_count; i++)
    // do not consider the node itself
    if (i != wireconf->my_id)
      // do not consider values received from nodes with which 
      // loss rate is 1
      // NOTE: should look actually at the reverse direction,
      // since links may be asymmetric!!!!!!!!!!!!!!!!
      if (binary_records_ucast[wireconf->my_id][i].loss_rate < 1.0)
    {
      total_channel_utilization_others +=
        wireconf->total_channel_utilizations[i];

      if (wireconf->total_channel_utilizations[i] > ACTIVE_THRESH)
        number_active_nodes++;
    }

  // always consider current node as active
  number_active_nodes++;

  /*
     // we then need to include the channel utilization of the other flows
     // going from this node to other destinations since collision with itself 
     // is not possible
     total_channel_utilization_others = total_channel_utilization_others + 
     wireconf->total_channel_utilizations[wireconf->my_id] -
     wireconf->self_channel_utilizations[binary_records_ucast->to_node];
   */

  wireconf->total_channel_utilization_others
    = total_channel_utilization_others;

  //DEBUG ("total_self_ch_util=%f  total_ch_util_others=%f total_ch_util=%f  number_active_nodes=%d", wireconf->total_channel_utilizations[wireconf->my_id], total_channel_utilization_others, wireconf->total_channel_utilizations[wireconf->my_id] + total_channel_utilization_others, number_active_nodes);

  // compute total transmission probability of other nodes
  for (i = 0; i < wireconf->node_count; i++)
    // do not consider the node itself
    if (i != wireconf->my_id)
      // do not consider values received from nodes with which 
      // loss rate is 1
      // NOTE: should look actually at the reverse direction,
      // since links may be asymmetric!!!!!!!!!!!!!!!!
      if (binary_records_ucast[wireconf->my_id][i].loss_rate < 1.0)
    total_transmission_probability_others +=
      wireconf->total_transmission_probabilities[i];

  // save value
  wireconf->total_transmission_probability_others
    = total_transmission_probability_others;

  //DEBUG ("total_transmission_probability_others = %f", total_transmission_probability_others);


  // compute total self channel utilization and transmission probability
  for (i = 0; i < wireconf->node_count; i++)
    if (i != wireconf->my_id)
      {
    total_self_channel_utilization
      += wireconf->self_channel_utilizations[i];
    total_self_transmission_probability
      += wireconf->self_transmission_probabilities[i];
      }

  //DEBUG ("total_self_channel_utilization=%f   total_self_transmission_probability=%f", total_self_channel_utilization, total_self_transmission_probability);

  // compute number of slots per packet
  if (total_self_transmission_probability != 0)
    number_slots_per_packet = total_self_channel_utilization
      / total_self_transmission_probability;
  else
    number_slots_per_packet = 0;

  //DEBUG ("number_slots_per_packet = %f", number_slots_per_packet);

  // compute number of packets sent by others
  number_packets_by_others = total_transmission_probability_others * (0.5 / 20e-6); //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //DEBUG ("number_packets_by_others = %f", number_packets_by_others);

  // copy deltaQ
  for (i = 0; i < wireconf->node_count; i++)
    // do not consider the node itself
    if (i != wireconf->my_id)
      {
    /* IGNORE FLAG FOR THE MOMENT  
       // copy the record data for the nodes for which this data changed
       if (binary_records_ucast_changed[i] == TRUE)
     */

    /*
       // save previous state ??????
       float orig_loss = adjusted_binary_records_ucast[i].loss_rate;
       float orig_delay = adjusted_binary_records_ucast[i].delay;
       float orig_bw = adjusted_binary_records_ucast[i].bandwidth;
     */

    //copy first all fields of the record
    io_binary_copy_record (&(adjusted_binary_records_ucast[i]),
                   &(binary_records_ucast[wireconf->my_id][i]));

    // restore state ?????????????
    /*
       adjusted_binary_records_ucast[i].loss_rate = orig_loss;
       adjusted_binary_records_ucast[i].delay = orig_delay;
       adjusted_binary_records_ucast[i].bandwidth = orig_bw;
     */
      }

  // make sure channel utilization doesn't exceed nor equal 1  
  if (total_channel_utilization_others >= 1.0)
    total_channel_utilization_others = 0.99;

  //    INFO ("Total channel utilization others >= 1.0 !!!!!!!!!!!!!!!!");
  /* SHOULD BE USED??????????????
     else if (total_channel_utilization_others == 0.0)
     {
     return SUCCESS;
     }
   */
  /*
     else
     {
   */

  //DEBUG ("total_channel_utilization_others=%f", total_channel_utilization_others);

  // adjustment is necessary even for 0 total_channel_utilization_others if 
  // we want to prevent sudden "bursts" in the beginning of an experiment
  if (total_channel_utilization_others < EPSILON)
    {
      //INFO("Interference traffic not present => not adjusting deltaQ parameters");
      return SUCCESS;
    }
  else
    {
      //INFO ("Adjusting deltaQ parameters because of interference from others");

      for (i = 0; i < wireconf->node_count; i++)
    // do not consider the node itself
    if (i != wireconf->my_id)
      {
        double loss_rate;
        double delay, jitter;
        double bandwidth;
        double adaptive_learning_rate;
        float coll_prob;
        double num_retransmissions;

        // initialize connection
        connection_init (&connection, "UNKNOWN", "UNKNOWN", "UNKNOWN",
                 (int) avg_frame_sizes[i],
                 binary_records_ucast[wireconf->my_id][i].
                 standard, 1, 2347, FALSE);


        coll_prob = compute_collision_probability (wireconf, i,
                               binary_records_ucast);
        //DEBUG ("NEW: avg_frame_size=%f coll_prob = %f", avg_frame_sizes[i], coll_prob);

        /*
           if ( number_active_nodes > 1)
           adaptive_learning_rate = LEARNING_RATE / (number_active_nodes - 1);
           else
           adaptive_learning_rate = LEARNING_RATE;
         */

        adaptive_learning_rate = LEARNING_RATE;

        //float new_bandwidth;
        //DEBUG ("Standard: %d  Operating rate: %f", binary_records_ucast[wireconf->my_id][i].standard, binary_records_ucast[wireconf->my_id][i].operating_rate);

        connection.operating_rate =
          wlan_operating_rate_index
          (&connection,
           binary_records_ucast[wireconf->my_id][i].operating_rate);
        //      adjusted_binary_records_ucast[i].operating_rate 
        //  = connection.operating_rate;

        if (connection.operating_rate == -1)
          {
        //DEBUG ("Unknown operating rate for WLAN: %f, assuming non-WLAN standard and returning 0.0 channel utilization.", binary_records_ucast[wireconf->my_id][i].operating_rate);

        return ERROR;
          }

        // compute first the fixed delay by subtracting from the 
        // static parameters the value computed for 0 channel utilization
        // of others
        //      wlan_do_compute_delay_jitter (&connection, &delay, &jitter, 0.0);
        //fixed_delay = binary_records_ucast[wireconf->my_id][i].delay - delay;
        //DEBUG ("fixed_delay=%f", fixed_delay);

        // compute frame error rate
        connection.frame_error_rate
          = binary_records_ucast[wireconf->my_id][i].frame_error_rate
          + coll_prob
          - (binary_records_ucast[wireconf->my_id][i].frame_error_rate
         * coll_prob);
        // limit error rate for numerical reasons
        if (connection.frame_error_rate > MAXIMUM_ERROR_RATE)
          connection.frame_error_rate = MAXIMUM_ERROR_RATE;
        adjusted_binary_records_ucast[i].frame_error_rate
          = connection.frame_error_rate;

        // compute number of retransmissions
        wlan_retransmissions (&connection, NULL /*scenario */ ,
                  &num_retransmissions);
        connection.num_retransmissions = num_retransmissions;
        adjusted_binary_records_ucast[i].num_retransmissions
          = connection.num_retransmissions;

        // compute loss rate & adjust by learning
        wlan_do_compute_loss_rate (&connection, &loss_rate);
        connection.loss_rate = loss_rate;
        adjusted_binary_records_ucast[i].loss_rate
          += ((connection.loss_rate
           - adjusted_binary_records_ucast[i].loss_rate)
          * adaptive_learning_rate);

        //DEBUG ("connection.loss_rate=%f adjusted_binary_records_ucast[i].loss_rate=%f", connection.loss_rate, adjusted_binary_records_ucast[i].loss_rate);

        /*
           // share channel fairly!!!!!!!!!!!!!!!!!!
           if (number_active_nodes > 1)
           if (total_self_channel_utilization < (1.0/number_active_nodes))
           {
           total_channel_utilization_others = (1 - 1.0/number_active_nodes);
           DEBUG ("corrected total_channel_utilization_others=%f", 
           total_channel_utilization_others);
           }
         */

        /*
           // share channel fairly!!!!!!!!!!!!!!!!!!
           if (number_active_nodes > 1)
           if (total_channel_utilization_others > (1 - 1.0/number_active_nodes))
           {
           total_channel_utilization_others = (1 - 1.0/number_active_nodes);
           DEBUG ("corrected total_channel_utilization_others=%f", 
           total_channel_utilization_others);
           }
         */

        // share channel fairly!!!!!!!!!!!!!!!!!!
        if (number_active_nodes > 1)
          if ((total_self_channel_utilization <
           (1.0 / number_active_nodes))
          && (total_channel_utilization_others >
              (1.0 - 1.0 / number_active_nodes)))
        {
          total_channel_utilization_others =
            (1.0 - 1.0 / number_active_nodes);
          //DEBUG ("corrected total_channel_utilization_others=%f", total_channel_utilization_others);
        }

        // compute delay & adjust by learning
        wlan_do_compute_delay_jitter (&connection, &delay, &jitter,
                      total_channel_utilization_others);
        connection.variable_delay = delay;

        connection.variable_delay = adjusted_binary_records_ucast[i].delay
          + ((connection.variable_delay
          - adjusted_binary_records_ucast[i].delay)
         * adaptive_learning_rate);
        // add fixed delay !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        connection.delay = fixed_delay + connection.variable_delay;
        adjusted_binary_records_ucast[i].delay = connection.delay;

        //DEBUG ("adjusted_binary_records_ucast[i].delay=%f", adjusted_binary_records_ucast[i].delay);

        // compute bandwidth (using adjusted variable delay from above)
        wlan_do_compute_bandwidth (&connection, &bandwidth);
        connection.bandwidth = bandwidth;
        adjusted_binary_records_ucast[i].bandwidth = connection.bandwidth;

        //DEBUG ("op_rate=%.4f; static FER=%.4f dynamic FER=%.4f; static num_retr=%.4f dynamic num_retr=%.4f; static bandwidth=%.4f dynamic bandwidth=%.4f; static loss_rate=%.4f dynamic loss_rate=%.4f; static delay=%.4f  dynamic delay=%.4f", binary_records_ucast[wireconf->my_id][i].operating_rate, binary_records_ucast[wireconf->my_id][i].frame_error_rate, adjusted_binary_records_ucast[i].frame_error_rate, binary_records_ucast[wireconf->my_id][i].num_retransmissions, adjusted_binary_records_ucast[i].num_retransmissions, binary_records_ucast[wireconf->my_id][i].bandwidth, adjusted_binary_records_ucast[i].bandwidth, binary_records_ucast[wireconf->my_id][i].loss_rate, adjusted_binary_records_ucast[i].loss_rate, binary_records_ucast[wireconf->my_id][i].delay, adjusted_binary_records_ucast[i].delay);

        /*
           adjusted_binary_records_ucast[i].delay 
           += ((connection.delay 
           - adjusted_binary_records_ucast[i].delay)
           * LEARNING_ALPHA);

           new_bandwidth = adjusted_binary_records_ucast[i].bandwidth
           + ((connection.bandwidth 
           - adjusted_binary_records_ucast[i].bandwidth)
           * LEARNING_ALPHA);

           DEBUG ("old_bw=%f  new_bw=%f",
           adjusted_binary_records_ucast[i].bandwidth, new_bandwidth);

           adjusted_binary_records_ucast[i].bandwidth = new_bandwidth;
         */

        binary_records_ucast_changed[i] = TRUE;
      }
    }

  return SUCCESS;
}

/*
float
adjust_delay(float delay, float channel_utilization_others)
{
  if (channel_utilization_others <0 || channel_utilization_others > 1)
    {
      //WARNING ("Channel utilization has incorrect value");
      return delay;
    }
  if (channel_utilization_others == 1.0)
    channel_utilization_others = 0.9999;

  return delay / (1-channel_utilization_others);
}

float
adjust_bandwidth(float bandwidth, float channel_utilization_others)
{
  if (channel_utilization_others <0 || channel_utilization_others > 1)
    {
      //WARNING ("Channel utilization has incorrect value");
      return bandwidth;
    }
  if (channel_utilization_others == 1.0)
    channel_utilization_others = 0.9999;

  return bandwidth * (1-channel_utilization_others);
}

float
adjust_loss_rate(float loss_rate, float channel_utilization_others)
{
  float new_loss_rate;

  if (channel_utilization_others <0 || channel_utilization_others > 1)
    {
      //WARNING ("Channel utilization has incorrect value");
      return loss_rate;
    }
  if (channel_utilization_others == 1.0)
    channel_utilization_others = 0.9999;

  new_loss_rate = loss_rate / (1-channel_utilization_others);
  return (new_loss_rate<1.0)?new_loss_rate:1.0;
}
*/

// compute collision probability
float
compute_collision_probability (struct wireconf_class *wireconf, int rcv_id,
                   struct bin_rec_cls
                   **binary_records_ucast)
{
  int i;
  float coll_prob;
  float coll_prob1, coll_prob2;
  //float trans_prob;

  // initialize collision probability
  coll_prob1 = 0.0;
  coll_prob2 = 0.0;

  for (i = 0; i < wireconf->node_count; i++)
    {
      // check whether the node i can be sensed by node rcv_i (receiver)
      if (binary_records_ucast[i][rcv_id].loss_rate < 1.0)
    {
      // check whether the node i can be sensed by node my_id (sender)
      if (binary_records_ucast[i][wireconf->my_id].loss_rate < 1.0)
        {
          // use formula to compute collision probability between 
          // two nodes in the same sensing area

          // since this situation only allows for collision between 
          // the start of two frames we ignore it for the moment
          //coll_prob += 0.0;
        }
      else
        {
          // do not consider the node itself, nor the sender
          if ((i != rcv_id) && (i != wireconf->my_id))
        {
          // collision probability due to node i will be equal 
          // with the channel utilization of node i; we assume
          // these are disjoint events, and the total probability
          // can be computed by summation (note that this is not 
          // true in the case two nodes cannot hear each other, 
          // in which case their channel utilizations become 
          // non-disjoint events
          //coll_prob += wireconf->total_channel_utilizations[i];

          // Lan add on Oct 21 for overlapp of pkts collision
          coll_prob1 += wireconf->total_channel_utilizations[i];

          //trans_prob = wireconf->total_channel_utilizations[i] / PKT_SIZE_IN_SLOT;
          //coll_prob2 += 1 - pow(1 - trans_prob, PKT_SIZE_IN_SLOT/2);
        }
        }


      /*
         NOT NEEDED!!!!!!!!!
         // so far we ignored the sender, but we should actually 
         // also include all its traffic to other destinations than 
         // node recv_id
         coll_prob += 
         (wireconf->total_channel_utilizations[wireconf->my_id] -
         wireconf->self_channel_utilizations[rcv_id]);
       */
    }
    }               // end for loop

  //DEBUG ("Link #%i - #%i has coll_prob1=%f ; coll_prob2=%f", wireconf->my_id, rcv_id, coll_prob1, coll_prob2);

  coll_prob = coll_prob1 + coll_prob2 - coll_prob1 * coll_prob2;
  //coll_prob  = coll_prob1 + coll_prob2; 

  // limit maximum value to 1
  if (coll_prob >= 1.0)
    coll_prob = 1.0;

  return coll_prob;
}

//Lan: function to compute channel utilization for cwb
float
compute_cwb_channel_utilization (struct wireconf_class *wireconf,
                 struct bin_rec_cls *adjusted_records_ucast)
{
  int node_i, pipe_i;
  float cwb_channel_utilization;
  // my_src[i]=1 means node i sends traffic through me 
  int my_src[MAX_NODES];

  // set value for my_src
  for (node_i = 0; node_i < wireconf->node_count; node_i++)
    {
      if (node_i == wireconf->my_id ||
      wireconf->channel_utilization_per_source[wireconf->my_id][node_i] >
      EPSILON)
    my_src[node_i] = 1;
      else
    my_src[node_i] = 0;
    }

  // initialize cwb channel utilization
  cwb_channel_utilization = 0.0;

  for (node_i = 0; node_i < wireconf->node_count; node_i++)
    {
      if (node_i == wireconf->my_id)
    continue;

      // check whether the node i can be sensed by myself (receiver)
      if (adjusted_records_ucast[node_i].loss_rate < 1.0)
    {
      for (pipe_i = 0; pipe_i < wireconf->node_count; pipe_i++)
        {
          // check whether the node i can be sensed by node my_id (sender)
          if (wireconf->channel_utilization_per_source[wireconf->my_id]
          [pipe_i] < EPSILON)
        {
          // ignore traffic of nodes in my_src list
          if (my_src[pipe_i] == 0)
            if (wireconf->channel_utilization_per_source[node_i]
            [pipe_i] > 0.05)
              {
            //INFO ("CWB_channel_utilization: node=%i, source=%i, \
value=%f", node_i, pipe_i, wireconf->channel_utilization_per_source[node_i][pipe_i]);
            cwb_channel_utilization +=
              wireconf->channel_utilization_per_source[node_i]
              [pipe_i];
              }
        }
        }
    }
    }
  return cwb_channel_utilization;
}
