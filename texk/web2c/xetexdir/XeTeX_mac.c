/****************************************************************************\
 Part of the XeTeX typesetting system
 Copyright (c) 1994-2008 by SIL International
 Copyright (c) 2009 by Jonathan Kew
 Copyright (c) 2012, 2013 by Jiang Jiang
 Copyright (c) 2012, 2013 by Khaled Hosny

 SIL Author(s): Jonathan Kew

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the copyright holders
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written
authorization from the copyright holders.
\****************************************************************************/

/* XeTeX_mac.c
 * additional plain C extensions for XeTeX - MacOS-specific routines
 */

#define EXTERN extern
#include "xetexd.h"

#include <ApplicationServices/ApplicationServices.h>

#include <teckit/TECkit_Engine.h>
#include "XeTeX_ext.h"
#include "XeTeXLayoutInterface.h"

#include "XeTeXswap.h"

static inline double
TeXtoPSPoints(double pts)
{
	return pts * 72.0 / 72.27;
}

static inline double
PStoTeXPoints(double pts)
{
	return pts * 72.27 / 72.0;
}

static inline Fixed
FixedPStoTeXPoints(double pts)
{
	return D2Fix(PStoTeXPoints(pts));
}

CTFontRef fontFromAttributes(CFDictionaryRef attributes)
{
	return CFDictionaryGetValue(attributes, kCTFontAttributeName);
}

CTFontRef fontFromInteger(integer font)
{
	CFDictionaryRef attributes = (CFDictionaryRef) fontlayoutengine[font];
	return fontFromAttributes(attributes);
}

void
DoAtsuiLayout(void* p, int justify)
{
	CFArrayRef glyphRuns;
	CFIndex i, j, runCount;
	CFIndex totalGlyphCount;
	UInt16* realGlyphIDs, *glyphIDs;
	void*   glyph_info;
	FixedPoint*	locations;
	Fixed lsUnit, lsDelta;
	int	realGlyphCount;
	CGFloat lastGlyphAdvance;

	long txtLen;
	const UniChar* txtPtr;

	CFDictionaryRef attributes;
	CFStringRef string;
	CFAttributedStringRef attrString;
	CTTypesetterRef typesetter;
	CTLineRef line;

	memoryword*	node = (memoryword*)p;

	unsigned	f = native_font(node);
	if (fontarea[f] != AAT_FONT_FLAG) {
		fprintf(stderr, "internal error: do_atsui_layout called for non-ATSUI font\n");
		exit(1);
	}

	txtLen = native_length(node);
	txtPtr = (UniChar*)(node + native_node_size);

	attributes = fontlayoutengine[native_font(node)];
	string = CFStringCreateWithCharactersNoCopy(NULL, txtPtr, txtLen, kCFAllocatorNull);
	attrString = CFAttributedStringCreate(NULL, string, attributes);
	CFRelease(string);

	typesetter = CTTypesetterCreateWithAttributedString(attrString);
	line = CTTypesetterCreateLine(typesetter, CFRangeMake(0, txtLen));
	if (justify) {
		CGFloat lineWidth = TeXtoPSPoints(Fix2D(node_width(node)));
		CTLineRef justifiedLine = CTLineCreateJustifiedLine(line, TeXtoPSPoints(Fix2D(fract1)), lineWidth);
		// TODO(jjgod): how to handle the case when justification failed? for
		// now we just fallback to use the original line.
		if (justifiedLine) {
			CFRelease(line);
			line = justifiedLine;
		}
	}

	glyphRuns = CTLineGetGlyphRuns(line);
	runCount = CFArrayGetCount(glyphRuns);
	totalGlyphCount = CTLineGetGlyphCount(line);
	realGlyphIDs = xmalloc(totalGlyphCount * sizeof(UInt16));
	glyph_info = xmalloc(totalGlyphCount * native_glyph_info_size);
	locations = (FixedPoint*)glyph_info;
	lsUnit = justify ? 0 : fontletterspace[f];
	lsDelta = 0;

	realGlyphCount = 0;
	lastGlyphAdvance = 0;
	for (i = 0; i < runCount; i++) {
		CTRunRef run = CFArrayGetValueAtIndex(glyphRuns, i);
		CFIndex count = CTRunGetGlyphCount(run);
		// TODO(jjgod): Avoid unnecessary allocation with CTRunGetFoosPtr().
		CGGlyph* glyphs = (CGGlyph*) xmalloc(count * sizeof(CGGlyph));
		CGPoint* positions = (CGPoint*) xmalloc(count * sizeof(CGPoint));
		CGSize*  advances = (CGSize*) xmalloc(count * sizeof(CGSize));
		CTRunGetGlyphs(run, CFRangeMake(0, 0), glyphs);
		CTRunGetPositions(run, CFRangeMake(0, 0), positions);
		CTRunGetAdvances(run, CFRangeMake(0, 0), advances);
		for (j = 0; j < count; j++) {
			if (glyphs[j] < 0xfffe) {
				realGlyphIDs[realGlyphCount] = glyphs[j];
				locations[realGlyphCount].x = FixedPStoTeXPoints(positions[j].x) + lsDelta;
				lastGlyphAdvance = advances[j].width;
				locations[realGlyphCount].y = FixedPStoTeXPoints(positions[j].y);
				lsDelta += lsUnit;
				realGlyphCount++;
			}
		}
		free(advances);
		free(glyphs);
		free(positions);
	}
	if (lsDelta != 0)
		lsDelta -= lsUnit;

	glyphIDs = (UInt16*)(locations + realGlyphCount);
	memcpy(glyphIDs, realGlyphIDs, realGlyphCount * sizeof(UInt16));
	free(realGlyphIDs);

	native_glyph_count(node) = realGlyphCount;
	native_glyph_info_ptr(node) = glyph_info;

	if (!justify) {
		node_width(node) =
			(realGlyphCount > 0 ? locations[realGlyphCount - 1].x : 0) +
			FixedPStoTeXPoints(lastGlyphAdvance) +
			lsDelta;
	}

	CFRelease(line);
	CFRelease(typesetter);
}

