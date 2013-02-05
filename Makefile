#===============================#
#=   CONFIGURATION OPTIONS     =#
#===============================#

# Change this to build for a different arch
ARCH = i386

#Change to N to produce a smaller kernel if debug functionality is not required
CONFIG_ALLSYMS = Y
DEBUG_FILE = N

ifeq ($(DEBUG_FILE), Y)
	POST_BUILD = objcopy --only-keep-debug kern kern.debug; strip kern; objcopy --add-gnu-debuglink=kern.debug kern
else
	POST_BUILD = strip -g kern;
endif

PWD = $(shell pwd)
INCLUDE_DIR := $(PWD)/include
override CFLAGS += -I$(INCLUDE_DIR)
CC = gcc
TAGS = ctags -I SYMBOL_EXPORT -I PACKED
override GENISO_FLAGS += -c Boot/boot.catalog -R -input-charset iso8859-1 -b Boot/Grub/stage2_eltorito -m'.svn' -no-emul-boot -boot-load-size 4 -boot-info-table -iso-level 2 -l CdRoot

#Make sure all makefiles can see everything
export ARCH

# All directories that are to be built or cleaned.
KERNEL_SUBDIRS = arch/$(ARCH) devices fs kernel lib memory net video
SUBDIRS = $(KERNEL_SUBDIRS) user

DIRMODULES = CdRoot/System/Modules CdRoot/System/Modules/Core CdRoot/System/Modules/Input CdRoot/System/Modules/Network CdRoot/System/Modules/Storage CdRoot/System/Modules/Filesystems CdRoot/System/Modules/Video
DIRLIST = CdRoot CdRoot/Boot CdRoot/Boot/Grub CdRoot/Applications CdRoot/Mount CdRoot/System/Devices CdRoot/System/Include CdRoot/System/Runtime CdRoot/System/Runtime/C CdRoot/System/Startup/ CdRoot/System/Config

all: cd	
	mkdir -p $(DIRLIST)
	$(MAKE) -C user install
	genisoimage -o cd.iso $(GENISO_FLAGS)
	@echo "====================================="
	@echo "|     Whitix built successfully.    |"
	@echo "| You can now run Whitix by using   |"
	@echo "| 'cd.iso' as your CD image. Enjoy! |"
	@echo "====================================="

floppy: kern
	
ifneq ($(CONFIG_ALLSYMS),Y)
	strip -s kern
endif

cd: kern
	mkdir -p CdRoot/Boot
	cp kern CdRoot/Boot/Kernel
	mv CdRoot/System/Modules/Filesystems/cdfs.sys CdRoot/System/Modules/Core/cdfs.sys
	mv CdRoot/System/Modules/Storage/ata_ide.sys CdRoot/System/Modules/Core/ata_ide.sys
	mv CdRoot/System/Modules/Video/console.sys CdRoot/System/Modules/Core/
	
	nm kern | sort -u > kernel.txt

ifneq ($(CONFIG_ALLSYMS),Y)
	strip -s kern -R .comment
endif

kern: subdirs
	ld -M -T link.ld arch/$(ARCH)/boot/*.o arch/$(ARCH)/acpi/*.o arch/$(ARCH)/lib/*.o arch/$(ARCH)/kernel/*.o \
	 arch/$(ARCH)/mm/*.o fs/icfs/*.o fs/kfs/*.o fs/vfs/*.o fs/devfs/*.o \
	 devices/kedev/*.o devices/acpi/*.o devices/misc/*.o kernel/*.o lib/*.o memory/*.o video/*.o \
	  -o kern > kernel.txt
	$(POST_BUILD)
	mkdir -p $(DIRMODULES)
	$(MAKE) -C net modules_install
	$(MAKE) -C fs modules_install
	$(MAKE) -C devices modules_install
	$(MAKE) -C memory modules_install
	$(MAKE) -C video modules_install

subdirs:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done;

$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: subdirs $(SUBDIRS)

clean:
	$(MAKE) -C arch/$(ARCH) clean
	$(MAKE) -C devices clean
	$(MAKE) -C fs clean
	$(MAKE) -C kernel clean
	$(MAKE) -C lib clean
	$(MAKE) -C net clean
	$(MAKE) -C video clean
	$(MAKE) -C memory clean
	$(MAKE) -C user clean
	rm -f cd.iso kern out.txt kernel.txt
	rm -rf CdRoot

#TODO: Update
help:
	@echo =====================
	@echo = Whitix build help =
	@echo =====================
	@echo -
	@echo "all - build the kernel and userspace"
	@echo "floppy - build the kernel and construct a floppy image"
	@echo "clean - remove all intermediate files"
	@echo "See the readme file for more help"

tags:
	find $(KERNEL_SUBDIRS) include -name "*.[ch]" | $(TAGS) -L -

# Phony targets.
.PHONY: clean help cd tags
