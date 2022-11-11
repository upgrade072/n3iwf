include ../MakeVars

BUILD_INC_PATH	= -I../build/include -I../../../include -I../PKG/MP/include
BUILD_LIBS_PATH	= ../build/lib
BUILD_LIBS		= $(BUILD_LIBS_PATH)/libutil.a \
				  $(BUILD_LIBS_PATH)/libnode.a \
				  $(BUILD_LIBS_PATH)/libconfig.a \
				  $(BUILD_LIBS_PATH)/libfort.a \
				  $(BUILD_LIBS_PATH)/libsctp.a \
				  $(BUILD_LIBS_PATH)/libevent_pthreads.a \
				  $(BUILD_LIBS_PATH)/libevent.a \
				  ../PKG/MP/lib/libcomm.a \
				  ../PKG/MP/lib/libmmc.a \
				  ../PKG/MP/lib/libmsgtrc.a \
				  -lpthread -lrt -lm -ldl

SRCS			= main.c list.c io_worker.c bf_worker.c stack.c mmc.c
OBJS			= $(SRCS:.c=.o)
AOUT			= sctpc
DEST			= ../BIN
CFLAG			= $(CFLAGS) -D_GNU_SOURCE -Wall -Wno-unused-function -std=c99

#---------------------------------------------------------------------
all:: clean $(AOUT) install

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAG) $(BUILD_INC_PATH) $(INC_PATH) -c $<

$(AOUT): $(OBJS)
	$(CC) $(CFLAG) -o $(AOUT) $(OBJS) $(BUILD_LIBS) $(LIB_PATH) $(LIBS) 

install:
	\cp $(AOUT) $(DEST)
	\cp ./sctp_intf.h ../build/include

clean:
	\rm -f $(OBJS) $(AOUT) core.* 
	rm -rf cscope.out tceetree.out tceetree.out.png
	rm -rf *.csv
