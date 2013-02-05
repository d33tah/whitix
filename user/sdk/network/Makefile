DEPTH = ../..
include $(DEPTH)/make.config

CFLAGS = -Wall -I../include -I../../libc/include -nostdlib -ffreestanding -fno-builtin -fPIC -m32

OBJS = socket.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $*.o

build: $(OBJS)
	$(LD_SHARED) -L../../libc -lstdc -o ../libs/libnetwork.so *.o

clean:
	rm -f *.o
	rm -f *.so
