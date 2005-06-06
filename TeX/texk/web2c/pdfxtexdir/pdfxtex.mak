################################################################################
#
# Makefile  : pdfxTeX, web2c win32.mak makefile fragment to build pdfx-TeX
# Author    : Fabrice Popineau <Fabrice.Popineau@supelec.fr>
# Platform  : Win32, Microsoft VC++ 6.0, depends upon fpTeX 0.5 sources
# Time-stamp: <04/03/15 12:23:26 popineau>
#
################################################################################

Makefile: $(srcdir)\pdfxtexdir\pdfxtex.mak

# We build pdfxtex
pdfxtex = $(objdir)\pdfxtex.exe
!ifdef TEX_DLL
pdfxtex = $(pdfxtex) $(objdir)\$(library_prefix)pdfxtex.dll
!endif

# The C sources.
pdfxtex_c = pdfxtex.c
pdfxtex_o = $(objdir)\pdfxtex.obj

!ifdef TEX_DLL
$(objdir)\$(library_prefix)pdfxtex.exp: $(objdir)\$(library_prefix)pdfxtex.lib

$(objdir)\$(library_prefix)pdfxtex.lib: $(pdfxtex_o)
	$(archive) /DEF $(pdfxtex_o)

$(objdir)\$(library_prefix)pdfxtex.dll: $(pdfxtex_o) $(objdir)\$(library_prefix)pdfxtex.exp $(objdir)\pdfxtex.res $(pdftexlibs) $(kpathsealib) $(proglib)
	$(link_dll) $(**) $(socketlibs) $(conlibs)

$(objdir)\pdfxtex.exe: $(objdir)\win32main.obj $(objdir)\$(library_prefix)pdfxtex.lib $(proglib)
	$(link) $(**) $(conlibs)
!else
$(objdir)\pdfxtex.exe: $(pdfxtex_o) $(objdir)\win32main.obj $(objdir)\pdfxtex.res $(pdftexlibs) $(kpathsealib) $(proglib)
	$(link) $(**) $(socketlibs) $(conlibs)
!endif

# C file dependencies.
$(pdfxtex_c) pdfxtexcoerce.h pdfxtexd.h: pdfxtex.p $(web2c_texmf)
	$(web2c) pdfxtex

# Tangling
pdfxtex.p pdfxtex.pool: $(objdir)\tangle.exe pdfxtex.web pdfxtex.ch
	.\$(objdir)\tangle pdfxtex.web pdfxtex.ch

pdfxtex_files = \
	 etexdir\etex.ch0 \
	 etexdir\etex.ch \
	 etexdir\etex.fix \
	 etexdir\etex.ch1 \
         pdfetexdir\pdfetex.ch1 \
         pdftexdir\pdftex.ch \
         pdfetexdir\pdfetex.ch2

pdfxtex_changefiles = \
	    pdfetexdir\tex.ch0 \
	    tex.ch \
	    tex-supp-w32.ch \
	    etexdir\tex.ch1 \
	    etexdir\tex.ech \
	    etexdir\tex.ch2 \
	    pdfetexdir\tex.ch1 \
	    pdftexdir\tex.pch \
	    pdfetexdir\tex.ch2

# Generation of the web and ch files.
pdfxtex.web: $(objdir)\tie.exe tex.web $(pdfxtex_files) \
	pdfxtexdir\pdfetex.h pdfetexdir\pdfetex.defines # pdfetexdir\pdfetex.mak
	.\$(objdir)\tie -m pdfetex.web tex.web $(pdfetex_files)

pdfetex.ch: $(objdir)\tie.exe pdfetex.web $(pdfetex_changefiles) # pdfetexdir/pdfetex.mak
	.\$(objdir)\tie -c pdfetex.ch pdfetex.web $(pdfetex_changefiles)

pdfxtexdir\pdfxtex.h: pdftexdir\pdftex.h
	-@$(del) $(@) $(redir_stderr)
	$(copy) $(**) $(@)

pdfxtexdir\pdfxtex.defines: pdftexdir\pdftex.defines
	-@$(del) $(@) $(redir_stderr)
	$(copy) $(**) $(@)

check: pdfxtex-check
pdfxtex-check: pdfxtex pdfxtex.xfmt

clean:: pdfxtex-clean
pdfxtex-clean:
#	$(LIBTOOL) --mode=clean $(RM) pdfxtex
	-@echo $(verbose) & ( \
		for %%i in ($(pdfxtex_o) $(pdfxtex_c) pdfxtexextra.c pdfxtexcoerce.h \
			    pdfxtexd.h pdfxtex.p pdfxtex.pool pdfxtex.web pdfxtex.ch \
			    pdfxtex.xfmt pdfxtex.log) do $(del) %%i $(redir_stderr) \
	)

# Dumps
all_pdfxfmts = pdfxtex.xfmt $(pdfxfmts)
pdfxfmts: $(all_pdfxfmts)

pdfxtex.xfmt: $(pdfxtex)
	$(dumpenv) $(make) progname=pdfxtex files="etex.src plain.tex cmr10.tfm" prereq-check
	$(dumpenv) ./pdfxtex --progname=pdfxtex --jobname=pdfxtex --ini "*\pdfoutput=1\input etex.src \dump" <nul

pdfxlatex.xfmt: $(pdfxtex)
	$(dumpenv) $(make) progname=pdfxlatex files="latex.ltx" prereq-check
	$(dumpenv) ./pdfxtex --progname=pdfxlatex --jobname=pdfxlatex --ini "*\pdfoutput=1\input latex.ltx" <nul

pdflatex.xfmt: $(pdfxtex)
	$(dumpenv) $(make) progname=pdflatex files="latex.ltx" prereq-check
	$(dumpenv) ./pdfxtex --progname=pdflatex --jobname=pdflatex --ini "*\pdfoutput=1\input latex.ltx" <nul

# 
# Installation.
install-pdfxtex: install-pdfxtex-exec install-pdfxtex-data
install-pdfxtex-exec: install-pdfxtex-links
@FMU@install-pdfxtex-data: install-pdfxtex-dumps
install-pdfxtex-dumps: install-pdfxtex-fmts

# The actual binary executables and pool files.
install-prograns: install-pdfxtex-programs
install-pdfxtex-programs: $(pdfxtex) $(bindir)
	-@echo $(verbose) & ( \
	  for %%p in ($(pdfxtex)) do $(copy) %%p $(bindir) \
	) $(redir_stdout)

install-links: install-pdfxtex-links
install-pdfxtex-links: install-pdfxtex-programs
#	-@echo $(verbose) & ( \
#	  pushd $(bindir) & \
#	    $(del) .\pdfxinitex.exe .\pdfxvirtex.exe & \
#	    $(lnexe) .\pdfxtex.exe $(bindir)\pdfxinitex.exe & \
#	    $(lnexe) .\pdfxtex.exe $(bindir)\pdfxvirtex.exe & \
#	  popd \
#	) $(redir_stdout)
	-@echo $(verbose) & ( \
	  if not "$(pdfxfmts)"=="" \
	    for %%i in ($(pdfxfmts)) do \
              pushd $(bindir) & \
                $(del) .\%%~ni.exe & \
	        $(lnexe) .\pdfxtex.exe $(bindir)\%%~ni.exe & \
	      popd \
	) $(redir_stdout)

install-fmts: install-pdfxtex-fmts
install-pdfxtex-fmts: pdfxfmts $(fmtdir)
	-@echo $(verbose) & ( \
	  for %%f in ($(all_pdfxfmts)) \
	    do $(copy) %%f $(fmtdir)\%%f \
	) $(redir_stdout)

# Auxiliary files.
install-data:: install-pdfxtex-data
install-pdfxtex-data: $(texpooldir)
	@$(copy) pdfxtex.pool $(texpooldir)\pdfxtex.pool $(redir_stdout)

# end of pdfxtex.mak
#  
# Local variables:
# page-delimiter: "^# \f"
# mode: Makefile
# End:
