include ../MakeVars

BUILD_INC_PATH	= -I../build/include -I../../../include
BUILD_LIBS_PATH	= ../build/lib
BUILD_LIBS		= 

SRCS			= util.c
OBJS			= $(SRCS:.c=.o)
AOUT			= libutil.a
DEST			= ../build
CFLAG			= $(CFLAGS) -D_GNU_SOURCE -Wall -Wno-unused-function

#---------------------------------------------------------------------
all:: clean $(AOUT) install

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAG) $(BUILD_INC_PATH) $(INC_PATH) -c $<

$(AOUT): $(OBJS)
	$(AR) -rc $(AOUT) $(OBJS)

install:
	\cp $(AOUT) $(DEST)/lib
	\cp ./*.h   $(DEST)/include

clean:
	\rm -f $(OBJS) $(AOUT) core.* 
	rm -rf cscope.out tceetree.out tceetree.out.png
	rm -rf *.csv
