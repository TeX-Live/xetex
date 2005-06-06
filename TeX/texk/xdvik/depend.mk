browser.o: browser.c xdvi-config.h c-auto.h xdvi.h xdvi-debug.h version.h \
  $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h \
  $(kpathsea_srcdir)/c-fopen.h $(kpathsea_srcdir)/c-stat.h \
  util.h $(kpathsea_srcdir)/hash.h \
  $(kpathsea_srcdir)/tex-file.h \
  events.h gui/message-window.h browser.h \
  string-utils.h gui/statusline.h
dvi-draw.o: dvi-draw.c xdvi-config.h c-auto.h xdvi.h xdvi-debug.h \
  version.h $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h \
  $(kpathsea_srcdir)/c-fopen.h $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/magstep.h $(kpathsea_srcdir)/tex-file.h \
  dvi.h string-utils.h util.h $(kpathsea_srcdir)/hash.h \
  events.h x_util.h \
  dvi-init.h gui/statusline.h hypertex.h special.h tfmload.h \
  read-mapfile.h my-snprintf.h gui/mag.h gui/message-window.h dvi-draw.h \
  search-internal.h gui/search-dialog.h dvisel.h print-internal.h \
  gui/print-dialog.h encodings.h gui/pagesel.h pagehist.h psgs.h 
dvi-init.o: dvi-init.c xdvi-config.h c-auto.h xdvi.h xdvi-debug.h \
  version.h $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h dvi-init.h dvi-draw.h \
  util.h $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/tex-file.h \
  events.h x_util.h \
  mime.h gui/pagesel.h special.h \
  hypertex.h $(kpathsea_srcdir)/c-fopen.h $(kpathsea_srcdir)/magstep.h \
  $(kpathsea_srcdir)/tex-glyph.h dvi.h string-utils.h browser.h gui/sfSelFile.h \
  gui/xm_toolbar.h pagehist.h \
  gui/message-window.h search-internal.h gui/search-dialog.h dvisel.h \
  print-internal.h gui/print-dialog.h gui/statusline.h font-open.h
dvisel.o: dvisel.c \
  dvi.h gui/pagesel.h \
  xdvi.h xdvi-config.h c-auto.h xdvi-debug.h version.h \
  $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h dvisel.h dvi-init.h \
  print-internal.h gui/print-dialog.h util.h $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h \
  $(kpathsea_srcdir)/tex-file.h \
  events.h dvi-draw.h gui/message-window.h \
  gui/statusline.h
encodings.o: encodings.c xdvi-config.h c-auto.h xdvi.h xdvi-debug.h \
  version.h $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h \
  util.h $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h \
  $(kpathsea_srcdir)/tex-file.h \
  events.h encodings.h my-snprintf.h \
  gui/message-window.h
events.o: events.c xdvi-config.h c-auto.h \
  $(kpathsea_srcdir)/getopt.h \
  xdvi.h xdvi-debug.h version.h \
  $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h \
  gui/pagesel.h filehist.h special.h events.h psgs.h \
  util.h $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/tex-file.h \
  x_util.h string-utils.h gui/print-dialog.h gui/search-dialog.h \
  gui/sfSelFile.h gui/mag.h gui/help-window.h gui/message-window.h \
  dvi-draw.h gui/statusline.h hypertex.h dvi-init.h gui/Tip.h browser.h \
  search-internal.h dvisel.h print-internal.h my-snprintf.h \
  gui/selection.h encodings.h pagehist.h gui/xm_colorsel.h \
  gui/xm_toolbar.h gui/xaw_menu.h gui/xm_menu.h gui/menu.h gui/xm_prefs.h \
  gui/xm_prefs_appearance.h gui/xm_prefs_fonts.h gui/xm_prefs_page.h 
filehist.o: filehist.c \
  xdvi-config.h c-auto.h \
  xdvi.h xdvi-debug.h version.h $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h \
  $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h util.h \
  $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/tex-file.h \
  events.h my-snprintf.h \
  string-utils.h dvi-init.h gui/message-window.h filehist.h
font-open.o: font-open.c xdvi-config.h c-auto.h xdvi.h xdvi-debug.h \
  version.h $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h dvi-draw.h util.h \
  $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/tex-file.h \
  events.h my-snprintf.h \
  gui/print-log.h gui/statusline.h font-open.h $(kpathsea_srcdir)/c-fopen.h \
  $(kpathsea_srcdir)/tex-glyph.h
