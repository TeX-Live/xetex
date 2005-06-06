$(objdir)/t1rewrit.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	t1rewrit.hh \
	../include/efont/t1interp.hh \
	../include/efont/t1cs.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/string.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/point.hh \
	../include/efont/t1font.hh \
	../include/lcdf/hashmap.hh \
	../include/lcdf/hashmap.cc \
	../include/lcdf/straccum.hh \
	../include/efont/t1item.hh \
	../include/efont/t1csgen.hh \
	../include/lcdf/error.hh

$(objdir)/myfont.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	myfont.hh \
	../include/efont/t1font.hh \
	../include/efont/t1cs.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/string.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/hashmap.hh \
	../include/lcdf/hashmap.cc \
	../include/efont/t1item.hh \
	../include/lcdf/straccum.hh \
	../include/efont/t1interp.hh \
	../include/lcdf/point.hh \
	t1rewrit.hh \
	../include/efont/t1mm.hh \
	../include/lcdf/error.hh

$(objdir)/t1minimize.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	t1minimize.hh \
	../include/efont/t1font.hh \
	../include/efont/t1cs.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/string.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/hashmap.hh \
	../include/lcdf/hashmap.cc \
	../include/efont/t1item.hh \
	../include/lcdf/straccum.hh

$(objdir)/main.obj: \
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
	../include/efont/t1mm.hh \
	../include/efont/t1cs.hh \
	myfont.hh \
	../include/efont/t1font.hh \
	t1rewrit.hh \
	../include/efont/t1interp.hh \
	../include/lcdf/point.hh \
	t1minimize.hh \
	../include/lcdf/clp.h \
	../include/lcdf/error.hh

