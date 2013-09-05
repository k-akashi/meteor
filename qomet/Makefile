CC=gcc

CFLAGS=-g -O2 -Wall

BINDIR=../bin
LIBDIR=../lib
INCDIR=../include

LIBS=-L${LIBDIR} -ldeltaQ -lexpat -lm
INCS=-I${INCDIR}

all: qomet

qomet: qomet.c
	${CC} ${CFLAGS} -o ${BINDIR}/$@ $< ${INCS} ${LIBS}
