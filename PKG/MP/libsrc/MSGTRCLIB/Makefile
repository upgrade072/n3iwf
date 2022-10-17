#!/bin/sh

include ../../mk/comm.mk

CFLAG	+= -DTRC_TIMER -D_NEW_KEEPALIVE -fPIC
#CFLAG	= -Wpointer-arith  -mpowerpc64 -maix64 -mcpu=common #-DTRC_TIMER
DEFINES	= -DDEBUG #-DRESTRICTED_CLIENT #-DSOREUSEPORT #-DTCPNODELAY

INC_DIR	= -I. -I../../include
INC_DIR	+= -I../../src/STACK/include

LIB_DIR	= -L../../lib
LIBS	= -lcomm -lm -lpthreads 
CFLAGS	= $(CFLAG) $(DEFINES) $(INC_DIR)

SRCS	= msgtrclib.c  

OBJS	= $(SRCS:.c=.o)

AOUT	= libmsgtrc.a

DEST	= ../../lib

#
#---------------------------------------------------------------------
#
all:: $(AOUT) install

$(AOUT): $(OBJS)
	ar rcus $@ $(OBJS)
#	ar $(ARFLAG) rcus $@ $(OBJS)
#	ar ts $(DEST)/$(AOUT)

install:
	\cp $(AOUT) $(DEST)

clean:
	\rm -f $(OBJS) $(AOUT) core
