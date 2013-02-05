DEPTH=../../

MODULES = super.sys vnode.sys file.sys dir.sys fat.sys

build: fatfs.sys

fatfs.sys: $(MODULES)
	$(LD) $(LD_R_FLAGS) $(MODULES) -o fatfs.sys

modules_install:
	cp fatfs.sys ../../CdRoot/System/Modules/Filesystems

include $(DEPTH)make.inc
-include $(DEPS)
