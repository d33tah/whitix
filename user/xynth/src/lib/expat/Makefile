
include ../../../Makefile.cfg
include ../../../Makefile.parse

DIR = src/lib/expat/

INCDIR = . ../../../../librtl ../../../../libc/include ../../../../posix/include
CFLAGS += -DHAVE_EXPAT_CONFIG_H
OBJS += xmlparse.o xmltok.o xmlrole.o

TARGET_O = libexpat.o
TARGET_SO = libexpat.so

include ../../../Makefile.rules
