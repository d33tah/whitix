build:
	$(MAKE) -C console
	$(MAKE) -C network
	$(MAKE) -C thread
	$(MAKE) -C file
	$(MAKE) -C registry

python:
	$(MAKE) -C python

clean:
	$(MAKE) -C console clean
	$(MAKE) -C network clean
	$(MAKE) -C thread clean
	$(MAKE) -C file clean
	$(MAKE) -C registry clean
	$(MAKE) -C python clean
	rm -f libs/*.so

install:
	cp include/*.h ../../CdRoot/System/Include
	cp libs/*.so ../../CdRoot/System/Runtime
	$(MAKE) -C python install
	
.PHONY: python
