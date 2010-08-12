
###########################################
# Makefile for qomet library wireconf
###########################################

#  $Revision: 128 $
#  $LastChangedDate: 2009-02-06 10:21:50 +0900 (Fri, 06 Feb 2009) $
#  $LastChangedBy: razvan $

.SUFFIXES:	.c .o
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
.o:
	$(CC) -o $@ $< $(LDFLAGS) $(LDLIBS)

CFLAGS	= -I. -g -Wall -Werror -DMESSAGE_WARNING -DMESSAGE_INFO
#CFLAGS	= -I. -g -Wall -Werror -DMESSAGE_WARNING -DMESSAGE_DEBUG -DMESSAGE_INFO
LDFLAGS	= -L.
LDLIBS	= -lwireconf -ltimer -lm

TARGETS	= libwireconf.a libtimer.a do_wireconf

all:	  $(TARGETS)

libwireconf.a: wireconf.h wireconf.c wireconf.o global.h message.h
	ar rc libwireconf.a wireconf.o && ranlib libwireconf.a

libtimer.a: timer.h timer.c timer.o global.h message.h
	ar rc libtimer.a timer.o && ranlib libtimer.a	

do_wireconf: do_wireconf.c routing_info.o libwireconf.a libtimer.a
	$(CC) $(CFLAGS) -o $@ do_wireconf.c routing_info.o $(LDFLAGS) $(LDLIBS)

routing_info.o: routing_info.c routing_info.h

clean:
	rm -f $(TARGETS) *.o *.a core

