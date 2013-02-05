CFLAGS = -Wall -I../include -nostdlib -ffreestanding -fno-builtin -fPIC -m32
OBJS = syscall.o sib.o

build: $(OBJS)

syscall.o: ../include/sysdefs.h syscall.S
	gcc -c syscall.S $(CFLAGS) -o syscall.o

clean:
	rm -f *.o
