DEPTH=../
OBJS = pg_alloc.o slab.o vmm.o mmap.o
MODULES = shmem.sys

build: $(OBJS) $(MODULES)

modules_install:
	cp *.sys ../CdRoot/System/Modules/Core

include $(DEPTH)make.inc
-include $(DEPS)
