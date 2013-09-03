###########################################
# Makefile for qomet: network emulation tool
###########################################

OS_NAME=$(shell uname)

ifeq ($(OS_NAME),FreeBSD)
MAKE = gmake
else
MAKE = make
endif

all: deltaQ extras wireconf

deltaQ: 
	${MAKE} -C deltaQ

extras:
	${MAKE} -C extras

wideconf:
	${MAKE} -C wireconf

clean: 
	${MAKE} -C deltaQ clean
	${MAKE} -C extras clean
	${MAKE} -C wireconf clean
