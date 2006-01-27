# Makefile fragment for XeTeX and web2c. -- Jonathan Kew -- Public domain.
# This fragment contains the parts of the makefile that are most likely to
# differ between releases of XeTeX.

Makefile: $(srcdir)/xetexdir/xetex.mk

target = @target@
target_os = @target_os@
target_cpu = @target_cpu@
target_vendor = @target_vendor@

# We build etex.
xetex = @XETEX@ xetex

# Platform specific defines and files to be built
ifeq '$(target_vendor)' 'apple' # or should we be testing target_os???

xetex_platform_o = XeTeX_mac.o XeTeXFontMgr_Mac.o
xetex_platform_layout_o = XeTeXFontInst_Mac.o
xetex_platform_layout_cxx = XeTeXFontInst_Mac.cpp

XETEX_DEFINES = -DXETEX_MAC

# System frameworks we link with on Mac OS X
FRAMEWORKS = -framework Carbon -framework Cocoa -framework QuickTime

else # not apple, currently assume a Linux-like system with X11, etc... no W32 option yet

#Linux is linux-gnu, ought to be same defines/files for other X11 systems like BSDs
xetex_platform_o = XeTeXFontMgr_Linux.o
xetex_platform_layout_o = XeTeXFontInst_FC.o
xetex_platform_layout_cxx = XeTeXFontInst_FC.cpp

XETEX_DEFINES = -DXETEX_OTHER

# FIXME: this should be handled through configure
# assumes FreeType, FontConfig, ImageMagick are available
FRAMEWORKS = -lfreetype -lfontconfig `Wand-config  --ldflags --libs`

endif

XDEFS = $(XETEX_DEFINES)

# Extract xetex version
xetexdir/xetex.version: $(srcdir)/xetexdir/xetex-new.ch
	test -d xetexdir || mkdir xetexdir
	grep '^@d XeTeX_version_string==' $(srcdir)/xetexdir/xetex-new.ch \
	  | sed "s/^.*'-//;s/'.*$$//" >xetexdir/xetex.version

# The C sources.
xetex_c = xetexini.c xetex0.c xetex1.c xetex2.c
xetex_o = xetexini.o xetex0.o xetex1.o xetex2.o xetexextra.o trans.o XeTeX_ext.o $(xetex_platform_o)

# Layout library sources
xetex_ot_layout_o = \
		XeTeXFontMgr.o \
		XeTeXLayoutInterface.o XeTeXOTLayoutEngine.o \
		XeTeXFontInst.o cmaps.o FontObject.o FontTableCache.o \
		$(xetex_platform_layout_o) 
xetex_ot_layout_cxx = \
		XeTeXFontMgr.cpp \
		XeTeXLayoutInterface.cpp XeTeXOTLayoutEngine.cpp \
		XeTeXFontInst.cpp cmaps.cpp FontObject.cpp FontTableCache.cpp \
		$(xetex_platform_layout_cxx)

icudir = icu-release-3-4-source

ICUCFLAGS = \
	-I../../libs/$(icudir)/common/unicode \
	-I../../libs/$(icudir)/common \
	-I../../../../TeX/libs/$(icudir)/common/unicode \
	-I../../../../TeX/libs/$(icudir)/common \
	-I../../../../TeX/libs/$(icudir)/layout/unicode \
	-I../../../../TeX/libs/$(icudir)/layout \
	-I../../../../TeX/libs/$(icudir) \
	-DLE_USE_CMEMORY

# FIXME: this should be handled through configure
FTFLAGS =  -I/usr/include/freetype2/

TECkitFLAGS = -I../../../../TeX/libs/teckit/source/Public-headers/
XCFLAGS = $(TECkitFLAGS) # actually, we need this for most compiles

XeTeXLayoutInterface.o: $(srcdir)/xetexdir/XeTeXLayoutInterface.cpp
	$(CXX) $(ICUCFLAGS) $(FTFLAGS) $(ALL_CXXFLAGS) $(DEFS) -c $< -o $@
XeTeXOTLayoutEngine.o: $(srcdir)/xetexdir/XeTeXOTLayoutEngine.cpp
	$(CXX) $(ICUCFLAGS) $(FTFLAGS) $(ALL_CXXFLAGS) $(DEFS) -c $< -o $@

XeTeXFontMgr.o: $(srcdir)/xetexdir/XeTeXFontMgr.cpp $(srcdir)/xetexdir/XeTeXFontMgr.h
	$(CXX) $(ICUCFLAGS) $(FTFLAGS) $(ALL_CXXFLAGS) $(DEFS) -c $< -o $@
XeTeXFontMgr_Linux.o: $(srcdir)/xetexdir/XeTeXFontMgr_Linux.cpp $(srcdir)/xetexdir/XeTeXFontMgr.h
	$(CXX) $(ICUCFLAGS) $(FTFLAGS) $(ALL_CXXFLAGS) $(DEFS) -c $< -o $@

# Changed this for the Cocoa version of the Mac font manager
# XeTeXFontMgr_Mac.o: $(srcdir)/xetexdir/XeTeXFontMgr_Mac.cpp $(srcdir)/xetexdir/XeTeXFontMgr.h
#	$(CXX) $(ICUCFLAGS) $(FTFLAGS) $(ALL_CXXFLAGS) $(DEFS) -c $< -o $@
XeTeXFontMgr_Mac.o: $(srcdir)/xetexdir/XeTeXFontMgr_Mac.mm $(srcdir)/xetexdir/XeTeXFontMgr.h
	gcc -ObjC++ $(ICUCFLAGS) $(FTFLAGS) $(ALL_CXXFLAGS) $(DEFS) -c $< -o $@

cmaps.o: $(srcdir)/xetexdir/cmaps.cpp
	$(CXX) $(ICUCFLAGS) $(ALL_CXXFLAGS) $(DEFS) -c $< -o $@
FontObject.o: $(srcdir)/xetexdir/FontObject.cpp
	$(CXX) $(ICUCFLAGS) $(ALL_CXXFLAGS) $(DEFS) -c $< -o $@
FontTableCache.o: $(srcdir)/xetexdir/FontTableCache.cpp
	$(CXX) $(ICUCFLAGS) $(ALL_CXXFLAGS) $(DEFS) -c $< -o $@

XeTeXFontInst.o: $(srcdir)/xetexdir/XeTeXFontInst.cpp
	$(CXX) $(ICUCFLAGS) $(FTFLAGS) $(ALL_CXXFLAGS) $(DEFS) -c $< -o $@
XeTeXFontInst_Mac.o: $(srcdir)/xetexdir/XeTeXFontInst_Mac.cpp
	$(CXX) $(ICUCFLAGS) $(ALL_CXXFLAGS) $(DEFS) -c $< -o $@
XeTeXFontInst_FC.o: $(srcdir)/xetexdir/XeTeXFontInst_FC.cpp
	$(CXX) $(ICUCFLAGS) $(FTFLAGS) $(ALL_CXXFLAGS) $(DEFS) -c $< -o $@


xetexlibs = \
	../../libs/$(icudir)/lib/libsicuuc.a \
	../../libs/$(icudir)/lib/libsicule.a \
	../../libs/$(icudir)/lib/libsicudata.a \
	../../libs/teckit/lib/.libs/libTECkit.a

../../libs/$(icudir)/lib/libsicuuc.a \
../../libs/$(icudir)/lib/libsicule.a \
../../libs/$(icudir)/lib/libsicudata.a:
	(cd ../../libs/$(icudir) ; make)

../../libs/teckit/lib/.libs/libTECkit.a:
	(cd ../../libs/teckit ; make)

# special rules for files that need the TECkit headers as well
XeTeX_ext.o: $(srcdir)/xetexdir/XeTeX_ext.c xetexd.h
	$(compile) $(ICUCFLAGS) $(FTFLAGS) $(TECkitFLAGS) $(ALL_CFLAGS) $(DEFS) -c $< -o $@
