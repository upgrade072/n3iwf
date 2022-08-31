include ../MakeVars

BUILD_INC_PATH	= -I./ -I../build/include -I../../../include
BUILD_LIBS_PATH	= ../build/lib
BUILD_LIBS      = $(BUILD_LIBS_PATH)/libutil.a

SRCS			= main.c eap5g.c
OBJS			= $(SRCS:.c=.o)
AOUT			= eap_test
DEST			= ../BIN
CFLAG			= $(CFLAGS) -std=c99 -D_GNU_SOURCE -Wall -Wno-unused-function

#---------------------------------------------------------------------
all:: clean $(AOUT) install

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAG) $(BUILD_INC_PATH) $(INC_PATH) -c $<

$(AOUT): $(OBJS)
	$(CC) $(CFLAG) -o $(AOUT) $(OBJS) $(BUILD_LIBS) $(LIB_PATH) $(LIBS) 

install:
	\cp *.h ../build/include
	\cp $(AOUT) $(DEST)

clean:
	\rm -f $(OBJS) $(AOUT) core.* 
	rm -rf cscope.out tceetree.out tceetree.out.png
	rm -rf *.csv
	rm -rf ./test.bin
