
include Makefile.cfg
include Makefile.parse

.PHONY := all clean dist install optimize doxy-doc xynth.pc test strip update pspdev

all:
	@$(MAKE) -C src

clean:
	@$(MAKE) --no-print-directory clean -C src
	@$(MAKE) --no-print-directory clean -C tools/theme
	@$(MAKE) --no-print-directory clean -C tools/optimize
	rm -f xynth.pc
	rm -rf dist
	rm -rf doc

dist: all xynth.pc
	rm -rf dist
	mkdir -p $(DISTTOPDIR)
	mkdir -p $(DISTINCDIR)
	mkdir -p $(DISTLIBDIR)
	mkdir -p $(DISTBINDIR)
	mkdir -p $(DISTCONFDIR)
	mkdir -p $(DISTFONTDIR)
	mkdir -p $(DISTTHEMEDIR)
	@$(MAKE) --no-print-directory dist -C src
	mkdir -p $(DISTPKGCONFIG)
	cp xynth.pc $(DISTPKGCONFIG)/xynth.pc

install:
	mkdir -p ../../CdRoot/Applications/Xynth
	mkdir -p ../../CdRoot/Applications/Xynth/Data
	mkdir -p ../../CdRoot/Applications/Xynth/Data/Themes
	mkdir -p ../../CdRoot/Applications/Xynth/Data/Conf
	mkdir -p ../../CdRoot/Applications/Xynth/Data/Fonts
	cp src/server/xynth ../../CdRoot/Applications/xy
	cp src/widget/libwidget.so ../../CdRoot/System/Runtime
	cp src/lib/libxynth.so ../../CdRoot/System/Runtime
	cp src/configs/xynth.conf ../../CdRoot/Applications/Xynth/Data/Conf
	cp src/themes/silverado/silverado.so ../../CdRoot/Applications/Xynth/Data/Themes
	cp src/fonts/* ../../CdRoot/Applications/Xynth/Data/Fonts
	cp demo/desktop/desktop ../../CdRoot/Applications
	mkdir -p ../../CdRoot/Applications/Xynth/Data/desktop/cfg
	mkdir -p ../../CdRoot/Applications/Xynth/Data/desktop/img
	cp demo/term/term ../../CdRoot/Applications/
	cp demo/desktop/cfg ../../CdRoot/Applications/Xynth/Data/desktop -Rf
	cp demo/desktop/img ../../CdRoot/Applications/Xynth/Data/desktop -Rf

optimize:
	$(MAKE) -C tools/optimize

doxy-doc:
	rm -rf doc
	EXCLUDE_PATTERNS='*/dist/* \
	                  */demo/* \
			  */tools/* \
			  */ports/* \
			  */themes/* \
			  */libz/* \
			  */libpng/* \
			  */freetype2/* \
			  */expat/* \
			  */pixman/* \
			  */lrmi-*/* \
			  */pthread_w32/*' \
	SRCDIR='.' \
	PROJECT='xynth' \
	DOCDIR='doc' \
	VERSION='0.8.40' \
	PERL_PATH='perl' \
	HAVE_DOT='NO' \
	GENERATE_MAN='NO' \
	GENERATE_RTF='NO' \
	GENERATE_XML='NO' \
	GENERATE_HTMLHELP='NO' \
	GENERATE_CHI='NO' \
	GENERATE_HTML='YES' \
	GENERATE_LATEX='NO' \
	doxygen ./doxygen.cfg

xynth.pc: Makefile.parse Makefile.cfg
	@rm -rf xynth.pc
	@echo > subs.sed
	@echo "s,@prefix@,${INSTALLDIR},g"     >> subs.sed
	@echo "s,@version@,${XYNTH_VERSION},g" >> subs.sed
ifeq ($(WIDGET_LIB), Y)
	@echo "s,@widget_libs@,-lwidget,g"     >> subs.sed
endif
	@sed -f subs.sed $@.in > $@
	@rm -rf subs.sed

test: clean all dist install

update: clean
	svn update

strip: dist
	@$(foreach F, $(wildcard $(DISTBINDIR)/*), $(STRIP) $(F) > /dev/null 2>&1;)
	@$(foreach F, $(wildcard $(DISTLIBDIR)/lib*), $(STRIP) $(F) > /dev/null 2>&1;)
	@$(foreach F, $(wildcard $(DISTTHEMEDIR)/*), $(STRIP) $(F) > /dev/null 2>&1;)

pspdev:
	mount /mnt/sdb1
	cp -Rf dist/psp/game/*xynth* /mnt/sdb1/psp/game/
	umount /mnt/sdb1
