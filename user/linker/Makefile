### General makefile includes ###

DEPTH = ..

include $(DEPTH)/make.config

### Configurables ###

OBJS = hash.o load_lib.o main.o relocate.o resolve.o tls.o
LINKER_LDFLAGS = -Wl,--warn-common -Wl,--warn-once -Wl,-z,combreloc -Wl,-z,relo -Wl,-e,_start -Wl,-z,now \
	 -Wl,-Bsymbolic -Wl,--export-dynamic -Wl,--sort-common -Wl,--discard-locals -Wl,--discard-all -Wl,--no-undefined

### Rules ###

.c.o:
	$(CC) $(BASE_CFLAGS) -fPIC -Os -I../libc/include -c $*.c -o $*.o

.S.o:
	$(CC) $(BASE_CFLAGS) -c $*.S -o $*.o

liblinker.so: $(OBJS)
	gcc $(BASE_LDFLAGS) $(LINKER_LDFLAGS) $(OBJS) ../libc/rtl/syscall.o -shared -o liblinker.so	

build: liblinker.so
	$(MAKE) -C dl

install:
	cp liblinker.so ../../CdRoot/System/Runtime

clean:
	rm -f *.o liblinker.so
	$(MAKE) -C dl clean
