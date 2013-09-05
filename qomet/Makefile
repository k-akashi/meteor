CC=gcc

CFLAGS=-g -O2 -Wall

BINDIR=../bin
LIBDIR=../lib
INCDIR=../include

INCS=-I${INCDIR}
LIBS=-L${LIBDIR} 

MESSAGE_FLAGS=-DMESSAGE_WARNING -DMESSAGE_INFO
CFLAGS=-g -O3 -Wall ${MESSAGE_FLAGS}

UNAME = $(shell uname)
ifeq ($(UNAME), Linux)
CFLAGS+=-D_GNU_SOURCE -fPIC -DTCDEBUG
LIBS+=-lnetlink -ltc -ldl
LIB_TARGET=libwireconf.a
BIN_TARGET=do_wireconf
TARGETS = ${LIB_TARGET} ${BIN_TARGET}
TOBJ=libnetlink.o
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

all: qomet

qomet: qomet.c
	${CC} ${CFLAGS} -o ${BINDIR}/$@ $< ${INCS} ${LIBS}
