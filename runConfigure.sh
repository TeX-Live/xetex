#!/bin/sh

# create the Work subtree where we'll actually build stuff
test -d Work || mkdir Work
cd Work

# run the TeX Live configure script, omitting a couple things that have given me trouble
../configure --prefix=/usr/local/teTeX/ --datadir=/usr/local/teTeX/share --without-omega --without-eomega --without-odvipsk --without-oxdvik

# the main configure doesn't know about ICU, so configure that separately
mkdir -p TeX/libs/icu-3.2
(	
	cd TeX/libs/icu-3.2
	../../../../TeX/libs/icu-3.2/source/configure --with-library-suffix=XeTeX --enable-static --disable-shared
	# hack the ICU platform.h file to claim that nl_langinfo() is not available - hoping for 10.2 compatibility :)
	perl -pi.bak -e 's/(define\s+U_HAVE_NL_LANGINFO(?:_CODESET)?\s+)1/$10/;' common/unicode/platform.h
)
