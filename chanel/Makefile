
#############################################################
#Makefile for chanel: communication channel emulation library
#############################################################

#  $Revision: 61 $
#  $LastChangedDate: 2008-06-09 16:24:16 +0900 (Mon, 09 Jun 2008) $
#  $LastChangedBy: razvan $

COMPILE_TYPE=debug
#COMPILE_TYPE=profile
#COMPILE_TYPE=release

GENERAL_FLAGS= -Wall -Werror

ifeq ($(COMPILE_TYPE), debug)
GCC_FLAGS = $(GENERAL_FLAGS) -g    # generate debugging info
endif
ifeq ($(COMPILE_TYPE), profile)
GCC_FLAGS = $(GENERAL_FLAGS) -pg   # generate profiling info
endif
ifeq ($(COMPILE_TYPE), release)
GCC_FLAGS = $(GENERAL_FLAGS)       # no additional info
endif

TARGETS	= do_chanel chanel_config

# only need to define EXPAT_PATH for non-standard installations of expat
EXPAT_PATH=/usr/local
EXPAT_INC=-I${EXPAT_PATH}/include
EXPAT_LIB=-L${EXPAT_PATH}/lib -lexpat

DELTAQ_PATH=../deltaQ
DELTAQ_INC=-I${DELTAQ_PATH}
DELTAQ_LIB=-L${DELTAQ_PATH} -ldeltaQ

# get the name of the operating system
OS_NAME=$(shell uname)

ifneq ($(OS_NAME),FreeBSD)
RT_LIB = -lrt
endif

INCS=${EXPAT_INC} ${DELTAQ_INC}
LDLIBS=-lpthread ${RT_LIB} -lm ${EXPAT_LIB} ${DELTAQ_LIB}

all:	  $(TARGETS)

do_chanel : do_chanel.c chanel.o
	gcc ${GCC_FLAGS} -o do_chanel do_chanel.c chanel.o ../deltaQ/libdeltaQ.a ${LDFLAGS} ${LDLIBS}

chanel_config : chanel_config.c chanel.o ../deltaQ/
	gcc ${GCC_FLAGS} -o chanel_config chanel_config.c ../deltaQ/libdeltaQ.a chanel.o ${INCS} ${LDFLAGS} ${LDLIBS}

chanel.o : chanel.c chanel.h
	gcc ${GCC_FLAGS} -c chanel.c

clean:
	rm -f $(TARGETS) *.o core
