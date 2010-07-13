# Public macros for the TeX Live (TL) tree.
# Copyright (C) 1995 - 2009 Karl Berry <tex-live@tug.org>
# Copyright (C) 2009, 2010 Peter Breitenlohner <tex-live@tug.org>
#
# This file is free software; the copyright holders
# give unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# serial 0

# KPSE_LIBS_PREPARE
# -----------------
# Enforce inclusion of this file.
AC_DEFUN([KPSE_LIBS_PREPARE], [])

# _KPSE_INIT()
# ------------
# Initialize infrastructure for libraries and programs in the TL tree.
# If in the TL tree, define kpse_TL as relative path to the TL root.
AC_DEFUN([_KPSE_INIT],
[## $0: Initialize TL infrastructure
m4_syscmd([test -f ../../texk/kpathsea/doc/kpathsea.texi])[]dnl
m4_if(m4_sysval, [0], [m4_define([kpse_TL], [../../])])[]dnl
m4_ifdef([kpse_TL],
[kpse_BLD=`(cd "./kpse_TL()." && pwd)`
kpse_SRC=`(cd "$srcdir/kpse_TL()." && pwd)`
])[]dnl
]) # _KPSE_INIT

# KPSE_INIT()
# -----------
# Initialize, if not automatically done so via KPSE_*_FLAGS
AC_DEFUN([KPSE_INIT],
[AC_REQUIRE([_KPSE_INIT])])

# _KPSE_USE_LIBTOOL()
# -------------------
AC_DEFUN([_KPSE_USE_LIBTOOL],
[## $0: Generate a libtool script for use in configure tests
AC_PROVIDE_IFELSE([LT_INIT], ,
                  [m4_fatal([$0: requires libtool])])[]dnl
LT_OUTPUT
m4_append([AC_LANG(C)],
[ac_link="./libtool --mode=link --tag=CC $ac_link"
])[]dnl
AC_PROVIDE_IFELSE([AC_PROG_CXX],
[m4_append([AC_LANG(C++)],
[ac_link="./libtool --mode=link --tag=CXX $ac_link"
])])[]dnl
AC_LANG(_AC_LANG)[]dnl
]) # _KPSE_USE_LIBTOOL

# _KPSE_LIB_FLAGS(LIBDIR, LIBNAME, OPTIONS,
#                 TL-INCLUDES, TL-LIBS, TL-EXTRA,
#                 [REBUILD-SRC-DEPENDENCIES],
#                 [REBUILD-BLD-DEPENDENCIES])
# -----------------------------------------------
# For generic libraries in libs/LIBDIR.
# Provide the configure options '--with-system-LIBDIR' (if in the TL tree),
# '--with-LIBDIR-includes', and '--with-LIBDIR-libdir'.
# Options:
#          lt - this is a Libtool library
#          tree - only use library from the TL tree
#
# Set the make variables LIBDIR_INCLUDES and LIBDIR_LIBS to the CPPFLAGS and
# LIBS required for the library in the directory libs/LIBDIR/ of the TL tree.
#
# If an installed (system) version of the library has been selected or is
# implied, then execute KPSE_LIBDIR_SYSTEM_FLAGS to set the two variables.
#
# Otherwise, set LIBDIR_INCLUDES based on the value of TL_INCLUDES and
# LIBDIR_LIBS based on the value of TL_LIBS; the optional shell code
# TL-EXTRA may modifiy these values.
# If OPTIONS specifies a Libtool library, then arrange that future configure
# test use Libtool. 
# Furthermore, set LIBDIR_DEPEND and LIBDIR_RULE to values suitable as
# dependency and (multiline) Make rule to rebuild the library (assuming that
# '${top_srcdir}/../../' and '${top_builddir}/../../' point to the root of
# the TL tree).
AC_DEFUN([_KPSE_LIB_FLAGS],
[AC_REQUIRE([_KPSE_INIT])[]dnl
## $0: Setup $1 (-l$2) flags
m4_ifdef([kpse_TL],
         [_KPSE_LIB_FLAGS_TL($@)],
         [_KPSE_LIB_FLAGS_STANDALONE($@)])[]dnl
AC_SUBST(AS_TR_CPP($1)[_INCLUDES])[]dnl
AC_SUBST(AS_TR_CPP($1)[_LIBS])[]dnl
AC_SUBST(AS_TR_CPP($1)[_DEPEND])[]dnl
AC_SUBST(AS_TR_CPP($1)[_RULE])[]dnl
m4_provide_if([AM_INIT_AUTOMAKE], [_AM_SUBST_NOTMAKE(AS_TR_CPP($1)[_RULE])])[]dnl
]) # _KPSE_LIB_FLAGS

