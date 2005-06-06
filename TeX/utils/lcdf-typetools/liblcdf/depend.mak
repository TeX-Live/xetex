$(objdir)/string.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/lcdf/string.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/straccum.hh

$(objdir)/transform.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/lcdf/transform.hh \
	../include/lcdf/bezier.hh \
	../include/lcdf/point.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc

$(objdir)/straccum.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/lcdf/straccum.hh \
	../include/lcdf/string.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc

$(objdir)/landmark.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/lcdf/landmark.hh \
	../include/lcdf/string.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h

$(objdir)/filename.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/lcdf/filename.hh \
	../include/lcdf/string.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/landmark.hh

$(objdir)/clp.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/lcdf/clp.h

$(objdir)/point.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/lcdf/point.hh

$(objdir)/strtonum.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/lcdf/strtonum.h

$(objdir)/fixlibc.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h

$(objdir)/permstr.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h

$(objdir)/error.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/lcdf/error.hh \
	../include/lcdf/string.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/straccum.hh \
	../include/lcdf/hashmap.hh \
	../include/lcdf/hashmap.cc

$(objdir)/slurper.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/lcdf/slurper.hh \
	../include/lcdf/landmark.hh \
	../include/lcdf/string.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/filename.hh

$(objdir)/bezier.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/lcdf/bezier.hh \
	../include/lcdf/point.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc

$(objdir)/vectorv.obj: \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc

