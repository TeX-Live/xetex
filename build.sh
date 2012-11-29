#!/usr/bin/env bash
# $Id$
#
# Copyright (c) 2005-2011 Martin Schröder <martin@luatex.org>
# Copyright (c) 2009-2011 Taco Hoekwater <taco@luatex.org>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
# new script to build xetex binaries
# ----------
# Options:
#      --make      : only make, no make distclean; configure
#      --parallel  : make -j 2 -l 3.0
#      --nostrip   : do not strip binary
#      --syslibs   : build with system libraries instead of TL tree ones
#      --warnings= : enable compiler warnings
#      --mingw     : crosscompile for mingw32 from i-386linux
#      --host=     : target system for mingw32 cross-compilation
#      --build=    : build system for mingw32 cross-compilation
#      --arch=     : crosscompile for ARCH on OS X
$DEBUG

# try to find bash, in case the standard shell is not capable of
# handling the generated configure's += variable assignments
if which bash >/dev/null
then
 CONFIG_SHELL=`which bash`
 export CONFIG_SHELL
fi

# try to find gnu make; we may need it
MAKE=make;
if make -v 2>&1| grep "GNU Make" >/dev/null
then 
  echo "Your make is a GNU-make; I will use that"
elif gmake -v >/dev/null 2>&1
then
  MAKE=gmake;
  export MAKE;
  echo "You have a GNU-make installed as gmake; I will use that"
else
  echo "I can't find a GNU-make; I'll try to use make and hope that works." 
  echo "If it doesn't, please install GNU-make."
fi

ONLY_MAKE=FALSE
STRIP_XETEX=TRUE
SYSTEM_LIBS=FALSE
WARNINGS=yes
MINGWCROSS=FALSE
CONFHOST=
CONFBUILD=
MACCROSS=FALSE
JOBS_IF_PARALLEL=${JOBS_IF_PARALLEL:-3}
MAX_LOAD_IF_PARALLEL=${MAX_LOAD_IF_PARALLEL:-2}

CFLAGS="$CFLAGS -Wdeclaration-after-statement"

until [ -z "$1" ]; do
  case "$1" in
    --make      ) ONLY_MAKE=TRUE     ;;
    --nostrip   ) STRIP_XETEX=FALSE ;;
    --syslibs   ) SYSTEM_LIBS=TRUE   ;;
    --warnings=*) WARNINGS=`echo $1 | sed 's/--warnings=\(.*\)/\1/' `        ;;
    --mingw     ) MINGWCROSS=TRUE    ;;
    --host=*    ) CONFHOST="$1"      ;;
    --build=*   ) CONFBUILD="$1"     ;;
    --parallel  ) MAKE="$MAKE -j $JOBS_IF_PARALLEL -l $MAX_LOAD_IF_PARALLEL" ;;
    --arch=*    ) MACCROSS=TRUE; ARCH=`echo $1 | sed 's/--arch=\(.*\)/\1/' ` ;;
    *           ) echo "ERROR: invalid build.sh parameter: $1"; exit 1       ;;
  esac
  shift
done

#
STRIP=strip
XETEXEXE=xetex

case `uname` in
  MINGW32*    ) XETEXEXE=xetex.exe ;;
  CYGWIN*    ) XETEXEXE=xetex.exe ;;
esac

WARNINGFLAGS=--enable-compiler-warnings=$WARNINGS

B=build

if [ "$MINGWCROSS" = "TRUE" ]
then
  B=build-windows
  XETEXEXE=xetex.exe
  OLDPATH=$PATH
  PATH=/usr/mingw32/bin:$PATH
  CFLAGS="-mtune=nocona -g -O3 $CFLAGS"
  CXXFLAGS="-mtune=nocona -g -O3 $CXXFLAGS"
  : ${CONFHOST:=--host=i586-mingw32msvc}
  : ${CONFBUILD:=--build=i686-linux-gnu}
  STRIP="${CONFHOST#--host=}-strip"
  LDFLAGS="-Wl,--large-address-aware $CFLAGS"
  export CFLAGS CXXFLAGS LDFLAGS
fi

if [ "$MACCROSS" = "TRUE" ]
then
  # make sure that architecture parameter is valid
  case $ARCH in
    i386 | x86_64 | ppc | ppc64 ) ;;
    * ) echo "ERROR: architecture $ARCH is not supported"; exit 1;;
  esac
  B=build-$ARCH
  CFLAGS="-arch $ARCH -g -O2 $CFLAGS"
  CXXFLAGS="-arch $ARCH -g -O2 $CXXFLAGS"
  LDFLAGS="-arch $ARCH $LDFLAGS" 
  export CFLAGS CXXFLAGS LDFLAGS
fi


# ----------
# clean up, if needed
if [ -r "$B"/Makefile -a $ONLY_MAKE = "FALSE" ]
then
  rm -rf "$B"
elif [ ! -r "$B"/Makefile ]
then
    ONLY_MAKE=FALSE
fi

if [ ! -r configure ]
then
    ./autogen.sh
    ONLY_MAKE=FALSE
fi

if [ ! -r "$B" ]
then
  mkdir "$B"
fi

cd "$B"

CONF_OPTIONS="\
    --disable-all-pkgs \
    --disable-shared    \
    --disable-largefile \
    --disable-ptex \
    --disable-ipc \
    --enable-dump-share  \
    --enable-xetex  \
    --without-system-ptexenc \
    --without-system-kpathsea "

if [ "$SYSTEM_LIBS" = "TRUE" ] ;
then
  CONF_OPTIONS="$CONF_OPTIONS \
    --with-system-poppler \
    --with-system-freetype2 \
    --with-system-libpng \
    --with-system-teckit \
    --with-system-zlib \
    --with-system-icu \
    --with-system-graphite \
    --with-system-harfbuzz \
    --without-mf-x-toolkit --without-x "
else
  CONF_OPTIONS="$CONF_OPTIONS \
    --enable-cxx-runtime-hack \
    --without-system-poppler \
    --without-system-freetype2 \
    --without-system-libpng \
    --without-system-teckit \
    --without-system-zlib \
    --without-system-icu \
    --without-system-graphite \
    --without-system-harfbuzz \
    --without-mf-x-toolkit --without-x "
fi

if [ "$ONLY_MAKE" = "FALSE" ]
then
TL_MAKE=$MAKE ../configure  $CONFHOST $CONFBUILD  $WARNINGFLAGS $CONF_OPTIONS\
   || exit 1 
fi

$MAKE
(cd texk/web2c; $MAKE $XETEXEXE )

# go back
cd ..

if [ "$STRIP_XETEX" = "TRUE" ] ;
then
  $STRIP "$B"/texk/web2c/$XETEXEXE
else
  echo "xetex binary not stripped"
fi

if [ "$MINGWCROSS" = "TRUE" ]
then
  PATH=$OLDPATH
fi

# show the results
ls -l "$B"/texk/web2c/$XETEXEXE