# _KPSE_TEXLIB_FLAGS(LIBDIR, LIBNAME, OPTIONS,
#                    TL-INCLUDES, TL-LIBS, TL-EXTRA,
#                    [REBUILD-SRC-DEPENDENCIES],
#                    [REBUILD-BLD-DEPENDENCIES])
# -----------------------------------------------
# As above, but for TeX specific libraries in texk/LIBDIR.
AC_DEFUN([_KPSE_TEXLIB_FLAGS],
[m4_pushdef([Kpse_TeX_Lib], [])_KPSE_LIB_FLAGS($@)m4_popdef([Kpse_TeX_Lib])])

# _KPSE_LIB_FLAGS_TL(LIBDIR, LIBNAME, OPTIONS,
#                    TL-INCLUDES, TL-LIBS, TL-EXTRA,
#                    [REBUILD-SRC-DEPENDENCIES],
#                    [REBUILD-BLD-DEPENDENCIES])
# --------------------------------------------------
# Internal subroutine for use of _KPSE_LIB_FLAGS inside the TL tree.
m4_define([_KPSE_LIB_FLAGS_TL],
[m4_if(m4_index([ $3 ], [ lt ]), [-1], ,
       [AC_REQUIRE([_KPSE_USE_LIBTOOL])])[]dnl m4_if
m4_if(m4_index([ $3 ], [ tree ]), [-1],
[KPSE_]AS_TR_CPP([$1])[_OPTIONS([with-system])[]dnl
if test "x$with_system_[]AS_TR_SH($1)" = xyes; then
  AS_TR_CPP([kpse-$1-system-flags])[]dnl
else
])[]dnl m4_if
  AS_TR_CPP($1)[_INCLUDES=`echo '$4' | sed \
    -e "s,SRC/,$kpse_SRC/,g" \
    -e "s,BLD/,$kpse_BLD/,g"`]
  AS_TR_CPP($1)[_LIBS=`echo '$5' | sed \
    -e "s,BLD/,$kpse_BLD/,g"`
  $6]
  m4_ifdef([Kpse_TeX_Lib],
  [AS_TR_CPP($1)[_DEPEND=`echo '$5' | sed \
    -e 's,BLD/texk/,${top_builddir}/../,g'`]
   AS_TR_CPP($1)[_RULE='# Rebuild lib$2
$(]AS_TR_CPP($1)[_DEPEND):]m4_ifval([$7],
                                    [[ $7]])m4_ifval([$8], [[ $8
	cd ${top_builddir}/../$1 && $(MAKE) $(AM_MAKEFLAGS) rebuild
$8:]])[
	cd ${top_builddir}/../$1 && $(MAKE) $(AM_MAKEFLAGS) rebuild']],
  [AS_TR_CPP($1)[_DEPEND=`echo '$5' | sed \
    -e 's,BLD/,${top_builddir}/../../,g'`]
   AS_TR_CPP($1)[_RULE='# Rebuild lib$2
$(]AS_TR_CPP($1)[_DEPEND):]m4_ifval([$7],
                                    [[ $7]])m4_ifval([$8], [[ $8
	cd ${top_builddir}/../../libs/$1 && $(MAKE) $(AM_MAKEFLAGS) rebuild
$8:]])[
	cd ${top_builddir}/../../libs/$1 && $(MAKE) $(AM_MAKEFLAGS) rebuild']])
m4_if(m4_index([ $3 ], [ tree ]), [-1],
      [fi
])[]dnl m4_if
]) # _KPSE_LIB_FLAGS_TL

# _KPSE_LIB_FLAGS_STANDALONE(LIBDIR, LIBNAME, OPTIONS)
# ----------------------------------------------------
# Internal subroutine for standalone use of _KPSE_LIB_FLAGS.
m4_define([_KPSE_LIB_FLAGS_STANDALONE],
[m4_if(m4_index([ $3 ], [ tree ]), [-1],
[KPSE_]AS_TR_CPP([$1])[_OPTIONS([])]dnl
[KPSE_]AS_TR_CPP([$1])[_SYSTEM_FLAGS],
[m4_fatal([$0: not in TL tree])])[]dnl m4_if
]) # _KPSE_LIB_FLAGS_STANDALONE

# _KPSE_LIB_OPTIONS(LIBDIR, [WITH-SYSTEM])
# ----------------------------------------
# Internal subroutine: default configure options for system library,
# including '--with-system-LIBDIR' if WITH-SYSTEM is nonempty.
m4_define([_KPSE_LIB_OPTIONS],
[m4_ifval([$2],
          [AC_ARG_WITH([system-$1],
                       AS_HELP_STRING([--with-system-$1],
                                      [use installed $1 headers and library]))])[]dnl
AC_ARG_WITH([$1-includes],
            AS_HELP_STRING([--with-$1-includes=DIR],
                           [$1 headers installed in DIR]))[]dnl
AC_ARG_WITH([$1-libdir],
            AS_HELP_STRING([--with-$1-libdir=DIR],
                           [$1 library installed in DIR]))[]dnl
]) # _KPSE_LIB_OPTIONS

# _KPSE_LIB_FLAGS_SYSTEM(LIBDIR, LIBNAME)
# ---------------------------------------
# Internal subroutine: default flags for system library.
m4_define([_KPSE_LIB_FLAGS_SYSTEM],
[if test "x$with_[]AS_TR_SH($1)_includes" != x && test "x$with_[]AS_TR_SH($1)_includes" != xyes; then
  AS_TR_CPP($1)_INCLUDES="-I$with_[]AS_TR_SH($1)_includes"
fi
AS_TR_CPP($1)_LIBS="-l$2"
if test "x$with_[]AS_TR_SH($1)_libdir" != x && test "x$with_[]AS_TR_SH($1)_libdir" != xyes; then
  AS_TR_CPP($1)_LIBS="-L$with_[]AS_TR_SH($1)_libdir $AS_TR_CPP($1)_LIBS"
fi
]) # _KPSE_LIB_FLAGS_SYSTEM

# KPSE_SAVE_FLAGS
# ---------------
# Save values of CPPFLAGS and LIBS.
AC_DEFUN([KPSE_SAVE_FLAGS],
[kpse_save_CPPFLAGS=$CPPFLAGS
kpse_save_LIBS=$LIBS
]) # KPSE_SAVE_FLAGS

# KPSE_RESTORE_FLAGS
# ------------------
# Restore values of CPPFLAGS and LIBS.
AC_DEFUN([KPSE_RESTORE_FLAGS],
[AC_REQUIRE([KPSE_SAVE_FLAGS])[]dnl
CPPFLAGS=$kpse_save_CPPFLAGS
LIBS=$kpse_save_LIBS
]) # KPSE_RESTORE_FLAGS

# KPSE_ADD_FLAGS(LIBDIR)
# ----------------------
# Add flags for LIBDIR to values of CPPFLAGS and LIBS.
AC_DEFUN([KPSE_ADD_FLAGS],
[AC_REQUIRE([KPSE_SAVE_FLAGS])[]dnl
eval CPPFLAGS=\"$[]AS_TR_CPP($1)_INCLUDES \$CPPFLAGS\"
eval LIBS=\"$[]AS_TR_CPP($1)_LIBS \$LIBS\"
]) # KPSE_ADD_FLAGS

# KPSE_COMMON(PACKAGE-NAME, [MORE-AUTOMAKE-OPTIONS])
# --------------------------------------------------
# Common Autoconf code for all programs using libkpathsea.
# Originally written by Karl Berry as texk/kpathsea/common.ac.
#
# Initialization of Automake and Libtool, some common tests.
AC_DEFUN([KPSE_COMMON],
[dnl Remember PACKAGE-NAME as Kpse_Package (for future messages)
m4_define([Kpse_Package], [$1])
dnl
AM_INIT_AUTOMAKE([foreign]m4_ifval([$2], [ $2]))
AM_MAINTAINER_MODE
dnl
LT_PREREQ([2.2.6])
LT_INIT([win32-dll])
dnl
AC_SYS_LARGEFILE
AC_FUNC_FSEEKO
dnl
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_FUNC_CLOSEDIR_VOID
AC_CHECK_HEADERS([assert.h float.h limits.h memory.h pwd.h stdlib.h \
                  string.h strings.h sys/param.h unistd.h])
dnl
dnl Replacement functions that may be required on ancient broken system.
AC_CHECK_FUNCS([putenv strcasecmp strtol strstr])
dnl
dnl More common functions
AC_CHECK_FUNCS([bcmp bcopy bzero getcwd getwd index memcmp memcpy mkstemp mktemp rindex strchr strrchr])
dnl
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
dnl
dnl Check whether struct stat provides high-res time.
AC_CHECK_MEMBERS([struct stat.st_mtim])
dnl
dnl Check whether prototypes work.
AC_CACHE_CHECK([whether the compiler accepts prototypes],
               [kb_cv_c_prototypes],
               [AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdarg.h>]],
                                                [[extern void foo(int i,...);]])],
                               [kb_cv_c_prototypes=yes],
                               [kb_cv_c_prototypes=no])])
if test "x$kb_cv_c_prototypes" = xno; then
  AC_MSG_ERROR([Sorry, your compiler does not understand prototypes.])
fi
dnl
dnl This is a GNU libc invention.
AC_CACHE_CHECK([whether program_invocation_name is predefined],
               [kb_cv_var_program_inv_name],
               [AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],
                                                [[extern char *program_invocation_name;
                                                  program_invocation_name = "love";]])],
                               [kb_cv_var_program_inv_name=yes],
                               [kb_cv_var_program_inv_name=no])])
