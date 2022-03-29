include ../MakeVars

BUILD_INC_PATH	= -I../build/include -I../../../include
BUILD_LIBS_PATH	= ../build/lib
BUILD_LIBS		= 

SRCS1			= util.c
OBJS1			= $(SRCS1:.c=.o)
AOUT1			= libutil.a

SRCS2			= node.c
OBJS2			= $(SRCS2:.c=.o)
AOUT2			= libnode.a

SRCS3			= cfg.c
OBJS3			= $(SRCS3:.c=.o)
AOUT3			= libcfg.a

DEST			= ../build
CFLAG			= $(CFLAGS) -std=c99 -D_GNU_SOURCE -Wall -Wno-unused-function

#---------------------------------------------------------------------
all:: clean $(AOUT1) $(AOUT2) $(AOUT3) install

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAG) $(BUILD_INC_PATH) $(INC_PATH) -c $<

$(AOUT1): $(OBJS1)
	$(AR) -rc $(AOUT1) $(OBJS1)

$(AOUT2): $(OBJS2)
	$(AR) -rc $(AOUT2) $(OBJS2)

$(AOUT3): $(OBJS3)
	$(AR) -rc $(AOUT3) $(OBJS3)

install:
	\cp ./*.a   $(DEST)/lib
	\cp ./*.h   $(DEST)/include

clean:
	\rm -f *.o *.a core.* 
	rm -rf cscope.out tceetree.out tceetree.out.png
	rm -rf *.csv