void getGlyphBBoxFromCTFont(CTFontRef font, UInt16 gid, GlyphBBox* bbox)
{
	CGRect rect;

	bbox->xMin = 65536.0;
	bbox->yMin = 65536.0;
	bbox->xMax = -65536.0;
	bbox->yMax = -65536.0;

	rect = CTFontGetBoundingRectsForGlyphs(font,
		0, /* Use default orientation for now, handle vertical later */
		(const CGGlyph *) &gid, NULL, 1);

	if (CGRectIsNull(rect))
		bbox->xMin = bbox->yMin = bbox->xMax = bbox->yMax = 0;
	else {
		bbox->yMin = PStoTeXPoints(rect.origin.y);
		bbox->yMax = PStoTeXPoints(rect.origin.y + rect.size.height);
		bbox->xMin = PStoTeXPoints(rect.origin.x);
		bbox->xMax = PStoTeXPoints(rect.origin.x + rect.size.width);
	}
}

void GetGlyphBBox_AAT(CFDictionaryRef attributes, UInt16 gid, GlyphBBox* bbox)
	/* returns glyph bounding box in TeX points */
{
	CTFontRef font = fontFromAttributes(attributes);
	return getGlyphBBoxFromCTFont(font, gid, bbox);
}

double getGlyphWidthFromCTFont(CTFontRef font, UInt16 gid)
{
	CGSize advances[1] = { CGSizeMake(0, 0) };
	return PStoTeXPoints(CTFontGetAdvancesForGlyphs(font, 0, &gid, advances, 1));
}

double GetGlyphWidth_AAT(CFDictionaryRef attributes, UInt16 gid)
	/* returns TeX points */
{
	CTFontRef font = fontFromAttributes(attributes);
	return getGlyphWidthFromCTFont(font, gid);
}