if test "x$kb_cv_var_program_inv_name" = xyes; then
  AC_DEFINE([HAVE_PROGRAM_INVOCATION_NAME], 1,
            [Define to 1 if you are using GNU libc or otherwise have global variables
             `program_invocation_name' and `program_invocation_short_name'.])
fi
dnl
dnl Enable flags for compiler warnings
KPSE_COMPILER_WARNINGS
]) # KPSE_COMMON

# KPSE_MSG_WARN(PROBLEM)
# ----------------------
# Same as AC_MSG_WARN, but terminate if `--disable-missing' was given.
AC_DEFUN([KPSE_MSG_WARN],
[AC_REQUIRE([_KPSE_MSG_WARN_PREPARE])[]dnl
AC_MSG_WARN([$1])
AS_IF([test "x$enable_missing" = xno],
      [AC_MSG_ERROR([terminating.])])
]) # KPSE_MSG_WARN

# _KPSE_MSG_WARN_PREPARE
# ----------------------
# Internal subroutine.
AC_DEFUN([_KPSE_MSG_WARN_PREPARE],
[AC_ARG_ENABLE([missing],
               AS_HELP_STRING([--disable-missing],
                              [terminate if a requested program or feature must
                               be disabled, e.g., due to missing libraries]))[]dnl
]) # _KPSE_MSG_WARN_PREPARE

# _KPSE_CHECK_PKG_CONFIG
# ----------------------
# Check for pkg-config
AC_DEFUN([_KPSE_CHECK_PKG_CONFIG],
[AC_REQUIRE([AC_CANONICAL_HOST])[]dnl
AC_CHECK_TOOL([PKG_CONFIG], [pkg-config], [false])[]dnl
]) # _KPSE_CHECK_PKG_CONFIG

# KPSE_CANONICAL_HOST
# -------------------
# Require both --host and --build for cross compilations; set kpse_build_alias.
AC_DEFUN([KPSE_CANONICAL_HOST],
[AC_REQUIRE([AC_CANONICAL_HOST])[]dnl
AS_IF([test "x$host_alias" != x && test "x$build_alias" = x],
      [AC_MSG_ERROR([when cross-compiling you must specify both --host and --build.])])
eval kpse_build_alias=\${build_alias-$build}
]) # KPSE_CANONICAL_HOST

