DEPTH = ..
include $(DEPTH)/make.config

AR = ar
RANLIB = ranlib

build:
	$(MAKE) -C file
	$(MAKE) -C signal
	$(MAKE) -C time
	$(MAKE) -C dir
	$(MAKE) -C wait
	$(MAKE) -C net
	$(MAKE) -C tty
	$(MAKE) -C locale
	$(MAKE) -C conf
	$(MAKE) -C user
	$(MAKE) -C process
	$(MAKE) -C opt
	$(MAKE) -C socket
	$(MAKE) -C memory
	$(MAKE) -C strings
	$(LD_SHARED) -lstdc dir/*.o file/*.o wait/*.o signal/*.o net/*.o time/*.o tty/*.o locale/*.o conf/*.o user/*.o process/*.o opt/*.o socket/*.o memory/*.o strings/*.o -o libposix.so

libpthread.so:
	$(MAKE) -C pthread
	$(LD_SHARED) -L../linker  pthread/*.o -o libpthread.so

install:
	cp libposix.so ../../CdRoot/System/Runtime
	cp libpthread.so ../../CdRoot/System/Runtime
	cp libdl.so ../../CdRoot/System/Runtime

clean:
	rm -rf *.so
	$(MAKE) -C file clean
	$(MAKE) -C signal clean
	$(MAKE) -C time clean
	$(MAKE) -C dir clean
	$(MAKE) -C wait clean
	$(MAKE) -C pthread clean
	$(MAKE) -C net clean
	$(MAKE) -C tty clean
	$(MAKE) -C locale clean
	$(MAKE) -C conf clean
	$(MAKE) -C user clean
	$(MAKE) -C process clean
	$(MAKE) -C opt clean
	$(MAKE) -C socket clean
	$(MAKE) -C memory clean
	$(MAKE) -C strings clean
