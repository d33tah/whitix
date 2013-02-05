DEPTH = ../..
include $(DEPTH)/make.config

CFLAGS = -Wall -I../include -I../../librtl -I../../libc/include -nostdlib -ffreestanding -fno-builtin -fPIC -m32

OBJS = mutex.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $*.o

build: $(OBJS)
	$(LD_SHARED) -o ../libs/libthread.so *.o -lstdc

clean:
	rm -f *.o
	rm -f *.so