gf.o: gf.c xdvi-config.h c-auto.h xdvi.h xdvi-debug.h version.h \
  $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h dvi-init.h util.h \
  $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/tex-file.h \
  events.h
hypertex.o: hypertex.c alloc-debug.h \
  xdvi-config.h c-auto.h xdvi.h xdvi-debug.h \
  version.h $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h $(kpathsea_srcdir)/c-memstr.h \
  $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h \
  $(kpathsea_srcdir)/c-fopen.h $(kpathsea_srcdir)/c-stat.h \
  events.h \
  dvi-init.h gui/message-window.h util.h $(kpathsea_srcdir)/hash.h \
  $(kpathsea_srcdir)/tex-file.h \
  x_util.h \
  mime.h gui/mag.h dvi-draw.h gui/statusline.h browser.h hypertex.h \
  special.h string-utils.h gui/xm_toolbar.h my-snprintf.h pagehist.h
image-magick.o: image-magick.c \
  xdvi-config.h c-auto.h \
  xdvi.h xdvi-debug.h version.h $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h \
  $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h dvi-init.h events.h \
  dvi-draw.h util.h $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/tex-file.h \
  special.h image-magick.h
mime.o: mime.c xdvi-config.h c-auto.h xdvi.h xdvi-debug.h version.h \
  $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h hypertex.h util.h \
  $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/tex-file.h \
  events.h \
  string-utils.h mime.h browser.h gui/message-window.h \
  $(kpathsea_srcdir)/c-fopen.h $(kpathsea_srcdir)/variable.h \
  $(kpathsea_srcdir)/c-pathmx.h
my-snprintf.o: my-snprintf.c \
  xdvi-config.h c-auto.h xdvi.h \
  xdvi-debug.h version.h $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h \
  $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h my-vsnprintf.h \
  my-snprintf.h
my-vsnprintf.o: my-vsnprintf.c \
  xdvi-config.h c-auto.h xdvi.h \
  xdvi-debug.h version.h $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h \
  $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h my-vsnprintf.h
pagehist.o: pagehist.c xdvi-config.h c-auto.h xdvi.h xdvi-debug.h \
  version.h $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h util.h \
  $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/tex-file.h \
  events.h \
  string-utils.h dvi-init.h gui/statusline.h gui/xm_toolbar.h pagehist.h
pk.o: pk.c xdvi-config.h c-auto.h xdvi.h xdvi-debug.h version.h \
  $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h dvi-init.h util.h \
  $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/tex-file.h \
  events.h
print-internal.o: print-internal.c xdvi-config.h c-auto.h xdvi.h \
  xdvi-debug.h version.h $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h \
  $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h dvisel.h dvi-init.h \
  print-internal.h gui/print-dialog.h gui/print-log.h search-internal.h \
  gui/search-dialog.h events.h gui/message-window.h util.h \
  $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/tex-file.h \
  x_util.h \
  string-utils.h my-snprintf.h
psdps.o: psdps.c
psgs.o: psgs.c \
  xdvi-config.h \
  c-auto.h xdvi.h xdvi-debug.h version.h $(kpathsea_dir)/c-auto.h \
  $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h \
  $(kpathsea_srcdir)/c-errno.h $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h \
  psgs.h $(kpathsea_srcdir)/c-pathmx.h dvi-init.h dvi-draw.h events.h \
  gui/statusline.h special.h util.h $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h \
  $(kpathsea_srcdir)/tex-file.h 
psheader.o: psheader.c
psnews.o: psnews.c
read-mapfile.o: read-mapfile.c xdvi-config.h c-auto.h xdvi.h xdvi-debug.h \
  version.h $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h util.h \
  $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/tex-file.h \
  events.h \
  read-mapfile.h dvi-draw.h
search-internal.o: search-internal.c xdvi-config.h c-auto.h xdvi.h \
  xdvi-debug.h version.h $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h \
  $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h \
  dvi-init.h dvi-draw.h search-internal.h gui/search-dialog.h dvisel.h \
  print-internal.h gui/print-dialog.h gui/message-window.h \
  gui/statusline.h events.h encodings.h gui/pagesel.h util.h \
  $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/tex-file.h \
  x_util.h \
  string-utils.h gui/mag.h pagehist.h
