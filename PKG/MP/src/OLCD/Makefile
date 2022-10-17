
TOP_DIR     = ../..
TOP_BIN     = ${TOP_DIR}/bin
TOP_INC     = ${TOP_DIR}/include
TOP_LIB     = ${TOP_DIR}/lib
LIB_DIR     = $(TOP_DIR)/libsrc


CFLAG = -g -Wall 

INC_PATH 	= -I. -I$(TOP_INC)		\
			  -I$(LIB_DIR)/COMM


LIB_PATH = -L../../lib

LIBS		= -lcomm -lm

SRCS		=	olcd_main.c olcd_cfg.c 

OBJS		= $(SRCS:.c=.o)

AOUT		= olcd

DEST		= ../../bin

#
#---------------------------------------------------------------------
#
all:: clean $(AOUT) install

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAG) $(INC_PATH) -c $<


$(AOUT): $(OBJS) 
	$(CC) $(CFLAG) -o $(AOUT) $(OBJS) $(LIB_PATH) $(LIBS)

install:
	\cp $(AOUT) $(DEST)
	\cp $(AOUT) ../../../../BIN

clean:
	\rm -f $(OBJS) $(AOUT) core

