###########################################
# Makefile for station library
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
GENERAL_FLAGS = -Wall ${MESSAGE_FLAGS}

# release-specific compile flags
ifeq (${COMPILE_TYPE}, debug)
# generate debugging info
GCC_FLAGS = -g ${GENERAL_FLAGS}
endif
ifeq (${COMPILE_TYPE}, profile)
# generate profiling info
GCC_FLAGS = -pg ${GENERAL_FLAGS}
endif
ifeq (${COMPILE_TYPE}, release)
# no additional info
GCC_FLAGS = ${GENERAL_FLAGS}
endif

LIBS = -lrt -lm -lpthread -L. -lstation -L../timer -ltimer

TARGETS = libstation.a test_station stationd

all: ${TARGETS}

libstation.a: station.o
	ar rc libstation.a station.o && ranlib libstation.a	

#sta_ap.o: sta_ap.c sta_ap.h station.h station_global.h station_message.h
#	gcc ${GCC_FLAGS} -c sta_ap.c

station.o: station.c station.h station_global.h station_message.h
	gcc ${GCC_FLAGS} -c station.c

test_station: test_station.o libstation.a
	gcc ${GCC_FLAGS} -o test_station test_station.o ${LIBS}

test_station.o: test_station.c station.h station_global.h station_message.h
	gcc ${GCC_FLAGS} -c test_station.c

stationd: stationd.o libstation.a
	gcc ${GCC_FLAGS} -o stationd stationd.o ${LIBS}

stationd.o: stationd.c station.h station_global.h station_message.h
	gcc ${GCC_FLAGS} -c stationd.c

clean:
	rm -f ${TARGETS} *.o *.a core

