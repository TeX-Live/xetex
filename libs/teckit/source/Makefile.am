teckitdir = $(top_srcdir)/source

ZLIBSRCDIR = @ZLIBSRCDIR@
ZLIBDIR = @ZLIBDIR@

if SYSTEM_ZLIB
ZLIB_CFLAGS = @ZLIBCPPFLAGS@
else
ZLIB_CFLAGS = -I$(ZLIBSRCDIR)
endif

AM_CPPFLAGS  += $(ZLIBCPPFLAGS)
AM_CFLAGS    = $(AM_CPPFLAGS)

if OLD_LIB_NAMES
OLD_LIB_LDFLAGS = -module -avoid-version -Wl,--add-stdcall-alias
TECkit_x86_la_CFLAGS = $(ZLIB_CFLAGS)
TECkit_x86_la_SOURCES = $(teckitdir)/Engine.cpp $(teckitdir)/Engine_ver.rc
TECkit_x86_la_LDFLAGS = $(LDFLAGS) $(OLD_LIB_LDFLAGS) -no-undefined $(LDZLIB)
TECkit_x86_la_DEPENDENCIES = $(ZLIBDEP)
TECkit_Compiler_x86_la_CFLAGS = $(ZLIB_CFLAGS)
TECkit_Compiler_x86_la_SOURCES = $(teckitdir)/Compiler.cpp
TECkit_Compiler_x86_la_SOURCES += $(teckitdir)/UnicodeNames.cpp
TECkit_Compiler_x86_la_LDFLAGS = $(LDFLAGS) $(OLD_LIB_LDFLAGS) -no-undefined $(LDZLIB)
TECkit_Compiler_x86_la_DEPENDENCIES = $(ZLIBDEP)
else
libTECkit_la_CFLAGS = $(ZLIB_CFLAGS)
libTECkit_la_SOURCES = $(teckitdir)/Engine.cpp
libTECkit_la_LDFLAGS = $(LDFLAGS) -no-undefined $(LDZLIB)
libTECkit_la_DEPENDENCIES = $(ZLIBDEP)
libTECkit_Compiler_la_CFLAGS = $(ZLIB_CFLAGS)
libTECkit_Compiler_la_SOURCES = $(teckitdir)/Compiler.cpp
libTECkit_Compiler_la_SOURCES += $(teckitdir)/UnicodeNames.cpp
libTECkit_Compiler_la_LDFLAGS = $(LDFLAGS) -no-undefined $(LDZLIB)
libTECkit_Compiler_la_DEPENDENCIES = $(ZLIBDEP)
endif

if OLD_LIB_NAMES
# --> RC Support, only for the Windows libs
TECkit_x86_la_DEPENDENCIES += Engine_ver.o
TECkit_x86_la_LDFLAGS      += -Wl,Engine_ver.o
Engine_ver.o: $(teckitdir)/Engine_ver.rc
	$(RC) $(AM_RCFLAGS) $(RCFLAGS) -o $@ $<

TECkit_Compiler_x86_la_DEPENDENCIES += Compiler_ver.o
TECkit_Compiler_x86_la_LDFLAGS      += -Wl,Compiler_ver.o
Compiler_ver.o: $(teckitdir)/Compiler_ver.rc
	$(RC) $(AM_RCFLAGS) $(RCFLAGS) -o $@ $<
# <-- RC Support	
endif


noinst_HEADERS = $(teckitdir)/Compiler.h
noinst_HEADERS += $(teckitdir)/Engine.h
noinst_HEADERS += $(teckitdir)/ulong_chartraits.h
noinst_HEADERS += $(teckitdir)/TECkit_Format.h

EXTRA_DIST = $(teckitdir)/NormalizationData.c

include $(top_srcdir)/source/Public-headers/Makefile.am

