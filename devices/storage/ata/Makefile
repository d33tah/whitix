DEPTH=../../../
MODULES = ata.sys ata-disk.sys ata-cd.sys ata-identify.sys

build: ata_ide.sys

ata_ide.sys: $(MODULES)
	ld $(LD_R_FLAGS) $(MODULES) -o ata_ide.sys
	
modules_install:
	cp ata_ide.sys ../../../CdRoot/System/Modules/Storage/

include ../../../make.inc
-include $(DEPS)
