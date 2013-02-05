DEPTH=../../
OBJS = misc.o
MODULES = serial.sys

build: $(OBJS) $(MODULES)

modules_install:
	cp *.sys $(DEPTH)CdRoot/System/Modules/Core/

include $(DEPTH)make.inc
-include $(DEPS)
