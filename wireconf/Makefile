###########################################
# Makefile for qomet library wireconf
###########################################

.SUFFIXES:  .c .o
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
.o:
	$(CC) -o $@ $< $(LDFLAGS) $(LDLIBS)

UNAME = $(shell uname)

CC      = gcc
HOSTCC  = gcc

ifeq ($(UNAME), Linux)
CFLAGS  = -g -O3 -I. -g -Wall -D_GNU_SOURCE -DTBF -fPIC

DEFINES = -DRESOLVE_HOSTNAMES
LDFLAGS = -L.
LDLIBS  = -lnetlink -ltc -lwireconf -ltimer -lm -ldl
TARGETS = libnetlink.a libtc.a libwireconf.a libtimer.a do_wireconf 
TOBJ=libnetlink.o 
ALLOBJ=$(TOBJ) $(RTMONOBJ)
NLOBJ=iproute.o libnetlink.o ll_map.o utils.o rt_names.o
TCOBJ=libtc.o tc.o tc_qdisc.o tc_util.o tc_core.o iplink.o filter.o htb.o netem.o tbf.o m_action.o u32.o ingress.o
WCOBJ=q_prio.o q_pfifo.o m_action.o m_mirred.o
TESTOBJ=test.o
endif

ifeq ($(UNAME), FreeBSD)
CFLAGS  = -I. -g -Wall -Werror -DMESSAGE_WARNING -DMESSAGE_INFO
LDFLAGS = -L.
LDLIBS  = -lwireconf -ltimer -lm
TARGETS = libwireconf.a libtimer.a do_wireconf
endif

all: $(TARGETS)

ifeq ($(UNAME), Linux)
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
	rm -f $(ALLOBJ) $(TARGETS) $(NLOBJ) *.o
