DEPTH = ../..
include $(DEPTH)/make.config

CFLAGS = -Wall -I../include -I../../libc/include -nostdinc -ffreestanding -fno-builtin -fPIC -m32 -fno-stack-protector

OBJS = registry.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $*.o

build: $(OBJS)
	$(LD_SHARED) -L../libs -lnetwork -o ../libs/libregistry.so *.o

clean:
	rm -f *.o
	rm -f *.so
