DEPTH = ../..
include $(DEPTH)/make.config

CFLAGS = -Wall -O2 -fomit-frame-pointer -fno-builtin -nostdinc -I../../libc/include \
 -I../../sdk/include -m32

OBJS = main.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $*.o

mkext3fs: $(OBJS)
	$(LD)  -L../../posix -lpthread ../../libc/init/init.o $(OBJS)  -lstdc -o mkext3fs
	strip mkext3fs

build: mkext3fs

install:
	cp mkext3fs ../../../CdRoot/Applications

clean:
	rm -rf *.o
	rm -f mkext3fs
