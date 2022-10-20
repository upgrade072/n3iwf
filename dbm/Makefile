include ../MakeVars

BUILD_INC_PATH	= -I./ -I../build/include -I../build/include/glib-2.0 -I../PKG/MP/include
BUILD_LIBS_PATH	= ../build/lib
BUILD_LIBS		= $(BUILD_LIBS_PATH)/libutil.a \
				  $(BUILD_LIBS_PATH)/libevent.a \
				  ../PKG/MP/lib/libcomm.a \
				  ../PKG/MP/lib/libmmc.a \
				  ../PKG/MP/lib/libmsgtrc.a \
				  -lrt -lm -ldl -g

SRCS			= main.c dbm_mmc.c dbm_trace.c
OBJS			= $(SRCS:.c=.o)
AOUT			= dbm
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
