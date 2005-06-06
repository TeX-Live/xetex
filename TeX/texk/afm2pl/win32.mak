################################################################################
#
# Makefile  : 
# Author    : Fabrice Popineau <Fabrice.Popineau@supelec.fr>
# Platform  : Win32, Microsoft VC++ 6.0, depends upon fpTeX 0.5 sources
# Time-stamp: <04/03/09 22:11:46 popineau>
#
################################################################################
root_srcdir = ..\..
!ifdef DEVELOPMENT
INCLUDE=$(INCLUDE);$(root_srcdir)\texk.development
!else
INCLUDE=$(INCLUDE);$(root_srcdir)\texk
!endif

# This is free software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License, see the file COPYING.

USE_GNUW32 = 1
USE_KPATHSEA = 1

!include <msvc/common.mak>

programs =  $(objdir)\afm2pl.exe 
scripts =
libfiles =
includefiles =
manfiles = afm2pl.1
infofiles =
#
# Object files
#
objects = $(objdir)\afm2pl.obj

DEFS = $(DEFS) -I../dvipsk

#
# Main target
#
all: $(programs)

#
# Link target. setargv.obj is provided in the compiler library directory.
#
$(objdir)\afm2pl.exe: $(objdir) $(objects) $(kpathsealib)
	$(link) $(objects) $(kpathsealib) $(conlibs)

!include <msvc/config.mak>
!include <msvc/install.mak>

install:: install-exec # install-lib install-include

!include <msvc/clean.mak>
!include <msvc/rdepend.mak>
!include "./depend.mak"

# End of .mak
#
# Local Variables:
# mode: makefile
# End:
