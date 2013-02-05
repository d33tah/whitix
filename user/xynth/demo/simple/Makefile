
include ../../Makefile.cfg
include ../../Makefile.parse

DIR = src/demo/simple/

INCDIR  = ../../src/lib ../../../libc/include ../../../librtl ../../../posix/include
LIBDIR  = ../../src/lib ../../../libc ../../../librtl ../../../posix
LDFLAGS = -lxynth -lrtl -lstdc -lposix -lm -ldl -lpthread ../../../libc/init/init.o

OBJS = simple.o

ifeq ($(SINGLE_APP), Y)
TARGET_A = single_simple.a
CFLAGS  += -Dmain=simple_main
else
TARGET = simple
endif

EXTRA_CLEAN = simple single_simple.a

include ../../Makefile.rules

dist: all
ifneq ($(TARGET),)
	cp -a $(TARGET) ../../$(DISTBINDIR)
endif
