test -d Work || mkdir Work

cd Work

mkdir -p TeX/libs/icu-3.2
(cd TeX/libs/icu-3.2; ../../../../TeX/libs/icu-3.2/source/runConfigureICU MacOSX --srcdir=../../../../TeX/libs/icu-3.2/source/ --with-library-suffix=XeTeX --enable-static --disable-shared)

../configure --prefix=/usr/local/teTeX/ --datadir=/usr/local/teTeX/share --without-omega --without-eomega --without-odvipsk --without-oxdvik
