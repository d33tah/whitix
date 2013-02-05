
include ../../Makefile.cfg
include ../../Makefile.parse

DIR = src/demo/term/

INCDIR  = ../../src/lib ../../../libc/include ../../../sdk/include ../../../posix/include
LIBDIR  = ../../src/lib ../../../libc ../../../posix ../../../linker
LDFLAGS = -t -llinker -lxynth -lstdc -lposix -lm -ldl -lpthread -llinker ../../../libc/init/init.o

OBJS = term.o

ifeq ($(SINGLE_APP), Y)
TARGET_A = single_term.a
CFLAGS  += -Dmain=term_main
else
TARGET = term
endif

EXTRA_CLEAN = term single_term.a

include ../../Makefile.rules

dist: all
ifneq ($(TARGET),)
	cp -a $(TARGET) ../../$(DISTBINDIR)
endif
