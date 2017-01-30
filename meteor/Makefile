###########################################
# Makefile for qomet library wireconf
###########################################

CC = clang
HOSTCC = clang

BINDIR = ../bin
LIBDIR = ../lib
INCDIR = ../include

INCS = -I${INCDIR} -I/usr/include/libnl3
LIBS = -L${LIBDIR} -ldeltaQ -ltimer -lm -lexpat -lrt -lnl-3 -lnl-route-3 -ljansson -lev

#MESSAGE_FLAGS = -DMESSAGE_WARNING -DMESSAGE_INFO -DTCDEBUG 
MESSAGE_FLAGS = -DTCDEBUG 
CFLAGS = -g -O3 -Wall ${MESSAGE_FLAGS}

CFLAGS += -D_GNU_SOURCE -fPIC 
BIN_TARGET = meteor meteord
TARGETS = ${LIB_TARGET} ${BIN_TARGET}
ALLOBJ=${TOBJ} ${TCOBJ} ${NLOBJ} ${WCOBJ}

.SUFFIXES:  .c .o
.c.o:
	${CC} ${CFLAGS} -c -o $@ $< ${INCS}
.o:
	${CC} -o $@ $< ${INCS}

all: $(TARGETS)

meteord: meteord.o libnlwrap.o json_parse.o
	${CC} ${CFLAGS} -g -o ${BINDIR}/$@ $^ $(LDFLAGS) ${INCS} ${LIBS}

meteor: meteor.o config.o libnlwrap.o
	${CC} ${CFLAGS} -g -o ${BINDIR}/$@ $^ $(LDFLAGS) ${INCS} ${LIBS}

config: config.c
	${CC} -DDEBUG -g -o $@ $< ${INCS}

meteor.o: meteor.c
meteord.o: meteord.c
config.o: config.c
libnlwrap.o: libnlwrap.c

clean:
	rm -f ${ALLOBJ} ${TARGETS} *.o
	cd ${BINDIR}; rm -f ${BIN_TARGET}
