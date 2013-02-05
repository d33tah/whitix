
include ../../../Makefile.cfg
include ../../../Makefile.parse

DIR = src/lib/libz/

INCDIR = ./ ../../../../librtl ../../../../libc/include ../../../../posix/include
OBJS += adler32.o compress.o crc32.o gzio.o uncompr.o deflate.o trees.o zutil.o \
        inflate.o infback.o inftrees.o inffast.o

TARGET_O = libz.o

TARGET_SO-$(PLATFORM_LINUX) = libz.so
LDFLAGS-$(PLATFORM_LINUX)  += -nodefaultlibs -nostartfiles -nostdlib -pthread -t -L../../../../libc -L../../../../posix -llinker -lstdc -lposix -lm 
TARGET_SO = $(TARGET_SO-Y)

include ../../../Makefile.rules
