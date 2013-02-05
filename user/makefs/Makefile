CFLAGS = -ffreestanding -fno-builtin -I../libc/include -I../librtl -fno-stack-protector

build:
	$(MAKE) -C fat
	$(MAKE) -C ext3

install:
	$(MAKE) -C fat install
	$(MAKE) -C ext3 install

clean:
	rm -f *.o
	$(MAKE) -C fat clean
	$(MAKE) -C ext3 clean
