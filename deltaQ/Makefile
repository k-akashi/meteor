#
# Copyright (c) 2006-2013 The StarBED Project  All rights reserved.
#
# See the file 'LICENSE' for licensing information.
#

###################################################################
#
# Makefile for deltaQ computation library
#
# $Id: Makefile 146 2013-06-20 00:50:48Z razvan $
#
###################################################################

ifndef $(COMPILE_TYPE)
COMPILE_TYPE=debug
#COMPILE_TYPE=profile
#COMPILE_TYPE=release
endif


#MESSAGE_FLAGS = -DMESSAGE_WARNING
#MESSAGE_FLAGS = -DMESSAGE_WARNING -DMESSAGE_INFO
MESSAGE_FLAGS = -DMESSAGE_WARNING -DMESSAGE_DEBUG -DMESSAGE_INFO

# only need these for non-standard installations
BINDIR=../bin
LIBDIR=../lib
INCDIR=../include
INCS=-I${INCDIR}
LIBS=-L${LIBDIR} -ldeltaQ -lm -lexpat

ifeq ($(COMPILE_TYPE), debug)
PROFILE=-g -Wall
endif
ifeq ($(COMPILE_TYPE), profile)
PROFILE=-pg -Wall
endif
ifeq ($(COMPILE_TYPE), release)
PROFILE=-Wall
endif

CFLAGS = ${PROFILE} ${MESSAGE_FLAGS}

DELTA_Q_OBJECTS = active_tag.o connection.o coordinate.o environment.o \
	ethernet.o fixed_deltaQ.o generic.o geometry.o io.o interface.o \
	motion.o node.o object.o scenario.o stack.o wimax.o wlan.o \
	xml_jpgis.o xml_scenario.o zigbee.o
OBJECTS = deltaQ.o ${DELTA_Q_OBJECTS}

all: libdeltaQ.a deltaQ all_test
all_test: test_wimax

libdeltaQ.a : ${OBJECTS}
	ar rc ${LIBDIR}/libdeltaQ.a ${OBJECTS} ${STATIC_EXPAT} && ranlib ${LIBDIR}/libdeltaQ.a

deltaQ : deltaQ.o ${LIBDIR}/libdeltaQ.a
	$(CC) $(LDFLAGS) $(GCC_FLAGS) deltaQ.o -o ${BINDIR}/deltaQ ${INCS} ${LIBS}

deltaQ.o : deltaQ.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) deltaQ.c -c ${INCS} ${LIBS}

active_tag.o : active_tag.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) active_tag.c -c ${INCS} ${LIBS}

connection.o : connection.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) connection.c -c ${INCS} ${LIBS}

coordinate.o : coordinate.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) coordinate.c -c ${INCS} ${LIBS}

environment.o : environment.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) environment.c -c ${INCS} ${LIBS}

ethernet.o : ethernet.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) ethernet.c -c ${INCS} ${LIBS}

fixed_deltaQ.o : fixed_deltaQ.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) fixed_deltaQ.c -c ${INCS} ${LIBS}

generic.o : generic.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) generic.c -c ${INCS} ${LIBS}

geometry.o : geometry.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) geometry.c -c ${INCS} ${LIBS}

io.o    : io.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) io.c -c ${INCS} ${LIBS}

interface.o : interface.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) interface.c -c ${INCS} ${LIBS}

motion.o : motion.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) motion.c -c ${INCS} ${LIBS}

node.o : node.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) node.c -c ${INCS} ${LIBS}

object.o : object.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) object.c -c ${INCS} ${LIBS}

scenario.o : scenario.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) scenario.c -c ${INCS} ${LIBS}

stack.o : stack.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) stack.c -c ${INCS} ${LIBS}

wimax.o : wimax.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) wimax.c -c ${INCS} ${LIBS}

wlan.o : wlan.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) wlan.c -c ${INCS} ${LIBS}

xml_jpgis.o : xml_jpgis.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) xml_jpgis.c -c ${INCS} ${LIBS}

xml_scenario.o : xml_scenario.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) xml_scenario.c -c ${INCS} ${LIBS}

zigbee.o : zigbee.c 
	$(CC) $(CFLAGS) $(GCC_FLAGS) zigbee.c -c ${INCS} ${LIBS}

# test files
TEST_FLAGS = -DMESSAGE_WARNING -DMESSAGE_DEBUG -DMESSAGE_INFO
test_wimax : test_wimax.c wimax.o 
	$(CC) $(LDLAGS) $(GCC_FLAGS) $(TEST_FLAGS) test_wimax.c -o test_wimax ${INCS} ${LIBS}

#deltaQ : deltaQ.o libdeltaQ.a
#	$(CC) $(LDFLAGS) $(GCC_FLAGS) deltaQ.o -o deltaQ ${INCS} ${LIBS}

clean:
	rm -f ${LIBDIR}/libdeltaQ.a ${BINDIR}/deltaQ test_wimax *.o core
