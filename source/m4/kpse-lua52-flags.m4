# Public macros for the TeX Live (TL) tree.
# Copyright (C) 2013 Peter Breitenlohner <tex-live@tug.org>
#
# This file is free software; the copyright holder
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# KPSE_LUA52_FLAGS
# ----------------
# Set the make variables LUA52_INCLUDES and LUA52_LIBS to
# the CPPFLAGS and LIBS required for the `-llua52' library in
# libs/luz52/ of the TL tree.
AC_DEFUN([KPSE_LUA52_FLAGS], [dnl
_KPSE_LIB_FLAGS([lua52], [lua52], [tree],
                [-IBLD/libs/lua52/include], [BLD/libs/lua52/liblua52.a], [],
                [], [${top_builddir}/../../libs/lua52/include/lua.h])[]dnl
]) # KPSE_LUA52_FLAGS

# KPSE_LUA52_DEFINES
# ------------------
# Set the make variable LUA52_DEFINES to the CPPFLAGS required when
# compiling or using the `-llua52' library.
AC_DEFUN([KPSE_LUA52_DEFINES], [dnl
AC_REQUIRE([KPSE_CHECK_WIN32])[]dnl
AC_SUBST([LUA52_DEFINES], [-DLUA_COMPAT_MODULE])
if test "x$kpse_cv_have_win32" = xno; then
  LUA52_DEFINES="$LUA52_DEFINES -DLUA_USE_POSIX"
  AC_SEARCH_LIBS([dlopen], [dl])
  if test "x$ac_cv_search_dlopen" != xno; then
    AC_CHECK_HEADER([dlfcn.h],
                    [LUA52_DEFINES="$LUA52_DEFINES -DLUA_USE_DLOPEN"],
                    [], [AC_INCLUDES_DEFAULT])
  fi
fi
]) # KPSE_LUA52_DEFINES
