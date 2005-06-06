################################################################################
#
# Makefile  : dvipdfmx / crypto
# Author    : Fabrice Popineau <Fabrice.Popineau@supelec.fr>
# Platform  : Win32, Microsoft VC++ 6.0, depends upon fpTeX 0.5 sources
# Time-stamp: <03/02/23 13:44:34 popineau>
#
################################################################################
root_srcdir=..\..\..
!ifdef DEVELOPMENT
INCLUDE=$(INCLUDE);$(root_srcdir)\texk.development
!else
INCLUDE=$(INCLUDE);$(root_srcdir)\texk
!endif

USE_ZLIB = 1
MAKE_ZLIB = 1
USE_GNUW32 = 1

!include <msvc/common.mak>

!ifdef MD5RC4_DLL
md5rc4dll = $(objdir)\md5rc4.dll
!endif
md5rc4lib = $(objdir)\md5rc4.lib

md5rc4 = $(md5rc4lib) $(md5rc4dll)

programs = $(md5rc4dll)
libfiles = $(md5rc4lib)
includefiles = md5.h rc4.h
manfiles =
objects = \
	$(objdir)\md5_dgst.obj	\
	$(objdir)\md5_one.obj	\
	$(objdir)\rc4_enc.obj	\
	$(objdir)\rc4_skey.obj

TEST_MD5_OBJS = $(objdir)\md5test.obj \
	$(objdir)\evp_key.obj $(objdir)\digest.obj \
	$(objdir)\m_md5.obj
TEST_RC4_OBJS = $(objdir)\rc4test.obj \
	$(objdir)\evp_key.obj $(objdir)\e_rc4.obj

default: all

all: $(objdir) $(md5rc4lib) $(programs)

lib: $(objdir) $(md5rc4lib)

!ifdef MD5RC4_DLL
DEFS = $(DEFS) -DMAKE_MD5RC4_DLL

md5rc4def = $(library_prefix)md5rc4.def

$(md5rc4def): md5rc4.def
	sed -e "/^NAME/s/^.*$$/NAME $(library_prefix)md5rc4.dll/" < $(**) > $@

$(md5rc4lib): $(md5rc4def) $(objects)
	$(archive) /DEF:$(md5rc4def) $(objects)

$(md5rc4dll): $(objects) $(objdir)\md5rc4.res $(gnuw32lib)
	$(link_dll) $(objects) $(md5rc4lib:.lib=.exp) $(objdir)\md5rc4.res $(conlibs) $(gnuw32lib)
!else
$(md5rc4lib): $(objects)
	$(archive) $(objects)
!endif

test: all $(objdir)\md5test.exe $(objdir)\rc4test.exe

$(objdir)\md5test.exe: $(TEST_MD5_OBJS) $(md5rc4lib) $(gnuw32lib)
	$(link) $(**) $(conlibs)

$(objdir)\rc4test.exe: $(TEST_RC4_OBJS) $(md5rc4lib) $(gnuw32lib)
	$(link) $(**) $(conlibs)

!include <msvc/config.mak>
!include <msvc/install.mak>

install:: install-exec install-lib install-include

!include <msvc/clean.mak>

clean::
	-@$(del) $(objdir)\md5test.exe $(objdir)\rc4test.exe
	-@$(del) $(objdir)\md5test.obj $(objdir)\rc4test.obj

!include <msvc/rdepend.mak>
!include "./depend.mak"

#
# Local Variables:
# mode: makefile
# End:
