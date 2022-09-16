include ../MakeVars

BUILD_INC_PATH	= -I./ -I../build/include -I../build/include/glib-2.0
BUILD_LIBS_PATH	= ../build/lib
BUILD_LIBS		= $(BUILD_LIBS_PATH)/libutil.a \
				  $(BUILD_LIBS_PATH)/libnode.a \
				  $(BUILD_LIBS_PATH)/libconfig.a \
				  $(BUILD_LIBS_PATH)/libjson-c.a \
				  $(BUILD_LIBS_PATH)/json_macro.a \
				  $(BUILD_LIBS_PATH)/libevent_pthreads.a \
				  $(BUILD_LIBS_PATH)/libevent.a \
				  $(BUILD_LIBS_PATH)/libglib-1.3.a \
				  -lpthread -lrt -lm -ldl -g

SRCS			= main.c amf.c ue.c ngap.c tcp.c io_worker.c
OBJS			= $(SRCS:.c=.o)
AOUT			= nwucp
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

clean:
	\rm -f $(OBJS) $(AOUT) core.* 
	rm -rf cscope.out tceetree.out tceetree.out.png
	rm -rf *.csv
