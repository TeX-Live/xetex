#!/bin/sh

# How I'm currently setting things up to build XeTeX....

# create the Work subtree where we'll actually build stuff

test -d Work || mkdir Work
cd Work

# run the TeX Live configure script, omitting a couple things that have given me trouble
../configure --prefix=/usr/local/teTeX/ --datadir=/usr/local/teTeX/share --without-omega --without-eomega --without-aleph --without-odvipsk --without-oxdvik

# the main configure doesn't know about ICU, so configure that separately
mkdir -p TeX/libs/icu-release-3-4
(
	cd TeX/libs/icu-release-3-4
	../../../../TeX/libs/icu-release-3-4/source/configure --enable-static --disable-shared
	# hack the resulting ICU platform.h file to claim that nl_langinfo() is not available - hoping for 10.2 compatibility :)
	perl -pi.bak -e 's/(define\s+U_HAVE_NL_LANGINFO(?:_CODESET)?\s+)1/$10/;' common/unicode/platform.h
)

# similarly, configure TECkit separately
mkdir -p TeX/libs/teckit
(cd TeX/libs/teckit; ../../../../TeX/libs/teckit/configure)

# To build:
# "make" in Work/libs/icu-release-3-4
# "make" in Work/libs/teckit
# "make xetex" in Work/TeX/texk/web2c
# "make xdv2pdf" in Work/TeX/texk/xdv2pdf