XeTeX_mac.o: $(srcdir)/xetexdir/XeTeX_mac.c xetexd.h
	$(compile) $(ICUCFLAGS) $(TECkitFLAGS) $(ALL_CFLAGS) $(DEFS) -c $< -o $@

trans.o: $(srcdir)/xetexdir/trans.c
	$(compile) $(ALL_CFLAGS) $(DEFS) -c $< -o $@

# Making xetex.
xetex: $(xetex_ot_layout_o) $(xetex_o) $(xetexlibs)
	$(kpathsea_cxx_link) $(xetex_o) $(xetex_ot_layout_o) $(socketlibs) $(LOADLIBES) \
	$(xetexlibs) $(FRAMEWORKS) -lz

# C file dependencies
$(xetex_c) xetexcoerce.h xetexd.h: xetex.p $(web2c_texmf)
	$(web2c) xetex
xetexextra.c: lib/texmfmp.c xetexdir/xetexextra.h
	sed s/TEX-OR-MF-OR-MP/xetex/ $(srcdir)/lib/texmfmp.c >$@
xetexdir/xetexextra.h: xetexdir/xetexextra.in xetexdir/xetex.version
	test -d xetexdir || mkdir xetexdir
	sed s/XETEX-VERSION/`cat xetexdir/xetex.version`/ \
	  $(srcdir)/xetexdir/xetexextra.in >$@

# Tangling
xetex.p xetex.pool: ./otangle xetex.web # xetex.ch
	./otangle xetex.web # xetex.ch

# Generation of the web [and ch] file.
#   Sources for xetex.web:
xetex_web_srcs = $(srcdir)/tex.web \
  $(srcdir)/etexdir/etex.ch \
  $(srcdir)/etexdir/tex.ch0 \
  $(srcdir)/tex.ch \
  $(srcdir)/etexdir/tex.ch1 \
  $(srcdir)/etexdir/tex.ech \
  $(srcdir)/xetexdir/xetex-new.ch \
  $(srcdir)/xetexdir/xetex-noenc.ch \
  $(srcdir)/xetexdir/xetex-upwards.ch
xetex.web: tie xetexdir/xetex.mk $(xetex_web_srcs)
	$(TIE) -m xetex.web $(xetex_web_srcs)

#   Sources for etex.ch:
#etex_ch_srcs = etex.web \
#  $(srcdir)/etexdir/tex.ch0 \
#  $(srcdir)/tex.ch \
#  $(srcdir)/etexdir/tex.ch1 \
#  $(srcdir)/etexdir/tex.ech
#   Rules:
#etex.web: tie etexdir/etex.mk $(etex_web_srcs)
#	$(TIE) -m etex.web $(etex_web_srcs)
#etex.ch: $(etex_ch_srcs)
#	$(TIE) -c etex.ch $(etex_ch_srcs)

################## FIXME: the rest of this isn't properly updated for xetex yet...
##################        e.g., we don't have real xetex tests to run!

# Tests...
check: @XETEX@ xetex-check
xetex-check: etrip xetex.fmt
# Test truncation (but don't bother showing the warning msg).
	./xetex --progname=xetex --output-comment="`cat $(srcdir)/PROJECTS`" \
	  $(srcdir)/tests/hello 2>/dev/null \
	  && ./dvitype hello.dvi | grep olaf@infovore.xs4all.nl >/dev/null
# \openout should show up in \write's.
	./xetex --progname=xetex $(srcdir)/tests/openout && grep xfoo openout.log
# one.two.tex -> one.two.log
	./xetex --progname=xetex $(srcdir)/tests/one.two && ls -l one.two.log
# uno.dos -> uno.log
	./xetex --progname=xetex $(srcdir)/tests/uno.dos && ls -l uno.log
	./xetex --progname=xetex $(srcdir)/tests/just.texi && ls -l just.log
	-./xetex --progname=xetex $(srcdir)/tests/batch.tex
	./xetex --progname=xetex --shell $(srcdir)/tests/write18 | grep echo
