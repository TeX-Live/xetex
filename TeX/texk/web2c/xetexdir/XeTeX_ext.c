/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

/* XeTeX_ext.c
 * additional plain C extensions for XeTeX - mostly platform-neutral
 */

#define EXTERN extern
#include "xetexd.h"

#ifdef XETEX_MAC
#undef input /* this is defined in texmfmp.h, but we don't need it and it confuses the carbon headers */
#include <Carbon/Carbon.h>
#endif

#include "TECkit_Engine.h"

#include "XeTeX_ext.h"

#include <kpathsea/c-ctype.h>
#include <kpathsea/line.h>
#include <kpathsea/readable.h>
#include <kpathsea/variable.h>
#include <kpathsea/absolute.h>

#include <time.h> /* For `struct tm'.  */
#if defined (HAVE_SYS_TIME_H)
#include <sys/time.h>
#elif defined (HAVE_SYS_TIMEB_H)
#include <sys/timeb.h>
#endif

#if defined(__STDC__)
#include <locale.h>
#endif

#include <signal.h> /* Catch interrupts.  */

#include "XeTeXLayoutInterface.h"

#include "unicode/ubidi.h"
#include "unicode/ubrk.h"
#include "unicode/ucnv.h"

extern char*	gettexstring(int strNumber);
void
setinputfileencoding(UFILE* f, int mode, int encodingData)
{
	if ((f->encodingMode == ICUMAPPING) && (f->conversionData != NULL))
		ucnv_close((UConverter*)(f->conversionData));
	f->conversionData = 0;
	
	switch (mode) {
		case UTF8:
		case UTF16BE:
		case UTF16LE:
		case RAW:
			f->encodingMode = mode;
			break;
		
		case ICUMAPPING:
			{
				char*	name = gettexstring(encodingData);
				UErrorCode	err = 0;
				UConverter*	cnv = ucnv_open(name, &err);
				if (cnv == NULL) {
					fprintf(stderr, "! Error %d creating Unicode converter for %s\n", err, name);
					f->encodingMode = RAW;
				}
				else {
					f->encodingMode = ICUMAPPING;
					f->conversionData = cnv;
				}
				free(name);
			}
			break;
	}
}

uclose(UFILE* f)
{
	if (f != 0) {
		fclose(f->f);
		if ((f->encodingMode == ICUMAPPING) && (f->conversionData != NULL))
			ucnv_close((UConverter*)(f->conversionData));
		free((void*)f);
	}
}

int
input_line_icu(UFILE* f)
{
static char* byteBuffer = NULL;

	UInt32	bytesRead = 0;
	int i;

	if (byteBuffer == NULL)
		byteBuffer = xmalloc(bufsize + 1);

	/* Recognize either LF or CR as a line terminator; skip initial LF if prev line ended with CR.  */
	i = getc(f->f);
	if (f->skipNextLF) {
		f->skipNextLF = 0;
		if (i == '\n')
			i = getc(f->f);
	}

	if (i != EOF && i != '\n' && i != '\r')
		byteBuffer[bytesRead++] = i;
	if (i != EOF && i != '\n' && i != '\r')
		while (bytesRead < bufsize && (i = getc(f->f)) != EOF && i != '\n' && i != '\r')
			byteBuffer[bytesRead++] = i;
	
	if (i == EOF && errno != EINTR && last == first)
		return false;

	if (i != EOF && i != '\n' && i != '\r') {
		fprintf (stderr, "! Unable to read an entire line---bufsize=%u.\n",
						 (unsigned) bufsize);
		fputs ("Please increase buf_size in texmf.cnf.\n", stderr);
		uexit (1);
	}

	/* If line ended with CR, remember to skip following LF. */
	if (i == '\r')
		f->skipNextLF = 1;
	
	/* now apply the mapping to turn external bytes into Unicode characters in buffer */
	UConverter*	cnv = (UConverter*)(f->conversionData);
	long		outLen;
	UErrorCode	errorCode = 0;
	outLen = ucnv_toUChars(cnv, &buffer[first], (bufsize - first), byteBuffer, bytesRead, &errorCode);
	if (errorCode != 0) {
		fprintf(stderr, "! Unicode conversion failed: error code = %d\n", (int)errorCode);
		return false;
	}
	last = first + outLen;
	buffer[last] = ' ';

	if (last >= maxbufstack)
		maxbufstack = last;

	/* Trim trailing whitespace.  */
	while (last > first && ISBLANK(buffer[last - 1]))
		--last;
	
	return true;
}

static void die(char*s, int i)
{
	fprintf(stderr, s, i);
	fprintf(stderr, " - exiting\n");
	exit(3);
}

static UBreakIterator*	brkIter = NULL;
static int				brkLocaleStrNum = 0;

void
linebreakstart(int localeStrNum, const UniChar* text, int textLength)
{
	UErrorCode	status = 0;
	int i;

	if ((localeStrNum != brkLocaleStrNum) && (brkIter != NULL)) {
		ubrk_close(brkIter);
		brkIter = NULL;
	}
	
	if (brkIter == NULL) {
		char* locale = (char*)gettexstring(localeStrNum);
		brkIter = ubrk_open(UBRK_LINE, locale, NULL, 0, &status);
		if (U_FAILURE(status)) {
			fprintf(stderr, "\n! error %d creating linebreak iterator for locale \"%s\", trying default. ", status, locale);
			if (brkIter != NULL)
				ubrk_close(brkIter);
			status = 0;
			brkIter = ubrk_open(UBRK_LINE, "en_us", NULL, 0, &status);
		}
		free(locale);
		brkLocaleStrNum = localeStrNum;
	}
	
	if (brkIter == NULL) {
		die("! failed to create linebreak iterator, status=%d", (int)status);
	}

	ubrk_setText(brkIter, text, textLength, &status);
}

int
linebreaknext()
{
	return ubrk_next((UBreakIterator*)brkIter);
}

long
getencodingmodeandinfo(long* info)
{
	/* \XeTeXinputencoding "enc-name"
	 *   -> name is packed in |nameoffile| as a C string, starting at [1]
	 * Check if it's a built-in name; if not, try to open an ICU converter by that name
	 */
	char*	name = nameoffile + 1;
	*info = 0;
	if (strcasecmp(name, "auto") == 0) {
		return AUTO;
	}
	if (strcasecmp(name, "utf8") == 0) {
		return UTF8;
	}
	if (strcasecmp(name, "utf16") == 0) {	/* depends on host platform */
#ifdef WORDS_BIGENDIAN
		return UTF16BE;
#else
		return UTF16LE;
#endif
	}
	if (strcasecmp(name, "utf16be") == 0) {
		return UTF16BE;
	}
	if (strcasecmp(name, "utf16le") == 0) {
		return UTF16LE;
	}
	if (strcasecmp(name, "bytes") == 0) {
		return RAW;
	}
	
	/* try for an ICU converter */
	UErrorCode	err = 0;
	UConverter*	cnv = ucnv_open(name, &err);
	if (cnv == NULL) {
		fprintf(stderr, "! unknown encoding \"%s\"; reading as raw bytes\n", name);
		return RAW;
	}
	else {
		ucnv_close(cnv);
		*info = maketexstring(name);
		return ICUMAPPING;
	}
}

void
printchars(const unsigned short* str, int len)
{
	while (len-- > 0)
		printchar(*(str++));
}

TECkit_Converter
load_mapping_file(const char* s, const char* e)
{
	TECkit_Converter	cnv = 0;
	char*	buffer = xmalloc(e - s + 5);
	strncpy(buffer, s, e - s);
	buffer[e - s] = 0;
	strcat(buffer, ".tec");
	UInt8*	mapPath = kpse_find_file(buffer, kpse_miscfonts_format, 1);
	free(buffer);

	if (mapPath) {
		FILE*	mapFile = fopen(mapPath, "r");
		free(mapPath);
		if (mapFile) {
			fseek(mapFile, 0, SEEK_END);
			UInt32	mappingSize = ftell(mapFile);
			fseek(mapFile, 0, SEEK_SET);
			Byte*	mapping = xmalloc(mappingSize);
			fread(mapping, 1, mappingSize, mapFile);
			fclose(mapFile);
			TECkit_Status	status = TECkit_CreateConverter(mapping, mappingSize,
											true, kForm_UTF16BE, kForm_UTF16BE, &cnv);
			free(mapping);
		}
	}

	return cnv;
}

static unsigned long
read_tag(const char* cp)
{
	unsigned long	tag = 0;
	int i;
	for (i = 0; i < 4; ++i) {
		tag <<= 8;
		if (*cp && /* *cp < 128 && */ *cp != ',' && *cp != ';' && *cp != ':') {
			tag += *(unsigned char*)cp;
			++cp;
		}
		else
			tag += ' ';
	}
	return tag;
}

