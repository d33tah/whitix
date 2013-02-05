CFLAGS += -nostdinc -fPIC -ffreestanding -fno-builtin -fno-leading-underscore -O2 -Wall -I../include -I../../sdk/include -I../../posix/include -fno-stack-protector -m32
ASFLAGS = $(CFLAGS)
