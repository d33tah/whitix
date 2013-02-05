DEPTH=../../

MODULES = net.sys pci.sys irq.sys dma.sys eth.sys mem.sys timer.sys resource.sys \
	param.sys

build: $(MODULES)
	ld $(LD_R_FLAGS) $(MODULES) -o linux.sys

modules_install:
	cp linux.sys ../../CdRoot/System/Modules/Core

include $(DEPTH)/make.inc