static long
loadOTfont(XeTeXFont font, const char* name, int nameLen, const char* cp1)
{
	unsigned long	scriptTag = kLatin;
	unsigned long	languageTag = 0;
	
	unsigned long*	addFeatures = 0;
	unsigned long*	removeFeatures = 0;
	
	int	nAdded = 0;
	int nRemoved = 0;
	
	const char*	cp2;
	const char*	cp3;

	unsigned long	tag;

	unsigned long	rgbValue = 0x000000FF;

	// scan the feature string (if any)
	while (*cp1) {
		if ((*cp1 == ':') || (*cp1 == ';') || (*cp1 == ','))
			++cp1;
		while ((*cp1 == ' ') || (*cp1 == '\t'))	// skip leading whitespace
			++cp1;
		if (*cp1 == 0)	// break if end of string
			break;

		cp2 = cp1;
		while (*cp2 && (*cp2 != ':') && (*cp2 != ';') && (*cp2 != ','))
			++cp2;
		
		if (strncmp(cp1, "script", 6) == 0) {
			cp3 = cp1 + 6;
			if (*cp3 != '=')
				goto bad_option;
			scriptTag = read_tag(cp3 + 1);
			goto next_option;
		}
		
		if (strncmp(cp1, "language", 8) == 0) {
			cp3 = cp1 + 8;
			if (*cp3 != '=')
				goto bad_option;
			languageTag = read_tag(cp3 + 1);
			goto next_option;
		}
		
		if (strncmp(cp1, "mapping", 7) == 0) {
			cp3 = cp1 + 7;
			if (*cp3 != '=')
				goto bad_option;
			loadedfontmapping = (long)load_mapping_file(cp3 + 1, cp2);
			goto next_option;
		}

		if (strncmp(cp1, "color", 5) == 0) {
			cp3 = cp1 + 5;
			if (*cp3 != '=')
				goto bad_option;
			++cp3;

			rgbValue = 0;
			unsigned	alpha = 0;
			int i;
			for (i = 0; i < 6; ++i) {
				if ((*cp3 >= '0') && (*cp3 <= '9'))
					rgbValue = (rgbValue << 4) + *cp3 - '0';
				else if ((*cp3 >= 'A') && (*cp3 <= 'F'))
					rgbValue = (rgbValue << 4) + *cp3 - 'A' + 10;
				else if ((*cp3 >= 'a') && (*cp3 <= 'f'))
					rgbValue = (rgbValue << 4) + *cp3 - 'a' + 10;
				else
					goto bad_option;
				++cp3;
			}
			rgbValue <<= 8;
			for (i = 0; i < 2; ++i) {
				if ((*cp3 >= '0') && (*cp3 <= '9'))
					alpha = (alpha << 4) + *cp3 - '0';
				else if ((*cp3 >= 'A') && (*cp3 <= 'F'))
					alpha = (alpha << 4) + *cp3 - 'A' + 10;
				else if ((*cp3 >= 'a') && (*cp3 <= 'f'))
					alpha = (alpha << 4) + *cp3 - 'a' + 10;
				else
					break;
				++cp3;
			}
			if (i == 2)
				rgbValue += alpha;
			else
				rgbValue += 0xFF;
			
			goto next_option;
		}
		
		if (*cp1 == '+') {
			tag = read_tag(cp1 + 1);
			++nAdded;
			if (nAdded == 1)
				addFeatures = xmalloc(sizeof(unsigned long));
			else
				addFeatures = xrealloc(addFeatures, nAdded * sizeof(long));
			addFeatures[nAdded-1] = tag;
			goto next_option;
		}
		
		if (*cp1 == '-') {
			tag = read_tag(cp1 + 1);
			++nRemoved;
			if (nRemoved == 1)
				removeFeatures = xmalloc(sizeof(unsigned long));
			else
				removeFeatures = xrealloc(removeFeatures, nRemoved * sizeof(long));
			removeFeatures[nRemoved-1] = tag;
			goto next_option;
		}
		
	bad_option:
		fontfeaturewarning(name, nameLen, cp1, cp2 - cp1, 0, 0);
	
	next_option:
		cp1 = cp2;
	}
	
	if (addFeatures != 0) {
		addFeatures = realloc(addFeatures, (nAdded + 1) * sizeof(long));
		addFeatures[nAdded] = 0;
	}
	if (removeFeatures != 0) {
		removeFeatures = realloc(removeFeatures, (nRemoved + 1) * sizeof(long));
		removeFeatures[nRemoved] = 0;
	}

	XeTeXLayoutEngine	engine = createLayoutEngine(font, scriptTag, languageTag, addFeatures, removeFeatures, rgbValue);
	if (engine == 0) {
		deleteFont(font);
		if (addFeatures)
			free(addFeatures);
		if (removeFeatures)
			free(removeFeatures);
	}
	else
		nativefonttypeflag = OT_FONT_FLAG;

	return (long)engine;
}

long
findatsufont(const char* name, long scaled_size)
{
	long	rval = 0;
	loadedfontmapping = 0;

	// NOTE that findFontByName can change TeX's nameoffile, to which /name/ is a pointer!
	name = xstrdup(name);

	unsigned int	nameLen = strlen(name);
	const char*	featureString = name;
	while (*featureString && *featureString != ':')
		++featureString;
#ifdef XETEX_MAC
	ATSFontRef	fontRef;
#else
#endif
	if (*featureString) {
		char* buf = xmalloc(featureString - name + 1);
		strncpy(buf, name, featureString - name);
		buf[featureString - name] = 0;
		fontRef = findFontByName(buf, Fix2X(scaled_size));
		free(buf);
	}
	else
		fontRef = findFontByName(name, Fix2X(scaled_size));

	if (fontRef != 0) {
		// decide whether to use AAT or OpenType rendering with this font
		if (gPrefEngine == 'A')
			goto load_aat;
		
		XeTeXFont	font = createFont(fontRef, scaled_size);
		if (font != 0) {
			if (gPrefEngine == 'I' || getFontTablePtr(font, kGSUB) != 0 || getFontTablePtr(font, kGPOS) != 0) {
				rval = loadOTfont(font, name, nameLen, featureString);
			}
			if (rval == 0)
				deleteFont(font);
		}
		if (rval == 0) {
		load_aat:
#ifdef XETEX_MAC
			rval = loadAATfont(FMGetFontFromATSFontRef((ATSFontRef)fontRef), scaled_size, name, nameLen, featureString);
#else
			;
#endif
		}
	}
	
	return rval;
}

