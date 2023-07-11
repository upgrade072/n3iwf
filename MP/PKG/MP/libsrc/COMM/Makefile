#!/bin/sh

include ../../mk/comm.mk

CFLAG	+= -DTRC_TIMER #-D_NEW_KEEPALIVE
DEFINES	= -DDEBUG #-DRESTRICTED_CLIENT #-DSOREUSEPORT #-DTCPNODELAY

INC_DIR	= -I. -I../../include
LIB_DIR	= -L../../lib
LIBS	= -lm -lpthreads 
CFLAGS	= $(CFLAG) $(DEFINES) $(INC_DIR) -fPIC

SRCS	 = comm_shm.c
SRCS	+= comm_util.c 
SRCS	+= trclib.c	
SRCS	+= comm_msgq.c 
SRCS	+= conflib.c
SRCS	+= keepalivelib.c
SRCS	+= loglib.c
SRCS	+= socklib.c  
SRCS	+= ixpc_qmsg.c 
SRCS	+= commlib.c 
SRCS	+= overloadlib.c 
SRCS	+= sys.c 
SRCS    += crc64.c


OBJS	= $(SRCS:.c=.o)

AOUT	= libcomm.a

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
