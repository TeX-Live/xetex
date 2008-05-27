#!/bin/sh

# defaults in case we don't find a teTeX installation
PREFIX=/usr/local/teTeX
DATADIR=/usr/local/teTeX/share

real_path() # function to resolve symlinks in path, as not everyone has realpath(1)
{
	current_path=`pwd` && relative_path="$1"
	cd -P "${relative_path}"
	real_path=`pwd` && cd "${current_path}"
	echo "${real_path}"
}

# try to figure out where the user's TeX is, and complain if we can't find it...
KPSEWHICH=`which kpsewhich`
if [ ! -e "${KPSEWHICH}" ]; then
	echo "### No kpsewhich found -- are you sure you have TeX installed?"
	echo "### Using default paths; hope that will be OK!"
	KPSEWHICH=
else
	using_tex_dist=`dirname ${KPSEWHICH} | grep -c texbin`
	if [ "${using_tex_dist}" = "1" ]; then
		# Looks like we've got a Mac OS X "TeX Distributions" structure;
		# we can't use this as a build prefix - must find the real location
		kpsewhich_dir=`dirname ${KPSEWHICH}`
		echo "### Resolving TeXDist path ${kpsewhich_dir} to real location"
		real_bin_dir=`real_path ${kpsewhich_dir}`
		echo "### Real binaries location: ${real_bin_dir}"
		PREFIX=`echo ${real_bin_dir} | sed -e 's!/bin/.*!!;'`
	else
		PREFIX=`echo ${KPSEWHICH} | sed -e 's!/bin/.*!!;'`
	fi
	if [ ! -d "${PREFIX}" ]; then
		echo "### PREFIX ${PREFIX} is not a directory; cannot continue."
		exit 1
	fi
	HYPHEN=`kpsewhich hyphen.tex`
	if [ ! -f "${HYPHEN}" ]; then
		echo "### hyphen.tex not found -- are you sure you have TeX installed?"
	else
		DATADIR=`echo ${HYPHEN} | sed -e 's!/texmf.*!!;'`
	fi
fi

# kpsewhich always returns realpath(1) and ignores symlinks
# whilst which(1) honours symlinks. Therefore the following can be useful,
# since maybe it's not very nice to mix real/non-real paths in configure.
# (In the case of standard TL installation, PREFIX and DATADIR are identical.)
# Of course, it would be better if kpsewhich could return non-realpath.

if [ -z "$KPSEWHICH" ]; then
	WEB2CDIR="${DATADIR}/texmf/web2c"
else
	WEB2CDIR=`dirname \`kpsewhich texmf.cnf\``
	REALPATHPREFIX=`real_path ${PREFIX}`
	if [ ! "${REALPATHPREFIX}" = "${PREFIX}" ]; then
		if [ "`echo ${DATADIR} | sed -e 's!${REALPATHPREFIX}!!'`" = "${DATADIR}" ]; then
			DATADIR=`echo ${DATADIR} | sed -e "s!${REALPATHPREFIX}!${PREFIX}!"`
			echo "### DATADIR seems to contain symlink; honouring it."
		fi
		if [ "`echo ${WEB2CDIR} | sed -e 's!${REALPATHPREFIX}!!'`" = "${WEB2CDIR}" ]; then
			WEB2CDIR=`echo ${WEB2CDIR} | sed -e "s!${REALPATHPREFIX}!${PREFIX}!"`
			echo "### WEB2CDIR seems to contain symlink; honouring it."
		fi
	fi
fi

echo "### Using paths:"
echo "###   PREFIX          = ${PREFIX}"
echo "###   PREFIX realpath = ${REALPATHPREFIX}"
echo "###   DATADIR         = ${DATADIR}"
echo "###   WEB2CDIR        = ${WEB2CDIR}"

# run the TeX Live configure script
echo "### running TeX Live configure script as:"
if [ "`uname`" = "FreeBSD" ]; then
	# Required system libraries are part of the following FreeBSD ports:
	# pnglib	--> graphics/png
	# freetype2	--> print/freetype2
	# zlib		--> graphics/imlib2
	# xpdf		--> (?? not an option?)
	# These are their default positions in FreeBSD:
	LOCALBASE=/usr/local
	SYSTEM_LIBS_PATHS="--with-system-freetype2 \
                           --with-freetype2-libdir=${LOCALBASE}/lib \
                           --with-freetype2-include=${LOCALBASE}/include \
                           --with-fontconfig=${LOCALBASE} \
                           --with-system-pnglib \
                           --with-pnglib-libdir=${LOCALBASE}/lib \
                           --with-system-zlib \
                           --with-zlib-dir=${LOCALBASE}/lib/imlib2/loaders"
	# He who doesn't want to use system libraries can delete SYSTEM_LIBS_PATHS;
	# of course, this will not be the choice when XeTeX becomes a FreeBSD
	# port; all needed libraries will then be dependencies and handled
	# automatically by the Ports System. In meantime...
	CONFIGURECMD="../configure --prefix=${PREFIX} --datadir=${DATADIR} ${SYSTEM_LIBS_PATHS} --disable-threads"
else
	CONFIGURECMD="../configure --prefix=${PREFIX} --datadir=${DATADIR} --with-system-zlib --with-old-mac-fonts --disable-threads"
fi
echo ${CONFIGURECMD}
${CONFIGURECMD} || exit 1

if [ "`uname`" = "Darwin" ]; then
  if [ "`uname -p`" = "powerpc" ]; then
	# hack the resulting ICU platform.h file to claim that nl_langinfo() is not available
	# ....hoping for 10.2 compatibility :)
	perl -pi.bak -e 's/(define\s+U_HAVE_NL_LANGINFO(?:_CODESET)?\s+)1/${1}0/;' libs/icu-xetex/common/unicode/platform.h
  fi
fi
