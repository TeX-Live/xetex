root_srcdir = ..\..\..
!ifdef DEVELOPMENT
INCLUDE=$(INCLUDE);$(root_srcdir)\texk.development
!else
INCLUDE=$(INCLUDE);$(root_srcdir)\texk
!endif

# ----------------------------------------------------------------------------
# UNIX version
# throw out -DA4 if you want letter format as default size
# use -DKNR for Kernighan/Ritchie compilers

USE_GNUW32 = 1

!include <msvc/common.mak>

DEFS = -I.. -I../include -DHAVE_CONFIG_H $(DEFS)

libs =  ..\libefont\$(objdir)\libefont.lib	\
	..\liblcdf\$(objdir)\liblcdf.lib

programs = $(objdir)\mmpfb.exe
manfiles = mmpfb.1

objects = \
	$(objdir)\myfont.obj	\
	$(objdir)\t1minimize.obj	\
	$(objdir)\t1rewrit.obj	\
	$(objdir)\main.obj

default: all

all: $(objdir) $(programs)

$(objdir)\mmpfb.exe: $(objects) $(libs) $(gnuw32lib)
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