void GetGlyphHeightDepth_AAT(CFDictionaryRef attributes, UInt16 gid, float* ht, float* dp)
	/* returns TeX points */
{
	GlyphBBox	bbox;
	
	GetGlyphBBox_AAT(attributes, gid, &bbox);

	*ht = bbox.yMax;
	*dp = -bbox.yMin;
}

void GetGlyphSidebearings_AAT(CFDictionaryRef attributes, UInt16 gid, float* lsb, float* rsb)
	/* returns TeX points */
{
	CTFontRef font = fontFromAttributes(attributes);
	CGSize advances[1] = { CGSizeMake(0, 0) };
	double advance = CTFontGetAdvancesForGlyphs(font, 0, &gid, advances, 1);
	GlyphBBox bbox;
	getGlyphBBoxFromCTFont(font, gid, &bbox);
	*lsb = bbox.xMin;
	*rsb = PStoTeXPoints(advance) - bbox.xMax;
}

double GetGlyphItalCorr_AAT(CFDictionaryRef attributes, UInt16 gid)
{
	CTFontRef font = fontFromAttributes(attributes);
	CGSize advances[1] = { CGSizeMake(0, 0) };
	double advance = CTFontGetAdvancesForGlyphs(font, 0, &gid, advances, 1);

	GlyphBBox bbox;
	getGlyphBBoxFromCTFont(font, gid, &bbox);

	if (bbox.xMax > PStoTeXPoints(advance))
		return bbox.xMax - PStoTeXPoints(advance);
	return 0;
}

int mapCharToGlyphFromCTFont(CTFontRef font, UInt32 ch, UInt32 vs)
{
	CGGlyph glyphs[2] = { 0 };
	UniChar	txt[4];
	int		len = 1;

	if (ch > 0xffff) {
		ch -= 0x10000;
		txt[0] = 0xd800 + ch / 1024;
		txt[1] = 0xdc00 + ch % 1024;
		len = 2;
	}
	else
		txt[0] = ch;

	if (vs) {
		if (vs > 0xffff) {
			vs -= 0x10000;
			txt[len] = 0xd800 + vs / 1024;
			txt[len + 1] = 0xdc00 + vs % 1024;
			len += 2;
		} else {
			txt[len] = vs;
			len += 1;
		}
	}

	if (CTFontGetGlyphsForCharacters(font, txt, glyphs, len))
		return glyphs[0];

	return 0;
}

int MapCharToGlyph_AAT(CFDictionaryRef attributes, UInt32 ch)
{
	CTFontRef font = fontFromAttributes(attributes);
	return mapCharToGlyphFromCTFont(font, ch, 0);
}

int MapGlyphToIndex_AAT(CFDictionaryRef attributes, const char* glyphName)
{
	CTFontRef font = fontFromAttributes(attributes);
	return GetGlyphIDFromCTFont(font, glyphName);
}

int GetGlyphIDFromCTFont(CTFontRef ctFontRef, const char* glyphName)
{
	CFStringRef glyphname = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,
															glyphName,
															kCFStringEncodingUTF8,
															kCFAllocatorNull);
	int rval = CTFontGetGlyphWithName(ctFontRef, glyphname);
	CFRelease(glyphname);
	return rval;
}

char*
GetGlyphNameFromCTFont(CTFontRef ctFontRef, UInt16 gid, int* len)
{
	CGFontRef cgfont;
	static char buffer[256];
	buffer[0] = 0;
	*len = 0;

	cgfont = CTFontCopyGraphicsFont(ctFontRef, 0);
	if (cgfont && gid < CGFontGetNumberOfGlyphs(cgfont)) {
		CFStringRef glyphname = CGFontCopyGlyphNameForGlyph(cgfont, gid);
		if (glyphname) {
			if (CFStringGetCString(glyphname, buffer, 256, kCFStringEncodingUTF8)) {
				*len = strlen(buffer);
			}
			CFRelease(glyphname);
		}
		CGFontRelease(cgfont);
	}

	return &buffer[0];
}

