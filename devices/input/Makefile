DEPTH=../../

MODULES := ps2mouse.sys keyboard.sys ps2.sys

build: $(MODULES)

modules_install:
	cp -f *.sys ../../CdRoot/System/Modules/Input

include $(DEPTH)make.inc
-include $(DEPS)
