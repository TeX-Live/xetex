AUTOMAKE_OPTIONS = 1.6

EXTRA_DIST = \
	burmeseText.wpx \
	romanText.wpx \
	xlineText.wpx \
	grtest_burmese.ttf \
	grtest_roman.ttf \
	grtest_xline.ttf xline.gdl \
	Makefile.vc makedebug.bat \
	readme.txt

noinst_PROGRAMS = regression-test

AM_CPPFLAGS  = -I$(top_srcdir)/include

regression_test_LDFLAGS = -L$(top_builddir)/src -lgraphite

regression_test_SOURCES  = \
	main.h \
	stdafx.cpp stdafx.h \
	RegressionTest.cpp \
	TestCase.cpp TestCase.h \
	RtTextSrc.h \
	SimpleTextSrc.cpp SimpleTextSrc.h \
	GrJustifier.cpp GrJustifier.h
	
