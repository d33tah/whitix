CFLAGS =

export CFLAGS

all:
	#System
	$(MAKE) -C libc pre
	$(MAKE) -C linker liblinker.so
	$(MAKE) -C posix libpthread.so
	$(MAKE) -C libc
	$(MAKE) -C linker/dl
	$(MAKE) -C posix
	$(MAKE) -C sdk

	#Applications

	$(MAKE) -C burn
	$(MAKE) -C fruity
	$(MAKE) -C makefs
	$(MAKE) -C xynth
	$(MAKE) -C system
	$(MAKE) -C grub
#	$(MAKE) -C wdb
	
	$(MAKE) -C sdk python

	$(MAKE) ports

ports:
	touch portlist # Create if not created already.
	while read dir junk; do \
		$(MAKE) -C $$dir; \
	done < portlist

$(PORTS):
	$(MAKE) -C $@

.PHONY: ports $(PORTS)

ports_install:
	while read dir junk; do \
		$(MAKE) -C $$dir install; \
	done < portlist

.PHONY: ports_install

ports_clean:
	for dir in $(PORTS); do \
		$(MAKE) -C $$dir clean; \
	done

install:
	#System
	$(MAKE) -C libc install
	$(MAKE) -C linker install
	$(MAKE) -C sdk install
	$(MAKE) -C posix install

	$(MAKE) -C grub install
	$(MAKE) -C burn install
	$(MAKE) -C fruity install
	$(MAKE) -C makefs install
	$(MAKE) -C xynth install
	$(MAKE) -C system install
#	$(MAKE) -C wdb install
	$(MAKE) ports_install

clean:
	$(MAKE) -C libc clean
	$(MAKE) -C linker clean
	$(MAKE) -C burn clean
	$(MAKE) -C fruity clean
	$(MAKE) -C posix clean
	$(MAKE) -C makefs clean
	$(MAKE) -C sdk clean
	$(MAKE) -C xynth clean
	$(MAKE) -C system clean
	$(MAKE) -C grub clean
#	$(MAKE) -C wdb clean
	$(MAKE) ports_clean
