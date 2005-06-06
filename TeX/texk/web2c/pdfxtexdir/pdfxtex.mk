# Makefile fragment for pdfxTeX and web2c. --infovore@xs4all.nl. Public domain.
# This fragment contains the parts of the makefile that are most likely to
# differ between releases of pdfxTeX.

Makefile: $(srcdir)/pdfxtexdir/pdfxtex.mk

# We build pdfxtex
pdfxtex = @PXTEX@ pdfxtex
pdfxtexdir = pdfxtexdir

# The C sources.
pdfxtex_c = pdfxtexini.c pdfxtex0.c pdfxtex1.c pdfxtex2.c pdfxtex3.c
pdfxtex_o = pdfxtexini.o pdfxtex0.o pdfxtex1.o pdfxtex2.o pdfxtex3.o pdfxtexextra.o

# Making pdfxtex
pdfxtex: pdftexd.h $(pdfxtex_o) $(pdfxtexextra_o) $(pdftexlibsdep)
	@CXXHACKLINK@ $(pdfxtex_o) $(pdfxtexextra_o) $(pdftexlibs) $(socketlibs) @CXXHACKLDLIBS@ @CXXLDEXTRA@

# C file dependencies.
$(pdfxtex_c) pdfxtexcoerce.h pdfxtexd.h: pdfxtex.p $(web2c_texmf)
	$(web2c) pdfxtex
pdfxtexextra.c: lib/texmfmp.c
	sed s/TEX-OR-MF-OR-MP/pdfxtex/ $(srcdir)/lib/texmfmp.c >$@
pdfxtexdir/pdfxtexextra.h: pdfxtexdir/pdfxtexextra.in pdftexdir/pdftex.version etexdir/etex.version
	sed -e s/PDFTEX-VERSION/`cat pdftexdir/pdftex.version`/ \
	    -e s/ETEX-VERSION/`cat etexdir/etex.version`/ \
	  $(srcdir)/pdfxtexdir/pdfxtexextra.in >$@

# Tangling
pdfxtex.p pdfxtex.pool: tangle pdfxtex.web pdfxtex.ch
	$(TANGLE) pdfxtex.web pdfxtex.ch

# Generation of the web and ch file.
#   Sources for pdfxtex.web:
pdfxtex_web_srcs = $(srcdir)/tex.web \
  $(srcdir)/etexdir/etex.ch \
  $(srcdir)/etexdir/etex.fix \
  $(srcdir)/pdfetexdir/pdfetex.ch1 \
  $(srcdir)/pdftexdir/pdftex.ch \
  $(srcdir)/pdftexdir/hz.ch \
  $(srcdir)/pdftexdir/misc.ch \
  $(srcdir)/pdftexdir/vadjust.ch \
  $(srcdir)/pdfxtexdir/strcmp.ch \
  $(srcdir)/pdfetexdir/pdfetex.ch2 \
  $(srcdir)/pdfxtexdir/pdfxtex.ch2
#   Sources for pdfxtex.ch:
pdfxtex_ch_srcs = pdfxtex.web \
  $(srcdir)/pdfxtexdir/tex.ch0 \
  $(srcdir)/tex.ch \
  $(srcdir)/etexdir/tex.ch1 \
  $(srcdir)/etexdir/tex.ech \
  $(srcdir)/pdfxtexdir/tex.ch1 \
  $(srcdir)/pdftexdir/tex.pch
#   Rules:
pdfxtex.web: tie pdfxtexdir/pdfxtex.mk $(pdfxtex_web_srcs)
	$(TIE) -m pdfxtex.web $(pdfxtex_web_srcs)
pdfxtex.ch: $(pdfxtex_ch_srcs)
	$(TIE) -c pdfxtex.ch $(pdfxtex_ch_srcs)

$(srcdir)/pdfxtexdir/pdfxtex.h: $(srcdir)/pdftexdir/pdftex.h
	cp -f $(srcdir)/pdftexdir/pdftex.h $@

$(srcdir)/pdfxtexdir/pdfxtex.defines: $(srcdir)/pdftexdir/pdftex.defines
	cp -f $(srcdir)/pdftexdir/pdftex.defines $@

check: @PXTEX@ pdfxtex-check
pdfxtex-check: pdfxtex pdfxtex.fmt

clean:: pdfxtex-clean
pdfxtex-clean:
	$(LIBTOOL) --mode=clean $(RM) pdfxtex
	rm -f $(pdfxtex_o) $(pdfxtex_c) pdfxtexextra.c pdfxtexcoerce.h
	rm -f pdfxtexdir/pdfxtexextra.h
	rm -f pdfxtexd.h pdfxtex.p pdfxtex.pool pdfxtex.web pdfxtex.ch
	rm -f pdfxtex.fmt pdfxtex.log

# Dumps
all_pdfxfmts = pdfxtex.fmt $(pdfxfmts)

dumps: @PXTEX@ pdfxfmts
pdfxfmts: $(all_pdfxfmts)

pdfxtex.fmt: pdfxtex
	$(dumpenv) $(MAKE) progname=pdfxtex files="etex.src plain.tex cmr10.tfm" prereq-check
	$(dumpenv) ./pdfxtex --progname=pdfxtex --jobname=pdfxtex --ini \*\\pdfoutput=1\\input etex.src \\dump </dev/null

pdfxlatex.fmt: pdfxtex
	$(dumpenv) $(MAKE) progname=pdfxlatex files="latex.ltx" prereq-check
	$(dumpenv) ./pdfxtex --progname=pdfxlatex --jobname=pdfxlatex --ini \*\\pdfoutput=1\\input latex.ltx </dev/null

#pdflatex.fmt: pdfxtex
#	$(dumpenv) $(MAKE) progname=pdflatex files="latex.ltx" prereq-check
#	$(dumpenv) ./pdfxtex --progname=pdflatex --jobname=pdflatex --ini \*\\pdfoutput=1\\input latex.ltx </dev/null


# Installation.
install-pdfxtex: install-pdfxtex-exec install-pdfxtex-data
install-pdfxtex-exec: install-pdfxtex-links
install-pdfxtex-data: install-pdfxtex-pool @FMU@ install-pdfxtex-dumps
install-pdfxtex-dumps: install-pdfxtex-fmts

# The actual binary executables and pool files.
install-programs: @PXTEX@ install-pdfxtex-programs
install-pdfxtex-programs: $(pdfxtex) $(bindir)
	for p in pdfxtex; do $(INSTALL_LIBTOOL_PROG) $$p $(bindir); done

install-links: @PXTEX@ install-pdfxtex-links
install-pdfxtex-links: install-pdfxtex-programs
	#cd $(bindir) && (rm -f pdfxinitex pdfxvirtex; \
	#  $(LN) pdfxtex pdfxinitex; $(LN) pdfxtex pdfvirxtex)

install-fmts: @PXTEX@ install-pdfxtex-fmts
install-pdfxtex-fmts: pdfxfmts $(fmtdir)
	pdfxfmts="$(all_pdfxfmts)";
	  for f in $$pdfxfmts; do $(INSTALL_DATA) $$f $(fmtdir)/$$f; done
	pdfxfmts="$(pdfxfmts)";
	  for f in $$pdfxfmts; do base=`basename $$f .fmt`; \
	    (cd $(bindir) && (rm -f $$base; $(LN) pdfxtex $$base)); done

# Auxiliary files.
install-data:: @PXTEX@ install-pdfxtex-data
install-pdfxtex-pool: pdfxtex.pool $(texpooldir)
	$(INSTALL_DATA) pdfxtex.pool $(texpooldir)/pdfxtex.pool

# end of pdfxtex.mk
