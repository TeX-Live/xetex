################################################################################
#
# Makefile  : mminstance, mmafm program
# Author    : Fabrice Popineau <Fabrice.Popineau@supelec.fr>
# Platform  : Win32, Microsoft VC++ 6.0, depends upon fpTeX 0.5 sources
# Time-stamp: <03/07/22 04:35:08 popineau>
#
################################################################################
root_srcdir = ..\..\..
!ifdef DEVELOPMENT
INCLUDE=$(INCLUDE);$(root_srcdir)\texk.development
!else
INCLUDE=$(INCLUDE);$(root_srcdir)\texk
!endif

USE_GNUW32 = 1
USE_NETWORK2 = 1

!include <msvc/common.mak>

DEFS = -I.. -I../include -DHAVE_CONFIG_H $(DEFS)

libs =  ..\libefont\$(objdir)\libefont.lib	\
	..\liblcdf\$(objdir)\liblcdf.lib

programs = $(objdir)\cfftot1.exe
manfiles = cfftot1.1

objects = $(objdir)\cfftot1.obj $(objdir)\maket1font.obj

default: all

all: $(objdir) $(programs)

$(objdir)\cfftot1.exe: $(objects) $(libs) $(gnuw32lib)
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
