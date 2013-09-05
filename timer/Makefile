
###########################################
# Makefile for timer library
###########################################

#  $Revision: 128 $
#  $LastChangedDate: 2009-02-06 10:21:50 +0900 (Fri, 06 Feb 2009) $
#  $LastChangedBy: razvan $

ifndef ${COMPILE_TYPE}
COMPILE_TYPE=debug
#COMPILE_TYPE=profile
#COMPILE_TYPE=release
endif

#MESSAGE_FLAGS = -DMESSAGE_WARNING -DMESSAGE_DEBUG -DMESSAGE_INFO
MESSAGE_FLAGS = -DMESSAGE_WARNING -DMESSAGE_INFO

ifeq (${COMPILE_TYPE}, debug)
GCC_FLAGS= -g
else ifeq (${COMPILE_TYPE}, profile)
GCC_FLAGS=-pg 
else ifeq (${COMPILE_TYPE}, release)
endif
CFLAGS+=-Wall ${MESSAGE_FLAGS}

BINDIR=../bin
LIBDIR=../lib
INCDIR=../include

INCS=-I${INCDIR}
LIBS=-L${LIBDIR} -ltimer -lrt -lm

TARGETS = libtimer.a test_timer

all: ${TARGETS}

libtimer.a: timer.o
	${AR} rc ${LIBDIR}/$@ $< && ranlib ${LIBDIR}/$@

timer.o: timer.c
	${CC} ${CFLAGS} -c $< ${INCS}

test_timer: test_timer.o 
	${CC} ${CFLAGS} -o $@ $< ${INCS} ${LIBS}

test_timer.o: test_timer.c 
	${CC} ${CFLAGS} -c $< ${INCS}

clean:
	rm -f ${TARGETS} *.o *.a core 
	cd ${LIBDIR}; rm -f libtimer.a
