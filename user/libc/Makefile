DEPTH = ..
include $(DEPTH)/make.config

build:
	cp ../linker/liblinker.so .
	$(MAKE) -C init
	$(MAKE) -C assert
	$(MAKE) -C ctype
	$(MAKE) -C stdio
	$(MAKE) -C stdlib
	$(MAKE) -C string
	$(MAKE) -C errno
	$(MAKE) -C setjmp
	$(MAKE) -C math
	$(MAKE) -C rtl
	$(LD_SHARED) -L. -L../posix -lpthread -o libstdc.so ctype/*.o errno/*.o \
		  rtl/*.o stdio/*.o stdlib/*.o assert/*.o string/*.o setjmp/*.o
	$(LD_SHARED) -o libm.so math/*.o

pre:
	$(MAKE) -C init
	$(MAKE) -C rtl

install:
	cp libstdc.so ../../CdRoot/System/Runtime
	cp libm.so ../../CdRoot/System/Runtime
	cp include/*.h ../../CdRoot/System/Include
	cp init/crt_begin.o ../../CdRoot/System/Runtime/C
	cp init/crt_end.o ../../CdRoot/System/Runtime/C
	cp init/init.o ../../CdRoot/System/Runtime/C/crt_static.o

clean:
	$(MAKE) -C assert clean
	$(MAKE) -C ctype clean
	$(MAKE) -C errno clean
	$(MAKE) -C stdio clean
	$(MAKE) -C stdlib clean
	$(MAKE) -C string clean
	$(MAKE) -C init clean
	$(MAKE) -C setjmp clean
	$(MAKE) -C math clean
	rm -f *.so