int
GetFontCharRange_AAT(CFDictionaryRef attributes, int reqFirst)
{
	if (reqFirst) {
		int	ch = 0;
		while (MapCharToGlyph_AAT(attributes, ch) == 0 && ch < 0x10ffff)
			++ch;
		return ch;
	}
	else {
		int ch = 0x10ffff;
		while (MapCharToGlyph_AAT(attributes, ch) == 0 && ch > 0)
			--ch;
		return ch;
	}
}

char* getNameFromCTFont(CTFontRef ctFontRef, CFStringRef nameKey)
{
	char *buf;
	CFStringRef name = CTFontCopyName(ctFontRef, nameKey);
	CFIndex len = CFStringGetLength(name);
	len = len * 6 + 1;
	buf = xmalloc(len);
	if (CFStringGetCString(name, buf, len, kCFStringEncodingUTF8))
		return buf;
	free(buf);
	return 0;
}

char* getFileNameFromCTFont(CTFontRef ctFontRef)
{
	char *ret = NULL;
	CFURLRef url = NULL;

#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_6
	/* kCTFontURLAttribute was not avialable before 10.6 */
	ATSFontRef atsFont;
	FSRef fsref;
	OSStatus status;
	atsFont = CTFontGetPlatformFont(ctFontRef, NULL);
	status = ATSFontGetFileReference(atsFont, &fsref);
	if (status == noErr)
		url = CFURLCreateFromFSRef(NULL, &fsref);
#else
	url = (CFURLRef) CTFontCopyAttribute(ctFontRef, kCTFontURLAttribute);
#endif
	if (url) {
		UInt8 pathname[PATH_MAX];
		if (CFURLGetFileSystemRepresentation(url, true, pathname, PATH_MAX)) {
			int index = 0;
			char buf[20];

			/* finding face index by searching for preceding font ids with the same FSRef */
			/* logic copied from FreeType but without using ATS/FS APIs */
			ATSFontRef id1 = CTFontGetPlatformFont(ctFontRef, NULL);
			ATSFontRef id2 = id1 - 1;
			while (id2 > 0) {
				FSRef dummy;
				CTFontRef ctFontRef2;
				CFURLRef url2;
				if (noErr != ATSFontGetFileReference(id2, &dummy)) /* check if id2 is valid, any better way? */
					break;
				ctFontRef2 = CTFontCreateWithPlatformFont(id2, 0.0, NULL, NULL);
#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_6
				status = ATSFontGetFileReference(id2, &fsref);
				if (status == noErr)
					url2 = CFURLCreateFromFSRef(NULL, &fsref);
#else
				url2 = (CFURLRef) CTFontCopyAttribute(ctFontRef2, kCTFontURLAttribute);
#endif
				if (!url2)
					break;
				if (!CFEqual(url, url2))
					break;
				id2--;
			}
			index = id1 - (id2 + 1);

			if (index > 0)
				sprintf(buf, ":%d", index);
			else
				buf[0] = '\0';

			ret = xmalloc(strlen((char*) pathname) + 2 + strlen(buf) + 1);
			sprintf(ret, "[%s%s]", pathname, buf);
		}
		CFRelease(url);
	}

	return ret;
}

CFDictionaryRef findDictionaryInArrayWithIdentifier(CFArrayRef array,
													const void* identifierKey,
													int identifier)
{
	CFDictionaryRef dict = NULL;

	if (array) {
		int value = -1;
		CFIndex i;
		for (i = 0; i < CFArrayGetCount(array); i++) {
			CFDictionaryRef item = CFArrayGetValueAtIndex(array, i);
			CFNumberRef itemId = CFDictionaryGetValue(item, identifierKey);
			if (itemId) {
				CFNumberGetValue(itemId, kCFNumberIntType, &value);
				if (value == identifier) {
					dict = item;
					break;
				}
			}
		}
	}
	return dict;
}

