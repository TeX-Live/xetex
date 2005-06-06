################################################################################
#
# Makefile  : Dvipdfmx / data
# Author    : Fabrice Popineau <Fabrice.Popineau@supelec.fr>
# Platform  : Win32, Microsoft VC++ 6.0, depends upon fpTeX 0.7 sources
# Time-stamp: <03/05/24 18:25:28 popineau>
#
################################################################################
root_srcdir = ..\..\..
!ifdef DEVELOPMENT
INCLUDE=$(INCLUDE);$(root_srcdir)\texk.development
!else
INCLUDE=$(INCLUDE);$(root_srcdir)\texk
!endif

!include <msvc/common.mak>

installdirs = $(texmf)\dvipdfm

# Package subdirectories, the library, and all subdirectories.

!include <msvc/install.mak>

all::

install:: install-data

install-data::
	-@echo $(verbose) && ( for %d in (CMap config) do \
	    $(copydir) %d $(texmf)\dvipdfm \
	 ) $(redir_stdout)

!include <msvc/config.mak>
!include <msvc/clean.mak>

# Local Variables:
# mode: Makefile
# End:
