
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
GENERAL_FLAGS = -Wall -Werror ${MESSAGE_FLAGS}

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

LIBS = -lrt -lm -L. -ltimer

TARGETS = libtimer.a test_timer

all: ${TARGETS}

libtimer.a: timer.o
	ar rc libtimer.a timer.o && ranlib libtimer.a	

timer.o: timer.c timer.h timer_global.h timer_message.h
	$(CC) ${GCC_FLAGS} -c timer.c

test_timer: test_timer.o libtimer.a
	$(CC) ${GCC_FLAGS} -o test_timer test_timer.o ${LIBS}

test_timer.o: test_timer.c timer.h timer_global.h timer_message.h
	$(CC) ${GCC_FLAGS} -c test_timer.c

clean:
	rm -f ${TARGETS} *.o *.a core

