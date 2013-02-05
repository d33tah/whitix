DEPTH=../../../

OBJS := tls.o pic.o init.o process.o ints.o idtstubs.o idt.o pit.o reboot.o process.o \
	 smp.o cpuid.o time.o ioperms.o multiboot.o sys.o syscall.o sib.o

build: $(OBJS)

include $(DEPTH)make.inc
-include $(DEPS)
