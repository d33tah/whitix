DEPTH=../
OBJS = earlyconsole.o
MODULES = console.sys virtual.sys

build: $(MODULES) $(OBJS)

modules_install:
	cp *.sys ../CdRoot/System/Modules/Video/

include $(DEPTH)make.inc
-include $(DEPS)
