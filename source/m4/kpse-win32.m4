# Public macros for the TeX Live (TL) tree.
# Copyright (C) 2009-2014 Peter Breitenlohner <tex-live@tug.org>
#
# This file is free software; the copyright holder
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# KPSE_CHECK_WIN32
# ----------------
# Check for WIN32 and distinguish between MINGW32 and native.
AC_DEFUN([KPSE_CHECK_WIN32], [dnl
AC_CACHE_CHECK([for native WIN32 or MINGW32],
               [kpse_cv_have_win32],
               [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#ifndef WIN32
  choke me
#endif]],                                          [[]])],
                                  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#ifndef __MINGW32__
  choke me
#endif]],                                                             [[]])],
                                                     [kpse_cv_have_win32=mingw32],
                                                     [kpse_cv_have_win32=native])],
                                  [kpse_cv_have_win32=no])])
]) # KPSE_CHECK_WIN32

# KPSE_COND_WIN32
# ---------------
# Define the conditional WIN32.
AC_DEFUN([KPSE_COND_WIN32], [dnl
AC_REQUIRE([KPSE_CHECK_WIN32])[]dnl
AM_CONDITIONAL([WIN32], [test "x$kpse_cv_have_win32" != xno])
]) # KPSE_COND_WIN32

# KPSE_COND_MINGW32
# -----------------
# Define the conditionals WIN32 and MINGW32.
AC_DEFUN([KPSE_COND_MINGW32], [dnl
AC_REQUIRE([KPSE_COND_WIN32])[]dnl
AM_CONDITIONAL([MINGW32], [test "x$kpse_cv_have_win32" = xmingw32])
]) # KPSE_COND_MINGW32

# KPSE_COND_WIN32_WRAP
# --------------------
# Define the conditionals WIN32 and WIN32_WRAP.
AC_DEFUN([KPSE_COND_WIN32_WRAP], [dnl
AC_REQUIRE([KPSE_COND_WIN32])[]dnl
AM_CONDITIONAL([WIN32_WRAP],
               [test -r "$srcdir/../../texk/texlive/w32_wrapper/runscript.exe"])
]) # KPSE_COND_WIN32_WRAP

# KPSE_WIN32_CALL
# ---------------
# Create a callexe.c symlink.
AC_DEFUN([KPSE_WIN32_CALL], [dnl
AC_REQUIRE([KPSE_COND_WIN32])[]dnl
AM_CONDITIONAL([WIN32_CALL],
               [test -r "$srcdir/../texlive/w32_wrapper/callexe.c"])
AM_COND_IF([WIN32],
           [AC_CONFIG_LINKS([callexe.c:../texlive/w32_wrapper/callexe.c])])
]) # KPSE_WIN32_CALL
