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

MESSAGE_FLAGS = -DMESSAGE_WARNING
#MESSAGE_FLAGS = -DMESSAGE_WARNING -DMESSAGE_INFO
#MESSAGE_FLAGS = -DMESSAGE_WARNING -DMESSAGE_DEBUG -DMESSAGE_INFO
GENERAL_FLAGS = -Wall ${MESSAGE_FLAGS}

# only need these for non-standard installations
EXPAT_PATH=/usr/local
EXPAT_INC=-I${EXPAT_PATH}/include
EXPAT_LIB=-L${EXPAT_PATH}/lib -lexpat
DELTAQ_PATH=.
DELTAQ_INC=-I${DELTAQ_PATH}
DELTAQ_LIB=-L. -ldeltaQ -lm -lexpat

INCS=${EXPAT_INC} -I$(STAGING_DIR)/usr/include
# order of libraries is importan, deltaQ must be first because it uses
# the other ones!
LIBS=${DELTAQ_LIB} -lm -lexpat ${EXPAT_LIB} -L$(STAGING_DIR)/usr/lib

# Uncomment the line below if compiling using Windows Visual C
#WIN_FLAGS = -D__WINDOWS__

# Remark: the flag "-std=c99" can be added below to make the function
# 'log2' available on some platforms (Linux); others don't need it (Mac);
# on Solaris 'log2' is not defined, so we defined it ourselves

ifeq ($(COMPILE_TYPE), debug)
GCC_FLAGS = -g ${GENERAL_FLAGS}     # generate debugging info
endif
ifeq ($(COMPILE_TYPE), profile)
GCC_FLAGS = -pg ${GENERAL_FLAGS}    # generate profiling info
endif
ifeq ($(COMPILE_TYPE), release)
GCC_FLAGS = ${GENERAL_FLAGS}        # no additional info
endif

DELTA_Q_OBJECTS = active_tag.o connection.o coordinate.o environment.o \
	ethernet.o fixed_deltaQ.o generic.o geometry.o io.o interface.o \
	motion.o node.o object.o scenario.o stack.o wimax.o wlan.o \
	xml_jpgis.o xml_scenario.o zigbee.o
OBJECTS = deltaQ.o ${DELTA_Q_OBJECTS}

# defining revision info
###############################################################################
# $(LDFLAGS) and $(CFLAGS) are defined externally by the OpenWrt build system
###############################################################################

all: libdeltaQ.a deltaQ all_test
all_test: test_wimax

libdeltaQ.a : ${OBJECTS}
	ar rc libdeltaQ.a ${OBJECTS} ${STATIC_EXPAT} && ranlib libdeltaQ.a
	cp libdeltaQ.a ../libs

deltaQ : deltaQ.o libdeltaQ.a
	$(CC) $(LDFLAGS) $(GCC_FLAGS) deltaQ.o -o deltaQ ${INCS} ${LIBS}

deltaQ.o : deltaQ.c deltaQ.h message.h global.h ${DELTA_Q_OBJECTS}
	$(CC) $(CFLAGS) $(SVN_DEFINE) $(GCC_FLAGS) deltaQ.c -c ${INCS} ${LIBS}

active_tag.o : active_tag.c active_tag.h deltaQ.h message.h generic.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) active_tag.c -c ${INCS} ${LIBS}

connection.o : connection.c connection.h deltaQ.h message.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) connection.c -c ${INCS} ${LIBS}

coordinate.o : coordinate.c coordinate.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) coordinate.c -c ${INCS} ${LIBS}

environment.o : environment.c environment.h deltaQ.h message.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) environment.c -c ${INCS} ${LIBS}

ethernet.o : ethernet.c ethernet.h deltaQ.h message.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) ethernet.c -c ${INCS} ${LIBS}

fixed_deltaQ.o : fixed_deltaQ.c fixed_deltaQ.h deltaQ.h message.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) fixed_deltaQ.c -c ${INCS} ${LIBS}

generic.o : generic.c generic.h message.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) generic.c -c ${INCS} ${LIBS}

geometry.o : geometry.c geometry.h message.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) geometry.c -c ${INCS} ${LIBS}

io.o    : io.c io.h message.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) io.c -c ${INCS} ${LIBS}

interface.o : interface.c interface.h message.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) interface.c -c ${INCS} ${LIBS}

motion.o : motion.c motion.h deltaQ.h message.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) motion.c -c ${INCS} ${LIBS}

node.o : node.c node.h deltaQ.h message.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) node.c -c ${INCS} ${LIBS}

object.o : object.c object.h deltaQ.h message.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) object.c -c ${INCS} ${LIBS}

scenario.o : scenario.c scenario.h deltaQ.h message.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) scenario.c -c ${INCS} ${LIBS}

stack.o : stack.c stack.h message.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) stack.c -c ${INCS} ${LIBS}

wimax.o : wimax.c wimax.h deltaQ.h message.h generic.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) wimax.c -c ${INCS} ${LIBS}

wlan.o : wlan.c wlan.h deltaQ.h message.h generic.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) wlan.c -c ${INCS} ${LIBS}

xml_jpgis.o : xml_jpgis.c xml_jpgis.h message.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) xml_jpgis.c -c ${INCS} ${LIBS}

xml_scenario.o : xml_scenario.c xml_scenario.h deltaQ.h message.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) xml_scenario.c -c ${INCS} ${LIBS}

zigbee.o : zigbee.c zigbee.h deltaQ.h message.h generic.h global.h
	$(CC) $(CFLAGS) $(GCC_FLAGS) zigbee.c -c ${INCS} ${LIBS}

# test files
TEST_FLAGS = -DMESSAGE_WARNING -DMESSAGE_DEBUG -DMESSAGE_INFO
test_wimax : test_wimax.c wimax.o deltaQ.h message.h generic.h global.h
	$(CC) $(LDLAGS) $(GCC_FLAGS) $(TEST_FLAGS) test_wimax.c -o test_wimax ${INCS} ${LIBS}

#deltaQ : deltaQ.o libdeltaQ.a
#	$(CC) $(LDFLAGS) $(GCC_FLAGS) deltaQ.o -o deltaQ ${INCS} ${LIBS}

clean:
	rm -f libdeltaQ.a deltaQ test_wimax *.o core
