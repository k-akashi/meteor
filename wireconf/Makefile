
###########################################
# Makefile for qomet library wireconf
###########################################

#  $Revision: 128 $
#  $LastChangedDate: 2009-02-06 10:21:50 +0900 (Fri, 06 Feb 2009) $
#  $LastChangedBy: razvan $

.SUFFIXES:  .c .o
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
.o:
	$(CC) -o $@ $< $(LDFLAGS) $(LDLIBS)

UNAME = $(shell uname)

CC      = gcc
HOSTCC  = gcc

ifeq ($(UNAME), Linux)
#CFLAGS  = -I. -g -Wall -Werror -D_GNU_SOURCE -DMESSAGE_WARNING -DMESSAGE_INFO -DTCDEBUG
CFLAGS  = -I. -g -Wall -Werror -D_GNU_SOURCE -DMESSAGE_WARNING -DMESSAGE_INFO
#CFLAGS  = -I. -g -Wall -Werror -D_GNU_SOURCE -O2 -Wstrict-prototypes -DMESSAGE_WARNING -DMESSAGE_INFO
DEFINES = -DRESOLVE_HOSTNAMES
LDFLAGS = -L.
LDLIBS  = -lnetlink -ltc -lwireconf -ltimer -lm -ldl
TARGETS = libnetlink.a libtc.a libwireconf.a libtimer.a q_netem.so q_tbf.so q_prio.so do_wireconf test
TOBJ=libnetlink.o #test.o iproute.o rtm_map.o
ALLOBJ=$(TOBJ) $(RTMONOBJ)
NLOBJ=iproute.o libnetlink.o ll_map.o utils.o rt_names.o
TCOBJ=libtc.o tc.o tc_qdisc.o tc_util.o tc_core.o q_netem.o iplink.o
WCOBJ=q_tbf.o q_prio.o q_pfifo.o
TESTOBJ=test.o
endif

ifeq ($(UNAME), FreeBSD)
CFLAGS  = -I. -g -Wall -Werror -DMESSAGE_WARNING -DMESSAGE_INFO
#CFLAGS = -I. -g -Wall -Werror -DMESSAGE_WARNING -DMESSAGE_DEBUG -DMESSAGE_INFO
LDFLAGS = -L.
LDLIBS  = -lwireconf -ltimer -lm
TARGETS = libwireconf.a libtimer.a do_wireconf
endif

all: $(TARGETS) 

ifeq ($(UNAME), Linux)
q_netem.so: q_netem.c
	$(CC) $(CFLAGS) -DCONFIG_GACT -DCONFIG_GACT_PROB $(LDFLAGS) $(LDLIBS) -shared -fpic $< tc_core.o -o $@ 

q_tbf.o: q_tbf.c
	$(CC) $(CFLAGS) -c -o $@ $<

q_tbf.so: q_tbf.c
	$(CC) $(CFLAGS) -DCONFIG_GACT -DCONFIG_GACT_PROB $(LDFLAGS) $(LDLIBS) -shared -fpic $< tc_core.o -o $@ 

q_prio.o: q_prio.c
	$(CC) $(CFLAGS) -c -o $@ $<

q_prio.so: q_prio.c
	$(CC) $(CFLAGS) -DCONFIG_GACT -DCONFIG_GACT_PROB $(LDFLAGS) $(LDLIBS) -shared -fpic $< -o $@

q_pfifo.o: q_pfifo.c
	$(CC) $(CFLAGS) -c -o $@ $<

q_pfifo.so: q_pfifo.c
	$(CC) $(CFLAGS) -DCONFIG_GACT -DCONFIG_GACT_PROB $(LDFLAGS) $(LDLIBS) -shared -fpic $< tc_core.o -o $@

libnetlink.a: $(NLOBJ)
	$(AR) rcs $@ $(NLOBJ) && ranlib $@

libtc.a: $(TCOBJ)
	$(AR) rcs $@ $(TCOBJ) && ranlib $@

#ip: $(IPOBJ) $(LIBNETLINK) $(LIBUTIL)
endif

libwireconf.a: wireconf.h wireconf.c wireconf.o global.h message.h
	ar rcs libwireconf.a wireconf.o $(TCOBJ) $(NLOBJ) && ranlib libwireconf.a

libtimer.a: timer.h timer.c timer.o global.h message.h
	ar rcs libtimer.a timer.o && ranlib libtimer.a   

ifeq ($(UNAME), Linux)
do_wireconf: do_wireconf.o routing_info.o libwireconf.a libtimer.a libtc.a libnetlink.a $(WCOBJ)
	$(CC) $(CFLAGS) -g -export-dynamic -o $@ do_wireconf.o routing_info.o $(WCOBJ) $(LDFLAGS) $(LDLIBS)
endif
ifeq ($(UNAME), FreeBSD)
do_wireconf: do_wireconf.c routing_info.o libwireconf.a libtimer.a 
	$(CC) $(CFLAGS) -o $@ do_wireconf.c routing_info.o $(LDFLAGS) $(LDLIBS)
endif

do_wireconf.o: do_wireconf.c

routing_info.o: routing_info.c routing_info.h

ifeq ($(UNAME), Linux)
test: libnetlink.a libtc.a $(TOBJ) $(LIBNETLINK) $(TESTOBJ)
	gcc -export-dynamic -o $@ test.o $(LDFLAGS) $(LDLIBS)
endif

clean:
	rm -f $(ALLOBJ) $(TARGETS) $(NLOBJ) *.o *.so
