$(objdir)/t1rw.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/t1rw.hh \
	../include/lcdf/string.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/straccum.hh \
	../include/efont/t1cs.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc

$(objdir)/metrics.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/metrics.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/hashmap.hh \
	../include/lcdf/hashmap.cc \
	../include/efont/encoding.hh \
	../include/efont/pairop.hh

$(objdir)/t1unparser.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/t1unparser.hh \
	../include/efont/t1interp.hh \
	../include/efont/t1cs.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/string.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/point.hh \
	../include/lcdf/straccum.hh

$(objdir)/pairop.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/pairop.hh \
	../include/efont/encoding.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc

$(objdir)/afm.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/afm.hh \
	../include/efont/metrics.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/hashmap.hh \
	../include/lcdf/hashmap.cc \
	../include/efont/encoding.hh \
	../include/efont/pairop.hh \
	../include/efont/afmparse.hh \
	../include/lcdf/slurper.hh \
	../include/lcdf/landmark.hh \
	../include/lcdf/string.hh \
	../include/lcdf/filename.hh \
	../include/lcdf/error.hh

$(objdir)/t1item.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/t1item.hh \
	../include/efont/t1cs.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/string.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/straccum.hh \
	../include/efont/t1rw.hh \
	../include/efont/t1interp.hh \
	../include/lcdf/point.hh \
	../include/efont/t1font.hh \
	../include/lcdf/hashmap.hh \
	../include/lcdf/hashmap.cc \
	../include/lcdf/strtonum.h

$(objdir)/t1csgen.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/t1csgen.hh \
	../include/efont/t1interp.hh \
	../include/efont/t1cs.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/string.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/point.hh \
	../include/lcdf/straccum.hh

$(objdir)/psres.obj: \
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
	../include/lcdf/slurper.hh \
	../include/lcdf/landmark.hh

$(objdir)/findmet.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/findmet.hh \
	../include/lcdf/hashmap.hh \
	../include/lcdf/hashmap.cc \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/efont/afmparse.hh \
	../include/lcdf/slurper.hh \
	../include/lcdf/landmark.hh \
	../include/lcdf/string.hh \
	../include/lcdf/filename.hh \
	../include/efont/afm.hh \
	../include/efont/metrics.hh \
	../include/efont/encoding.hh \
	../include/efont/pairop.hh \
	../include/efont/amfm.hh \
	../include/efont/t1mm.hh \
	../include/efont/t1cs.hh \
	../include/efont/psres.hh

$(objdir)/otfdescrip.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/otf.hh \
	../include/efont/otfdata.hh \
	../include/lcdf/string.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc

$(objdir)/otfcmap.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/otfcmap.hh \
	../include/efont/otf.hh \
	../include/efont/otfdata.hh \
	../include/lcdf/string.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/error.hh

$(objdir)/cff.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/cff.hh \
	../include/lcdf/hashmap.hh \
	../include/lcdf/hashmap.cc \
	../include/efont/t1cs.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/string.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/error.hh \
	../include/efont/t1item.hh \
	../include/lcdf/straccum.hh \
	../include/efont/t1unparser.hh \
	../include/efont/t1interp.hh \
	../include/lcdf/point.hh

$(objdir)/otfgsub.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/otfgsub.hh \
	../include/efont/otf.hh \
	../include/efont/otfdata.hh \
	../include/lcdf/string.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/error.hh \
	../include/lcdf/straccum.hh

$(objdir)/encoding.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/encoding.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc

$(objdir)/afmparse.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/afmparse.hh \
	../include/lcdf/slurper.hh \
	../include/lcdf/landmark.hh \
	../include/lcdf/string.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/filename.hh \
	../include/efont/t1cs.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/efont/metrics.hh \
	../include/lcdf/hashmap.hh \
	../include/lcdf/hashmap.cc \
	../include/efont/encoding.hh \
	../include/efont/pairop.hh \
	../include/lcdf/strtonum.h

$(objdir)/t1font.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
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
	../include/efont/t1rw.hh \
	../include/efont/t1mm.hh \
	../include/lcdf/error.hh

$(objdir)/afmw.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/afmw.hh \
	../include/efont/afm.hh \
	../include/efont/metrics.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/hashmap.hh \
	../include/lcdf/hashmap.cc \
	../include/efont/encoding.hh \
	../include/efont/pairop.hh

$(objdir)/otfdata.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/otfdata.hh \
	../include/lcdf/string.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc

$(objdir)/otf.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/otf.hh \
	../include/efont/otfdata.hh \
	../include/lcdf/string.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/error.hh \
	../include/lcdf/straccum.hh

$(objdir)/t1bounds.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/t1bounds.hh \
	../include/efont/t1interp.hh \
	../include/efont/t1cs.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/string.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/point.hh \
	../include/lcdf/transform.hh \
	../include/lcdf/bezier.hh

$(objdir)/t1mm.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/t1mm.hh \
	../include/efont/t1cs.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/string.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/efont/t1interp.hh \
	../include/lcdf/point.hh \
	../include/lcdf/error.hh

$(objdir)/t1interp.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/t1interp.hh \
	../include/efont/t1cs.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/string.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/point.hh \
	../include/efont/t1item.hh \
	../include/lcdf/straccum.hh

$(objdir)/otfgpos.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/otfgpos.hh \
	../include/efont/otf.hh \
	../include/efont/otfdata.hh \
	../include/lcdf/string.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/error.hh \
	../include/lcdf/straccum.hh

$(objdir)/amfm.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/amfm.hh \
	../include/efont/metrics.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/lcdf/hashmap.hh \
	../include/lcdf/hashmap.cc \
	../include/efont/encoding.hh \
	../include/efont/pairop.hh \
	../include/efont/t1mm.hh \
	../include/efont/t1cs.hh \
	../include/lcdf/string.hh \
	../include/efont/afm.hh \
	../include/efont/afmparse.hh \
	../include/lcdf/slurper.hh \
	../include/lcdf/landmark.hh \
	../include/lcdf/filename.hh \
	../include/lcdf/error.hh \
	../include/efont/findmet.hh \
	../include/lcdf/straccum.hh

$(objdir)/t1cs.obj: \
	../config.h \
	$(gnuw32dir)/win32lib.h \
	../include/efont/t1cs.hh \
	../include/lcdf/permstr.hh \
	../include/lcdf/inttypes.h \
	../include/lcdf/string.hh \
	../include/lcdf/vector.hh \
	../include/lcdf/vector.cc \
	../include/efont/t1interp.hh \
	../include/lcdf/point.hh

