
###########################################
# Makefile for qomet_routing library
###########################################

#  $Revision: 128 $
#  $LastChangedDate: 2009-02-06 10:21:50 +0900 (Fri, 06 Feb 2009) $
#  $LastChangedBy: razvan $

# only need these for non-standard installations
EXPAT_PATH=/usr/local
EXPAT_INC=-I${EXPAT_PATH}/include
EXPAT_LIB=-L${EXPAT_PATH}/lib

INCS=${EXPAT_INC}

GCC_FLAGS = -g -Wall -Werror
LDFLAGS	= -L. ${EXPAT_LIB}
LDLIBS	= -lqomet_routing -lexpat

TARGETS	= do_routing libqomet_routing.a routing_tester generate_aodv

all:	  $(TARGETS)

do_routing: do_routing.c libqomet_routing.a
	gcc ${GCC_FLAGS} -o $@ do_routing.c $(LDFLAGS) $(LDLIBS) ${INCS}

libqomet_routing.a : qomet_routing.o routing.o route_xml.o
	ar rc libqomet_routing.a qomet_routing.o routing.o route_xml.o && \
ranlib libqomet_routing.a

routing_tester:	routing_tester.c routing.o message.h
	gcc ${GCC_FLAGS} -o $@ routing_tester.c routing.o

generate_aodv :	generate_aodv.c message.h libqomet_routing.a
	gcc ${GCC_FLAGS} -o $@ generate_aodv.c $(LDFLAGS) $(LDLIBS) ${INCS}

qomet_routing.o : qomet_routing.c qomet_routing.h routing.o message.h
	gcc $(GCC_FLAGS) qomet_routing.c -c

route_xml.o : route_xml.c route_xml.h message.h
	gcc $(GCC_FLAGS) route_xml.c -c ${INCS}

routing.o : routing.c routing.h message.h
	gcc $(GCC_FLAGS) routing.c -c

clean:
	rm -f *.o *.a core $(TARGETS)

