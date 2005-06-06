$(srcdir)/Array.o: Array.cc ../goo/gmem.h Object.h ../goo/gtypes.h \
 ../goo/GString.h Array.h Dict.h Stream.h
$(srcdir)/BuiltinFont.o: BuiltinFont.cc ../goo/gmem.h FontEncodingTables.h \
 BuiltinFont.h ../goo/gtypes.h
$(srcdir)/BuiltinFontTables.o: BuiltinFontTables.cc FontEncodingTables.h \
 BuiltinFontTables.h BuiltinFont.h ../goo/gtypes.h
$(srcdir)/CMap.o: CMap.cc ../goo/gmem.h ../goo/gfile.h ../goo/gtypes.h \
 ../goo/GString.h Error.h config.h GlobalParams.h CharTypes.h \
 PSTokenizer.h CMap.h
$(srcdir)/Catalog.o: Catalog.cc ../goo/gmem.h Object.h ../goo/gtypes.h \
 ../goo/GString.h Array.h Dict.h Stream.h XRef.h Page.h Error.h \
 config.h Link.h Catalog.h
$(srcdir)/CharCodeToUnicode.o: CharCodeToUnicode.cc ../goo/gmem.h ../goo/gfile.h \
 ../goo/gtypes.h ../goo/GString.h Error.h config.h GlobalParams.h \
 CharTypes.h PSTokenizer.h CharCodeToUnicode.h
$(srcdir)/Decrypt.o: Decrypt.cc ../goo/gmem.h Decrypt.h ../goo/gtypes.h \
 ../goo/GString.h
$(srcdir)/Dict.o: Dict.cc ../goo/gmem.h Object.h ../goo/gtypes.h \
 ../goo/GString.h Array.h Dict.h Stream.h XRef.h
$(srcdir)/Error.o: Error.cc GlobalParams.h ../goo/gtypes.h CharTypes.h Error.h \
 config.h
$(srcdir)/FontEncodingTables.o: FontEncodingTables.cc FontEncodingTables.h
$(srcdir)/Function.o: Function.cc ../goo/gmem.h Object.h ../goo/gtypes.h \
 ../goo/GString.h Array.h Dict.h Stream.h Error.h config.h Function.h
$(srcdir)/Gfx.o: Gfx.cc ../goo/gmem.h GlobalParams.h ../goo/gtypes.h CharTypes.h \
 Object.h ../goo/GString.h Array.h Dict.h Stream.h Lexer.h Parser.h \
 GfxFont.h GfxState.h Function.h OutputDev.h Page.h Error.h config.h \
 Gfx.h
$(srcdir)/GfxFont.o: GfxFont.cc ../goo/gmem.h Error.h config.h Object.h \
 ../goo/gtypes.h ../goo/GString.h Array.h Dict.h Stream.h \
 GlobalParams.h CharTypes.h CMap.h CharCodeToUnicode.h \
 FontEncodingTables.h BuiltinFontTables.h BuiltinFont.h \
 ../fofi/FoFiType1.h ../fofi/FoFiBase.h ../fofi/FoFiType1C.h \
 ../fofi/FoFiTrueType.h GfxFont.h
$(srcdir)/GfxState.o: GfxState.cc ../goo/gmem.h Error.h config.h Object.h \
 ../goo/gtypes.h ../goo/GString.h Array.h Dict.h Stream.h Page.h \
 GfxState.h Function.h
$(srcdir)/GlobalParams.o: GlobalParams.cc ../goo/gmem.h ../goo/GString.h \
 ../goo/GList.h ../goo/gtypes.h ../goo/GHash.h ../goo/gfile.h Error.h \
 config.h NameToCharCode.h CharTypes.h CharCodeToUnicode.h \
 UnicodeMap.h CMap.h BuiltinFontTables.h BuiltinFont.h \
 FontEncodingTables.h GlobalParams.h NameToUnicodeTable.h \
 UnicodeMapTables.h UTF8.h
$(srcdir)/JArithmeticDecoder.o: JArithmeticDecoder.cc Object.h ../goo/gtypes.h \
 ../goo/gmem.h ../goo/GString.h Array.h Dict.h Stream.h \
 JArithmeticDecoder.h
$(srcdir)/JBIG2Stream.o: JBIG2Stream.cc ../goo/GList.h ../goo/gtypes.h Error.h \
 config.h JArithmeticDecoder.h JBIG2Stream.h Object.h ../goo/gmem.h \
 ../goo/GString.h Array.h Dict.h Stream.h Stream-CCITT.h
$(srcdir)/JPXStream.o: JPXStream.cc ../goo/gmem.h Error.h config.h \
 JArithmeticDecoder.h ../goo/gtypes.h JPXStream.h Object.h \
 ../goo/GString.h Array.h Dict.h Stream.h
$(srcdir)/Lexer.o: Lexer.cc Lexer.h Object.h ../goo/gtypes.h ../goo/gmem.h \
 ../goo/GString.h Array.h Dict.h Stream.h Error.h config.h
$(srcdir)/Link.o: Link.cc ../goo/gmem.h ../goo/GString.h Error.h config.h \
 Object.h ../goo/gtypes.h Array.h Dict.h Stream.h Link.h
$(srcdir)/NameToCharCode.o: NameToCharCode.cc ../goo/gmem.h NameToCharCode.h \
 CharTypes.h
$(srcdir)/Object.o: Object.cc Object.h ../goo/gtypes.h ../goo/gmem.h \
 ../goo/GString.h Array.h Dict.h Stream.h Error.h config.h XRef.h
$(srcdir)/Outline.o: Outline.cc ../goo/gmem.h ../goo/GString.h ../goo/GList.h \
 ../goo/gtypes.h Link.h Object.h Array.h Dict.h Stream.h \
 PDFDocEncoding.h CharTypes.h Outline.h
$(srcdir)/OutputDev.o: OutputDev.cc Object.h ../goo/gtypes.h ../goo/gmem.h \
 ../goo/GString.h Array.h Dict.h Stream.h GfxState.h Function.h \
 OutputDev.h CharTypes.h
$(srcdir)/PDFDoc.o: PDFDoc.cc ../goo/GString.h config.h GlobalParams.h \
 ../goo/gtypes.h CharTypes.h Page.h Object.h ../goo/gmem.h Array.h \
 Dict.h Stream.h Catalog.h XRef.h Link.h OutputDev.h Error.h \
 ErrorCodes.h Lexer.h Parser.h Outline.h PDFDoc.h
$(srcdir)/PDFDocEncoding.o: PDFDocEncoding.cc PDFDocEncoding.h CharTypes.h
$(srcdir)/PSTokenizer.o: PSTokenizer.cc PSTokenizer.h ../goo/gtypes.h
$(srcdir)/Page.o: Page.cc GlobalParams.h ../goo/gtypes.h CharTypes.h Object.h \
 ../goo/gmem.h ../goo/GString.h Array.h Dict.h Stream.h XRef.h Link.h \
 OutputDev.h Error.h config.h Page.h
$(srcdir)/Parser.o: Parser.cc Object.h ../goo/gtypes.h ../goo/gmem.h \
 ../goo/GString.h Array.h Dict.h Stream.h Parser.h Lexer.h XRef.h \
 Error.h config.h Decrypt.h
$(srcdir)/Stream.o: Stream.cc ../goo/gmem.h ../goo/gfile.h ../goo/gtypes.h \
 config.h Error.h Object.h ../goo/GString.h Array.h Dict.h Stream.h \
 Decrypt.h JBIG2Stream.h JPXStream.h Stream-CCITT.h
$(srcdir)/UnicodeMap.o: UnicodeMap.cc ../goo/gmem.h ../goo/gfile.h \
 ../goo/gtypes.h ../goo/GString.h ../goo/GList.h Error.h config.h \
 GlobalParams.h CharTypes.h UnicodeMap.h
$(srcdir)/XRef.o: XRef.cc ../goo/gmem.h Object.h ../goo/gtypes.h \
 ../goo/GString.h Array.h Dict.h Stream.h Lexer.h Parser.h Decrypt.h \
 Error.h config.h ErrorCodes.h XRef.h
