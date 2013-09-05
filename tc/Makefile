CC=gcc
CFLAGS=-g -O3 -Wall

LIBDIR=../lib
INCDIR=../include
INCS=-I${INCDIR}


TARGET=libnetlink.a libtc.a

TOBJ=libnetlink.o
NLOBJ=iproute.o libnetlink.o ll_map.o utils.o rt_names.o
TCOBJ=libtc.o tc.o tc_qdisc.o tc_util.o tc_core.o iplink.o filter.o htb.o netem.o tbf.o m_action.o u32.o ingress.o
WCOBJ=q_prio.o q_pfifo.o m_action.o m_mirred.o

.SUFFIXES:  .c .o
.c.o:
	${CC} ${CFLAGS} -c -o $@ $< ${INCS}
.o:
	${CC} -o $@ $< ${INCS}

all: ${TARGET}

libnetlink.a: ${NLOBJ}
	${AR} rcs ${LIBDIR}/$@ ${NLOBJ} && ranlib ${LIBDIR}/$@

libtc.a: ${TCOBJ}
	${AR} rcs ${LIBDIR}/$@ ${TCOBJ} && ranlib ${LIBDIR}/$@

#ip: $(IPOBJ) $(LIBNETLINK) $(LIBUTIL)

clean:
	rm -f  *.o
	cd ${LIBDIR}; rm -f ${TARGET}
