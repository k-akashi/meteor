
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

CC=gcc

ifndef $(COMPILE_TYPE)
COMPILE_TYPE=debug
#COMPILE_TYPE=profile
#COMPILE_TYPE=release
endif

MESSAGE_FLAGS = -DMESSAGE_WARNING
#MESSAGE_FLAGS = -DMESSAGE_WARNING -DMESSAGE_INFO
#MESSAGE_FLAGS = -DMESSAGE_WARNING -DMESSAGE_DEBUG -DMESSAGE_INFO
GENERAL_FLAGS = -Werror ${MESSAGE_FLAGS}

INCS=-I../include
# order of libraries is importan, deltaQ must be first because it uses
# the other ones!
LIBS=-L../lib -ldeltaQ -lm -lexpat

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
SVN_REVISION := "$(shell svnversion -n .)"
SVN_FILE := svn_revision.txt

# if SVN tools are not installed, try to get it from a file
ifeq ($(SVN_REVISION), "")
#$(warning No SVN revision info, trying to get it from file '$(SVN_FILE)')
SVN_REVISION := "$(shell cat $(SVN_FILE))"

# if revision info cannot be obtained from SVN, try to get it from a file
else 
ifeq ($(SVN_REVISION), "exported")
#$(warning No SVN revision info, trying to get it from file '$(SVN_FILE)')
SVN_REVISION := "$(shell cat $(SVN_FILE))"

# otherwise assume correct information was obtained
else
#$(warning Writing revision info to file '$(SVN_FILE)')
$(shell echo "$(SVN_REVISION)" | cat > $(SVN_FILE))
endif
endif

# create definition containing the revision information
SVN_DEFINE = -D'SVN_REVISION=$(SVN_REVISION)'

###############################################################################
# $(LDFLAGS) and $(CFLAGS) are defined externally by the OpenWrt build system
###############################################################################

all: libdeltaQ.a deltaQ all_test
all_test: test_wimax

libdeltaQ.a: ${OBJECTS}
	ar rc ../lib/libdeltaQ.a ${OBJECTS} ${STATIC_EXPAT} && ranlib ../lib/libdeltaQ.a

deltaQ: deltaQ.o
	$(CC) $(LDFLAGS) $(GCC_FLAGS) deltaQ.o -o deltaQ ${INCS} ${LIBS}

deltaQ.o: deltaQ.c
	$(CC) $(CFLAGS) $(SVN_DEFINE) $(GCC_FLAGS) deltaQ.c -c ${INCS}

active_tag.o: active_tag.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) active_tag.c -c ${INCS}

connection.o: connection.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) connection.c -c ${INCS}

coordinate.o: coordinate.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) coordinate.c -c ${INCS}

environment.o: environment.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) environment.c -c ${INCS}

ethernet.o: ethernet.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) ethernet.c -c ${INCS}

fixed_deltaQ.o: fixed_deltaQ.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) fixed_deltaQ.c -c ${INCS}

generic.o: generic.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) generic.c -c ${INCS}

geometry.o: geometry.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) geometry.c -c ${INCS}

io.o: io.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) io.c -c ${INCS}

interface.o: interface.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) interface.c -c ${INCS}

motion.o: motion.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) motion.c -c ${INCS}

node.o: node.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) node.c -c ${INCS}

object.o: object.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) object.c -c ${INCS}

scenario.o: scenario.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) scenario.c -c ${INCS}

stack.o: stack.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) stack.c -c ${INCS}

wimax.o: wimax.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) wimax.c -c ${INCS}

wlan.o: wlan.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) wlan.c -c ${INCS}

xml_jpgis.o: xml_jpgis.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) xml_jpgis.c -c ${INCS}

xml_scenario.o: xml_scenario.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) xml_scenario.c -c ${INCS}

zigbee.o: zigbee.c
	$(CC) $(CFLAGS) $(GCC_FLAGS) zigbee.c -c ${INCS}

# test files
TEST_FLAGS = -DMESSAGE_WARNING -DMESSAGE_DEBUG -DMESSAGE_INFO
test_wimax: test_wimax.c
	$(CC) $(LDLAGS) $(GCC_FLAGS) $(TEST_FLAGS) test_wimax.c -o test_wimax ${INCS} ${LIBS}

#deltaQ : deltaQ.o libdeltaQ.a
#	$(CC) $(LDFLAGS) $(GCC_FLAGS) deltaQ.o -o deltaQ ${INCS} ${LIBS}

clean:
	rm -f ../lib/libdeltaQ.a deltaQ test_wimax *.o core
