################################################################################
#
# Makefile  : Top-Level Makefile for programs using kpathsea 
# Author    : Fabrice Popineau <Fabrice.Popineau@supelec.fr>
# Platform  : Win32, Microsoft VC++ 6.0, depends upon fpTeX 0.5 sources
# Time-stamp: <01/12/03 15:20:02 popineau>
#
################################################################################
root_srcdir = ..\..
!ifdef DEVELOPMENT
INCLUDE=$(INCLUDE);$(root_srcdir)\texk.development
!else
INCLUDE=$(INCLUDE);$(root_srcdir)\texk
!endif

version = 5.92a

USE_GNUW32 = 1
USE_KPATHSEA = 1

c_auto_h_dir = ../dvipsk

!include <msvc/common.mak>

{../dvipsk}.c{$(objdir)}.obj:
	$(compile) $<

# Add -DDEFRES=dpi to DEFS to change the default resolution from 600.
# Add -DSECURE if you will install dvips with special privileges.
# Add -DNO_DEBUG to omit debugging support.
# Add -DNO_EMTEX to omit EMTEX specials.
# Add -DNO_HPS to omit hypertex support.
# Add -DNO_TPIC to omit tpic support.
# 
# For VM/CMS or (perhaps) VMS or DOS compilation, need the corresponding
# subdirectory from the original dvips distribution.  (Maybe dvipsk
# won't work at all on such systems; if you have info one way or the
# other, please let kb@cs.umb.edu know.)

DEFS =  -DHAVE_CONFIG_H -I../dvipsk $(DEFS)
!ifdef USE_KPATHSEA
DEFS = $(DEFS) -DDEFRES=600 -DA4 -DSHIFTLOWCHARS \
	-DKPATHSEA                               \
	-DDOWNLOAD_USING_PDFTEX			 \
	-DNeedFunctionPrototypes -DOmega

!else
DEFS = $(DEFS) -DDEFRES=600
!endif

# writet1.c is taken from pdftex
pdftexdir = ../web2c/pdftexdir

# The `t[mp]-' business is so that we don't create an empty .pro file,
# if running squeeze fails for some reason.  A dependency on squeeze
# fails here, so we include it below.
.SUFFIXES: .pro .lpro
.lpro.pro:
	.\$(objdir)\squeeze <$< >t-$@
	$(MV) t-$@ $@ && $(RM) t-$@
prologues = tex.pro texps.pro texc.pro special.pro finclude.pro \
            color.pro crop.pro hps.pro

objects = $(objdir)\dospecial.obj $(objdir)\dviinput.obj   \
	$(objdir)\emspecial.obj $(objdir)\fontdef.obj      \
	$(objdir)\loadfont.obj $(objdir)\dvips.obj         \
	$(objdir)\tfmload.obj $(objdir)\download.obj       \
	$(objdir)\prescan.obj $(objdir)\scanpage.obj       \
	$(objdir)\skippage.obj $(objdir)\output.obj        \
	$(objdir)\scalewidth.obj $(objdir)\dosection.obj   \
	$(objdir)\dopage.obj $(objdir)\resident.obj        \
	$(objdir)\search.obj $(objdir)\unpack.obj          \
	$(objdir)\drawPS.obj $(objdir)\header.obj          \
	$(objdir)\repack.obj $(objdir)\virtualfont.obj     \
	$(objdir)\dpicheck.obj $(objdir)\finclude.obj      \
	$(objdir)\writet1.obj $(objdir)\pprescan.obj       \
	$(objdir)\papersiz.obj $(objdir)\color.obj         \
	$(objdir)\bbox.obj $(objdir)\hps.obj

program = $(objdir)\odvips.exe
programs = $(program)

default: all

all: $(objdir) $(programs)

$(program): $(objects) $(kpathsealib)
	$(link) $(**) $(conlibs)

$(objdir)\writet1.obj: $(pdftexdir)\writet1.c
	$(compile) -I$(pdftexdir) $(pdftexdir)\writet1.c

c-auto.in: ..\dvipsk\c-auto.in
	@echo $(verbose) & $(copy) ..\dvipsk\c-auto.in . $(redir_stdout)

!include <msvc/config.mak>

installdirs = $(scriptdir) $(fontdir)

!include <msvc/install.mak>

install:: install-exec 

!include <msvc/clean.mak>

!ifdef MAINT
depend:	
#	pushd ../dvipsk & $(MAKE) -$(MAKEFLAGS) $@ & popd
# FIXME: this does not work !
	sed -e "s@\([ 	]\)\([A-z0-9_-][A-z0-9_-]*\)\.\([ch]\)\>@\1../dvipsk/\2.\3@g" \
		< ../dvipsk/depend.mak > ./depend.mak
!endif

!include "./depend.mak"

#
# Local Variables:
# mode: Makefile
# End:
