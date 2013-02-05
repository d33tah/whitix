DEPTH=../../
IGNORE_CLEAN = y
MODULES = floppy.sys ramdisk.sys

build: $(MODULES)
	$(MAKE) -C ata

modules_install:
	$(MAKE) -C ata modules_install
	cp *.sys ../../CdRoot/System/Modules/Storage

clean:
	$(MAKE) -C ata clean
	$(RM) .deps/*.d *.o *.sys

include $(DEPTH)make.inc
-include $(DEPS)
