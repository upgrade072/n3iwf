include ../MakeVars

BUILD_INC_PATH	= -I../build/include -I../../../include
BUILD_LIBS_PATH	= ../build/lib
BUILD_LIBS		= $(BUILD_LIBS_PATH)/libutil.a \
				  $(BUILD_LIBS_PATH)/libconfig.a \
				  $(BUILD_LIBS_PATH)/libevent_pthreads.a \
				  $(BUILD_LIBS_PATH)/libevent.a \
				  $(BUILD_LIBS_PATH)/libtcmalloc_minimal.a \
				  -lpthread -lrt -lm -ldl -lstdc++

SRCS			= main.c
OBJS			= $(SRCS:.c=.o)
AOUT			= dispatcher
DEST			= ../BIN
CPUFLAME_FLAG	= -fno-omit-frame-pointer
TCMALLOC_FLAG	= -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free
CFLAG			= $(CFLAGS) -std=c99 -D_GNU_SOURCE -Wall -Wno-unused-function $(TCMALLOC_FLAG) $(CPUFLAME_FLAG)

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
