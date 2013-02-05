CFLAGS = -ffreestanding -fleading-underscore -fomit-frame-pointer -fno-builtin

build:
	gcc $(CFLAGS) -I../libc/include -c main.c -o main.o
	ld -Bstatic -L../libc -L../librtl ../libc/init/init.o main.o -lstdc -lrtl -o diskformat
	strip diskformat
	
clean:
	rm *.o -f
