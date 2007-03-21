AC_PREREQ(2.54)

AC_INIT(silgraphite, 2.2.0, silgraphite-devel@lists.sourceforge.net)
AC_CONFIG_SRCDIR(src/segment/GrEngine.cpp)
AC_CONFIG_AUX_DIR(config)

# setup automake
AM_INIT_AUTOMAKE(foreign)
AM_MAINTAINER_MODE

# System checks.
AC_CANONICAL_HOST
AC_C_BIGENDIAN

# Optional features
AC_ARG_ENABLE(tracing,
    AC_HELP_STRING([--enable-tracing],
        [build with support for trace Graphite logs (default=no)]))
AC_ARG_ENABLE(debug,
    AC_HELP_STRING([--enable-debug],
        [build with debugging. (default=no)]))
AC_ARG_ENABLE(final,
    AC_HELP_STRING([--enable-final],
        [build with optimizations and no debugging. (default=no)]))
AC_ARG_ENABLE(profile,
    AC_HELP_STRING([--enable-profile],
        [allow profiling (default=no)]))
AC_ARG_ENABLE(profilefn,
    AC_HELP_STRING([--enable-profilefn],
        [allow functioncheck profiling (default=no)]))

# Checks for programs.
AC_LANG_CPLUSPLUS
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_CHECK_SIZEOF(wchar_t)
AC_SUBST(SIZEOF_WCHAR_T)

# Checks for library functions.

# Setup the build compiler flags and linker flags.
# Enable debug
build_flags=""
if test "$enable_debug" = yes; then
  build_flags="$build_flags -O0 -g -Wall -Wno-unknown-pragmas -Wparentheses -Werror"
else
  build_flags="$build_flags -DNDEBUG"
fi

# Enable Graphite Trace logs - these are only used if a 
# LayoutEnvironment has a valid ostream pointer
if test "$enable_tracing" = yes; then
  build_flags="$build_flags -DTRACING"
fi

# Enable final
if test "$enable_final" = yes; then
  build_flags="$build_flags -O3 -DNDEBUG"
fi

# Enable profile
if test "$enable_profile" = yes; then
  build_flags="$build_flags -pg "
fi
if test "$enable_profilefn" = yes; then
  build_flags="$build_flags -g -finstrument-functions "
  LIBS="$LIBS -g -finstrument-functions -lfnccheck "
fi

CFLAGS="$CFLAGS $build_flags"
CXXFLAGS="$CXXFLAGS $build_flags"

AC_CONFIG_FILES(Makefile \
		silgraphite.pc:installed-top.pc.in:silgraphite.pc.in \
		silgraphite-uninstalled.pc:uninstalled-top.pc.in:silgraphite.pc.in \
		src/Makefile \
		test/Makefile \
		test/RegressionTest/Makefile)

AC_OUTPUT