special.o: special.c \
  xdvi-config.h \
  c-auto.h xdvi.h xdvi-debug.h version.h $(kpathsea_dir)/c-auto.h \
  $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h \
  $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h \
  $(kpathsea_srcdir)/c-fopen.h $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/line.h \
  $(kpathsea_srcdir)/tex-file.h special.h events.h hypertex.h dvi.h \
  gui/message-window.h dvi-init.h dvi-draw.h gui/statusline.h util.h \
  $(kpathsea_srcdir)/hash.h \
  image-magick.h gui/pagesel.h my-snprintf.h \
  string-utils.h psgs.h
squeeze.o: squeeze.c \
  $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/c-memstr.h 
string-utils.o: string-utils.c \
  xdvi-config.h c-auto.h xdvi.h xdvi-debug.h \
  version.h $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h string-utils.h util.h \
  $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/tex-file.h \
  events.h \
  my-vsnprintf.h my-snprintf.h
tfmload.o: tfmload.c \
  xdvi-config.h c-auto.h xdvi.h \
  xdvi-debug.h version.h $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h \
  $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h util.h \
  $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/tex-file.h \
  events.h tfmload.h
util.o: util.c xdvi-config.h c-auto.h \
  xdvi.h \
  xdvi-debug.h version.h $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h \
  $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h hypertex.h dvi-init.h \
  special.h events.h string-utils.h $(kpathsea_srcdir)/tex-file.h util.h \
  $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h \
  x_util.h \
  gui/message-window.h search-internal.h \
  gui/search-dialog.h dvisel.h print-internal.h gui/print-dialog.h \
  encodings.h filehist.h gui/xm_prefs.h 
vf.o: vf.c xdvi-config.h c-auto.h dvi.h xdvi.h xdvi-debug.h version.h \
  $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h util.h \
  $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/tex-file.h \
  events.h dvi-init.h \
  dvi-draw.h
x_util.o: x_util.c xdvi-config.h c-auto.h \
  x_util.h xdvi.h xdvi-debug.h \
  version.h $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h \
  string-utils.h util.h $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/tex-file.h \
  events.h gui/statusline.h gui/message-window.h
xdvi.o: xdvi.c xdvi-config.h c-auto.h xdvi.h xdvi-debug.h version.h \
  $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h \
  gui/xm_menu.h gui/menu.h events.h gui/xaw_menu.h gui/xm_toolbar.h \
  xserver-info.h $(kpathsea_srcdir)/c-fopen.h $(kpathsea_srcdir)/c-pathch.h \
  $(kpathsea_srcdir)/c-ctype.h $(kpathsea_srcdir)/c-stat.h \
  $(kpathsea_srcdir)/proginit.h $(kpathsea_srcdir)/tex-file.h \
  $(kpathsea_srcdir)/tex-hush.h $(kpathsea_srcdir)/tex-make.h string-utils.h \
  $(kpathsea_srcdir)/expand.h translations.h dvi-init.h c-openmx.h gui/xicon.h \
  x_util.h gui/message-window.h gui/mag.h gui/pagesel.h dvi-draw.h \
  gui/statusline.h util.h $(kpathsea_srcdir)/hash.h \
  my-snprintf.h \
  hypertex.h pagehist.h filehist.h gui/sfSelFile.h print-internal.h \
  gui/print-dialog.h gui/xm_prefsP.h pixmaps/time16.xbm \
  pixmaps/time16_mask.xbm pixmaps/magglass.xbm pixmaps/magglass_mask.xbm \
  pixmaps/drag_vert.xbm pixmaps/drag_vert_mask.xbm pixmaps/drag_horiz.xbm \
  pixmaps/drag_horiz_mask.xbm pixmaps/drag_omni.xbm \
  pixmaps/drag_omni_mask.xbm 
xserver-info.o: xserver-info.c xdvi.h xdvi-config.h c-auto.h xdvi-debug.h \
  version.h $(kpathsea_dir)/c-auto.h $(kpathsea_srcdir)/config.h $(kpathsea_srcdir)/c-std.h \
  $(kpathsea_srcdir)/c-unistd.h \
  $(kpathsea_srcdir)/systypes.h \
  $(kpathsea_srcdir)/getopt.h \
  $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
  $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
  $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
  $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
  $(kpathsea_srcdir)/c-dir.h \
  $(kpathsea_srcdir)/c-vararg.h xserver-info.h
