DEPTH=..

include $(DEPTH)/make.config
include $(DEPTH)/make.rules

CFLAGS = -ffreestanding -fno-builtin -I../libc/include -I../sdk/include -fno-stack-protector -m32 -Wall -nostdinc

OBJS = main.o builtins.o help.o ls.o

burn: $(OBJS)
	$(LD) -L../posix -L../sdk/libs ../libc/init/init.o $(OBJS) -lstdc -lconsole -lregistry -lnetwork -lfile -lpthread -o burn
	
clean:
	rm -f *.o burn

install:
	cp burn ../../CdRoot/Applications