CFDictionaryRef findDictionaryInArray(CFArrayRef array, const void* nameKey,
									  const char* name, int nameLength)
{
	CFDictionaryRef dict = NULL;

	if (array) {
		CFStringRef itemName;
		CFIndex i;
		itemName = CFStringCreateWithBytes(NULL, name, nameLength,
										   kCFStringEncodingUTF8, false);
		for (i = 0; i < CFArrayGetCount(array); i++) {
			CFDictionaryRef item = CFArrayGetValueAtIndex(array, i);
			CFStringRef iName = CFDictionaryGetValue(item, nameKey);
			if (iName && !CFStringCompare(itemName, iName, kCFCompareCaseInsensitive)) {
				dict = item;
				break;
			}
		}
		CFRelease(itemName);
	}
	return dict;
}

CFNumberRef findSelectorByName(CFDictionaryRef feature, const char* name, int nameLength)
{
	CFArrayRef selectors = CFDictionaryGetValue(feature, kCTFontFeatureTypeSelectorsKey);
	CFDictionaryRef s = findDictionaryInArray(selectors, kCTFontFeatureSelectorNameKey, name, nameLength);
	CFNumberRef selector = NULL;
	if (s)
		selector = CFDictionaryGetValue(s, kCTFontFeatureSelectorIdentifierKey);
	return selector;
}

