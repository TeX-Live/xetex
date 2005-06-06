dnl ### Check for whether setsid() is allowed within vfork()
dnl (Mac OS X 10.3 (Panther, 11/2003) is one O/S which does not allow this.)

AC_DEFUN(XDVI_FUNC_SETSID_IN_VFORK,
[if test $ac_cv_func_vfork_works = yes; then
AC_CACHE_CHECK([for whether setsid() is allowed within vfork()],
xdvi_cv_setsid_in_vfork,
[AC_TRY_RUN(
[/* Test adapted from Gnu autoconf */
/* Thanks to Paul Eggert for this test.  */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_VFORK_H
#include <vfork.h>
#endif
int
main() {
  pid_t parent = getpid ();
  pid_t child;

  child = vfork ();

  if (child == 0) {
    if (setsid () == -1)
      _exit(1);
    _exit(0);
  } else {
    int status;

    while (wait(&status) != child)
      ;
    exit(
	 /* Was there some problem with vforking?  */
	 child < 0

	 /* Did the child fail?  (This shouldn't happen.)  */
	 || status
	 );
  }
}],
xdvi_cv_setsid_in_vfork=yes,
xdvi_cv_setsid_in_vfork=no)])
if test $xdvi_cv_setsid_in_vfork = yes; then
  AC_DEFINE(HAVE_GOOD_SETSID_VFORK)
fi]
fi)

dnl ### Check if the --allow-multiple-definition linker flag is
dnl ### available (assuming that we need to use it if it is)

AC_DEFUN(XDVI_LINKER_MULTIPLE_DEFNS,
[AC_CACHE_CHECK([whether linker supports the --allow-multiple-definition flag],
xdvi_linker_multiple_defns,
xdvi_save_LDFLAGS="$LDFLAGS"
LDFLAGS="-Xlinker --allow-multiple-definition"
x_linker_options=""
[AC_TRY_LINK(
   [#include <stdio.h>
   ],
   [void foo(void);
   ],
   xdvi_linker_multiple_defns=yes, xdvi_linker_multiple_defns=no
)]
)
if test $xdvi_linker_multiple_defns = yes; then
x_linker_options="-Xlinker --allow-multiple-definition"
fi
LDFLAGS="$xdvi_save_LDFLAGS"
AC_SUBST(x_linker_options)])


dnl ### Determine integer type to use for bitmaps

AC_DEFUN(XDVI_C_BITMAP_TYPE,
[AC_MSG_CHECKING(for integer type to use in bitmaps)
AC_CACHE_VAL(xdvi_cv_bitmap_type,
[AC_TRY_RUN(
[#include <stdio.h>
main()
{
  FILE *f=fopen("conftestval", "w");
  if (!f) exit(1);
  if ((sizeof(unsigned long) == 4 || sizeof(unsigned long) == 2)
    && sizeof(unsigned long) != sizeof(unsigned int))
      fprintf(f, "BMTYPE=long BMBYTES=%d\n", sizeof(unsigned long));
  if (sizeof(unsigned int) == 4 || sizeof(unsigned int) == 2)
    fprintf(f, "BMTYPE=int BMBYTES=%d\n", sizeof(unsigned int));
  else if (sizeof(unsigned short) == 4 || sizeof(unsigned short) == 2)
    fprintf(f, "BMTYPE=short BMBYTES=%d\n", sizeof(unsigned short));
  else fprintf(f, "BMTYPE=char BMBYTES=%d\n", sizeof(unsigned char));
  exit(0);
}],
xdvi_cv_bitmap_type="`cat conftestval`",
AC_MSG_ERROR(could not determine integer type for bitmap))])
eval "$xdvi_cv_bitmap_type"
AC_DEFINE_UNQUOTED(BMTYPE, $BMTYPE)
AC_DEFINE_UNQUOTED(BMBYTES, $BMBYTES)
AC_MSG_RESULT([unsigned $BMTYPE, size = $BMBYTES])])


dnl ### Check for whether the C compiler does string concatenation

AC_DEFUN(XDVI_CC_CONCAT,
[AC_CACHE_CHECK([whether C compiler supports string concatenation], xdvi_cc_concat,
[AC_TRY_COMPILE(
[#include <stdio.h>
], [puts("Testing" " string" " concatenation");
], xdvi_cc_concat=yes, xdvi_cc_concat=no)])
if test $xdvi_cc_concat = yes; then
  AC_DEFINE(HAVE_CC_CONCAT)
fi])


dnl ### Check for at-least-pretend Streams capability

