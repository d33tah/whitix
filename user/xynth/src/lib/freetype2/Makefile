include ../../../Makefile.cfg
include ../../../Makefile.parse

DIR = src/lib/freetype2/

INCDIR = ./ ./include ../../../../libc/include ../../../../posix/include
CFLAGS += -DFT2_BUILD_LIBRARY
OBJS += ftbase.o ftbitmap.o ftdebug.o ftglyph.o \
        ftinit.o ftsystem.o autofit.o sfnt.o \
        smooth.o truetype.o


TARGET_O = libfreetype2.o
TARGET_SO = libfreetype.so

LDFLAGS-$(PLATFORM_LINUX)  += -nodefaultlibs -nostartfiles -nostdlib -pthread -L../../../../libc -L../../../../posix -lstdc -lposix -lm -dynamic-linker=/Linker

include ../../../Makefile.rules