static CFDictionaryRef createFeatureSettingDictionary(CFNumberRef featureTypeIdentifier,
													  CFNumberRef featureSelectorIdentifier)
{
	const void* settingKeys[] = { kCTFontFeatureTypeIdentifierKey, kCTFontFeatureSelectorIdentifierKey };
	const void* settingValues[] = { featureTypeIdentifier, featureSelectorIdentifier };

	return CFDictionaryCreate(kCFAllocatorDefault, settingKeys, settingValues, 2,
							  &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
}

const CFStringRef kXeTeXEmboldenAttributeName = CFSTR("XeTeXEmbolden");

void*
loadAATfont(CTFontDescriptorRef descriptor, integer scaled_size, const char* cp1)
{
	CTFontRef font, actualFont;
	CGFloat ctSize;
	CFMutableDictionaryRef stringAttributes, attributes;
	CGAffineTransform matrix;
	double  tracking	= 0.0;
	float   extend		= 1.0;
	float   slant		= 0.0;
	float   embolden	= 0.0;
	float   letterspace	= 0.0;
	uint32_t  rgbValue;

	// create a base font instance for applying further attributes
	ctSize = TeXtoPSPoints(Fix2D(scaled_size));
	font = CTFontCreateWithFontDescriptor(descriptor, ctSize, NULL);
	if (!font)
		return NULL;

	stringAttributes = CFDictionaryCreateMutable(NULL, 0,
								  &kCFTypeDictionaryKeyCallBacks,
								  &kCFTypeDictionaryValueCallBacks);
	attributes = CFDictionaryCreateMutable(NULL, 0,
								  &kCFTypeDictionaryKeyCallBacks,
								  &kCFTypeDictionaryValueCallBacks);
	if (cp1) {
		CFArrayRef features = CTFontCopyFeatures(font);
		CFArrayRef axes = CTFontCopyVariationAxes(font);
		CFMutableArrayRef featureSettings =
			CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
		CFMutableDictionaryRef variation =
			CFDictionaryCreateMutable(NULL, 0,
									  &kCFTypeDictionaryKeyCallBacks,
									  &kCFTypeDictionaryValueCallBacks);

		// interpret features & variations following ":"
		while (*cp1) {
			CFDictionaryRef feature, axis;
			int ret;
			const char* cp2;
			const char* cp3;
			// locate beginning of name=value pair
			if (*cp1 == ':' || *cp1 == ';') // skip over separator
				++cp1;
			while (*cp1 == ' ' || *cp1 == '\t') // skip leading whitespace
				++cp1;
			if (*cp1 == 0) // break if end of string
				break;

			// scan to end of pair
			cp2 = cp1;
			while (*cp2 && (*cp2 != ';') && (*cp2 != ':'))
				++cp2;

			// look for the '=' separator
			cp3 = cp1;
			while ((cp3 < cp2) && (*cp3 != '='))
				++cp3;
			if (cp3 == cp2)
				goto bad_option;

			// now cp1 points to option name, cp3 to '=', cp2 to ';' or null

			// first try for a feature by this name
			feature = findDictionaryInArray(features, kCTFontFeatureTypeNameKey, cp1, cp3 - cp1);
			if (feature) {
				// look past the '=' separator for setting names
				int featLen = cp3 - cp1;
				int zeroInteger = 0;
				CFNumberRef zero = CFNumberCreate(NULL, kCFNumberIntType, &zeroInteger);
				++cp3;
				while (cp3 < cp2) {
					CFNumberRef selector;
					int disable = 0;
					const char* cp4;
					// skip leading whitespace
					while (*cp3 == ' ' || *cp3 == '\t')
						++cp3;

					// possibly multiple settings...
					if (*cp3 == '!') { // check for negation
						disable = 1;
						++cp3;
					}

					// scan for end of setting name
					cp4 = cp3;
					while (cp4 < cp2 && *cp4 != ',')
						++cp4;

					// now cp3 points to name, cp4 to ',' or ';' or null
					selector = findSelectorByName(feature, cp3, cp4 - cp3);
					if (selector && CFNumberCompare(selector, zero, NULL) >= 0) {
						CFNumberRef featureType = CFDictionaryGetValue(feature, kCTFontFeatureTypeIdentifierKey);
						CFDictionaryRef featureSetting = createFeatureSettingDictionary(featureType, selector);
						CFArrayAppendValue(featureSettings, featureSetting);
						CFRelease(featureSetting);
					} else {
						fontfeaturewarning(cp1, featLen, cp3, cp4 - cp3);
					}

					// point beyond setting name terminator
					cp3 = cp4 + 1;
				}
				CFRelease(zero);

				goto next_option;
			}

			// try to find a variation by this name
			axis = findDictionaryInArray(axes, kCTFontVariationAxisNameKey, cp1, cp3 - cp1);
			if (axis) {
				CFNumberRef axisIdentifier, axisValue;
				double value = 0.0, decimal = 1.0;
				bool	negate = false;
				// look past the '=' separator for the value
				++cp3;
				if (*cp3 == '-') {
					++cp3;
					negate = true;
				}
				while (cp3 < cp2) {
					int v = *cp3 - '0';
					if (v >= 0 && v <= 9) {
						if (decimal != 1.0) {
							value += v / decimal;
							decimal *= 10.0;
						}
						else
							value = value * 10.0 + v;
					}
					else if (*cp3 == '.') {
						if (decimal != 1.0)
							break;
						decimal = 10.0;
					}
					else
						break;
					++cp3;
				}
				if (negate)
					value = -value;

				axisIdentifier = CFDictionaryGetValue(axis, kCTFontVariationAxisIdentifierKey);
				axisValue = CFNumberCreate(NULL, kCFNumberDoubleType, &value);
				CFDictionaryAddValue(variation, axisIdentifier, axisValue);
				CFRelease(axisValue);

				goto next_option;
			}

			// didn't find feature or variation, try other options...
			ret = readCommonFeatures(cp1, cp2, &extend, &slant, &embolden, &letterspace, &rgbValue);
			if (ret == 1)
				goto next_option;
			else if (ret == -1)
				goto bad_option;

			if (strncmp(cp1, "tracking", 8) == 0) {
				CFNumberRef trackingNumber;
				cp3 = cp1 + 8;
				if (*cp3 != '=')
					goto bad_option;
				++cp3;
				tracking = read_double(&cp3);
				trackingNumber = CFNumberCreate(NULL, kCFNumberDoubleType, &tracking);
				CFDictionaryAddValue(stringAttributes, kCTKernAttributeName, trackingNumber);
				CFRelease(trackingNumber);
				goto next_option;
			}

			bad_option:
				// not a name=value pair, or not recognized....
				// check for plain "vertical" before complaining
				if (strncmp(cp1, "vertical", 8) == 0) {
					cp3 = cp2;
					if (*cp3 == ';' || *cp3 == ':')
						--cp3;
					while (*cp3 == '\0' || *cp3 == ' ' || *cp3 == '\t')
						--cp3;
					if (*cp3)
						++cp3;
					if (cp3 == cp1 + 8) {
						int orientation = kCTFontVerticalOrientation;
						CFNumberRef orientationNumber = CFNumberCreate(NULL, kCFNumberIntType, &orientation);
						CFDictionaryAddValue(attributes, kCTFontOrientationAttribute, orientationNumber);
						CFRelease(orientationNumber);
						CFDictionaryAddValue(stringAttributes, kCTVerticalFormsAttributeName, kCFBooleanTrue);
						goto next_option;
					}
				}

				fontfeaturewarning(cp1, cp2 - cp1, 0, 0);

			next_option:
				// go to next name=value pair
				cp1 = cp2;
		}

		if (features)
			CFRelease(features);
		if (axes)
			CFRelease(axes);

		if (CFArrayGetCount(featureSettings))
			CFDictionaryAddValue(attributes, kCTFontFeatureSettingsAttribute, featureSettings);
		CFRelease(featureSettings);

		if (CFDictionaryGetCount(variation))
			CFDictionaryAddValue(attributes, kCTFontVariationAttribute, variation);
		CFRelease(variation);
	}

	if ((loadedfontflags & FONT_FLAGS_COLORED) != 0) {
		CGFloat red  = ((rgbValue & 0xFF000000) >> 24) / 255.0;
		CGFloat green   = ((rgbValue & 0x00FF0000) >> 16) / 255.0;
		CGFloat blue	= ((rgbValue & 0x0000FF00) >> 8 ) / 255.0;
		CGFloat alpha   = ((rgbValue & 0x000000FF)	) / 255.0;
		CGColorRef color = CGColorCreateGenericRGB(red, green, blue, alpha);
		CFDictionaryAddValue(stringAttributes, kCTForegroundColorAttributeName, color);
		CGColorRelease(color);
	}

	matrix = CGAffineTransformIdentity;
	if (extend != 1.0 || slant != 0.0)
		matrix = CGAffineTransformMake(extend, slant, 0, 1.0, 0, 0);

	if (embolden != 0.0) {
		CFNumberRef emboldenNumber;
		embolden = embolden * Fix2D(scaled_size) / 100.0;
		emboldenNumber = CFNumberCreate(NULL, kCFNumberFloatType, &embolden);
		CFDictionaryAddValue(stringAttributes, kXeTeXEmboldenAttributeName, emboldenNumber);
		CFRelease(emboldenNumber);
	}

	if (letterspace != 0.0)
		loadedfontletterspace = (letterspace / 100.0) * scaled_size;

	descriptor = CTFontDescriptorCreateWithAttributes(attributes);
	CFRelease(attributes);
	actualFont = CTFontCreateCopyWithAttributes(font, ctSize, &matrix, descriptor);
	CFRelease(font);
	CFDictionaryAddValue(stringAttributes, kCTFontAttributeName, actualFont);
	CFRelease(actualFont);

	nativefonttypeflag = AAT_FONT_FLAG;
	return (void *) stringAttributes;
}

int
countpdffilepages()
{
	int	rval = 0;

    char*		pic_path = kpse_find_file((char*)nameoffile + 1, kpse_pict_format, 1);
	CFURLRef	picFileURL = NULL;
	if (pic_path) {
		picFileURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (UInt8*)pic_path, strlen(pic_path), false);
		if (picFileURL != NULL) {
			FSRef	picFileRef;
			CGPDFDocumentRef document;
			CFURLGetFSRef(picFileURL, &picFileRef);
			document = CGPDFDocumentCreateWithURL(picFileURL);
			if (document != NULL) {
				rval = CGPDFDocumentGetNumberOfPages(document);
				CGPDFDocumentRelease(document);
			}
			CFRelease(picFileURL);
		}
		free(pic_path);
	}

	return rval;
}