long
otfontget(int what, XeTeXLayoutEngine engine)
{
	XeTeXFont	fontInst = getFont(engine);
	switch (what) {
		case XeTeX_count_glyphs:
			return countGlyphs(fontInst);
			break;
			
		case XeTeX_OT_count_scripts:
			return countScripts(fontInst);
			break;
	}
	return 0;
}


long
otfontget1(int what, XeTeXLayoutEngine engine, long param)
{
	XeTeXFont	fontInst = getFont(engine);
	switch (what) {
		case XeTeX_OT_count_languages:
			return countScriptLanguages(fontInst, param);
			break;

		case XeTeX_OT_script_code:
			return getIndScript(fontInst, param);
			break;
	}
	return 0;
}


long
otfontget2(int what, XeTeXLayoutEngine engine, long param1, long param2)
{
	XeTeXFont	fontInst = getFont(engine);
	switch (what) {
		case XeTeX_OT_language_code:
			return getIndScriptLanguage(fontInst, param1, param2);
			break;

		case XeTeX_OT_count_features:
			return countFeatures(fontInst, param1, param2);
			break;
	}
	
	return 0;
}


long
otfontget3(int what, XeTeXLayoutEngine engine, long param1, long param2, long param3)
{
	XeTeXFont	fontInst = getFont(engine);
	switch (what) {
		case XeTeX_OT_feature_code:
			return getIndFeature(fontInst, param1, param2, param3);
			break;
	}
	
	return 0;
}


long
makefontdef(long f)
{
#ifdef XETEX_MAC
	if (fontarea[f] == AAT_FONT_FLAG) {
		// AAT font
		ATSUStyle	style = (ATSUStyle)fontlayoutengine[f];
	
		ItemCount	featureCount, variationCount;
		ATSUGetAllFontFeatures(style, 0, 0, 0, &featureCount);
		ATSUGetAllFontVariations(style, 0, 0, 0, &variationCount);
		
		ATSUFontID	fontID;
		ATSUGetAttribute(style, kATSUFontTag, sizeof(ATSUFontID), &fontID, 0);
		ByteCount	nameLength;
		ATSUFindFontName(fontID, kFontPostscriptName, kFontNoPlatform, kFontNoScript, kFontNoLanguage, 0, 0, &nameLength, 0);
	
		// parameters after internal font ID: s[4] t[2] nf[2] f[2nf] s[2nf] nv[2] a[4nv] v[4nv] c[16] l[2] n[l]
		long	fontDefLength = 0
			+ 4 // size
			+ 2	// font technology flag
			+ 2	// number of features
			+ 2 * featureCount
			+ 2 * featureCount	// features and settings
			+ 2	// number of variations
			+ 4 * variationCount
			+ 4 * variationCount	// axes and values
			+ 16 // ATSURGBAlphaColor float[4]
			+ 1 // vertical?
			+ 2	// name length
			+ nameLength;
	
		fontdef = (char*)xmalloc(fontDefLength);
		char*	cp = fontdef;
	
		Fixed	size;
		ATSUGetAttribute(style, kATSUSizeTag, sizeof(Fixed), &size, 0);
		*(Fixed*)cp = size;
		cp += 4;
		
		*(UInt16*)cp = fontarea[f];
		cp += 2;
		
		*(UInt16*)cp = featureCount;
		cp += 2;
		if (featureCount > 0) {
			ATSUGetAllFontFeatures(style, featureCount,
									(UInt16*)(cp),
									(UInt16*)(cp + 2 * featureCount),
									0);
			cp += 2 * featureCount + 2 * featureCount;
		}
	
		*(UInt16*)cp = variationCount;
		cp += 2;
		if (variationCount > 0) {
			ATSUGetAllFontVariations(style, variationCount,
									(UInt32*)(cp),
									(SInt32*)(cp + 4 * variationCount),
									0);
			cp += 4 * variationCount + 4 * variationCount;
		}
	
		ATSURGBAlphaColor*	rgba = (ATSURGBAlphaColor*)cp;
		ATSUGetAttribute(style, kATSURGBAlphaColorTag, sizeof(ATSURGBAlphaColor), rgba, 0);
		cp += sizeof(ATSURGBAlphaColor);
	
		ATSUVerticalCharacterType	vert;
		ATSUGetAttribute(style, kATSUVerticalCharacterTag, sizeof(ATSUVerticalCharacterType), &vert, 0);
		*cp = (vert == kATSUStronglyVertical);
		++cp;
	
		*(UInt16*)cp = nameLength;
		cp += 2;
		ATSUFindFontName(fontID, kFontPostscriptName, kFontNoPlatform, kFontNoScript, kFontNoLanguage, nameLength, cp, 0, 0);

		return fontDefLength;
	}
	else
#endif /* XETEX_MAC */
		if (fontarea[f] == OT_FONT_FLAG) {
		// OT font...
		XeTeXLayoutEngine	engine = (XeTeXLayoutEngine)fontlayoutengine[f];

		UInt32	nameLength;
#ifdef XETEX_MAC		
		ATSFontRef	fontRef = getFontRef(engine);
		ATSUFindFontName(fontRef, kFontPostscriptName, kFontNoPlatform, kFontNoScript, kFontNoLanguage, 0, 0, &nameLength, 0);
#else
#endif

		// parameters after internal font ID: s[4] t[2] c[16] l[2] n[l]
		long	fontDefLength = 0
			+ 4 // size
			+ 2	// font technology flag
			+ 16 // ATSURGBAlphaColor float[4]
			+ 2	// name length
			+ nameLength;

		fontdef = (char*)xmalloc(fontDefLength);
		char*	cp = fontdef;

		*(Fixed*)cp = X2Fix(getPointSize(engine));
		cp += 4;
		
		*(UInt16*)cp = fontarea[f];
		cp += 2;
		
		UInt32	rgbValue = getRgbValue(engine);
		float	*rgba = (float*)cp;
		*rgba++ = (rgbValue >> 24) / 255.0;
		*rgba++ = ((rgbValue >> 16) & 0xff) / 255.0;
		*rgba++ = ((rgbValue >> 8) & 0xff) / 255.0;
		*rgba++ = (rgbValue & 0xff) / 255.0;
		cp += 4 * sizeof(float);

		*(UInt16*)cp = nameLength;
		cp += 2;
#ifdef XETEX_MAC
		ATSUFindFontName(fontRef, kFontPostscriptName, kFontNoPlatform, kFontNoScript, kFontNoLanguage, nameLength, cp, 0, 0);
#else
#endif

		return fontDefLength;
	}
	else {
		fprintf(stderr, "\n! Internal error: bad native font flag\n");
		exit(3);
	}
}

int
applymapping(TECkit_Converter cnv, const UniChar* txtPtr, int txtLen)
{
	UInt32	inUsed, outUsed;
	TECkit_Status	status;
	static UInt32	outLength = 0;

	// allocate outBuffer if not big enough
	if (outLength < txtLen * sizeof(UniChar) + 32) {
		if (mappedtext != 0)
			free(mappedtext);
		outLength = txtLen * sizeof(UniChar) + 32;
		mappedtext = xmalloc(outLength);
	}
	
	// try the mapping
retry:
	status = TECkit_ConvertBuffer(cnv,
			(Byte*)txtPtr, txtLen * sizeof(UniChar), &inUsed,
			(Byte*)mappedtext, outLength, &outUsed, true);
	
	switch (status) {
		case kStatus_NoError:
			txtPtr = (const UniChar*)mappedtext;
			return outUsed / sizeof(UniChar);
			
		case kStatus_OutputBufferFull:
			outLength += (txtLen * sizeof(UniChar)) + 32;
			free(mappedtext);
			mappedtext = xmalloc(outLength);
			goto retry;
			
		default:
			return 0;
	}
}

void
measure_native_node(void* p)
{
#define width_offset		1
#define depth_offset		2
#define height_offset		3
#define native_info_offset	4
#define native_glyph_info_offset	5

#define width(node)			node[width_offset].cint
#define depth(node)			node[depth_offset].cint
#define height(node)		node[height_offset].cint
#define native_length(node)	node[native_info_offset].hh.v.RH
#define native_font(node)	node[native_info_offset].hh.b1
#define native_glyph_count(node)	node[native_glyph_info_offset].hh.v.LH
#define native_glyph_info_ptr(node)	node[native_glyph_info_offset].hh.v.RH
#define native_glyph_info_size		10	/* info for each glyph is location (FixedPoint) + glyph ID (UInt16) */

	memoryword*	node = (memoryword*)p;
	long		txtLen = native_length(node);
	const UniChar*	txtPtr = (UniChar*)(node + native_node_size);

	if (fontmapping[native_font(node)] != 0) {
		txtLen = applymapping((TECkit_Converter)(fontmapping[native_font(node)]), txtPtr, txtLen);
		txtPtr = mappedtext;
	}

#ifdef XETEX_MAC
	if (fontarea[native_font(node)] == AAT_FONT_FLAG) {
		// we're using this font in AAT mode
		static ATSUTextLayout	sTextLayout = 0;
		OSStatus	status = noErr;
		
		if (sTextLayout == 0) {
			status = ATSUCreateTextLayout(&sTextLayout);
			ATSUFontFallbacks fallbacks;
			status = ATSUCreateFontFallbacks(&fallbacks);
			status = ATSUSetObjFontFallbacks(fallbacks, 0, 0, kATSULastResortOnlyFallback);
			ATSUAttributeTag		tag = kATSULineFontFallbacksTag;
			ByteCount				valueSize = sizeof(fallbacks);
			ATSUAttributeValuePtr	value = &fallbacks;
			status = ATSUSetLayoutControls(sTextLayout, 1, &tag, &valueSize, &value);
			status = ATSUSetTransientFontMatching(sTextLayout, true);
		}

		status = ATSUSetTextPointerLocation(sTextLayout, txtPtr, 0, txtLen, txtLen);
		
		ATSUStyle	style = (ATSUStyle)(fontlayoutengine[native_font(node)]);
		status = ATSUSetRunStyle(sTextLayout, style, 0, txtLen);
	
		Fixed		before, after, ascent, descent;
		status = ATSUGetUnjustifiedBounds(sTextLayout, 0, txtLen, &before, &after, &ascent, &descent);
	
		width(node) = after - before;
		depth(node) = descent;
		height(node) = ascent;
	}
#endif
	else
	if (fontarea[native_font(node)] == OT_FONT_FLAG) {
		// using this font in OT Layout mode, so fontlayoutengine[native_font(node)] is actually a XeTeXLayoutEngine
		
		XeTeXLayoutEngine engine = (XeTeXLayoutEngine)(fontlayoutengine[native_font(node)]);

		// need to find direction runs within the text, and call layoutChars separately for each

		long nGlyphs;
		float	x, y;
		void*	glyph_info = 0;
		static	float*	positions = 0;
		static	UInt32*	glyphs = 0;
		static	long	maxGlyphs = 0;

		UBiDi*	pBiDi = ubidi_open();
		
		UErrorCode	errorCode = (UErrorCode)0;
		ubidi_setPara(pBiDi, txtPtr, txtLen, getDefaultDirection(engine), NULL, &errorCode);
		
		UBiDiDirection	dir = ubidi_getDirection(pBiDi);
		if (dir == UBIDI_MIXED) {
			// we actually do the layout twice here, once to count glyphs and then again to get them;
			// which is inefficient, but i figure that MIXED is a relatively rare occurrence, so i can't be
			// bothered to deal with the memory reallocation headache of doing it differently
			int	nRuns = ubidi_countRuns(pBiDi, &errorCode);
			double		wid = 0;
			long		totalGlyphs = 0;
			int			realGlyphCount = 0;
			int 		i, runIndex;
			int32_t		logicalStart, length;
			OSStatus	status;
			for (runIndex = 0; runIndex < nRuns; ++runIndex) {
				dir = ubidi_getVisualRun(pBiDi, runIndex, &logicalStart, &length);
				nGlyphs = layoutChars(engine, (UniChar*)txtPtr, logicalStart, length, txtLen, (dir == UBIDI_RTL), 0.0, 0.0, &status);
				totalGlyphs += nGlyphs;

				if (nGlyphs >= maxGlyphs) {
					if (glyphs != 0) {
						free(glyphs);
						free(positions);
					}
					maxGlyphs = nGlyphs + 20;
					glyphs = xmalloc(maxGlyphs * sizeof(UInt32));
					positions = xmalloc((maxGlyphs * 2 + 2) * sizeof(float));
				}

				getGlyphs(engine, glyphs, &status);
				for (i = 0; i < nGlyphs; ++i)
					if (glyphs[i] < 0xfffe)
						++realGlyphCount;
			}
			
			if (realGlyphCount > 0) {
				glyph_info = xmalloc(realGlyphCount * native_glyph_info_size);
				FixedPoint*	locations = (FixedPoint*)glyph_info;
				UInt16*		glyphIDs = (UInt16*)(locations + realGlyphCount);
				realGlyphCount = 0;
				
				double	x = 0.0, y = 0.0;
				for (runIndex = 0; runIndex < nRuns; ++runIndex) {
					dir = ubidi_getVisualRun(pBiDi, runIndex, &logicalStart, &length);
					nGlyphs = layoutChars(engine, (UniChar*)txtPtr, logicalStart, length, txtLen,
											(dir == UBIDI_RTL), x, y, &status);
	
					getGlyphs(engine, glyphs, &status);
					getGlyphPositions(engine, positions, &status);
				
					for (i = 0; i < nGlyphs; ++i) {
						if (glyphs[i] < 0xfffe) {
							glyphIDs[realGlyphCount] = glyphs[i];
							locations[realGlyphCount].x = X2Fix(positions[2*i]);
							locations[realGlyphCount].y = X2Fix(positions[2*i+1]);
							++realGlyphCount;
						}
					}
					x = positions[2*i];
					y = positions[2*i+1];
				}
				wid = x;
			}

			width(node) = X2Fix(wid);
			native_glyph_count(node) = realGlyphCount;
			native_glyph_info_ptr(node) = (long)glyph_info;
		}
		else {
			OSStatus	status = 0;
			nGlyphs = layoutChars(engine, (UniChar*)txtPtr, 0, txtLen, txtLen, (dir == UBIDI_RTL), 0.0, 0.0, &status);
			getGlyphPosition(engine, nGlyphs, &x, &y, &status);
			width(node) = X2Fix(x);

			if (nGlyphs >= maxGlyphs) {
				if (glyphs != 0) {
					free(glyphs);
					free(positions);
				}
				maxGlyphs = nGlyphs + 20;
				glyphs = xmalloc(maxGlyphs * sizeof(UInt32));
				positions = xmalloc((maxGlyphs * 2 + 2) * sizeof(float));
			}
			getGlyphs(engine, glyphs, &status);
			getGlyphPositions(engine, positions, &status);

			int i;
			int	realGlyphCount = 0;
			for (i = 0; i < nGlyphs; ++i)
				if (glyphs[i] < 0xfffe)
					++realGlyphCount;

			if (realGlyphCount > 0) {
				glyph_info = xmalloc(realGlyphCount * native_glyph_info_size);
				FixedPoint*	locations = (FixedPoint*)glyph_info;
				UInt16*		glyphIDs = (UInt16*)(locations + realGlyphCount);
				realGlyphCount = 0;
				for (i = 0; i < nGlyphs; ++i) {
					if (glyphs[i] < 0xfffe) {
						glyphIDs[realGlyphCount] = glyphs[i];
						locations[realGlyphCount].x = X2Fix(positions[2*i]);
						locations[realGlyphCount].y = X2Fix(positions[2*i+1]);
						++realGlyphCount;
					}
				}
			}
						
			native_glyph_count(node) = realGlyphCount;
			native_glyph_info_ptr(node) = (long)glyph_info;
		}

		ubidi_close(pBiDi);
		
		getAscentAndDescent(engine, &x, &y);
		height(node) = X2Fix(x);
		depth(node) = X2Fix(-y);
	}
	else {
		fprintf(stderr, "\n! Internal error: bad native font flag\n");
		exit(3);
	}
}

void
measure_native_glyph(void* p)
{
#define native_glyph(node)	native_length(node)

	memoryword*	node = (memoryword*)p;
	UInt16		gid = native_glyph(node);
	
	long	font = native_font(node);
#ifdef XETEX_MAC
	if (fontarea[font] == AAT_FONT_FLAG) {
		ATSUStyle	style = (ATSUStyle)(fontlayoutengine[font]);
		ATSGlyphIdealMetrics	metrics;
		OSStatus	status = ATSUGlyphGetIdealMetrics(style, 1, &gid, 0, &metrics);
		width(node) = X2Fix(metrics.advance.x);
	}
	else
#endif
	if (fontarea[font] == OT_FONT_FLAG) {
		XeTeXLayoutEngine	engine = (XeTeXLayoutEngine)fontlayoutengine[font];
		XeTeXFont		fontInst = getFont(engine);
		width(node) = X2Fix(getGlyphWidth(fontInst, gid));
	}
	else {
		fprintf(stderr, "\n! Internal error: bad native font flag\n");
		exit(3);
	}
	depth(node) = depthbase[font];
	height(node) = heightbase[font];
}

#ifndef XETEX_MAC
Fixed X2Fix(double d)
{
	Fixed rval = (int)(d * 65536.0 + 0.5);
}

double Fix2X(Fixed f)
{
	double rval = f / 65536.0;
	return rval;
}
#endif

/* these are here, not XeTeX_mac.c, because we need stubs on other platforms */
#ifndef XETEX_MAC
typedef void* ATSUStyle;
#endif

long
atsufontget(int what, ATSUStyle style)
{
	long	rval = -1;

#ifdef XETEX_MAC
	ATSUFontID	fontID;
	ATSUGetAttribute(style, kATSUFontTag, sizeof(ATSUFontID), &fontID, 0);
	ItemCount	count;

	switch (what) {
		case XeTeX_count_glyphs:
			{
				ByteCount	tableSize;
				if (ATSFontGetTable(fontID, 'maxp', 0, 0, 0, &tableSize) == noErr) {
					Byte*	table = xmalloc(tableSize);
					ATSFontGetTable(fontID, 'maxp', 0, tableSize, table, 0);
					rval = *(unsigned short*)(table + 4);
					free(table);
				}
			}
			break;

		case XeTeX_count_variations:
			if (ATSUCountFontVariations(fontID, &count) == noErr)
				rval = count;
			break;

		case XeTeX_count_features:
			if (ATSUCountFontFeatureTypes(fontID, &count) == noErr)
				rval = count;
			break;
	}
#endif
	return rval;
}

long
atsufontget1(int what, ATSUStyle style, int param)
{
	long	rval = -1;

#ifdef XETEX_MAC
	ATSUFontID	fontID;
	ATSUGetAttribute(style, kATSUFontTag, sizeof(ATSUFontID), &fontID, 0);
	
	ATSUFontVariationAxis	axis;
	ATSUFontVariationValue	value;
	ItemCount	count;
	Boolean		exclusive;
	switch (what) {
		case XeTeX_variation:
			if (ATSUGetIndFontVariation(fontID, param, &axis, 0, 0, 0) == noErr)
				rval = axis;
			break;

		case XeTeX_variation_min:
		case XeTeX_variation_max:
		case XeTeX_variation_default:
			if (ATSUCountFontVariations(fontID, &count) == noErr)
				while (count-- > 0)
					if (ATSUGetIndFontVariation(fontID, count, &axis,
							(what == XeTeX_variation_min) ? &value : 0,
							(what == XeTeX_variation_max) ? &value : 0,
							(what == XeTeX_variation_default) ? &value : 0) == noErr)
						if (axis == param) {
							rval = value;
							break;
						}
			break;

		case XeTeX_feature_code:
			if (ATSUCountFontFeatureTypes(fontID, &count) == noErr) {
				if (param < count) {
					ATSUFontFeatureType*	types = xmalloc(count * sizeof(ATSUFontFeatureType));
					if (ATSUGetFontFeatureTypes(fontID, count, types, 0) == noErr)
						rval = types[param];
					free(types);
				}
			}
			break;

		case XeTeX_is_exclusive_feature:
			if (ATSUGetFontFeatureSelectors(fontID, param, 0, 0, 0, 0, &exclusive) == noErr)
				rval = exclusive ? 1 : 0;
			break;

		case XeTeX_count_selectors:
			if (ATSUCountFontFeatureSelectors(fontID, param, &count) == noErr)
				rval = count;
			break;
	}
#endif
	
	return rval;
}

long
atsufontget2(int what, ATSUStyle style, int param1, int param2)
{
	long	rval = -1;

#ifdef XETEX_MAC
	ATSUFontID	fontID;
	ATSUGetAttribute(style, kATSUFontTag, sizeof(ATSUFontID), &fontID, 0);

	ItemCount	count;
	if (ATSUCountFontFeatureSelectors(fontID, param1, &count) == noErr) {
		ATSUFontFeatureSelector*	selectors = xmalloc(count * sizeof(ATSUFontFeatureSelector));
		Boolean*					isDefault = xmalloc(count * sizeof(Boolean));
		if (ATSUGetFontFeatureSelectors(fontID, param1, count, selectors, isDefault, 0, 0) == noErr) {
			switch (what) {
				case XeTeX_selector_code:
					if (param2 < count)
							rval = selectors[param2];
					break;
					
				case XeTeX_is_default_selector:
					while (count-- > 0)
						if (selectors[count] == param2) {
							rval = isDefault[count] ? 1 : 0;
							break;
						}
					break;
			}
		}
		free(isDefault);
		free(selectors);
	}
#endif
	
	return rval;
}

long
atsufontgetnamed(int what, ATSUStyle style)
{
	long	rval = -1;

#ifdef XETEX_MAC
	ATSUFontID	fontID;
	ATSUGetAttribute(style, kATSUFontTag, sizeof(ATSUFontID), &fontID, 0);
	
	switch (what) {
		case XeTeX_find_variation_by_name:
			rval = find_axis_by_name(fontID, nameoffile + 1, namelength);
			if (rval == 0)
				rval = -1;
			break;
		
		case XeTeX_find_feature_by_name:
			rval = find_feature_by_name(fontID, nameoffile + 1, namelength);
			if (rval == 0x0000FFFF)
				rval = -1;
			break;
	}
#endif
	
	return rval;
}

long
atsufontgetnamed1(int what, ATSUStyle style, int param)
{
	long	rval = -1;

#ifdef XETEX_MAC
	ATSUFontID	fontID;
	ATSUGetAttribute(style, kATSUFontTag, sizeof(ATSUFontID), &fontID, 0);
	
	switch (what) {
		case XeTeX_find_selector_by_name:
			rval = find_selector_by_name(fontID, param, nameoffile + 1, namelength);
			if (rval == 0x0000FFFF)
				rval = -1;
			break;
	}
#endif
	
	return rval;
}

void
atsuprintfontname(int what, ATSUStyle style, int param1, int param2)
{
#ifdef XETEX_MAC
	ATSUFontID	fontID;
	ATSUGetAttribute(style, kATSUFontTag, sizeof(ATSUFontID), &fontID, 0);

	FontNameCode	code;
	OSStatus	status = 1;
	switch (what) {
		case XeTeX_variation_name:
			status = ATSUGetFontVariationNameCode(fontID, param1, &code);
			break;
			
		case XeTeX_feature_name:
			status = ATSUGetFontFeatureNameCode(fontID, param1, kATSUNoSelector, &code);
			break;
			
		case XeTeX_selector_name:
			status = ATSUGetFontFeatureNameCode(fontID, param1, param2, &code);
			break;
	}

	if (status == noErr) {
		ByteCount	len = 0;
		char		name[1024];
		do {
			if (ATSUFindFontName(fontID, code, kFontMacintoshPlatform, kFontRomanScript, kFontEnglishLanguage, 1024, name, &len, 0) == noErr) break;
			if (ATSUFindFontName(fontID, code, kFontMacintoshPlatform, kFontRomanScript, kFontNoLanguageCode, 1024, name, &len, 0) == noErr) break;
		} while (0);
		if (len > 0) {
			char*	cp = &name[0];
			while (len-- > 0)
				printchar(*(cp++));
		}
		else {
			do {
				if (ATSUFindFontName(fontID, code, kFontUnicodePlatform, kFontNoScriptCode, kFontEnglishLanguage, 1024, name, &len, 0) == noErr) break;
				if (ATSUFindFontName(fontID, code, kFontMicrosoftPlatform, kFontNoScriptCode, kFontEnglishLanguage, 1024, name, &len, 0) == noErr) break;
				if (ATSUFindFontName(fontID, code, kFontUnicodePlatform, kFontNoScriptCode, kFontNoLanguageCode, 1024, name, &len, 0) == noErr) break;
				if (ATSUFindFontName(fontID, code, kFontMicrosoftPlatform, kFontNoScriptCode, kFontNoLanguageCode, 1024, name, &len, 0) == noErr) break;
			} while (0);
			if (len > 0) {
				printchars((unsigned short*)(&name[0]), len / 2);
			}
		}
	}
#endif
}