AC_DEFUN(XDVI_SYS_STREAMS,
[AC_CACHE_CHECK([for stropts.h and isastream()], xdvi_cv_sys_streams,
[AC_TRY_LINK(
[#include <stropts.h>
], [#ifndef I_SETSIG
choke me
#else
isastream(0);
#endif], xdvi_cv_sys_streams=yes, xdvi_cv_sys_streams=no)])
if test $xdvi_cv_sys_streams = yes; then
  AC_DEFINE(HAVE_STREAMS)
fi])


dnl ### Check for poll()

AC_DEFUN(XDVI_FUNC_POLL,
[AC_CACHE_CHECK([for poll.h and poll()], xdvi_cv_func_poll,
[AC_TRY_LINK(
[#include <poll.h>
], [poll((struct pollfd *) 0, 0, 0);],
xdvi_cv_func_poll=yes, xdvi_cv_func_poll=no)])
if test $xdvi_cv_func_poll = yes; then
  AC_DEFINE(HAVE_POLL)
else
  AC_CHECK_HEADERS(sys/select.h select.h)
fi])


dnl ### Check for SunOS 4

AC_DEFUN(XDVI_SYS_SUNOS_4,
[AC_CACHE_CHECK([for SunOS 4], xdvi_cv_sys_sunos_4,
[case "`(uname -sr) 2>/dev/null`" in
"SunOS 4."*)
  xdvi_cv_sys_sunos_4=yes ;;
*) xdvi_cv_sys_sunos_4=no ;;
esac])
if test $xdvi_cv_sys_sunos_4 = yes; then
  AC_DEFINE(SUNOS4)
fi])

dnl ### Check for certain broken versions of Linux

AC_DEFUN(XDVI_SYS_OLD_LINUX,
[AC_CACHE_CHECK([for certain old versions of Linux], xdvi_cv_sys_old_linux,
[case "`(uname -sr) 2>/dev/null`" in
"Linux 2."[[01]].* | "Linux 2.2."[[0-8]] | "Linux 2.2."[[0-8]]-*)
  xdvi_cv_sys_old_linux=yes ;;
*) xdvi_cv_sys_old_linux=no ;;
esac])
if test $xdvi_cv_sys_old_linux = yes; then
  AC_DEFINE(FLAKY_SIGPOLL)
fi])

dnl ### Process a string argument

dnl XDVI_ARG_STRING(PACKAGE, HELP-STRING, VARIABLE, DEFAULT_VALUE)
AC_DEFUN(XDVI_ARG_STRING,
[AC_ARG_WITH($1, [$2], [AC_DEFINE_UNQUOTED($3, "$withval")
], [AC_DEFINE($3, [$4])
])])


dnl dnl ### Check for vsnprintf() added by SU 2000/03/07
dnl AC_DEFUN(AC_FUNC_VSNPRINTF,
dnl [AC_CACHE_CHECK([for vsnprintf], xdvi_cv_vsnprintf,
dnl [AC_TRY_LINK(
dnl [#include <stdio.h>
dnl ], [(void)vsnprintf((char *)NULL, 0, (char *)NULL, NULL);],
dnl xdvi_cv_vsnprintf=yes, xdvi_cv_vsnprintf=no)])
dnl if test $xdvi_cv_vsnprintf = yes; then
dnl   AC_DEFINE(HAVE_VSNPRINTF)
dnl fi])

dnl ### Check for a working implementation of (v)snprintf()
dnl ### which should either return a negative result or the size actually needed
dnl ### (latter is C99 standard).
AC_DEFUN(XDVI_FUNC_WORKING_VSNPRINTF,
[AC_CACHE_CHECK([for a working implementation of (v)snprintf()],
xdvi_cv_func_good_vsnprintf,
[AC_TRY_RUN(
[
#ifdef IRIX
#define _XOPEN_SOURCE 500
#endif
#include <stdio.h>
int main()
{
  char s[2];
  int retval = snprintf(s, 2, "test");
  if (retval > -1 && retval != 4) /* neither negative nor desired size, not OK */
    return 1;
  return 0;
}],
xdvi_cv_func_good_vsnprintf=yes,
xdvi_cv_func_good_vsnprintf=no)])
if test $xdvi_cv_func_good_vsnprintf = yes; then
  AC_DEFINE(HAVE_GOOD_VSNPRINTF)
fi])

dnl ### Check for memicmp(), which some installations have in string.h
AC_DEFUN(AC_FUNC_MEMICMP,
[AC_CACHE_CHECK([for memicmp], xdvi_cv_memicmp,
[AC_TRY_LINK(
[#include <string.h>
], [(void)memicmp((char *)NULL, (char *)NULL, 0);],
xdvi_cv_memicmp=yes, xdvi_cv_memicmp=no)])
if test $xdvi_cv_memicmp = yes; then
  AC_DEFINE(HAVE_MEMICMP)
fi])

