###########################################
# Makefile for qomet library wireconf
###########################################

CC = clang
HOSTCC = clang

BINDIR = ../bin
LIBDIR = ../lib
INCDIR = ../include

INCS = -I${INCDIR} -I/usr/include/libnl3
LIBS = -L${LIBDIR} -ldeltaQ -ltimer -lm -lexpat -lrt -ljson-c -lnl-3 -lnl-route-3

#MESSAGE_FLAGS = -DMESSAGE_WARNING -DMESSAGE_INFO -DTCDEBUG 
CFLAGS = -g -O3 -Wall ${MESSAGE_FLAGS}

CFLAGS += -D_GNU_SOURCE -fPIC 
BIN_TARGET = meteor 
TARGETS = ${LIB_TARGET} ${BIN_TARGET}
ALLOBJ=${TOBJ} ${TCOBJ} ${NLOBJ} ${WCOBJ}

.SUFFIXES:  .c .o
.c.o:
	${CC} ${CFLAGS} -c -o $@ $< ${INCS}
.o:
	${CC} -o $@ $< ${INCS}

all: $(TARGETS)

meteor: meteor.o config.o libnlwrap.o wireconf.o config.o
	${CC} ${CFLAGS} -g -o ${BINDIR}/$@ $^ $(LDFLAGS) ${INCS} ${LIBS}

config: config.c
	${CC} -DDEBUG -g -o $@ $< ${INCS} -ljson-c

meteor.o: meteor.c
config.o: config.c
libnlwrap.o: libnlwrap.c

clean:
	rm -f ${ALLOBJ} ${TARGETS} *.o
	cd ${BINDIR}; rm -f ${BIN_TARGET}
