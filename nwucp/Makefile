include ../MakeVars

BUILD_INC_PATH	= -I./ -I../build/include -I../build/include/glib-2.0 -I../PKG/MP/include
BUILD_LIBS_PATH	= ../build/lib
BUILD_LIBS		= $(BUILD_LIBS_PATH)/libutil.a \
				  $(BUILD_LIBS_PATH)/libnode.a \
				  $(BUILD_LIBS_PATH)/libconfig.a \
				  $(BUILD_LIBS_PATH)/libjson-c.a \
				  $(BUILD_LIBS_PATH)/json_macro.a \
				  $(BUILD_LIBS_PATH)/libngapp.a \
				  $(BUILD_LIBS_PATH)/libevent_pthreads.a \
				  $(BUILD_LIBS_PATH)/libevent.a \
				  $(BUILD_LIBS_PATH)/libglib-1.3.a \
				  ../PKG/MP/lib/libmsgtrc.a \
				  ../PKG/MP/lib/libcomm.a \
				  ../PKG/MP/lib/libmmc.a \
				  -lpthread -lrt -lm -ldl -g

SRCS			= main.c amf.c ue.c ngap.c tcp.c io_worker.c handler.c send.c parse.c pdu.c watch.c trace.c overload.c
OBJS			= $(SRCS:.c=.o)
AOUT			= nwucp
DEST			= ../BIN
CPUFLAME_FLAG   = -fno-omit-frame-pointer
TCMALLOC_FLAG   = -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free
CFLAG           = $(CFLAGS) -std=c99 -D_GNU_SOURCE -Wall -Wno-unused-function $(TCMALLOC_FLAG) $(CPUFLAME_FLAG)

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
