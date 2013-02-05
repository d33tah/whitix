DEPTH = ..
include $(DEPTH)/make.config
include $(DEPTH)/make.rules

CFLAGS = -Wall -O2 -fomit-frame-pointer -fno-builtin -nostdinc -I../libc/include -I../sdk/include -fno-stack-protector -m32

OBJS = main.o file.o

fruity: $(OBJS)
	$(LD) -L../posix -llinker -lpthread -L../sdk/libs ../libc/init/init.o $(OBJS) -lconsole -lstdc -o fruity

clean:
	rm -rf *.o fruity

install:
	cp fruity ../../CdRoot/Applications
