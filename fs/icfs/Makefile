DEPTH=../../
OBJS = icsetup.o info.o info_cpu.o config.o

build: $(OBJS) icfs.sys

modules_install:
	cp icfs.sys ../../CdRoot/System/Modules/Filesystems

include $(DEPTH)make.inc
-include $(DEPS)
