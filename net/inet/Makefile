DEPTH=../../
MODULES = ipv4.sys tcp.sys
build: $(MODULES)
	ld -melf_i386 -r $(MODULES) -o inet.sys

modules_install:
	cp inet.sys ../../CdRoot/System/Modules/Core

include $(DEPTH)make.inc
-include $(DEPS)
