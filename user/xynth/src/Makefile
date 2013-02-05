
include ../Makefile.cfg
include ../Makefile.parse

DIR = src/

SUBDIR-Y += fonts
SUBDIR-Y += configs
SUBDIR-Y += lib
SUBDIR-$(WIDGET_LIB) += widget
SUBDIR-Y += ../demo/desktop
SUBDIR-Y += ../demo/term
SUBDIR-$(THEME_PLUGIN)   += themes
SUBDIR-Y += server

include ../Makefile.rules

dist:
	@$(foreach F, $(SUBDIR-Y), $(MAKE) dist -C $F || exit 1;)
