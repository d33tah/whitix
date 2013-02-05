DEPTH=../../
MODULES = init.sys journals.sys transaction.sys commit.sys recovery.sys checkpoint.sys

build: journal.sys

journal.sys: $(MODULES)
	$(LD) $(LD_R_FLAGS) $(MODULES) -o journal.sys
	
modules_install:
	cp journal.sys ../../CdRoot/System/Modules/Filesystems

include $(DEPTH)make.inc
-include $(DEPS)
