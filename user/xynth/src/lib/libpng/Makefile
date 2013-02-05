
include ../../../Makefile.cfg
include ../../../Makefile.parse

DIR = src/lib/libpng/

INCDIR = ./ ../libz ../../../../libc/include ../../../../posix/include
OBJS += png.o pngerror.o pngget.o pngmem.o \
        pngpread.o pngread.o pngrio.o pngrtran.o \
        pngrutil.o pngset.o pngtrans.o pngwio.o \
        pngwrite.o pngwtran.o pngwutil.o

TARGET_O = libpng.o
TARGET_SO = libpng.so

LDFLAGS-$(PLATFORM_LINUX)  += -nodefaultlibs -nostartfiles -nostdlib -pthread -L../../../../libc -L../../../../posix -lstdc -lposix -lm -dynamic-linker=/Linker

include ../../../Makefile.rules
