###########################################
# Makefile for qomet library wireconf
###########################################

CC = clang
HOSTCC = clang

BINDIR = ../bin
LIBDIR = ../lib
INCDIR = ../include

INCS = -I${INCDIR}
LIBS = -L${LIBDIR} -ldeltaQ -lwireconf -ltimer -lm -lexpat -lrt

MESSAGE_FLAGS = -DMESSAGE_WARNING -DMESSAGE_INFO -DTCDEBUG 
CFLAGS = -g -O3 -Wall ${MESSAGE_FLAGS}

UNAME = $(shell uname)
ifeq ($(UNAME), Linux)
CFLAGS += -D_GNU_SOURCE -fPIC 
LIBS += -ltc -lnetlink -ldl
LIB_TARGET = libwireconf.a 
BIN_TARGET = meteor 
TARGETS = ${LIB_TARGET} ${BIN_TARGET}
TOBJ = libnetlink.o 
ALLOBJ=${TOBJ} ${TCOBJ} ${NLOBJ} ${WCOBJ}
else ifeq ($(UNAME), FreeBSD)
LDFLAGS = -L.
TARGETS = libwireconf.a meteor
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
meteor: meteor.o routing_info.o ${WCOBJ} ${TCOBJ} ${NLOBJ}
	${CC} ${CFLAGS} -g -export-dynamic -o ${BINDIR}/$@ meteor.o routing_info.o ${WCOBJ} $(LDFLAGS) ${INCS} ${LIBS}
endif
ifeq ($(UNAME), FreeBSD)
meteor: meteor.c routing_info.o 
	${CC} ${CFLAGS} -o $@ meteor.c routing_info.o ${INCS} ${LIBS}
endif

meteor.o: meteor.c
routing_info.o: routing_info.c

ifeq ($(UNAME), Linux)
test: $(TCOBJ) $(LIBNETLINK) $(TESTOBJ)
	gcc -export-dynamic -o $@ test.o $(LDFLAGS) $(LDLIBS)
endif

clean:
	rm -f ${ALLOBJ} ${TARGETS} *.o
	cd ${LIBDIR}; rm -f ${LIB_TARGET}
	cd ${BINDIR}; rm -f ${BIN_TARGET}