# tcx files are a bad idea.
#	./etex --translate-file=$(srcdir)/share/isol1-t1.tcx \
#	  $(srcdir)/tests/eight && ./dvitype eight.dvi >eigh.typ
	./xetex --mltex --progname=xeinitex $(srcdir)/tests/mltextst
	-./xetex --progname=xetex </dev/null
	-PATH=`pwd`:$(kpathsea_dir):$(kpathsea_srcdir):$$PATH \
	  WEB2C=$(kpathsea_srcdir) TMPDIR=.. \
	  ./xetex --progname=xetex '\nonstopmode\font\foo=nonesuch\end'

# Cleaning up.
clean:: xetex-clean
xetex-clean: # etrip-clean
	$(LIBTOOL) --mode=clean $(RM) xetex
	rm -f $(xetex_o) $(xetex_c) xetexextra.c xetexcoerce.h xetexd.h
	rm -f xetexdir/xetexextra.h xetexdir/xetex.version
	rm -f xetex.p xetex.pool xetex.web xetex.ch
	rm -f xetex.fmt xetex.log
	rm -f hello.dvi hello.log xfoo.out openout.log one.two.log uno.log
	rm -f just.log batch.log write18.log mltextst.log texput.log
	rm -f missfont.log
	rm -rf tfm

# etrip
#etestdir = $(srcdir)/etexdir/etrip
#etestenv = TEXMFCNF=$(etestdir)

#triptrap: @XETEX@ etrip
#etrip: pltotf tftopl etex dvitype etrip-clean
#	@echo ">>> See $(etestdir)/etrip.diffs for example of acceptable diffs." >&2
#	@echo "*** TRIP test for e-TeX in compatibility mode ***."
#	./pltotf $(testdir)/trip.pl trip.tfm
#	./tftopl ./trip.tfm trip.pl
#	-diff $(testdir)/trip.pl trip.pl
#	$(LN) $(testdir)/trip.tex . # get same filename in log
#	-$(SHELL) -c '$(etestenv) ./etex --progname=einitex --ini <$(testdir)/trip1.in >ctripin.fot'
#	mv trip.log ctripin.log
#	-diff $(testdir)/tripin.log ctripin.log
#	-$(SHELL) -c '$(etestenv) ./etex --progname=etex <$(testdir)/trip2.in >ctrip.fot'
#	mv trip.log ctrip.log
#	-diff $(testdir)/trip.fot ctrip.fot
#	-$(DIFF) $(DIFFFLAGS) $(testdir)/trip.log ctrip.log
#	$(SHELL) -c '$(etestenv) ./dvitype $(dvitype_args) trip.dvi >ctrip.typ'
#	-$(DIFF) $(DIFFFLAGS) $(testdir)/trip.typ ctrip.typ
#	@echo "*** TRIP test for e-TeX in extended mode ***."
#	-$(SHELL) -c '$(etestenv) ./etex --progname=einitex --ini <$(etestdir)/etrip1.in >xtripin.fot'
#	mv trip.log xtripin.log
#	-diff ctripin.log xtripin.log
#	-$(SHELL) -c '$(etestenv) ./etex --progname=etex <$(etestdir)/trip2.in >xtrip.fot'
#	mv trip.log xtrip.log
#	-diff ctrip.fot xtrip.fot
#	-$(DIFF) $(DIFFFLAGS) ctrip.log xtrip.log
#	$(SHELL) -c '$(etestenv) ./dvitype $(dvitype_args) trip.dvi >xtrip.typ'
#	-$(DIFF) $(DIFFFLAGS) ctrip.typ xtrip.typ
#	@echo "*** e-TeX specific part of e-TRIP test ***."
#	./pltotf $(etestdir)/etrip.pl etrip.tfm
#	./tftopl ./etrip.tfm etrip.pl
#	-diff $(etestdir)/etrip.pl etrip.pl
#	$(LN) $(etestdir)/etrip.tex . # get same filename in log
#	-$(SHELL) -c '$(etestenv) ./etex --progname=einitex --ini <$(etestdir)/etrip2.in >etripin.fot'
#	mv etrip.log etripin.log
#	-diff $(etestdir)/etripin.log etripin.log
#	-$(SHELL) -c '$(etestenv) ./etex --progname=etex <$(etestdir)/etrip3.in >etrip.fot'
#	-diff $(etestdir)/etrip.fot etrip.fot
#	-$(DIFF) $(DIFFFLAGS) $(etestdir)/etrip.log etrip.log
#	diff $(etestdir)/etrip.out etrip.out
#	$(SHELL) -c '$(etestenv) ./dvitype $(dvitype_args) etrip.dvi >etrip.typ'
#	-$(DIFF) $(DIFFFLAGS) $(etestdir)/etrip.typ etrip.typ

