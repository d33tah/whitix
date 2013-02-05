DEPTH=../../

MODULES := nvidia.sys

build: $(MODULES)

modules_install:
	cp -f *.sys ../../CdRoot/System/Modules/Video

include $(DEPTH)make.inc
-include $(DEPS)