int
find_pic_file(char** path, realrect* bounds, int pdfBoxType, int page)
	/* returns bounds in TeX points */
{
	OSStatus	result = fnfErr;
    char*		pic_path = kpse_find_file((char*)nameoffile + 1, kpse_pict_format, 1);
	CFURLRef	picFileURL = NULL;

	*path = NULL;

	if (pic_path) {
		picFileURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (UInt8*)pic_path, strlen(pic_path), false);

		if (picFileURL != NULL) {
			if (pdfBoxType > 0) {
				CGPDFDocumentRef	document = CGPDFDocumentCreateWithURL(picFileURL);
				if (document != NULL) {
					CGPDFPageRef pageRef;
					CGRect r;
					CGPDFBox boxType;
					int	nPages = CGPDFDocumentGetNumberOfPages(document);
					if (page < 0)
						page = nPages + 1 + page;
					if (page < 1)
						page = 1;
					if (page > nPages)
						page = nPages;

					pageRef = CGPDFDocumentGetPage(document, page);
					switch (pdfBoxType) {
						case pdfbox_crop:
						default:
							boxType = kCGPDFCropBox;
							break;
						case pdfbox_media:
							boxType = kCGPDFMediaBox;
							break;
						case pdfbox_bleed:
							boxType = kCGPDFBleedBox;
							break;
						case pdfbox_trim:
							boxType = kCGPDFTrimBox;
							break;
						case pdfbox_art:
							boxType = kCGPDFArtBox;
							break;
					}
					r = CGPDFPageGetBoxRect(pageRef, boxType);

					bounds->x = r.origin.x * 72.27 / 72.0;
					bounds->y = r.origin.y * 72.27 / 72.0;
					bounds->wd = r.size.width * 72.27 / 72.0;
					bounds->ht = r.size.height * 72.27 / 72.0;
					CGPDFDocumentRelease(document);
					result = noErr;
				}
			}
			else {
				CGImageSourceRef picFileSource = CGImageSourceCreateWithURL(picFileURL, NULL);
				if (picFileSource) {
					CFDictionaryRef properties = CGImageSourceCopyPropertiesAtIndex(picFileSource, 0, NULL);

					CFNumberRef imageWidth  = CFDictionaryGetValue(properties, kCGImagePropertyPixelWidth);
					CFNumberRef imageHeight = CFDictionaryGetValue(properties, kCGImagePropertyPixelHeight);
					CFNumberRef DPIWidth	= CFDictionaryGetValue(properties, kCGImagePropertyDPIWidth);
					CFNumberRef DPIHeight   = CFDictionaryGetValue(properties, kCGImagePropertyDPIHeight);
					int w = 0, h = 0, hRes = 72, vRes = 72;
					CFNumberGetValue(imageWidth,  kCFNumberIntType, &w);
					CFNumberGetValue(imageHeight, kCFNumberIntType, &h);
					CFNumberGetValue(DPIWidth,	kCFNumberIntType, &hRes);
					CFNumberGetValue(DPIHeight,   kCFNumberIntType, &vRes);

					bounds->x = 0;
					bounds->y = 0;
					bounds->wd = w * 72.27 / hRes;
					bounds->ht = h * 72.27 / vRes;
					CFRelease(properties);
					CFRelease(picFileSource);
					result = noErr;
				}
			}
			
			CFRelease(picFileURL);
		}

		/* if we couldn't import it, toss the pathname as we'll be returning an error */
		if (result != noErr)
			free(pic_path);
		else
			*path = (char*)pic_path;
	}
	
	return result;
}
