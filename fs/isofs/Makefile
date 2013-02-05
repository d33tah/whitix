DEPTH=../../
MODULES = super.sys dir.sys date.sys

build: cdfs.sys

cdfs.sys: $(MODULES)
	ld $(LD_R_FLAGS) $(MODULES) -o cdfs.sys	

modules_install:
	cp cdfs.sys ../../CdRoot/System/Modules/Filesystems

include $(DEPTH)make.inc
-include $(DEPS)
