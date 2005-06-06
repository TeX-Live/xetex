################################################################################
#
# Makefile  : Dvipdfm
# Author    : Fabrice Popineau <Fabrice.Popineau@supelec.fr>
# Platform  : Win32, Microsoft VC++ 6.0, depends upon fpTeX 0.5 sources
# Time-stamp: <02/02/19 18:32:30 popineau>
#
################################################################################
root_srcdir = ..\..
!ifdef DEVELOPMENT
INCLUDE=$(INCLUDE);$(root_srcdir)\texk.development
!else
INCLUDE=$(INCLUDE);$(root_srcdir)\texk
!endif

# Makefile for ttf2pfb / fpTeX

version = 1.3

USE_GNUW32 = 1
USE_GSW32 = 1
USE_KPATHSEA = 1
USE_TTF = 1

!include <msvc/common.mak>

DEFS = $(DEFS) -DHAVE_LIBKPATHSEA

# manfiles = ttf2pfb.1

ttf2pfbobjs = $(objdir)\ttf2pfb.obj

programs = $(objdir)\ttf2pfb.exe $(objdir)\getafm.exe
etcfiles = getafm.ps
etcdir = $(psheaderdir)\getafm

all: $(objdir) $(programs)

default: all

$(objdir)\ttf2pfb.exe: $(ttf2pfbobjs) $(ttflib) $(kpathsealib)
	$(link) $(**) $(conlibs)

$(objdir)\getafm.exe: $(objdir)\getafm.obj $(kpathsealib) $(gsw32lib)
	$(link) $(**) $(advapiflags) $(conlibs) $(advapilibs)

!include <msvc/config.mak>

!include <msvc/install.mak>

install:: install-exec install-data # install-man

!include <msvc/clean.mak>
!include <msvc/rdepend.mak>
!include "./depend.mak"

#  
# Local variables:
# page-delimiter: "^# \f"
# mode: Makefile
# End:


