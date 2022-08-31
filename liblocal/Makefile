include ../MakeVars

BUILD_INC_PATH	= -I./ -I../build/include -I../../../include -I../build/include/libxml2 -I../build/share/asn1c
BUILD_LIBS_PATH	= ../build/lib
BUILD_LIBS		= 

SRCS1			= util.c
OBJS1			= $(SRCS1:.c=.o)
AOUT1			= libutil.a

SRCS2			= node.c
OBJS2			= $(SRCS2:.c=.o)
AOUT2			= libnode.a

SRCS3			= json_macro.c
OBJS3			= $(SRCS3:.c=.o)
AOUT3			= json_macro.a

SRCS4			= eap5g.c
OBJS4			= $(SRCS4:.c=.o)
AOUT4			= eap5g.a

DEST			= ../build
CFLAG			= $(CFLAGS) -std=gnu99 -D_GNU_SOURCE -Wall -Wno-unused-function

#---------------------------------------------------------------------
all:: clean $(AOUT1) $(AOUT2) $(AOUT3) $(AOUT4) install

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAG) $(BUILD_INC_PATH) $(INC_PATH) -c $<

$(AOUT1): $(OBJS1)
	$(AR) -rc $(AOUT1) $(OBJS1)

$(AOUT2): $(OBJS2)
	$(AR) -rc $(AOUT2) $(OBJS2)

$(AOUT3): $(OBJS3)
	$(AR) -rc $(AOUT3) $(OBJS3)

$(AOUT4): $(OBJS4)
	$(AR) -rc $(AOUT4) $(OBJS4)

install:
	\cp ./*.a   $(DEST)/lib
	\cp ./*.h   $(DEST)/include

clean:
	\rm -f *.o *.a core.* 
	rm -rf cscope.out tceetree.out tceetree.out.png
	rm -rf *.csv
