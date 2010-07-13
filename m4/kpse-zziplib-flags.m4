# Public macros for the TeX Live (TL) tree.
# Copyright (C) 2010 Peter Breitenlohner <tex-live@tug.org>
#
# This file is free software; the copyright holder
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# serial 0

# KPSE_ZZIPLIB_FLAGS
# -----------------
# Provide the configure options '--with-system-zziplib' (if in the TL tree),
# '--with-zziplib-includes', and '--with-zziplib-libdir'.
#
# Set the make variables ZZIPLIB_INCLUDES and ZZIPLIB_LIBS to the CPPFLAGS and
# LIBS required for the `-lpng' library in libs/zziplib/ of the TL tree.
AC_DEFUN([KPSE_ZZIPLIB_FLAGS],
[AC_REQUIRE([KPSE_ZLIB_FLAGS])[]dnl
_KPSE_LIB_FLAGS([zziplib], [zzip], [],
                [-IBLD/libs/zziplib/include], [BLD/libs/zziplib/libzzip.a], [],
                [], [${top_builddir}/../../libs/zziplib/include/zzip/zzip.h])[]dnl
]) # KPSE_ZZIPLIB_FLAGS

# KPSE_ZZIPLIB_OPTIONS([WITH-SYSTEM])
# ----------------------------------
AC_DEFUN([KPSE_ZZIPLIB_OPTIONS],
[m4_ifval([$1],
          [AC_ARG_WITH([system-zziplib],
                       AS_HELP_STRING([--with-system-zziplib],
                                      [use installed zziplib headers and library
                                       (requires pkg-config)]))])[]dnl
]) # KPSE_ZZIPLIB_OPTIONS

# KPSE_ZZIPLIB_SYSTEM_FLAGS
# ------------------------
AC_DEFUN([KPSE_ZZIPLIB_SYSTEM_FLAGS],
[AC_REQUIRE([_KPSE_CHECK_PKG_CONFIG])[]dnl
if $PKG_CONFIG zziplib --atleast-version=0.12; then
  ZZIPLIB_INCLUDES=`$PKG_CONFIG zziplib --cflags`
  ZZIPLIB_LIBS=`$PKG_CONFIG zziplib --libs`
elif test "x$need_zziplib:$with_system_zziplib" = xyes:yes; then
  AC_MSG_ERROR([did not find zziplib-0.12 or better])
fi
]) # KPSE_ZZIPLIB_SYSTEM_FLAGS
