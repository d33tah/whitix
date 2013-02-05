build:
	$(MAKE) -C fat
	$(MAKE) -C vfs
	$(MAKE) -C ext3
	$(MAKE) -C isofs
	$(MAKE) -C devfs
	$(MAKE)	-C reiserfs
	$(MAKE) -C journal
	$(MAKE) -C icfs
	$(MAKE) -C kfs

modules_install:
	$(MAKE) -C fat modules_install
	$(MAKE) -C isofs modules_install
	$(MAKE) -C ext3 modules_install
	$(MAKE)	-C reiserfs modules_install
	$(MAKE) -C journal modules_install
	$(MAKE) -C icfs modules_install

clean:
	$(MAKE) -C fat clean
	$(MAKE) -C vfs clean
	$(MAKE) -C ext3 clean
	$(MAKE) -C isofs clean
	$(MAKE) -C devfs clean
	$(MAKE) -C reiserfs clean
	$(MAKE) -C journal clean
	$(MAKE) -C icfs clean
	$(MAKE) -C kfs clean
