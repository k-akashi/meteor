###########################################
# Makefile for qomet: network emulation tool
###########################################

OS_NAME=$(shell uname)

ifeq ($(OS_NAME),FreeBSD)
MAKE = gmake
else
MAKE = make
endif

DELTAQ_PATH=./deltaQ
EXTRAS_PATH=./extras
#STATION_PATH=./station
TIMER_PATH=./timer
TC_PATH=./tc
WIRECONF_PATH=./meteor

all :
	make -C ${DELTAQ_PATH} all && make -C ${EXTRAS_PATH} all && make -C ${TIMER_PATH} all && make -C ${TC_PATH} all && make -C ${WIRECONF_PATH} all

clean:
	make -C ${DELTAQ_PATH} clean && make -C ${EXTRAS_PATH} clean && make -C ${TIMER_PATH} clean && make -C ${TC_PATH} clean && make -C ${WIRECONF_PATH} clean
