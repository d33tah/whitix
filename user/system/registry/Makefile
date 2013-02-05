DEPTH = ../..
include $(DEPTH)/make.config

CFLAGS = -g -ffreestanding -fno-builtin -I../../libc/include \
	 -I../../sdk/include -fno-stack-protector -DMODULE -m32 -Wall -Wextra
OBJS = main.o keyset.o key.o file.o socket.o handle.o

LIBDIRS = -L../../librtl -L../../posix -L../../sdk/libs
LIBS = -lstdc -lpthread -lnetwork -lfile

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $*.o

build: $(OBJS)
	$(LD)  $(LIBDIRS) $(LIBS) ../../libc/init/init.o $(OBJS) -o regserver

install:
	cp regserver ../../../CdRoot/System/Startup/
	make -C textreg install
	
clean:
	rm -f *.o
	rm -f regserver
