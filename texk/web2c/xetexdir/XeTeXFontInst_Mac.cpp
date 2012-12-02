/****************************************************************************\
 Part of the XeTeX typesetting system
 Copyright (c) 1994-2008 by SIL International
 Copyright (c) 2009 by Jonathan Kew

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

/*
 *   file name:  XeTeXFontInst_Mac.cpp
 *
 *   created on: 2005-10-22
 *   created by: Jonathan Kew
 */


#include "XeTeXFontInst_Mac.h"
#include "XeTeX_ext.h"

XeTeXFontInst_Mac::XeTeXFontInst_Mac(CTFontDescriptorRef descriptor, float pointSize, int &status)
    : XeTeXFontInst(pointSize, status)
    , fDescriptor(descriptor)
    , fFontRef(0)
    , fFirstCharCode(-1)
    , fLastCharCode(-1)
{
    if (status != 0) {
        return;
    }

	initialize(status);
}

XeTeXFontInst_Mac::~XeTeXFontInst_Mac()
{
	if (fDescriptor != 0)
		CFRelease(fDescriptor);
	if (fFontRef != 0)
		CFRelease(fFontRef);
}

void XeTeXFontInst_Mac::initialize(int &status)
{
    if (fDescriptor == 0) {
        status = 1;
        return;
    }

	if (status != 0)
		fDescriptor = 0;

	// Create a copy of original font descriptor with font cascading (fallback) disabled
	CFArrayRef emptyCascadeList = CFArrayCreate(NULL, NULL, 0, &kCFTypeArrayCallBacks);
	const void* values[] = { emptyCascadeList };
	static const void* attributeKeys[] = { kCTFontCascadeListAttribute };
	CFDictionaryRef attributes = CFDictionaryCreate(NULL, attributeKeys, values, 1,
		&kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFRelease(emptyCascadeList);

	fDescriptor = CTFontDescriptorCreateCopyWithAttributes(fDescriptor, attributes);
	CFRelease(attributes);
	fFontRef = CTFontCreateWithFontDescriptor(fDescriptor, fPointSize * 72.0 / 72.27, NULL);
	if (!fFontRef) {
		status = 1;
		CFRelease(fDescriptor);
		fDescriptor = 0;
	}
	
	XeTeXFontInst::initialize(status);
}

const void *XeTeXFontInst_Mac::readTable(OTTag tag, uint32_t *length) const
{
	if (!fFontRef) {
		*length = 0;
		return NULL;
	}
	CFDataRef tableData = CTFontCopyTable(fFontRef, tag, 0);
	if (!tableData) {
		*length = 0;
		return NULL;
	}
	*length = CFDataGetLength(tableData);
	UInt8* table = (UInt8*) xmalloc(*length * sizeof(UInt8));
	if (table != NULL)
		CFDataGetBytes(tableData, CFRangeMake(0, *length), table);

	return table;
}

void XeTeXFontInst_Mac::getGlyphBounds(GlyphID gid, GlyphBBox* bbox)
{
	getGlyphBBoxFromCTFont(fFontRef, gid, bbox);
}

GlyphID
XeTeXFontInst_Mac::mapCharToGlyph(UChar32 ch) const
{
	return MapCharToGlyph_AAT(fFontRef, ch);
}

GlyphID
XeTeXFontInst_Mac::mapGlyphToIndex(const char* glyphName) const
{
	GlyphID rval = XeTeXFontInst::mapGlyphToIndex(glyphName);
	if (rval)
		return rval;
	return GetGlyphIDFromCTFont(fFontRef, glyphName);
}

const char*
XeTeXFontInst_Mac::getGlyphName(GlyphID gid, int& nameLen)
{
	const char* rval = XeTeXFontInst::getGlyphName(gid, nameLen);
	if (rval)
		return rval;
	return GetGlyphNameFromCTFont(fFontRef, gid, &nameLen);
}

UChar32
XeTeXFontInst_Mac::getFirstCharCode()
{
	if (fFirstCharCode == -1) {
		int ch = 0;
		while (mapCharToGlyph(ch) == 0 && ch < 0x10ffff)
			++ch;
		fFirstCharCode = ch;
	}
	return fFirstCharCode;
}

UChar32
XeTeXFontInst_Mac::getLastCharCode()
{
	if (fLastCharCode == -1) {
		int ch = 0x10ffff;
		while (mapCharToGlyph(ch) == 0 && ch > 0)
			--ch;
		fLastCharCode = ch;
	}
	return fLastCharCode;
}
