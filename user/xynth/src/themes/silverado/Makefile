
include ../../../Makefile.cfg
include ../../../Makefile.parse

DIR = src/themes/silverado/

OBJS          = silverado.o
INCDIR        = ../../lib ../../server ../../../../librtl ../../../../libc/include ../../../../posix/include
TARGET_SO     = silverado.so
LDFLAGS	  += -L../../../../libc -lstdc ../../../../libc/init/crt_begin.o ../../../../libc/init/crt_end.o ../../../../libc/init/init.o -L../../lib -lxynth -L../../../../posix -lpthread

include ../../../Makefile.rules

dist: all
	cp -a $(TARGET_SO) ../../../$(DISTTHEMEDIR)
