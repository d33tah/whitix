DEPTH = ../..
include $(DEPTH)/make.config

CFLAGS = -Wall -I../include -I../../librtl -I../../libc/include -nostdlib -ffreestanding -fno-builtin -fPIC -m32 -fno-stack-protector

OBJS = dir.o file.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $*.o

build: $(OBJS)
	$(LD_SHARED) -lstdc -o ../libs/libfile.so *.o

clean:
	rm -f *.o *.so
