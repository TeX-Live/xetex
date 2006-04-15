# # # # # # # # # #
# PREAMBLE
# # # # # # # # # #

Summary: An extension of TeX (and LaTeX/ConTeXt) with Unicode and OpenType support
Name: xetex
Icon: xetex.xpm
Version: 0.991
Release: 1
Copyright: CPL
Group: Productivity/Publishing/TeX/Base
Source: http://scripts.sil.org/svn-view/xetex/TAGS/xetex-%{version}.tar.gz
URL: http://scripts.sil.org/xetex
Vendor: SIL International
Packager: Jonathan Kew <jonathan_kew@sil.org>

# not sure if the tetex version *really* needs to be 3.0 for xetex, but let's be safe...
# setup of older versions might be different enough to break things for us
Requires: tetex >= 3.0

# te_latex is required because we intend to build a xelatex format during installation
Requires: te_latex >= 3.0

# we also need fontconfig, freetype2 and ImageMagick libraries
Requires: fontconfig
Requires: freetype2
Requires: ImageMagick

# finally, we need the xdvipdfmx driver to be present
Requires: xdvipdfmx

# to build, we need flex, bison, and various -devel packages...
BuildRequires: flex
BuildRequires: bison
BuildRequires: fontconfig-devel
BuildRequires: freetype2-devel
BuildRequires: ImageMagick-devel

%description
XeTeX extends the TeX typesetting system (and macro packages
such as LaTeX and ConTeXt) to have native support for the
Unicode character set, including complex Asian scripts, and
for OpenType and TrueType fonts.

# # # # # # # # # #
# PREP
# # # # # # # # # #

%prep

# setup macro does standard clean-and-unpack
%setup

# # # # # # # # # #
# BUILD
# # # # # # # # # #

%build
sh ./build-xetex

# # # # # # # # # #
# INSTALL
# # # # # # # # # #

%install

# the makefile from the xetex tarball configuration doesn't know how to
# "make install" properly, so we have a custom script for now
# (eventually, a merge with texlive should lead to this getting fixed)

# this script also creates the xetex-installed-files list
sh ./install-xetex

# update the kpathsearch databases
texhash

# # # # # # # # # #
# CONFIGURATION
# # # # # # # # # #

%post

# we build the format files here, to get language config of the target system
sh ./rebuild-formats

# # # # # # # # # #
# REMOVAL
# # # # # # # # # #

%postun

# after uninstalling, remove format files and disable entries in fmtutil.cnf
formats=xetex xelatex
fmtutil=`type -p fmtutil-sys` || fmtutil=`type -p fmtutil`
for f in formats; do
	fmt=`kpsewhich --progname=xetex ${f}.fmt`
	if [ "x${fmt}" != "x" ]; then rm ${fmt}; fi
	if [ "x${fmtutil}" != "x" ]; then ${fmtutil} --disablefmt ${f}; fi
done

# update symlinks and the filename databases
texlinks
texhash

# # # # # # # # # #
# FILE LIST
# # # # # # # # # #

%files -f Work/xetex-installed-files
