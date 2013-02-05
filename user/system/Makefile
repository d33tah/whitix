DEPTH = ..
include $(DEPTH)/make.config

CFLAGS = -g -ffreestanding -fno-builtin -I../libc/include -I../sdk/include -I../../include  -fno-stack-protector -DMODULE -m32
OBJS = moduleadd.o startup.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $*.o

build: $(OBJS)
	$(LD) -L../posix -L../sdk/libs ../libc/init/init.o moduleadd.o -lstdc -lpthread -o moduleadd
	$(LD) -L../posix -L../sdk/libs ../libc/init/init.o startup.o -lstdc -llinker -lpthread -o startup
	make -C registry
	make -C installer

install:
	cp startup ../../CdRoot/System/Startup/
	cp moduleadd ../../CdRoot/System/Modules/
	make -C registry install
	make -C installer install
	
clean:
	rm -f *.o
	rm -f moduleadd startup
	make -C registry clean
#	make -C installer clean
