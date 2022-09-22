include ../MakeVars

BUILD_INC_PATH	= -I./ -I../build/include -I../../../include
BUILD_LIBS_PATH	= ../build/lib
BUILD_LIBS      = $(BUILD_LIBS_PATH)/libconfig.a \
				  $(BUILD_LIBS_PATH)/libudp.a \
				  $(BUILD_LIBS_PATH)/libevent_pthreads.a \
				  $(BUILD_LIBS_PATH)/libevent.a \
				  $(BUILD_LIBS_PATH)/eap5g.a \
				  $(BUILD_LIBS_PATH)/libutil.a \
				  -lpthread -lrt -lm -ldl -g

SRCS			= main.c io_worker.c proc.c intf.c comm.c
OBJS			= $(SRCS:.c=.o)
AOUT			= eap5g
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
