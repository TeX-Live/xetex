################################################################################
#
# Makefile  : mminstance
# Author    : Fabrice Popineau <Fabrice.Popineau@supelec.fr>
# Platform  : Win32, Microsoft VC++ 6.0, depends upon fpTeX 0.5 sources
# Time-stamp: <04/03/04 11:51:47 popineau>
#
################################################################################
root_srcdir = ..\..
!ifdef DEVELOPMENT
INCLUDE=$(INCLUDE);$(root_srcdir)\texk.development
!else
INCLUDE=$(INCLUDE);$(root_srcdir)\texk
!endif

!include <msvc/common.mak>

# Kpathsea needs to be build before 
subdirs = \
	libefont      	\
	liblcdf		\
	mmafm		\
	mmpfb		\
	cfftot1		\
	otfinfo		\
	otftotfm	\
	t1dotlessj	\
	t1lint		\
	t1testpage

installdirs = $(installdirs) $(texmf)\fonts\type1

default:: all

all:: config.h

!include <msvc/subdirs.mak>
!include <msvc/config.mak>

installdirs = $(installdirs) $(texmf)\fonts\type1

install::
	-@$(copy) glyphlist.txt $(texmf)\fonts\type1\glyphlist.txt

!include <msvc/clean.mak>

distclean::
	-$(del) config.h

#
# Local Variables:
# mode: makefile
# End:
