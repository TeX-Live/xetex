################################################################################
#
# Makefile  : mminstance, edlib subdirectory
# Author    : Fabrice Popineau <Fabrice.Popineau@supelec.fr>
# Platform  : Win32, Microsoft VC++ 6.0, depends upon fpTeX 0.5 sources
# Time-stamp: <03/07/22 05:24:16 popineau>
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

DEFS = -I.. -I../include -DHAVE_CONFIG_H -DHAVE_STRDUP=1 $(DEFS)

programs = $(objdir)\liblcdf.lib

objects = \
	$(objdir)\bezier.obj	\
	$(objdir)\clp.obj	\
	$(objdir)\fixlibc.obj	\
	$(objdir)\strtonum.obj	\
	$(objdir)\error.obj	\
	$(objdir)\filename.obj	\
	$(objdir)\landmark.obj	\
	$(objdir)\permstr.obj	\
	$(objdir)\point.obj	\
	$(objdir)\slurper.obj	\
	$(objdir)\straccum.obj	\
	$(objdir)\string.obj	\
	$(objdir)\transform.obj	\
	$(objdir)\vectorv.obj

default: all

all: $(objdir) $(programs)

$(objdir)\liblcdf.lib: $(objects)
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
