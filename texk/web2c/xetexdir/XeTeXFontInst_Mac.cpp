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

XeTeXFontInst_Mac::XeTeXFontInst_Mac(ATSFontRef atsFont, float pointSize, int &status)
    : XeTeXFontInst(pointSize, status)
    , fFontRef(atsFont)
    , fStyle(0)
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
	if (fStyle != 0)
		ATSUDisposeStyle(fStyle);
}

void XeTeXFontInst_Mac::initialize(int &status)
{
    if (fFontRef == 0) {
        status = 1;
        return;
    }

	XeTeXFontInst::initialize(status);

	if (status != 0)
		fFontRef = 0;

	if (ATSUCreateStyle(&fStyle) == noErr) {
		ATSUFontID	font = FMGetFontFromATSFontRef(fFontRef);
		Fixed		size = D2Fix(fPointSize * 72.0 / 72.27); /* convert TeX to Quartz points */
		ATSStyleRenderingOptions	options = kATSStyleNoHinting;
		ATSUAttributeTag		tags[3] = { kATSUFontTag, kATSUSizeTag, kATSUStyleRenderingOptionsTag };
		ByteCount				valueSizes[3] = { sizeof(ATSUFontID), sizeof(Fixed), sizeof(ATSStyleRenderingOptions) };
		ATSUAttributeValuePtr	values[3] = { &font, &size, &options };
		ATSUSetAttributes(fStyle, 3, tags, valueSizes, values);
	}
	else {
		status = 1;
		fFontRef = 0;
	}
	
    return;
}

const void *XeTeXFontInst_Mac::readTable(OTTag tag, uint32_t *length) const
{
	OSStatus status = ATSFontGetTable(fFontRef, tag, 0, 0, 0, (ByteCount*)length);
	if (status != noErr) {
		*length = 0;
		return NULL;
	}
	void*	table = xmalloc(*length * sizeof(char));
	if (table != NULL) {
		status = ATSFontGetTable(fFontRef, tag, 0, *length, table, (ByteCount*)length);
		if (status != noErr) {
			*length = 0;
			free((void*) table);
			return NULL;
		}
	}

    return table;
}

void XeTeXFontInst_Mac::getGlyphBounds(GlyphID gid, GlyphBBox* bbox)
{
	GetGlyphBBox_AAT(fStyle, gid, bbox);
}

GlyphID
XeTeXFontInst_Mac::mapCharToGlyph(UChar32 ch) const
{
	return MapCharToGlyph_AAT(fStyle, ch);
}

GlyphID
XeTeXFontInst_Mac::mapGlyphToIndex(const char* glyphName) const
{
	GlyphID rval = XeTeXFontInst::mapGlyphToIndex(glyphName);
	if (rval)
		return rval;
	return GetGlyphIDFromCGFont(fFontRef, glyphName);
}

const char*
XeTeXFontInst_Mac::getGlyphName(GlyphID gid, int& nameLen)
{
	const char* rval = XeTeXFontInst::getGlyphName(gid, nameLen);
	if (rval)
		return rval;
	return GetGlyphNameFromCGFont(fFontRef, gid, &nameLen);
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
