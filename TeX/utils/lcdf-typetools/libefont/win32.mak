################################################################################
#
# Makefile  : mminstance, edlib subdirectory
# Author    : Fabrice Popineau <Fabrice.Popineau@supelec.fr>
# Platform  : Win32, Microsoft VC++ 6.0, depends upon fpTeX 0.5 sources
# Time-stamp: <04/01/05 15:13:08 popineau>
#
################################################################################
root_srcdir = ..\..\..
!ifdef DEVELOPMENT
INCLUDE=$(INCLUDE);$(root_srcdir)\texk.development
!else
INCLUDE=$(INCLUDE);$(root_srcdir)\texk
!endif

USE_GNUW32 = 1

!include <msvc/common.mak>

DEFS = -I.. -I../include -DHAVE_CONFIG_H $(DEFS)

programs = $(objdir)\libefont.lib

objects = \
	$(objdir)\afm.obj	\
	$(objdir)\afmparse.obj	\
	$(objdir)\afmw.obj	\
	$(objdir)\amfm.obj	\
	$(objdir)\cff.obj	\
	$(objdir)\encoding.obj	\
	$(objdir)\findmet.obj	\
	$(objdir)\metrics.obj	\
	$(objdir)\otf.obj	\
	$(objdir)\otfcmap.obj	\
	$(objdir)\otfdata.obj	\
	$(objdir)\otfdescrip.obj	\
	$(objdir)\otfgpos.obj	\
	$(objdir)\otfgsub.obj	\
	$(objdir)\otfname.obj	\
	$(objdir)\pairop.obj	\
	$(objdir)\psres.obj	\
	$(objdir)\t1bounds.obj	\
	$(objdir)\t1cs.obj	\
	$(objdir)\t1csgen.obj	\
	$(objdir)\t1font.obj	\
	$(objdir)\t1fontskel.obj	\
	$(objdir)\t1interp.obj	\
	$(objdir)\t1item.obj	\
	$(objdir)\t1mm.obj	\
	$(objdir)\t1rw.obj	\
	$(objdir)\t1unparser.obj

default: all

all: $(objdir) $(programs)

$(objdir)\libefont.lib: $(objects)
	$(archive) $(objects)

..\config.h: ..\config.h.in
	$(perl) $(win32perldir)\conf-cauto.pl ..\config.h.in $@

!include <msvc/config.mak>
!include <msvc/install.mak>

install::

!include <msvc/clean.mak>
!include <msvc/rdepend.mak>
!include "./depend.mak"

#
# Local Variables:
# mode: makefile
# End:
