# Public macros for the TeX Live (TL) tree.
# Copyright (C) 2009-2012 Peter Breitenlohner <tex-live@tug.org>
#
# This file is free software; the copyright holder
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# KPSE_XPDF_FLAGS
# ---------------
# Provide the configure option '--with-system-xpdf' (if in the TL tree).
#
# Set the make variables XPDF_INCLUDES and XPDF_LIBS to the CPPFLAGS and
# LIBS required for the `-lxpdf' library in libs/xpdf/ of the TL tree.
AC_DEFUN([KPSE_XPDF_FLAGS],
[_KPSE_LIB_FLAGS([xpdf], [xpdf], [],
                 [-DPDF_PARSER_ONLY -IBLD/libs/xpdf -IBLD/libs/xpdf/goo -IBLD/libs/xpdf/xpdf],
                 [BLD/libs/xpdf/libxpdf.a], [],
                 [], [${top_builddir}/../../libs/xpdf/xpdf/Stream.h])[]dnl
]) # KPSE_XPDF_FLAGS

# KPSE_XPDF_OPTIONS([WITH-SYSTEM])
# --------------------------------
AC_DEFUN([KPSE_XPDF_OPTIONS],
[m4_ifval([$1],
          [AC_ARG_WITH([system-xpdf],
                       AS_HELP_STRING([--with-system-xpdf],
                                      [use installed poppler headers and library instead of xpdf library from TL (requires pkg-config)]))])[]dnl
]) # KPSE_XPDF_OPTIONS

# KPSE_XPDF_SYSTEM_FLAGS
# ----------------------
AC_DEFUN([KPSE_XPDF_SYSTEM_FLAGS],
[AC_REQUIRE([_KPSE_CHECK_PKG_CONFIG])[]dnl
if $PKG_CONFIG poppler --atleast-version=0.12; then
  POPPLER_VERSION='-DPOPPLER_VERSION=\"'`$PKG_CONFIG poppler --modversion`'\"'
  XPDF_INCLUDES="$POPPLER_VERSION `$PKG_CONFIG poppler --cflags`"
  XPDF_LIBS=`$PKG_CONFIG poppler --libs`
elif test "x$need_xpdf:$with_system_xpdf" = xyes:yes; then
  AC_MSG_ERROR([did not find poppler-0.12 or better])
fi
]) # KPSE_XPDF_SYSTEM_FLAGS
