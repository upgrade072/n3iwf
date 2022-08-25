
CFLAG       = $(CFLAGS) -D_GNU_SOURCE -Wall -std=gnu99 -fPIC

BUILD_INC_PATH  = -I../build/include -I../build/include/libxml2 -I../build/share/asn1c

SRCS        = json2xml.c xml2json.c
OBJS        = $(SRCS:.c=.o)

AOUT        = libxml2json2xml.a

DEST_BUILD  = ../build

#
#---------------------------------------------------------------------
#
all:: clean $(AOUT) install

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAG) $(BUILD_INC_PATH) $(INC_PATH) -c $<

$(AOUT): $(OBJS)
	$(AR) -rc $(AOUT) $(OBJS)

install:
	\cp ./*.h $(DEST_BUILD)/include
	\cp $(AOUT) $(DEST_BUILD)/lib

clean:
	\rm -f $(OBJS) $(AOUT) core.* sample
