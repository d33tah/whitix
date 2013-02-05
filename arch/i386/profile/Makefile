DEPTH=../../../

MODULES := timer_rtc.sys

build: $(MODULES)
	ld $(LD_R_FLAGS) $(MODULES) -o profile.sys

include $(DEPTH)make.inc
-include $(DEPS)
