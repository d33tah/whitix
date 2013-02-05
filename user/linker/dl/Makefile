DEPTH = ../..
include $(DEPTH)/make.config

CFLAGS = -Wextra -Wall -nostdlib -nostdinc -ffreestanding -fno-stack-protector \
 -fno-omit-frame-pointer -fno-builtin -fno-leading-underscore -fPIC  \
 -I../../libc/include -I../../posix/include -I../../sdk/include -O2 -m32

OBJS = dl.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $*.o

build: $(OBJS)
	$(LD) -shared -lstdc $(OBJS) ../resolve.o -o libdl.so
	cp *.so ../../posix

clean:
	rm -f *.o
	rm -f *.so