# Cleaning up for the etrip.
#etrip-clean:
#	rm -f trip.tfm trip.pl trip.tex trip.fmt ctripin.fot ctripin.log
#	rm -f ctrip.fot ctrip.log trip.dvi ctrip.typ
#	rm -f xtripin.fot xtripin.log
#	rm -f xtrip.fot xtrip.log xtrip.typ
#	rm -f etrip.tfm etrip.pl etrip.tex etrip.fmt etripin.fot etripin.log
#	rm -f etrip.fot etrip.log etrip.dvi etrip.out etrip.typ
#	rm -f tripos.tex 8terminal.tex
#	rm -rf tfm

# Distfiles ...
#@MAINT@triptrapdiffs: etexdir/etrip/etrip.diffs
#@MAINT@etexdir/etrip/etrip.diffs: etex
#@MAINT@	$(MAKE) etrip | tail +1 >etexdir/etrip/etrip.diffs


# Dumps
all_xefmts = xetex.fmt $(xefmts)

dumps: @XETEX@ xefmts
xefmts: $(all_xefmts)

xefmtdir = $(web2cdir)/xetex
$(xefmtdir)::
	$(SHELL) $(top_srcdir)/../mkinstalldirs $(xefmtdir)

xetex.fmt: xetex
	$(dumpenv) $(MAKE) progname=xetex files="xetex.src plain.tex cmr10.tfm" prereq-check
	$(dumpenv) ./xetex --progname=xetex --jobname=xetex --ini \*\\input xetex.src \\dump </dev/null

xelatex.fmt: xetex
	$(dumpenv) $(MAKE) progname=xelatex files="latex.ltx" prereq-check
	$(dumpenv) ./xetex --progname=xelatex --jobname=xelatex --ini \*\\input latex.ltx </dev/null


# Install
install-xetex: install-xetex-exec install-xetex-data
install-xetex-exec: install-xetex-programs install-xetex-links
install-xetex-data: install-xetex-pool @FMU@ install-xetex-dumps
install-xetex-dumps: install-xetex-fmts

install-programs: @XETEX@ install-xetex-programs
install-xetex-programs: xetex $(bindir)
	for p in xetex; do $(INSTALL_LIBTOOL_PROG) $$p $(bindir); done

install-links: @XETEX@ install-xetex-links
install-xetex-links: install-xetex-programs
	#cd $(bindir) && (rm -f xeinitex xevirtex; \
	#  $(LN) xetex xeinitex; $(LN) xetex xevirtex)

install-fmts: @XETEX@ install-xetex-fmts
install-xetex-fmts: xefmts $(xefmtdir)
	xefmts="$(all_xefmts)"; \
	  for f in $$xefmts; do $(INSTALL_DATA) $$f $(xefmtdir)/$$f; done
	xefmts="$(xefmts)"; \
	  for f in $$xefmts; do base=`basename $$f .fmt`; \
	    (cd $(bindir) && (rm -f $$base; $(LN) xetex $$base)); done

install-data:: @XETEX@ install-xetex-data
install-xetex-pool: xetex.pool $(texpooldir)
	$(INSTALL_DATA) xetex.pool $(texpooldir)/xetex.pool

# end of xetex.mk
