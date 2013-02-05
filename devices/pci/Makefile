DEPTH=../../
MODULES = pci_core.sys pci_lib.sys pci_bios.sys

pci.sys: $(MODULES)
	ld $(LD_R_FLAGS) $(MODULES) -o pci.sys

modules_install:
	cp pci.sys ../../CdRoot/System/Modules/Core

include $(DEPTH)make.inc
-include $(DEPS)
