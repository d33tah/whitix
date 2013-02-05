DEPTH=../
MODULES = local.sys socket.sys network.sys device.sys eth.sys

build: $(MODULES)
	$(MAKE) -C inet
	ld $(LD_R_FLAGS) socket.sys network.sys device.sys eth.sys -o net.sys

modules_install:
	$(MAKE) -C inet modules_install
	cp net.sys local.sys ../CdRoot/System/Modules/Core

include $(DEPTH)make.inc
-include $(DEPS)
