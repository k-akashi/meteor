
###########################################
# Makefile for qomet: network emulation tool
###########################################

#  $Revision: 140 $
#  $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
#  $LastChangedBy: razvan $


#COMPILE_TYPE=debug
#COMPILE_TYPE=profile
COMPILE_TYPE=release

#SVN_DEFINE = -D'SVN_REVISION="$(shell svnversion -n .)"'

#ifneq ($SVN_DEFINE, aaa)
#	@echo $SVN_DEFINE
#endif

# only need to define EXPAT_PATH for non-standard installations of expat
ifeq ($(OS_NAME),FreeBSD)
EXPAT_PATH=/usr/local
EXPAT_INC=-I${EXPAT_PATH}/include
EXPAT_LIB=-L${EXPAT_PATH}/lib -lexpat
else 
EXPAT_PATH=/usr/lib/x86_64-linux-gnu
EXPAT_INC=-I${EXPAT_PATH}/include
EXPAT_LIB=-L${EXPAT_PATH} -lexpat
endif

# paths for various QOMET components

CHANEL_PATH=./chanel

DELTAQ_PATH=./deltaQ
DELTAQ_INC=-I${DELTAQ_PATH}
DELTAQ_LIB=-L${DELTAQ_PATH} -ldeltaQ -lm

ROUTING_PATH=./routing

WIRECONF_PATH=./wireconf

# different options and flags
INCS=${EXPAT_INC} ${DELTAQ_INC}
LIBS=-lm ${EXPAT_LIB} ${DELTAQ_LIB}

GENERAL_FLAGS=$(SVN_DEFINE) -Wall

# compiler flags
ifeq ($(COMPILE_TYPE), debug)
GCC_FLAGS = $(GENERAL_FLAGS) -g    # generate debugging info
endif

ifeq ($(COMPILE_TYPE), profile)
GCC_FLAGS = $(GENERAL_FLAGS) -pg   # generate profiling info
endif

ifeq ($(COMPILE_TYPE), release)
GCC_FLAGS = $(GENERAL_FLAGS)       # no additional info
endif

# get the name of the operating system
OS_NAME=$(shell uname)

ANY_OS_TARGETS = qomet generate_scenario ${DELTAQ_PATH}/libdeltaQ.a ${CHANEL_PATH}/do_chanel ${CHANEL_PATH}/chanel_config
FREEBSD_TARGETS = ${WIRECONF_PATH}/wireconf ${ROUTING_PATH}/routing
LINUX_TARGETS = ${WIRECONF_PATH}/wireconf

# compile wireconf only of FreeBSD systems
ifeq ($(OS_NAME),FreeBSD)
all : ${ANY_OS_TARGETS} ${FREEBSD_TARGETS}
endif
ifeq ($(OS_NAME),Linux)
all : ${ANY_OS_TARGETS} ${LINUX_TARGETS}
endif

# decide what make command will be used for submodules
ifeq ($(OS_NAME),FreeBSD)
MAKE_CMD = gmake
else
MAKE_CMD = make
endif

# defining revision info
SVN_REVISION := "$(shell svnversion -n .)"

# if revision info cannot be obtained from SVN, try to get it from a file
ifeq ($(SVN_REVISION), "")
#$(warning No SVN revision info, trying to get it from 'svn_revision.txt')
SVN_REVISION := "$(shell cat svn_revision.txt)"
else
#$(warning Writing revision info to file 'svn_revision.txt')
$(shell echo "$(SVN_REVISION)" | cat > svn_revision.txt)
endif
SVN_DEFINE = -D'SVN_REVISION=$(SVN_REVISION)'


# created object qomet.o so that module dependencies
# can be visualized using "nmdepend"

#qomet : qomet.c qomet.h ${DELTAQ_PATH}/libdeltaQ.a
#	gcc $(GCC_FLAGS) qomet.c -o qomet ${INCS} ${LIBS}

#.PHONY: revision

qomet : qomet.o
	gcc $(GCC_FLAGS) qomet.o ./deltaQ/libdeltaQ.a -o qomet ${INCS} ${LIBS}

qomet.o : qomet.c ${DELTAQ_PATH}/libdeltaQ.a
	gcc $(GCC_FLAGS) -c qomet.c ${INCS}

generate_scenario : generate_scenario.c ${DELTAQ_PATH}/libdeltaQ.a
	gcc $(GCC_FLAGS) generate_scenario.c ./deltaQ/libdeltaQ.a -o generate_scenario ${INCS} ${LIBS}

${CHANEL_PATH}/do_chanel : ${CHANEL_PATH}/*.c ${CHANEL_PATH}/*.h
	cd ${CHANEL_PATH}; ${MAKE_CMD} COMPILE_TYPE=$(COMPILE_TYPE)

${CHANEL_PATH}/chanel_config : ${CHANEL_PATH}/*.c ${CHANEL_PATH}/*.h
	cd ${CHANEL_PATH}; ${MAKE_CMD} COMPILE_TYPE=$(COMPILE_TYPE)

${DELTAQ_PATH}/libdeltaQ.a :  ${DELTAQ_PATH}/*.c ${DELTAQ_PATH}/*.h
	cd ${DELTAQ_PATH}; ${MAKE_CMD} COMPILE_TYPE=$(COMPILE_TYPE)

${WIRECONF_PATH}/wireconf : ${WIRECONF_PATH}/*.c ${WIRECONF_PATH}/*.h
	cd ${WIRECONF_PATH}; ${MAKE_CMD}

${ROUTING_PATH}/routing : ${ROUTING_PATH}/*.c ${ROUTING_PATH}/*.h
	cd ${ROUTING_PATH}; ${MAKE_CMD}

clean:
	rm -f ${ANY_OS_TARGETS} ${FREEBSD_TARGETS} *.o core; (cd ${CHANEL_PATH} && ${MAKE_CMD} clean); (cd ${DELTAQ_PATH} && ${MAKE_CMD} clean); (cd ${WIRECONF_PATH} && ${MAKE_CMD} clean); (cd ${ROUTING_PATH} && ${MAKE_CMD} clean)
