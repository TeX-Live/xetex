$(objdir)/t1lint.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/psres.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/hashmap.hh \
	../include/lcdf/hashmap.cc \
	../include/lcdf/string.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/filename.hh \
	../include/efont/t1rw.hh \
	../include/lcdf/straccum.hh \
	../include/efont/t1font.hh \
	../include/efont/t1cs.hh \
	../include/efont/t1item.hh \
	../include/efont/t1mm.hh \
	cscheck.hh \
	../include/efont/t1interp.hh \
	../include/lcdf/point.hh \
	../include/lcdf/clp.h \
	../include/lcdf/error.hh

$(objdir)/cscheck.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	cscheck.hh \
	../include/efont/t1interp.hh \
	../include/efont/t1cs.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/string.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/point.hh \
	../include/efont/t1item.hh \
	../include/lcdf/straccum.hh \
	../include/lcdf/error.hh \
	../include/efont/t1unparser.hh

