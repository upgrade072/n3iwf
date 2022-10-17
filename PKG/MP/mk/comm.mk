SHELL   		= /bin/sh
RM      		= rm -f
RMDIR   		= rmdir
MOVE    		= mv
CP      		= cp
 
CC      		= gcc
GCC     		= gcc
LD      		= cc
YACC    		= yacc
LEX     		= lex
AR      		= ar
AROPTS  		= cr
RANLIB  		= ranlib
 
DEPEND      	= makedepend
LINT        	= lint
CFLAG       	= -g -m64 -Wall -Wno-unused -Werror
 
LIBDL       	=
LIBCRYPT    	=
LIBSOCKET   	= -lnsl
LIBKSTAT    	=
LIBELF      	= -lelf
LIBW        	=
 
# mysql path
MYSQL_INC_PATH 	= /usr/include/mysql
MYSQL_LIB_PATH 	= /usr/lib64/mysql
