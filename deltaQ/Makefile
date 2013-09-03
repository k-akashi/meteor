
###########################################
# Makefile for deltaQ computation library
###########################################

#  $Revision: 114 $
#  $LastChangedDate: 2008-10-21 14:12:35 +0900 (Tue, 21 Oct 2008) $
#  $LastChangedBy: razvan $

ifndef $(COMPILE_TYPE)
#COMPILE_TYPE=debug
COMPILE_TYPE=profile
#COMPILE_TYPE=release
endif

# only need these for non-standard installations
EXPAT_PATH=/usr/local
EXPAT_INC=-I${EXPAT_PATH}/include

INCS=${EXPAT_INC}

# Uncomment the line below if compiling using Windows Visual C
#WIN_FLAGS = -D__WINDOWS__

# Remark: the flag "-std=c99" can be added below to make the function
# 'log2' available on some platforms (Linux); others don't need it (Mac);
# on Solaris 'log2' is not defined, so we defined it ourselves

ifeq ($(COMPILE_TYPE), debug)
GCC_FLAGS = -g -Wall ${WIN_FLAGS}      # generate debugging info
endif
ifeq ($(COMPILE_TYPE), profile)
GCC_FLAGS = -pg -Wall ${WIN_FLAGS}     # generate profiling info
endif
ifeq ($(COMPILE_TYPE), release)
GCC_FLAGS = -Wall ${WIN_FLAGS}    # no additional info
endif

DELTA_Q_OBJECTS = node.o object.o coordinate.o environment.o motion.o connection.o scenario.o xml_scenario.o wlan.o ethernet.o active_tag.o zigbee.o generic.o stack.o io.o xml_jpgis.o
OBJECTS = deltaQ.o ${DELTA_Q_OBJECTS}

all: libdeltaQ.a

libdeltaQ.a : ${OBJECTS}
	ar rc libdeltaQ.a ${OBJECTS} && ranlib libdeltaQ.a

deltaQ.o : deltaQ.c deltaQ.h message.h global.h ${DELTA_Q_OBJECTS}
	gcc $(GCC_FLAGS) deltaQ.c -c ${INCS}

node.o : node.c node.h deltaQ.h message.h global.h
	gcc $(GCC_FLAGS) node.c -c ${INCS}

object.o : object.c object.h deltaQ.h message.h global.h
	gcc $(GCC_FLAGS) object.c -c ${INCS}

coordinate.o : coordinate.c coordinate.h global.h
	gcc $(GCC_FLAGS) coordinate.c -c ${INCS}

environment.o : environment.c environment.h deltaQ.h message.h global.h
	gcc $(GCC_FLAGS) environment.c -c ${INCS}

motion.o : motion.c motion.h deltaQ.h message.h global.h
	gcc $(GCC_FLAGS) motion.c -c ${INCS}

connection.o : connection.c connection.h deltaQ.h message.h global.h
	gcc $(GCC_FLAGS) connection.c -c ${INCS}

scenario.o : scenario.c scenario.h deltaQ.h message.h global.h
	gcc $(GCC_FLAGS) scenario.c -c ${INCS}

xml_scenario.o : xml_scenario.c xml_scenario.h deltaQ.h message.h global.h
	gcc $(GCC_FLAGS) xml_scenario.c -c ${INCS}

wlan.o : wlan.c wlan.h deltaQ.h message.h generic.h global.h
	gcc $(GCC_FLAGS) wlan.c -c ${INCS}

ethernet.o : ethernet.c ethernet.h deltaQ.h message.h global.h
	gcc $(GCC_FLAGS) ethernet.c -c ${INCS}

active_tag.o : active_tag.c active_tag.h deltaQ.h message.h generic.h global.h
	gcc $(GCC_FLAGS) active_tag.c -c ${INCS}

zigbee.o : zigbee.c zigbee.h deltaQ.h message.h generic.h global.h
	gcc $(GCC_FLAGS) zigbee.c -c ${INCS}

generic.o : generic.c generic.h message.h global.h
	gcc $(GCC_FLAGS) generic.c -c ${INCS}

stack.o : stack.c stack.h message.h global.h
	gcc $(GCC_FLAGS) stack.c -c ${INCS}

io.o    : io.c io.h message.h global.h
	gcc $(GCC_FLAGS) io.c -c ${INCS}

xml_jpgis.o : xml_jpgis.c xml_jpgis.h message.h global.h
	gcc $(GCC_FLAGS) xml_jpgis.c -c ${INCS}

clean:
	rm -f *.o *.a core
