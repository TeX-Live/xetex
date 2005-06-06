################################################################################
#
# Makefile  : mminstance, otftotfm program
# Author    : Fabrice Popineau <Fabrice.Popineau@supelec.fr>
# Platform  : Win32, Microsoft VC++ 6.0, depends upon fpTeX 0.5 sources
# Time-stamp: <03/08/12 17:32:08 popineau>
#
################################################################################
root_srcdir = ..\..\..
!ifdef DEVELOPMENT
INCLUDE=$(INCLUDE);$(root_srcdir)\texk.development
!else
INCLUDE=$(INCLUDE);$(root_srcdir)\texk
!endif

USE_GNUW32 = 1
USE_KPATHSEA = 1
USE_NETWORK2 = 1

!include <msvc/common.mak>

DEFS = -I.. -I../include -DHAVE_CONFIG_H $(DEFS)

libs =  ..\libefont\$(objdir)\libefont.lib	\
	..\liblcdf\$(objdir)\liblcdf.lib

programs = $(objdir)\otftotfm.exe
manfiles = otftotfm.1

objects = \
	$(objdir)\automatic.obj	\
	$(objdir)\dvipsencoding.obj	\
	$(objdir)\kpseinterface.obj	\
	$(objdir)\md5.obj	\
	$(objdir)\metrics.obj	\
	$(objdir)\otftotfm.obj	\
	$(objdir)\secondary.obj	\
	$(objdir)\util.obj

default: all

all: $(objdir) $(programs)

$(objdir)\otftotfm.exe: $(objects) $(libs) $(kpathsealib)
	$(link) $(**) $(conlibs)

..\config.h: ..\config.h.in
	$(perl) $(win32perldir)\conf-cauto.pl ..\config.h.in $@

!include <msvc/config.mak>

!include <msvc/install.mak>

install:: install-exec

!include <msvc/clean.mak>
!include <msvc/rdepend.mak>
!include "./depend.mak"

#
# Local Variables:
# mode: makefile
# End:
