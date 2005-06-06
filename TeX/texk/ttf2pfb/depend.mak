$(objdir)/ttf2pfb.obj: \
	$(gnuw32dir)/win32lib.h \
	$(ttfdir)/freetype.h \
	$(ttfdir)/fterrid.h \
	$(ttfdir)/ftnameid.h \
	$(ttfdir)/extend/ftxpost.h \
	gbk2uni.h \
	big52uni.h

$(objdir)/getafm.obj: \
	$(gnuw32dir)/win32lib.h \
	$(gsw32dir)/gs32lib.h \
	../../gstools/ghostscript/src/gsdll.h \
	../../gstools/ghostscript/src/iapi.h \
	../../gstools/ghostscript/src/errors.h \
	$(kpathseadir)/config.h \
	$(kpathseadir)/c-auto.h \
	$(kpathseadir)/c-std.h \
	$(kpathseadir)/c-unistd.h \
	$(kpathseadir)/systypes.h \
	$(kpathseadir)/c-memstr.h \
	$(kpathseadir)/c-errno.h \
	$(kpathseadir)/c-minmax.h \
	$(kpathseadir)/c-limits.h \
	$(kpathseadir)/c-proto.h \
	$(kpathseadir)/debug.h \
	$(kpathseadir)/types.h \
	$(kpathseadir)/lib.h \
	$(kpathseadir)/progname.h \
	$(kpathseadir)/c-fopen.h \
	$(kpathseadir)/tex-file.h \
	$(kpathseadir)/c-vararg.h \
	$(kpathseadir)/line.h

$(objdir)/t1asm.obj: \
	

