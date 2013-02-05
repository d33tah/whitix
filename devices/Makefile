build:
	$(MAKE) -C acpi
	$(MAKE) -C input
	$(MAKE) -C linux
	$(MAKE) -C misc
	$(MAKE) -C net
	$(MAKE) -C pci
	$(MAKE) -C storage
	$(MAKE) -C video
	$(MAKE) -C kedev

modules_install:
	$(MAKE) -C input modules_install
	$(MAKE) -C misc modules_install
	$(MAKE) -C linux modules_install
	$(MAKE) -C net modules_install
	$(MAKE) -C pci modules_install
	$(MAKE) -C video modules_install
	$(MAKE) -C storage modules_install

clean:
	$(MAKE) -C acpi clean
	$(MAKE) -C misc clean
	$(MAKE) -C net clean
	$(MAKE) -C linux clean
	$(MAKE) -C storage clean
	$(MAKE) -C input clean
	$(MAKE) -C pci clean
	$(MAKE) -C video clean
	$(MAKE) -C kedev clean
