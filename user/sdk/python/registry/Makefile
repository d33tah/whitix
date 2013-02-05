CFLAGS = -fno-stack-protector -I../../../python/Include -I../../include -I../../../libc/include -I../../../librtl -I../../../posix/include -nostdinc
OBJS = main.o regset.o iters.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $*.o

build: $(OBJS)
	ld -shared -nostdlib -L../../../libc  -L../../../posix -L../../../linker -L../../../python -L../../libs \
		-lstdc -lposix -lpthread -lpython -lconsole -lm -lregistry -llinker $(OBJS) ../../../libc/init/crt_begin.o  \
		../../../libc/init/crt_end.o -o wregistry.so

clean:
	rm -f *.o *.so

install:
	cp *.so ../../../../CdRoot/System/Runtime/Python
