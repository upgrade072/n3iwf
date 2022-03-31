include ../MakeVars

BUILD_INC_PATH  = -I../build/include -I../build/include/libxml2 -I../build/share/asn1c
BUILD_LIBS_PATH = ../build/lib
BUILD_LIBS      = $(BUILD_LIBS_PATH)/libconfig.a \
				  $(BUILD_LIBS_PATH)/libjson-c.a \
				  $(BUILD_LIBS_PATH)/libxml2.a \
				  $(BUILD_LIBS_PATH)/libdata.a \
				  $(BUILD_LIBS_PATH)/libutil.a \
				  $(BUILD_LIBS_PATH)/libxml2json2xml.a \
				  $(BUILD_LIBS_PATH)/libasncodec.a \
                  -lpthread -lrt -lm -ldl -lz -llzma

SRCS            = cp_test.c
OBJS            = $(SRCS:.c=.o)
AOUT            = cp_test
DEST            = ../BIN
CFLAG           = -std=c99 -D_GNU_SOURCE -Wall -Wno-unused-function $(TCMALLOC_FLAG) $(CPUFLAME_FLAG)

#---------------------------------------------------------------------
all:: clean $(AOUT) install

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAG) $(BUILD_INC_PATH) $(INC_PATH) -c $<

$(AOUT): $(OBJS)
	$(CC) $(CFLAG) -o $(AOUT) $(OBJS) $(BUILD_LIBS) $(LIB_PATH) $(LIBS) 

install:

clean:
	\rm -f $(OBJS) $(AOUT) core.* 
	rm -rf cscope.out tceetree.out tceetree.out.png
	rm -rf *.csv
	rm -rf *.bin
