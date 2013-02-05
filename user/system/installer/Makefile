DEPTH = ../..
include $(DEPTH)/make.config

CFLAGS = -g -ffreestanding -fno-builtin -I../../libc/include -I../../sdk/include \
	-fno-stack-protector -DMODULE -m32
OBJS = textinstaller.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $*.o

textinstaller: $(OBJS)
	$(LD) -L../../posix -L../../sdk/libs ../../libc/init/init.o textinstaller.o -lstdc -lpthread -o textinstaller

build: textinstaller

install:
	cp textinstaller ../../../CdRoot/Applications/textinstall
	cp installer.py ../../../CdRoot/Applications/installer
	cp partitioner.py ../../../CdRoot/Applications/partitioner
