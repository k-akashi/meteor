##############################################
# Makefile for qomet tools
##############################################

COMPILE_TYPE=debug
#COMPILE_TYPE=profile
#COMPILE_TYPE=release


BINDIR=../bin
LIBDIR=../lib
INCDIR=../include

LIBS=-L${LIBDIR} -ldeltaQ -lm -lexpat
INCS=-I${INCDIR}
CFLAGS=-Wall

ifeq ($(COMPILE_TYPE), debug)
GCC_FLAGS = $(GENERAL_FLAGS) -g -O0
else ifeq ($(COMPILE_TYPE), profile)
GCC_FLAGS = $(GENERAL_FLAGS) -pg
else ifeq ($(COMPILE_TYPE), release)
# no additional info
GCC_FLAGS = $(GENERAL_FLAGS)
endif

ANY_OS_TARGETS = show_bin generate_scenario

OS_NAME=$(shell uname)
ifeq ($(OS_NAME),FreeBSD)
MAKE_CMD = gmake
else
MAKE_CMD = make
endif

all : ${ANY_OS_TARGETS}

generate_scenario : generate_scenario.c ${LIBDIR}/libdeltaQ.a
	gcc ${CDLAGS} generate_scenario.c -o ${BINDIR}/generate_scenario ${INCS} ${LIBS}

show_bin : show_bin.c ${LIBDIR}/libdeltaQ.a
	gcc ${CFLAGS} show_bin.c -o ${BINDIR}/show_bin ${INCS} ${LIBS}

clean:
	rm -f  *.o core
	cd ${BINDIR}; rm -f ${ANY_OS_TARGETS}
