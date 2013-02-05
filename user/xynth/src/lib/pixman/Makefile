
include ../../../Makefile.cfg
include ../../../Makefile.parse

DIR = src/lib/pixman/

INCDIR = . ../../../../librtl ../../../../libc/include ../../../../posix/include
CFLAGS += -DHAVE_CONFIG_H -fno-builtin
OBJS += fbcompose.o fbedge.o fbpict.o fbtrap.o icblt.o \
        icbltone.o iccolor.o icformat.o icimage.o \
        icpixels.o icrect.o icstipple.o ictransform.o \
        ictrap.o ictri.o icutil.o pixregion.o renderedge.o

TARGET_O = libpixman.o

include ../../../Makefile.rules
