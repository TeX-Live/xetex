$(srcdir)/Hash.o: GHash.cc gmem.h GString.h GHash.h gtypes.h
$(srcdir)/List.o: GList.cc gmem.h GList.h gtypes.h
$(srcdir)/String.o: GString.cc gtypes.h GString.h
$(srcdir)/file.o: gfile.cc GString.h gfile.h gtypes.h
$(srcdir)/mempp.o: gmempp.cc gmem.h
$(srcdir)/mem.o: gmem.c gmem.h
$(srcdir)/arseargs.o: parseargs.c parseargs.h gtypes.h
