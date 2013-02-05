DEPTH = ../..
include $(DEPTH)/make.config

CFLAGS = -Wall -Werror -I../include -I../../librtl -I../../libc/include -nostdlib -ffreestanding -fno-builtin -fPIC -m32

OBJS = console.o readline.o history.o tab.o token.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $*.o

build: $(OBJS)
	$(LD_SHARED) -o ../libs/libconsole.so $(OBJS) -lstdc

clean:
	rm -f *.o
	rm -f *.so
