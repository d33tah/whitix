DEPTH=../../
MODULES = super.sys vnode.sys stree.sys dir.sys file.sys

build: reiser3fs.sys

reiser3fs.sys: $(MODULES)
	$(LD) $(LD_R_FLAGS) $(MODULES) -o reiser3fs.sys

modules_install:
	cp reiser3fs.sys ../../CdRoot/System/Modules/Core

include $(DEPTH)make.inc
-include $(DEPS)
