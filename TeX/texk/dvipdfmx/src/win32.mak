################################################################################
#
# Makefile  : Dvipdmfx / Src
# Author    : Fabrice Popineau <Fabrice.Popineau@supelec.fr>
# Platform  : Win32, Microsoft VC++ 6.0, depends upon fpTeX 0.7 sources
# Time-stamp: <04/02/15 22:53:23 popineau>
#
################################################################################
root_srcdir = ..\..\..
!ifdef DEVELOPMENT
INCLUDE=$(INCLUDE);$(root_srcdir)\texk.development
!else
INCLUDE=$(INCLUDE);$(root_srcdir)\texk
!endif

# Makefile for ttfdump

USE_GNUW32 = 1
USE_KPATHSEA = 1
USE_ZLIB = 1
USE_PNG = 1

!include <msvc/common.mak>

DEFS = -I.. $(DEFS) -DHAVE_CONFIG_H \
	-I..\crypto
#	-DWITHOUT_OPENSSL
#	-DWITHOUT_TOUNICODE

manfiles =
objects = \
	$(objdir)\agl.obj	\
	$(objdir)\cff.obj	\
	$(objdir)\cff_dict.obj	\
	$(objdir)\cid.obj	\
	$(objdir)\cidtype0.obj	\
	$(objdir)\cidtype2.obj	\
	$(objdir)\cmap.obj	\
	$(objdir)\cmap_parse.obj	\
	$(objdir)\colorsp.obj	\
	$(objdir)\cs_type2.obj	\
	$(objdir)\dpxcrypt.obj	\
	$(objdir)\dpxutil.obj	\
	$(objdir)\dvi.obj	\
	$(objdir)\dvipdfmx.obj	\
	$(objdir)\encodings.obj	\
	$(objdir)\epdf.obj	\
	$(objdir)\error.obj	\
	$(objdir)\fontmap.obj	\
	$(objdir)\htex.obj	\
	$(objdir)\jpegimage.obj	\
	$(objdir)\mem.obj	\
	$(objdir)\mfileio.obj	\
	$(objdir)\mpost.obj	\
	$(objdir)\numbers.obj	\
	$(objdir)\pdfdev.obj	\
	$(objdir)\pdfdoc.obj	\
	$(objdir)\pdfencrypt.obj	\
	$(objdir)\pdfobj.obj	\
	$(objdir)\pdfparse.obj	\
	$(objdir)\pdfresource.obj	\
	$(objdir)\pdfspecial.obj	\
	$(objdir)\pkfont.obj	\
	$(objdir)\pngimage.obj	\
	$(objdir)\psimage.obj	\
	$(objdir)\psspecial.obj	\
	$(objdir)\pst.obj	\
	$(objdir)\pst_obj.obj	\
	$(objdir)\sfnt.obj	\
	$(objdir)\subfont.obj	\
	$(objdir)\t1crypt.obj	\
	$(objdir)\tfm.obj	\
	$(objdir)\thumbnail.obj	\
	$(objdir)\tpic.obj	\
	$(objdir)\ttf.obj	\
	$(objdir)\tt_aux.obj	\
	$(objdir)\tt_build.obj	\
	$(objdir)\tt_cmap.obj	\
	$(objdir)\tt_gsub.obj	\
	$(objdir)\tt_table.obj	\
	$(objdir)\type0.obj	\
	$(objdir)\type1.obj	\
	$(objdir)\type1c.obj	\
	$(objdir)\unicode.obj	\
	$(objdir)\vf.obj

program = $(objdir)\dvipdfmx.exe
programs = $(program)

default: all

all: $(objdir) $(programs)

$(program): $(objects) ..\crypto\$(objdir)\md5rc4.lib $(pnglib) $(zliblib) $(kpathsealib)
	$(link) $(**) $(conlibs)

!include <msvc/config.mak>
!include <msvc/install.mak>

..\config.h: ..\config.h.in
	$(perl) $(win32perldir)\conf-cauto.pl $(**) $@

install:: install-exec install-man

!include <msvc/clean.mak>

!include <msvc/rdepend.mak>
!include "./depend.mak"

#  
# Local variables:
# page-delimiter: "^# \f"
# mode: Makefile
# End:
