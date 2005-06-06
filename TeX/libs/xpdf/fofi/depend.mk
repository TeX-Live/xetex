$(srcdir)/FoFiBase.o: FoFiBase.cc ../goo/gmem.h FoFiBase.h ../goo/gtypes.h
$(srcdir)/FoFiEncodings.o: FoFiEncodings.cc FoFiEncodings.h ../goo/gtypes.h
$(srcdir)/FoFiTrueType.o: FoFiTrueType.cc ../goo/gtypes.h ../goo/gmem.h \
 ../goo/GString.h ../goo/GHash.h FoFiTrueType.h FoFiBase.h
$(srcdir)/FoFiType1.o: FoFiType1.cc ../goo/gmem.h FoFiEncodings.h \
 ../goo/gtypes.h FoFiType1.h FoFiBase.h
$(srcdir)/FoFiType1C.o: FoFiType1C.cc ../goo/gmem.h ../goo/GString.h \
 FoFiEncodings.h ../goo/gtypes.h FoFiType1C.h FoFiBase.h
