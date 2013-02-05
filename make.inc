#Define DEPTH before including this file

.PHONY: clean
.IGNORE: clean

CC = gcc
CPP = gpp
ASM = nasm
RM  = rm -f
SED = sed
override CFLAGS += -g -ffreestanding -fno-stack-protector -fno-builtin \
	-nostdlib -m32 -Wall -Wextra -Os -I$(DEPTH)include -Wno-unused-parameter
BE_VERBOSE = 0
LD_R_FLAGS = -r -melf_i386

# Build the dependency list
DEPS := $(MODULES:%.sys=.deps/%.d)
DEPS += $(OBJS:%.o=.deps/%.d)

%.o : %.c
	@echo "  \033[32mCC $*.c"
	@tput sgr0
	@$(CC) $(CFLAGS) -c $*.c -o $*.o
	@$(CC) -MM $(CFLAGS) -c $*.c > .deps/$*.d

%.sys : %.c
	@echo "  \033[32mCC $*.c"
	@tput sgr0
	@$(CC) $(CFLAGS) -DMODULE -fno-common -c $*.c -o $*.sys
	@$(CC) -MM $(CFLAGS) -DMODULE -c $*.c > .deps/$*.d
	@$(SED) -i "s/$*.o/$*.sys/" .deps/$*.d

#gas assembly files
%.o : %.S
	$(CC) -I$(DEPTH)include -ffreestanding -fleading-underscore -fno-builtin -m32 -c $*.S -o $*.o
#	@echo "AS $*.S"

ifneq ($(IGNORE_CLEAN), y)
clean:
	-rm -f *.o *.sys .deps/*.d
endif
