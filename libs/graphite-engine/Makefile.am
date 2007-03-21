AUTOMAKE_OPTIONS = 1.6

SUBDIRS = src test

EXTRA_DIST = doc license \
             build.bat build_d.bat makefile.vc \
             README

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = silgraphite.pc

$(pkgconfig_DATA): config.status

dist-hook:
	find $(distdir) -name .svn | xargs rm -rf
