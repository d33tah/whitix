
include ../../Makefile.cfg
include ../../Makefile.parse

DIR = src/python/

INCDIR += ../lib ../../../python/Include ../../../python ../../../libc/include \
	../../../librtl ../../../posix/include
LIBDIR += ../lib ../../../libc ../../../librtl ../../../posix ../../../python \
	../widget

OBJS = window.o xynthpy.o frame.o

ifeq ($(PLATFORM_LINUX), Y)
TARGET_SO = xynth.so
LDFLAGS   = -lpython -lxynth -lwidget
EXTRA_DEPEND_FILES = *.h
else
ifeq ($(PLATFORM_PSPDEV), Y)
else
ifeq ($(PLATFORM_MINGW), Y)
else
ifeq ($(PLATFORM_GP2X), Y)
endif
endif
endif
endif

include ../../Makefile.rules

dist: all
