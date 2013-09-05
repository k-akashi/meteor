###########################################
# Makefile for qomet library wireconf
###########################################

CC=gcc
HOSTCC=gcc

BINDIR=../bin
LIBDIR=../lib
INCDIR=../include

INCS=-I${INCDIR}
LIBS=-L${LIBDIR} -ldeltaQ -lwireconf -ltimer -lm -lexpat

MESSAGE_FLAGS=-DMESSAGE_WARNING -DMESSAGE_INFO
CFLAGS=-g -O3 -Wall ${MESSAGE_FLAGS}

UNAME = $(shell uname)
ifeq ($(UNAME), Linux)
CFLAGS+=-D_GNU_SOURCE -fPIC -DTCDEBUG 
LIBS+=-lnetlink -ltc -ldl
LIB_TARGET=libnetlink.a libtc.a libwireconf.a 
BIN_TARGET=do_wireconf 
TARGETS = ${LIB_TARGET} ${BIN_TARGET}
TOBJ=libnetlink.o 
NLOBJ=iproute.o libnetlink.o ll_map.o utils.o rt_names.o
TCOBJ=libtc.o tc.o tc_qdisc.o tc_util.o tc_core.o iplink.o filter.o htb.o netem.o tbf.o m_action.o u32.o ingress.o
WCOBJ=q_prio.o q_pfifo.o m_action.o m_mirred.o
ALLOBJ=${TOBJ} ${TCOBJ} ${NLOBJ} ${WCOBJ}
else ifeq ($(UNAME), FreeBSD)
LDFLAGS = -L.
TARGETS = libwireconf.a do_wireconf
endif

.SUFFIXES:  .c .o
.c.o:
	${CC} ${CFLAGS} -c -o $@ $< ${INCS}
.o:
	${CC} -o $@ $< ${INCS}

all: $(TARGETS)

ifeq ($(UNAME), Linux)
libnetlink.a: ${NLOBJ}
	${AR} rcs ${LIBDIR}/$@ ${NLOBJ} && ranlib ${LIBDIR}/$@

libtc.a: ${TCOBJ}
	${AR} rcs ${LIBDIR}/$@ ${TCOBJ} && ranlib ${LIBDIR}/$@

#ip: $(IPOBJ) $(LIBNETLINK) $(LIBUTIL)
endif

libwireconf.a: wireconf.c wireconf.o statistics.o
	ar rcs ${LIBDIR}/$@ wireconf.o statistics.o ${TCOBJ} ${NLOBJ} && ranlib ${LIBDIR}/$@

statistics.o: statistics.c

ifeq ($(UNAME), Linux)
do_wireconf: do_wireconf.o routing_info.o ${WCOBJ} ${TCOBJ} ${NLOBJ}
	${CC} ${CFLAGS} -g -export-dynamic -o ${BINDIR}/$@ do_wireconf.o routing_info.o ${WCOBJ} $(LDFLAGS) ${INCS} ${LIBS}
endif
ifeq ($(UNAME), FreeBSD)
do_wireconf: do_wireconf.c routing_info.o 
	${CC} ${CFLAGS} -o $@ do_wireconf.c routing_info.o ${INCS} ${LIBS}
endif

do_wireconf.o: do_wireconf.c
routing_info.o: routing_info.c

ifeq ($(UNAME), Linux)
test: $(TCOBJ) $(LIBNETLINK) $(TESTOBJ)
	gcc -export-dynamic -o $@ test.o $(LDFLAGS) $(LDLIBS)
endif

clean:
	rm -f ${ALLOBJ} ${TARGETS} *.o
	cd ${LIBDIR}; rm -f ${LIB_TARGET}
	cd ${BINDIR}; rm -f ${BIN_TARGET}