dnl dnl ### Check for realpath() added by SU 2002/04/10
dnl AC_DEFUN(AC_FUNC_REALPATH,
dnl [AC_CACHE_CHECK([for realpath], xdvi_cv_realpath,
dnl [AC_TRY_LINK(
dnl [#include <stdlib.h>
dnl ], [(void)realpath((const char *)NULL, NULL);],
dnl xdvi_cv_realpath=yes, xdvi_cv_realpath=no)])
dnl if test $xdvi_cv_realpath = yes; then
dnl   AC_DEFINE(HAVE_REALPATH)
dnl fi])


dnl ### for Xpm libraries and headers.
dnl Put Xpm include directory in xpm_includes,
dnl put Xpm library directory in xpm_libraries,
dnl and add appropriate flags to X_CFLAGS and X_LIBS.
dnl
AC_DEFUN(XDVI_FIND_XPM,
[
AC_REQUIRE([AC_PATH_XTRA])
xpm_includes=
xpm_libraries=
AC_ARG_WITH(xpm,
[  --without-xpm           Do not use the Xpm library (will disable the toolbar)])
dnl Treat --without-xpm like
dnl --without-xpm-includes --without-xpm-libraries.
if test "$with_xpm" = "no"
then
xpm_includes=no
xpm_libraries=no
fi
AC_ARG_WITH(xpm-includes,
[  --with-xpm-include=DIR
                          Specify the location of Xpm include files],
xpm_includes="$withval")
AC_ARG_WITH(xpm-libraries,
[  --with-xpm-libdir=DIR
                          Specify the location of Xpm libraries],
xpm_libraries="$withval")
AC_MSG_CHECKING(for Xpm)
#
#
# Search the include files.  Note that XPM can come in <X11/xpm.h> (as
# in X11R6) or in <xpm.h> if installed locally.
#
if test "$xpm_includes" = ""; then
AC_CACHE_VAL(xdvi_cv_xpm_includes,
[
xdvi_xpm_save_LIBS="$LIBS"
xdvi_xpm_save_CFLAGS="$CFLAGS"
xdvi_xpm_save_CPPFLAGS="$CPPFLAGS"
xdvi_xpm_save_LDFLAGS="$LDFLAGS"
#
LIBS="$X_PRE_LIBS -lXpm -lXt -lX11 $X_EXTRA_LIBS $LIBS"
CFLAGS="$X_CFLAGS $CFLAGS"
CPPFLAGS="$X_CFLAGS $CPPFLAGS"
LDFLAGS="$X_LIBS $LDFLAGS"
#
AC_TRY_COMPILE([
#include <X11/xpm.h>
],[int a;],
[
# X11/xpm.h is in the standard search path.
xdvi_cv_xpm_includes="default"
],
[
# X11/xpm.h is not in the standard search path.
# Locate it and put its directory in `xpm_includes'
#
# /usr/include/Motif* are used on HP-UX (Motif).
# /usr/include/X11* are used on HP-UX (X and Xaw).
# /usr/dt is used on Solaris (Motif).
# /usr/openwin is used on Solaris (X and Xaw).
# Other directories are just guesses.
for dir in "$x_includes" "${prefix}/include" /usr/include /usr/local/include \
	   /usr/X11/include /usr/X11R5/include /usr/X11R6/include \
           /usr/include/Motif2.0 /usr/include/Motif1.2 /usr/include/Motif1.1 \
           /usr/include/X11R6 /usr/include/X11R5 /usr/include/X11R4 \
           /usr/dt/include /usr/openwin/include \
           /usr/dt/*/include /opt/*/include /usr/include/Motif* \
	   "${prefix}"/*/include /usr/*/include /usr/local/*/include \
	   "${prefix}"/include/* /usr/include/* /usr/local/include/*; do
if test -f "$dir/X11/xpm.h" || test -f "$dir/xpm.h"; then
xdvi_cv_xpm_includes="$dir"
break
fi
done
])
#
LIBS="$xdvi_xpm_save_LIBS"
CFLAGS="$xdvi_xpm_save_CFLAGS"
CPPFLAGS="$xdvi_xpm_save_CPPFLAGS"
LDFLAGS="$xdvi_xpm_save_LDFLAGS"
])
xpm_includes="$xdvi_cv_xpm_includes"
fi

#
#
# Now for the libraries.
#
if test "$xpm_libraries" = ""; then
AC_CACHE_VAL(xdvi_cv_xpm_libraries,
[
xdvi_xpm_save_LIBS="$LIBS"
xdvi_xpm_save_CFLAGS="$CFLAGS"
xdvi_xpm_save_CPPFLAGS="$CPPFLAGS"
xdvi_xpm_save_LDFLAGS="$LDFLAGS"
#
LIBS="$X_PRE_LIBS -lXpm -lXt -lX11 $X_EXTRA_LIBS $LIBS"
CFLAGS="$X_CFLAGS $CFLAGS"
CPPFLAGS="$X_CFLAGS $CPPFLAGS"
LDFLAGS="$X_LIBS $LDFLAGS"
xdvi_cv_xpm_libraries=
#
#
# We use XtToolkitInitialize() here since it takes no arguments
# and thus also works with a C++ compiler.
AC_TRY_LINK([
#include <X11/Intrinsic.h>
#include <X11/xpm.h>
],[XtToolkitInitialize();],
[
# libxpm.a is in the standard search path.
xdvi_cv_xpm_libraries="default"
],
[
# libXpm.a is not in the standard search path.
# Locate it and put its directory in `xpm_libraries'
#
#
# /usr/lib/Motif* are used on HP-UX (Motif).
# /usr/lib/X11* are used on HP-UX (X and Xpm).
# /usr/dt is used on Solaris (Motif).
# /usr/openwin is used on Solaris (X and Xpm).
# Other directories are just guesses.
for dir in "$x_libraries" "${prefix}/lib" /usr/lib /usr/local/lib \
	   /usr/lib/Motif2.0 /usr/lib/Motif1.2 /usr/lib/Motif1.1 \
	   /usr/lib/X11R6 /usr/lib/X11R5 /usr/lib/X11R4 /usr/lib/X11 \
           /usr/dt/lib /usr/openwin/lib \
	   /usr/dt/*/lib /opt/*/lib /usr/lib/Motif* \
	   "${prefix}"/*/lib /usr/*/lib /usr/local/*/lib \
	   "${prefix}"/lib/* /usr/lib/* /usr/local/lib/*; do
    if test -d "$dir" && test "`ls $dir/libXpm.* 2> /dev/null`" != ""; then
        xdvi_cv_xpm_libraries="$dir"
        break
    fi
done
])
#
LIBS="$xdvi_xpm_save_LIBS"
CFLAGS="$xdvi_xpm_save_CFLAGS"
CPPFLAGS="$xdvi_xpm_save_CPPFLAGS"
LDFLAGS="$xdvi_xpm_save_LDFLAGS"
])
#
xpm_libraries="$xdvi_cv_xpm_libraries"
fi
#
# Add Xpm definitions to X flags
#
if test "x$xpm_includes" != "xdefault" && test "$xpm_includes" != "" \
    && test "$xpm_includes" != "$x_includes" && test "$xpm_includes" != "no"
then
    X_CFLAGS="-I$xpm_includes $X_CFLAGS"
fi

if test "x$xpm_libraries" != "xdefault" && test "$xpm_libraries" != "" \
    && test "$xpm_libraries" != "$x_libraries" && test "$xpm_libraries" != "no"
then
case "$X_LIBS" in
  *-R\ *) X_LIBS="-L$xpm_libraries -R $xpm_libraries $X_LIBS";;
  *-R*)   X_LIBS="-L$xpm_libraries -R$xpm_libraries $X_LIBS";;
  *)      X_LIBS="-L$xpm_libraries $X_LIBS";;
esac
fi

#
#
x_xpm_libs="-lXpm"
xpm_libraries_result="$xpm_libraries"
xpm_includes_result="$xpm_includes"
if test "x$xpm_libraries_result" = "xdefault" ; then
  xpm_libraries_result="in default path"
  AC_DEFINE(USE_XPM, 1)
fi
if test "x$xpm_includes_result" = "xdefault"; then
  xpm_includes_result="in default path"
  AC_DEFINE(USE_XPM, 1)
fi
if test "$xpm_libraries_result" = no || test "x$xpm_libraries_result" = "x"; then
  xpm_libraries_result="(none)"
  AC_DEFINE(USE_XPM, 0)
  x_xpm_libs=""
fi
if test "$xpm_includes_result" = no || test "x$xpm_includes_result" = "x"; then
  AC_DEFINE(USE_XPM, 0)
  xpm_includes_result="(none)"
fi

AC_SUBST(x_xpm_libs)
AC_MSG_RESULT(
  [libraries $xpm_libraries_result, headers $xpm_includes_result])
])dnl



dnl ### Check for Motif libraries and headers
dnl Put Motif include directory in motif_include,
dnl put Motif library directory in motif_libdir,
dnl and add appropriate flags to X_CFLAGS and X_LIBS.
dnl If $1 is not `none', use Xaw as default toolkit if Motif isn't found.
AC_DEFUN(XDVI_FIND_MOTIF,
[
default_toolkit="$1"
motif_include="$2"
motif_libdir="$3"
AC_REQUIRE([AC_PATH_XTRA])
AC_MSG_CHECKING(for Motif)
#
#
# Search the include files.
#
if test "$motif_include" = ""; then
AC_CACHE_VAL(xdvi_cv_motif_include,
[
xdvi_motif_save_LIBS="$LIBS"
xdvi_motif_save_CFLAGS="$CFLAGS"
xdvi_motif_save_CPPFLAGS="$CPPFLAGS"
xdvi_motif_save_LDFLAGS="$LDFLAGS"
#
LIBS="$X_PRE_LIBS -lXm -lXt -lX11 $X_EXTRA_LIBS $LIBS"
CFLAGS="$X_CFLAGS $CFLAGS"
CPPFLAGS="$X_CFLAGS $CPPFLAGS"
LDFLAGS="$X_LIBS $LDFLAGS"
#
AC_TRY_COMPILE([#include <Xm/Xm.h>],[int a;],
[
# Xm/Xm.h is in the standard search path.
xdvi_cv_motif_include=
],
[
# Xm/Xm.h is not in the standard search path.
# Locate it and put its directory in `motif_include'
#
# /usr/include/Motif* are used on HP-UX (Motif).
# /usr/include/X11* are used on HP-UX (X and Athena).
# /usr/dt is used on Solaris (Motif).
# /usr/openwin is used on Solaris (X and Athena).
# Other directories are just guesses.
for dir in "$x_includes" "${prefix}/include" /usr/include /usr/local/include \
           /usr/include/Motif2.0 /usr/include/Motif1.2 /usr/include/Motif1.1 \
           /usr/include/X11R6 /usr/include/X11R5 /usr/include/X11R4 \
           /usr/dt/include /usr/openwin/include \
           /usr/dt/*/include /opt/*/include /usr/include/Motif* \
	   "${prefix}"/*/include /usr/*/include /usr/local/*/include \
	   "${prefix}"/include/* /usr/include/* /usr/local/include/*; do
    if test -f "$dir/Xm/Xm.h"; then
        xdvi_cv_motif_include="$dir"
        break
    fi
done
if test "$xdvi_cv_motif_include" = ""; then
xdvi_cv_motif_include=no
fi
])
#
LIBS="$xdvi_motif_save_LIBS"
CFLAGS="$xdvi_motif_save_CFLAGS"
CPPFLAGS="$xdvi_motif_save_CPPFLAGS"
LDFLAGS="$xdvi_motif_save_LDFLAGS"
])
motif_include="$xdvi_cv_motif_include"
fi
#
#
# Now for the libraries.
#
if test "$motif_libdir" = ""; then
AC_CACHE_VAL(xdvi_cv_motif_libdir,
[
xdvi_motif_save_LIBS="$LIBS"
xdvi_motif_save_CFLAGS="$CFLAGS"
xdvi_motif_save_CPPFLAGS="$CPPFLAGS"
xdvi_motif_save_LDFLAGS="$LDFLAGS"
#
LIBS="$X_PRE_LIBS -lXm -lXt -lX11 $X_EXTRA_LIBS $LIBS"
CFLAGS="$X_CFLAGS $CFLAGS"
CPPFLAGS="$X_CFLAGS $CPPFLAGS"
LDFLAGS="$X_LIBS $LDFLAGS"
#
# We use XtToolkitInitialize() here since it takes no arguments
# and thus also works with a C++ compiler.
AC_TRY_LINK([
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
],[XtToolkitInitialize();],
[
# libXm.a is in the standard search path.
xdvi_cv_motif_libdir=
],
[
# libXm.a is not in the standard search path.
# Locate it and put its directory in `motif_libdir'
#
# /usr/lib/Motif* are used on HP-UX (Motif).
# /usr/lib/X11* are used on HP-UX (X and Athena).
# /usr/dt is used on Solaris (Motif).
# /usr/lesstif is used on Linux (Lesstif).
# /usr/openwin is used on Solaris (X and Athena).
# Other directories are just guesses.
for dir in "$x_libraries" "${prefix}/lib" /usr/lib /usr/local/lib \
	   /usr/lib/Motif2.0 /usr/lib/Motif1.2 /usr/lib/Motif1.1 \
	   /usr/lib/X11R6 /usr/lib/X11R5 /usr/lib/X11R4 /usr/lib/X11 \
           /usr/dt/lib /usr/openwin/lib \
	   /usr/dt/*/lib /opt/*/lib /usr/lib/Motif* \
           /usr/lesstif*/lib /usr/lib/Lesstif* \
	   "${prefix}"/*/lib /usr/*/lib /usr/local/*/lib \
	   "${prefix}"/lib/* /usr/lib/* /usr/local/lib/*; do
    if test -d "$dir" && test "`ls $dir/libXm.* 2> /dev/null`" != ""; then
        xdvi_cv_motif_libdir="$dir"
        break
    fi
done
if test "$xdvi_cv_motif_libdir" = ""; then
xdvi_cv_motif_libdir=no
fi
])
#
LIBS="$xdvi_motif_save_LIBS"
CFLAGS="$xdvi_motif_save_CFLAGS"
CPPFLAGS="$xdvi_motif_save_CPPFLAGS"
LDFLAGS="$xdvi_motif_save_LDFLAGS"
])
#
motif_libdir="$xdvi_cv_motif_libdir"
fi
# Add Motif definitions to X flags
#
if test "$motif_include" != "" && test "$motif_include" != "$x_includes" && test "$motif_include" != "no"
then
X_CFLAGS="-I$motif_include $X_CFLAGS"
fi
if test "$motif_libdir" != "" && test "$motif_libdir" != "$x_libraries" && test "$motif_libdir" != "no"
then
case "$X_LIBS" in
  *-R\ *) X_LIBS="-L$motif_libdir -R $motif_libdir $X_LIBS";;
  *-R*)   X_LIBS="-L$motif_libdir -R$motif_libdir $X_LIBS";;
  *)      X_LIBS="-L$motif_libdir $X_LIBS";;
esac
fi
#
#
motif_libdir_result="$motif_libdir"
motif_include_result="$motif_include"
test "$motif_libdir_result" = "" && 
  motif_libdir_result="in default path" && AC_DEFINE(MOTIF, 1)
test "$motif_include_result" = "" && 
  motif_include_result="in default path" && AC_DEFINE(MOTIF, 1)
if test "$motif_libdir_result" = "no"; then
    if test "$default_toolkit" = "none"; then
        motif_libdir_result="(none)"
    else
	motif_libdir_result=""
    fi
fi
if test "$motif_include_result" = "no"; then
    if test "$default_toolkit" = "none"; then
        motif_include_result="(none)"
    else
	motif_include_result=""
    fi
fi

xdvi_using_lesstif=no

if test "$motif_include_result" != "" && test "$motif_libdir_result" != ""; then
    AC_MSG_RESULT([libraries $motif_libdir_result, headers $motif_include_result])
    prog_extension="motif"
    AC_DEFINE(MOTIF)
    x_tool_libs="-lXm"
dnl ### now check for lesstif, which doesn't work correctly
    xdvi_motif_save_LIBS="$LIBS"
    xdvi_motif_save_CFLAGS="$CFLAGS"
    xdvi_motif_save_CPPFLAGS="$CPPFLAGS"
    xdvi_motif_save_LDFLAGS="$LDFLAGS"

    LIBS="$X_PRE_LIBS -lXm -lXt -lX11 $X_EXTRA_LIBS $LIBS"
    CFLAGS="$X_CFLAGS $CFLAGS"
    CPPFLAGS="$X_CFLAGS $CPPFLAGS"
    LDFLAGS="$X_LIBS $LDFLAGS"

    AC_TRY_RUN(
    [
    #include <X11/Intrinsic.h>
    #include <Xm/Xm.h>
    int main()
    {
    #ifdef LESSTIF_VERSION
      return 0;
    #else
      return 1;
    #endif
    }],
    xdvi_using_lesstif=yes,)

    LIBS="$xdvi_motif_save_LIBS"
    CFLAGS="$xdvi_motif_save_CFLAGS"
    CPPFLAGS="$xdvi_motif_save_CPPFLAGS"
    LDFLAGS="$xdvi_motif_save_LDFLAGS"
else
    AC_MSG_RESULT([not found, using Xaw])
    prog_extension="xaw"
    AC_DEFINE(XAW)
    x_tool_libs="-lXaw"
fi

#
AC_MSG_CHECKING(whether to compile in panner (requires Xaw))
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $X_CFLAGS"
AC_TRY_COMPILE([
    #include <X11/X.h>
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/Xos.h>

    #include <X11/Xfuncs.h>
    #include <X11/Intrinsic.h>

    #include <X11/Xaw/Reports.h>
],[
],
xdvi_use_xaw_panner=yes, xdvi_use_xaw_panner=no
)
CPPFLAGS=$save_CPPFLAGS
if test $xdvi_use_xaw_panner = yes; then
    AC_MSG_RESULT(yes)
    AC_DEFINE(USE_XAW_PANNER)
else
    AC_MSG_RESULT(no)
fi
])dnl

dnl SU: the following is copied from gnome/compiler-flags.m4: turn on warnings for gcc
dnl
dnl COMPILER_WARNINGS
dnl Turn on many useful compiler warnings
dnl For now, only works on GCC
AC_DEFUN([COMPILER_WARNINGS],[
  AC_ARG_ENABLE(compiler-warnings, 
    [  --enable-compiler-warnings=[no/minimum/yes/maximum]
                          Turn on compiler warnings],,enable_compiler_warnings=minimum)

  AC_MSG_CHECKING(what warning flags to pass to the C compiler)
  warnCFLAGS=
  if test "x$GCC" != xyes; then
    enable_compiler_warnings=no
  fi

  if test "x$enable_compiler_warnings" != "xno"; then
    if test "x$GCC" = "xyes"; then
      case " $CFLAGS " in
      *[\ \	]-Wall[\ \	]*) ;;
      *) warnCFLAGS="-W -Wall -Wunused" ;;
      esac

      ## -W is not all that useful.  And it cannot be controlled
      ## with individual -Wno-xxx flags, unlike -Wall
      if test "x$enable_compiler_warnings" = "xyes"; then
        warnCFLAGS="$warnCFLAGS -pedantic -Wmissing-prototypes -Wmissing-declarations"
      elif test "x$enable_compiler_warnings" = "xmaximum"; then
      ## just turn on about everything:
      	warnCFLAGS="-Wall -Wunused -Wmissing-prototypes -Wmissing-declarations -Wimplicit -Wparentheses -Wreturn-type -Wswitch -Wtrigraphs -Wunused -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings"
      fi
    fi
  fi
  AC_MSG_RESULT($warnCFLAGS)

  ### FIXME: if we restrict setting the flags to this case only, they will get overridden
  ### somehwere else further up in the configure process.
  if test "x$cflags_set" != "xyes"; then
    XTRA_WARN_CFLAGS=$warnCFLAGS
    AC_SUBST(XTRA_WARN_CFLAGS)
#    CFLAGS="$CFLAGS $warnCFLAGS"
    cflags_set=yes
    AC_SUBST(cflags_set)
  fi
])

dnl ### Check for Xaw version
AC_DEFUN(XDVI_OLD_XAW,[
if test "${with_x_toolkit}XXX" = XXX || test "${with_x_toolkit}" = xaw; then
	AC_MSG_CHECKING(for Xaw library)
	save_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS $X_CFLAGS"
	AC_TRY_COMPILE([
	    #include <X11/X.h>
	    #include <X11/Xlib.h>
	    #include <X11/Xutil.h>
	    #include <X11/Xos.h>

	    #include <X11/Xfuncs.h>
	    #include <X11/Intrinsic.h>
	    #include <X11/Xaw/Form.h>
	],[
	],
	xdvi_have_xaw=yes, xdvi_have_xaw=no
	)
	CPPFLAGS=$save_CPPFLAGS
	if test $xdvi_have_xaw = yes; then
	    AC_MSG_RESULT(yes)
	else
	    AC_MSG_RESULT(not found)
	    AC_MSG_ERROR([Sorry, you will need at least the Xaw header/library files to compile xdvik.])
	fi

	AC_MSG_CHECKING(version of Xaw library)
	save_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS $X_CFLAGS"
	AC_TRY_COMPILE([
	    #include <X11/IntrinsicP.h>
	    #include <X11/StringDefs.h>
	    #include <X11/Intrinsic.h>
	    #include <X11/StringDefs.h>
	    #include <X11/Xos.h>
	    #include <X11/Xaw/Text.h>
	    #include <X11/Xaw/AsciiText.h>
	    #include <X11/Xaw/ToggleP.h>
	    typedef void (*XawDiamondProc) (Widget);
	    
	    /* New fields for the Radio widget class record */
	    typedef struct _RadioClass  {
	        Dimension	dsize;		/* diamond size */
	        XawDiamondProc drawDiamond;
	        /* pixmaps for the button */
	        Pixmap sel_radio;		/* selected state */
	        Pixmap unsel_radio;		/* unselected state */
	        Pixmap sel_menu;		/* selected state */
	        Pixmap unsel_menu;		/* unselected state */
	        /* TODO: 3-d and xaw-xpm features? */
	        XtPointer	extension;
	    } RadioClassPart;
	    
	    /* Full class record declaration */
	    typedef struct _RadioClassRec {
	        CoreClassPart	core_class;
	        SimpleClassPart	simple_class;
	        LabelClassPart	label_class;
	        CommandClassPart	command_class;
	        ToggleClassPart	toggle_class;
	        RadioClassPart	radio_class;
	    } RadioClassRec;
	    /* New fields for the Radio widget record */
	    typedef struct {
	        /* resources */
	        Boolean isRadio;		/* radio if True, checkbox else */
	        /* TODO: 3-d and xaw-xpm features? */
	    
	        /* private data */
	        XtPointer	extension;
	    } RadioPart;
	    
	       /* Full widget declaration */
	    typedef struct _RadioRec {
	        CorePart	core;
	        SimplePart	simple;
	    #ifdef _ThreeDP_h
	        ThreeDPart	threeD;
	    #endif
	        LabelPart	label;
	        CommandPart	command;
	        TogglePart	toggle;
	        RadioPart	radio;
	    } RadioRec;
	    
	    
	    /* The actions table from Toggle is almost perfect, but we need
	     * to override Highlight, Set, and Unset.
	     */
	    
	    #define SuperClass ((ToggleWidgetClass)&toggleClassRec)
	    
	    #define	BOX_SIZE	16
	    #define PIXMAP_OFFSET 2    /* additional space between pixmap and label */
	],[
	    RadioClassRec radioClassRec = {
	      {
	        (WidgetClass) SuperClass,		/* superclass		*/
	        "Radio",				/* class_name		*/
	        sizeof(RadioRec),			/* size			*/
	        NULL,			/* class_initialize	*/
	        NULL,			/* class_part_initialize  */
	        FALSE,				/* class_inited		*/
	        NULL,				/* initialize		*/
	        NULL,				/* initialize_hook	*/
	        XtInheritRealize,			/* realize		*/
	        NULL,			/* actions		*/
	        0,		/* num_actions		*/
	        NULL,				/* resources		*/
	        0,		/* resource_count	*/
	        NULLQUARK,				/* xrm_class		*/
	        TRUE,				/* compress_motion	*/
	        TRUE,				/* compress_exposure	*/
	        TRUE,				/* compress_enterleave	*/
	        FALSE,				/* visible_interest	*/
	        NULL,			/* destroy		*/
	        NULL,			/* resize		*/
	        NULL,			/* expose		*/
	        NULL,			/* set_values		*/
	        NULL,				/* set_values_hook	*/
	        XtInheritSetValuesAlmost,		/* set_values_almost	*/
	        NULL,				/* get_values_hook	*/
	        NULL,				/* accept_focus		*/
	        XtVersion,				/* version		*/
	        NULL,				/* callback_private	*/
	        XtInheritTranslations,		/* tm_table		*/
	        NULL,			/* query_geometry	*/
	        XtInheritDisplayAccelerator,	/* display_accelerator	*/
	        NULL				/* extension		*/
	      },  /* CoreClass fields initialization */
	      {
	        XtInheritChangeSensitive,		/* change_sensitive	*/
		NULL
              },  /* SimpleClass fields initialization */
	      {
	        0					  /* field not used	*/
	      },  /* LabelClass fields initialization */
	      {
	        0					  /* field not used	*/
	      },  /* CommandClass fields initialization */
	      {
	          NULL,				/* Set Procedure.	*/
	          NULL,			/* Unset Procedure.	*/
	          NULL				/* extension.		*/
	      },  /* ToggleClass fields initialization */
	      {
	          BOX_SIZE,
	          NULL,			/* draw procedure */
	          None,				/* selected radiobutton */
	          None,				/* unselected radiobutton */
	          None,				/* selected menubutton */
	          None,				/* unselected menubutton */
	          NULL				/* extension. */
	      }  /* RadioClass fields initialization */
	    };
	    (void)radioClassRec;
	],
	xdvi_old_xaw=no, xdvi_old_xaw=yes
	)
	CPPFLAGS=$save_CPPFLAGS
if test $xdvi_old_xaw = yes; then
    AC_MSG_RESULT(old)
    AC_DEFINE(HAVE_OLD_XAW)
else
    AC_MSG_RESULT(current)
fi
fi
])
