DEPTH = ../..
include $(DEPTH)/make.config

CFLAGS = -Wall -O2 -fomit-frame-pointer -fno-builtin -nostdinc -I../../libc/include -I../../sdk/include -m32 -fno-stack-protector

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $*.o

OBJS = main.o

mkfatfs: $(OBJS)
	$(LD) -L../../posix -lpthread ../../libc/init/init.o $(OBJS) -lstdc  -o mkfatfs
	strip mkfatfs

install:
	cp mkfatfs ../../../CdRoot/Applications

clean:
	rm -rf *.o
	rm -f mkfatfs
