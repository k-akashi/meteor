
##############################################
# Makefile for qomet tools
##############################################

#  $Revision: 190 $
#  $LastChangedDate: 2010-04-05 10:07:00 +0900 (Mon, 05 Apr 2010) $
#  $LastChangedBy: razvan $


COMPILE_TYPE=debug
#COMPILE_TYPE=profile
#COMPILE_TYPE=release

# paths for various QOMET components
DELTAQ_PATH=../deltaQ
DELTAQ_INC=-I${DELTAQ_PATH}
DELTAQ_LIB=-L${DELTAQ_PATH} -ldeltaQ -lm -lexpat

# different options and flags
INCS=${DELTAQ_INC}
LIBS=-lm -lexpat ${DELTAQ_LIB}
GENERAL_FLAGS=$(SVN_DEFINE) -Wall

# compiler flags
ifeq ($(COMPILE_TYPE), debug)
# generate debugging info
GCC_FLAGS = $(GENERAL_FLAGS) -g -O0
endif

ifeq ($(COMPILE_TYPE), profile)
# generate profiling info
GCC_FLAGS = $(GENERAL_FLAGS) -pg
endif

ifeq ($(COMPILE_TYPE), release)
# no additional info
GCC_FLAGS = $(GENERAL_FLAGS)
endif

# get the name of the operating system
OS_NAME=$(shell uname)

ANY_OS_TARGETS = show_bin generate_scenario

# decide what make command will be used for submodules
ifeq ($(OS_NAME),FreeBSD)
all : ${ANY_OS_TARGETS}
MAKE_CMD = gmake
else
all : ${ANY_OS_TARGETS}
MAKE_CMD = make
endif

generate_scenario : generate_scenario.c ${DELTAQ_PATH}/libdeltaQ.a
	gcc $(GCC_FLAGS) generate_scenario.c -o generate_scenario ${INCS} ${LIBS}
show_bin : show_bin.c ${DELTAQ_PATH}/libdeltaQ.a
	gcc $(GCC_FLAGS) show_bin.c -o show_bin ${INCS} ${LIBS}

clean:
	rm -f ${ANY_OS_TARGETS} *.o core
