#
# Local tests written by Mark A. Wicks.
#
AC_DEFUN(AC_EXT_TIMEZONE,
[AC_MSG_CHECKING([whether time.h defines timezone as an external variable])
AC_TRY_LINK([#include <time.h>], [ -timezone; ],
	[AC_MSG_RESULT(yes); AC_DEFINE(HAVE_TIMEZONE)], [AC_MSG_RESULT(no)])])
#
#
AC_DEFUN(AC_HAVE_BASENAME,
       [AC_MSG_CHECKING([whether basename is in either libgen.h or string.h])
       AC_TRY_COMPILE([#include <libgen.h>
#include <string.h>], [extern char basename(void)],
	[AC_MSG_RESULT(no)], [AC_MSG_RESULT(yes); AC_DEFINE(HAVE_BASENAME)])])
#
#
AC_DEFUN(AC_TZ_HAS_TM_GMTOFF,
[AC_MSG_CHECKING([whether struct tz has tm_gmtoff as a member])
AC_TRY_COMPILE([#include <time.h>], [struct tm *tp; tp->tm_gmtoff],
	[AC_MSG_RESULT(yes); AC_DEFINE(HAVE_TM_GMTOFF)], [AC_MSG_RESULT(no)])])
#
# Check for kpathsea
#
AC_DEFUN(AC_HAS_KPSE_FORMATS,
	[AC_MSG_CHECKING([whether you have kpathsea headers and they whether they know about the required file formats])
	 AC_TRY_COMPILE([
#include <stdio.h>
#include <kpathsea/tex-file.h>],
	     	        [kpse_tex_ps_header_format;
			 kpse_type1_format;kpse_vf_format;
			 kpse_ofm_format;kpse_truetype_format],
			[AC_MSG_RESULT(yes); AC_DEFINE(HAVE_KPSE_FORMATS)],
			[AC_MSG_RESULT(no);
AC_MSG_ERROR([
This version of dvipdfm requires that kpathsea and its headers be installed.
If you are sure they are installed and in a standard place, maybe you need a
newer version of kpathsea?  You also might try setting the environment
variable CPPFLAGS (or CFLAGS) with -I pointing to the directory containing
the file "tex-file.h"
])])])

#
# Check for enc, cmap, sfd formats
#
AC_DEFUN(AC_HAS_KPSE_ENC_FORMATS,
	[AC_MSG_CHECKING([whether kpse_enc_format is in kpathsea/tex-file.h])
	 AC_TRY_COMPILE([
#include <stdio.h>
#include <kpathsea/tex-file.h>],
	     	        [kpse_enc_format;kpse_cmap_format;kpse_sfd_format],
			[AC_MSG_RESULT(yes); AC_DEFINE(HAVE_KPSE_ENC_FORMATS)],
			[AC_MSG_RESULT(no)])])
#
# Check for opentype format
#
AC_DEFUN(AC_HAS_KPSE_OPENTYPE_FORMAT,
	[AC_MSG_CHECKING([whether kpse_opentype_format is in kpathsea/tex-file.h])
	 AC_TRY_COMPILE([
#include <stdio.h>
#include <kpathsea/tex-file.h>],
	     	        [kpse_opentype_format],
			[AC_MSG_RESULT(yes); AC_DEFINE(HAVE_KPSE_OPENTYPE_FORMAT)],
			[AC_MSG_RESULT(no)])])

#
# Check for zlib
#
AC_DEFUN(AC_HAS_ZLIB,
[_cppflags=$CPPFLAGS
 _ldflags=$LDFLAGS
AC_ARG_WITH(zlib,  
[  --with-zlib=DIR         use zlib include/library files from DIR],[
  if test -d "$withval"; then
    CPPFLAGS="$CPPFLAGS -I$withval/include"
    LDFLAGS="$LDFLAGS -L$withval/lib"
  fi
])
AC_MSG_CHECKING([for zlib header files])
AC_TRY_COMPILE([#include <zlib.h>], [z_stream p;],
[AC_MSG_RESULT(yes)
 AC_CHECK_LIB(z, compress,
[AC_DEFINE(HAVE_ZLIB)
 LIBS="$LIBS -lz"
 AC_CHECK_LIB(z, compress2,
[AC_DEFINE(HAVE_ZLIB_COMPRESS2)])])],
[CPPFLAGS=$_cppflags
 LDDFLAGS=$_ldflags
 AC_MSG_RESULT(no)])])
#
#   Check for libpng
#
AC_DEFUN(AC_HAS_LIBPNG,
[_cppflags=$CPPFLAGS
_ldflags=$LDFLAGS
AC_ARG_WITH(png,
[  --with-png=DIR          use png include/library files from DIR],[
  if test -d "$withval"; then
    CPPFLAGS="$CPPFLAGS -I$withval/include"
    LDFLAGS="$LDFLAGS -L$withval/lib"
  fi
])
AC_CHECK_FUNC(pow, , AC_CHECK_LIB(m, pow, LIBS="$LIBS -lm"))
AC_MSG_CHECKING([for png header files])
AC_TRY_COMPILE([#include <png.h>], [png_infop p;], [
  AC_MSG_RESULT(yes)
  AC_CHECK_LIB(png, png_get_image_width, [
    AC_DEFINE(HAVE_LIBPNG)
    LIBS="$LIBS -lpng"], , -lz)], [
  AC_MSG_RESULT(no)])])
#
#   Check for libpaper
#
AC_DEFUN(AC_HAS_LIBPAPER,
[_cppflags=$CPPFLAGS
_ldflags=$LDFLAGS
AC_ARG_WITH(paper,
[  --with-paper=DIR        use paper include/library files from DIR],[
  if test -d "$withval"; then
    CPPFLAGS="$CPPFLAGS -I$withval/include"
    LDFLAGS="$LDFLAGS -L$withval/lib"
  fi
])
AC_MSG_CHECKING([for paper header files])
AC_TRY_COMPILE([#include <paper.h>], [struct paper *p;], [
  AC_MSG_RESULT(yes)
  AC_CHECK_LIB(paper, paperpswidth, [
    AC_DEFINE(HAVE_LIBPAPER)
    LIBS="$LIBS -lpaper"])], [
  AC_MSG_RESULT(no)])])
