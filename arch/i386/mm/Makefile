DEPTH = ../../../

OBJS = init.o virt.o
DEPS := $(OBJS:%.o=.deps/%.d)

build: $(OBJS)

include $(DEPTH)make.inc
-include $(DEPS)
