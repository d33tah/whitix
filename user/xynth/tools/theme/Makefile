include ../../Makefile.cfg
include ../../Makefile.parse
 
INCDIR  = ../../src/lib ../../src/server ../../../libc/include ../../../posix/include
LIBDIR  = ../../src/lib
LDFLAGS = ../../src/lib/libxynth.a -lpthread -lm -m32 -L../../../libc -nostdlib -lstdc -L../../../posix -lposix ../../../libc/init/init.o
 
OBJS = theme.o
 
TARGET_HOST = theme
 
include ../../Makefile.rules
