################################################################################
#
# Makefile  : TeXk / CJKutils / Thaï conversion
# Author    : Fabrice Popineau <Fabrice.Popineau@supelec.fr>
# Platform  : Win32, Microsoft VC++ 6.0, depends upon fpTeX 0.5 sources
# Time-stamp: <01/11/19 16:29:48 popineau>
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

!include <msvc/common.mak>

#
# Object files
#
objects = $(objdir)\extconv.obj # $(objdir)\thaiconv.obj
programs = $(objdir)\extconv.exe # $(objdir)\thaiconv.exe 

#
# Main target
#
default: all

all: $(objdir) $(programs)

#
# Link target. setargv.obj is provided in the compiler library directory.
#
$(objdir)\thaiconv.exe: $(objdir)\thaiconv.obj $(gnuw32lib)
	$(link) $(**) $(conlibs)

$(objdir)\extconv.exe: $(objdir)\extconv.obj $(gnuw32lib)
	$(link) $(**) $(conlibs)

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
