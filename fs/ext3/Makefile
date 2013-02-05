DEPTH=../../
MODULES = super.sys inode.sys file.sys ialloc.sys balloc.sys

build: ext3.sys

ext3.sys: $(MODULES)
	$(LD) $(LD_R_FLAGS) $(MODULES) -o ext3.sys	

modules_install:
	cp ext3.sys ../../CdRoot/System/Modules/Filesystems

include $(DEPTH)make.inc
-include $(DEPS)
