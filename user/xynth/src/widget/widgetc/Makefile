
include ../../../Makefile.cfg
include ../../../Makefile.parse

DIR = src/widget/widgetc/

LDFLAGS = ../../lib/expat/libexpat.o -L../../../../libc -L../../../../posix -llinker -lstdc -lposix -lm -lpthread ../../../../libc/init/init.o
INCDIR = ./ ../ ../../lib ../../../../sdk/include ../../../../libc/include ../../../../posix/include

OBJS = widgetc.o list.o code.o header.o xml.o

TARGET_HOST = widgetc

#CC := PATH=/usr/bin:${PATH} gcc

include ../../../Makefile.rules

dist: all
ifneq ($(TARGET),)
	cp -a $(TARGET) ../../../$(DISTBINDIR)
endif
