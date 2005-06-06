dnl aclocal.m4 generated automatically by aclocal 1.4-p5

dnl Copyright (C) 1994, 1995-8, 1999, 2001 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY, to the extent permitted by law; without
dnl even the implied warranty of MERCHANTABILITY or FITNESS FOR A
dnl PARTICULAR PURPOSE.

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

# Do all the work for Automake.  This macro actually does too much --
# some checks are only needed if your package does certain things.
# But this isn't really a big deal.

# serial 1

dnl Usage:
dnl AM_INIT_AUTOMAKE(package,version, [no-define])

AC_DEFUN([AM_INIT_AUTOMAKE],
[AC_REQUIRE([AC_PROG_INSTALL])
PACKAGE=[$1]
AC_SUBST(PACKAGE)
VERSION=[$2]
AC_SUBST(VERSION)
dnl test to see if srcdir already configured
if test "`cd $srcdir && pwd`" != "`pwd`" && test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured; run "make distclean" there first])
fi
ifelse([$3],,
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package]))
AC_REQUIRE([AM_SANITY_CHECK])
AC_REQUIRE([AC_ARG_PROGRAM])
dnl FIXME This is truly gross.
missing_dir=`cd $ac_aux_dir && pwd`
AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)
AM_MISSING_PROG(MAKEINFO, makeinfo, $missing_dir)
AC_REQUIRE([AC_PROG_MAKE_SET])])

#
# Check to make sure that the build environment is sane.
#

AC_DEFUN([AM_SANITY_CHECK],
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftestfile
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftestfile 2> /dev/null`
   if test "[$]*" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftestfile`
   fi
   if test "[$]*" != "X $srcdir/configure conftestfile" \
      && test "[$]*" != "X conftestfile $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "[$]2" = conftestfile
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
rm -f conftest*
AC_MSG_RESULT(yes)])

dnl AM_MISSING_PROG(NAME, PROGRAM, DIRECTORY)
dnl The program must properly implement --version.
AC_DEFUN([AM_MISSING_PROG],
[AC_MSG_CHECKING(for working $2)
# Run test in a subshell; some versions of sh will print an error if
# an executable is not found, even if stderr is redirected.
# Redirect stdin to placate older versions of autoconf.  Sigh.
if ($2 --version) < /dev/null > /dev/null 2>&1; then
   $1=$2
   AC_MSG_RESULT(found)
else
   $1="$3/missing $2"
   AC_MSG_RESULT(missing)
fi
AC_SUBST($1)])

# Like AC_CONFIG_HEADER, but automatically create stamp file.

AC_DEFUN([AM_CONFIG_HEADER],
[AC_PREREQ([2.12])
AC_CONFIG_HEADER([$1])
dnl When config.status generates a header, we must update the stamp-h file.
dnl This file resides in the same directory as the config header
dnl that is generated.  We must strip everything past the first ":",
dnl and everything past the last "/".
AC_OUTPUT_COMMANDS(changequote(<<,>>)dnl
ifelse(patsubst(<<$1>>, <<[^ ]>>, <<>>), <<>>,
<<test -z "<<$>>CONFIG_HEADERS" || echo timestamp > patsubst(<<$1>>, <<^\([^:]*/\)?.*>>, <<\1>>)stamp-h<<>>dnl>>,
<<am_indx=1
for am_file in <<$1>>; do
  case " <<$>>CONFIG_HEADERS " in
  *" <<$>>am_file "*<<)>>
    echo timestamp > `echo <<$>>am_file | sed -e 's%:.*%%' -e 's%[^/]*$%%'`stamp-h$am_indx
    ;;
  esac
  am_indx=`expr "<<$>>am_indx" + 1`
done<<>>dnl>>)
changequote([,]))])

