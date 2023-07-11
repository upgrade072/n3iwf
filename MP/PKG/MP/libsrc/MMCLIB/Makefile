#!/bin/sh

include ../../mk/comm.mk

CFLAG	+= -DTRC_TIMER -D_NEW_KEEPALIVE 
DEFINES	= -DDEBUG 

INC_DIR	= -I. -I../../include
LIB_DIR	= -L../../lib
LIBS	= -lm 
CFLAGS	= $(CFLAG) $(DEFINES) $(INC_DIR) $(LIBS)

SRCS	=	mmclib.c  \

OBJS	= $(SRCS:.c=.o)

AOUT	= libmmc.a

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
