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
 *   file name:  XeTeXFontInst.cpp
 *
 *   created on: 2005-10-22
 *   created by: Jonathan Kew
 *	
 *	originally based on PortableFontInstance.cpp from ICU
 */

#include "XeTeXFontInst.h"
#include "XeTeXLayoutInterface.h"
#include "XeTeX_ext.h"

#include <string.h>
#include FT_GLYPH_H

FT_Library gFreeTypeLibrary = 0;

XeTeXFontInst::XeTeXFontInst(const char* pathname, int index, float pointSize, int &status)
    : fPointSize(pointSize)
    , fUnitsPerEM(0)
    , fAscent(0)
    , fDescent(0)
    , fCapHeight(0)
    , fXHeight(0)
    , fItalicAngle(0)
    , fNumGlyphs(0)
    , fNumGlyphsInited(false)
    , fVertical(false)
    , fFilename(NULL)
    , ftFace(0)
    , hbFont(NULL)
    , fMath(NULL)
{
	if (pathname != NULL)
		initialize(pathname, index, status);
}

XeTeXFontInst::~XeTeXFontInst()
{
	if (ftFace != 0) {
		FT_Done_Face(ftFace);
		ftFace = 0;
	}
	hb_font_destroy(hbFont);
}

void
XeTeXFontInst::initialize(const char* pathname, int index, int &status)
{
	TT_Postscript *postTable;
	TT_OS2* os2Table;
	FT_Error error;

	if (!gFreeTypeLibrary) {
		error = FT_Init_FreeType(&gFreeTypeLibrary);
		if (error != 0) {
			fprintf(stderr, "FreeType initialization failed! (%d)\n", error);
			exit(1);
		}
	}

	error = FT_New_Face(gFreeTypeLibrary, (char*)pathname, index, &ftFace);

	if (error != 0) {
        status = 1;
        return;
    }

	/* for non-sfnt-packaged fonts (presumably Type 1), see if there is an AFM file we can attach */
	if (index == 0 && !FT_IS_SFNT(ftFace)) {
		char* afm = new char[strlen((const char*)pathname) + 5]; // room to append ".afm"
		strcpy(afm, (const char*)pathname);
		char* p = strrchr(afm, '.');
		if (p == NULL || strlen(p) != 4 || tolower(*(p+1)) != 'p' || tolower(*(p+2)) != 'f')
			strcat(afm, ".afm");   // append .afm if the extension didn't seem to be .pf[ab]
		else
			strcpy(p, ".afm"); 	   // else replace extension with .afm
		FT_Attach_File(ftFace, afm); // ignore error code; AFM might not exist
		delete[] afm;
	}

	if (ftFace == 0) {
		status = 1;
		return;
	}

	FT_Set_Char_Size(ftFace, fPointSize * 64, 0, 0, 0);
	hbFont = hb_ft_font_create(ftFace, NULL);

	char buf[20];
	if (index > 0)
		sprintf(buf, ":%d", index);
	else
		buf[0] = 0;
	fFilename = new char[strlen(pathname) + 2 + strlen(buf) + 1];
	sprintf(fFilename, "[%s%s]", pathname, buf);
	fUnitsPerEM = ftFace->units_per_EM;
	fAscent = unitsToPoints(ftFace->ascender);
	fDescent = unitsToPoints(ftFace->descender);
	fItalicAngle = 0;

	postTable = (TT_Postscript *) getFontTable(ft_sfnt_post);

	if (postTable != NULL) {
		fItalicAngle = Fix2D(postTable->italicAngle);
	}

	os2Table = (TT_OS2*) getFontTable(ft_sfnt_os2);
	if (os2Table) {
		fCapHeight = unitsToPoints(os2Table->sCapHeight);
		fXHeight = unitsToPoints(os2Table->sxHeight);
	}

    return;
}

void
XeTeXFontInst::setLayoutDirVertical(bool vertical)
{
	fVertical = vertical;
}

const void *
XeTeXFontInst::getFontTable(OTTag tag) const
{
	FT_ULong tmpLength = 0;
	FT_Error error = FT_Load_Sfnt_Table(ftFace, tag, 0, NULL, &tmpLength);
	if (error != 0)
		return NULL;

	void* table = xmalloc(tmpLength * sizeof(char));
	if (table != NULL) {
		error = FT_Load_Sfnt_Table(ftFace, tag, 0, (FT_Byte*)table, &tmpLength);
		if (error != 0) {
			free((void *) table);
			return NULL;
		}
	}

    return table;
}

const char *
XeTeXFontInst::getMathTable()
{
	if (fMath == NULL)
		fMath = (const char*) getFontTable(MATH_TAG);
	return fMath;
}

const void *
XeTeXFontInst::getFontTable(FT_Sfnt_Tag tag) const
{
	return FT_Get_Sfnt_Table(ftFace, tag);
}

void
XeTeXFontInst::getGlyphBounds(GlyphID gid, GlyphBBox* bbox)
{
	bbox->xMin = bbox->yMin = bbox->xMax = bbox->yMax = 0.0;

	FT_Error error = FT_Load_Glyph(ftFace, gid, FT_LOAD_NO_SCALE);
	if (error != 0)
		return;

    FT_Glyph glyph;
    error = FT_Get_Glyph(ftFace->glyph, &glyph);
	if (error == 0) {
		FT_BBox ft_bbox;
		FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_UNSCALED, &ft_bbox);
		bbox->xMin = unitsToPoints(ft_bbox.xMin);
		bbox->yMin = unitsToPoints(ft_bbox.yMin);
		bbox->xMax = unitsToPoints(ft_bbox.xMax);
		bbox->yMax = unitsToPoints(ft_bbox.yMax);
		FT_Done_Glyph(glyph);
	}
}

GlyphID
XeTeXFontInst::mapCharToGlyph(UChar32 ch) const
{
	return FT_Get_Char_Index(ftFace, ch);
}

uint16_t
XeTeXFontInst::getNumGlyphs() const
{
	return ftFace->num_glyphs;
}

void
XeTeXFontInst::getGlyphAdvance(GlyphID glyph, realpoint &advance) const
{
	FT_Error error = FT_Load_Glyph(ftFace, glyph, FT_LOAD_NO_SCALE);
	if (error != 0) {
		advance.x = advance.y = 0;
	}
	else {
		advance.x = fVertical ? 0 : unitsToPoints(ftFace->glyph->metrics.horiAdvance);
		advance.y = fVertical ? unitsToPoints(ftFace->glyph->metrics.vertAdvance) : 0;
	}
}

float
XeTeXFontInst::getGlyphWidth(GlyphID gid)
{
	realpoint advance;
	getGlyphAdvance(gid, advance);
	return advance.x;
}

void
XeTeXFontInst::getGlyphHeightDepth(GlyphID gid, float* ht, float* dp)
{
	GlyphBBox bbox;
	getGlyphBounds(gid, &bbox);

	if (ht)
		*ht = bbox.yMax;
	if (dp)
		*dp = -bbox.yMin;
}

void
XeTeXFontInst::getGlyphSidebearings(GlyphID gid, float* lsb, float* rsb)
{
	realpoint adv;
	getGlyphAdvance(gid, adv);

	GlyphBBox bbox;
	getGlyphBounds(gid, &bbox);

	if (lsb)
		*lsb = bbox.xMin;
	if (rsb)
		*rsb = adv.x - bbox.xMax;
}

float
XeTeXFontInst::getGlyphItalCorr(GlyphID gid)
{
	float rval = 0.0;

	realpoint adv;
	getGlyphAdvance(gid, adv);

	GlyphBBox bbox;
	getGlyphBounds(gid, &bbox);
	
	if (bbox.xMax > adv.x)
		rval = bbox.xMax - adv.x;
	
	return rval;
}

GlyphID
XeTeXFontInst::mapGlyphToIndex(const char* glyphName) const
{
	return FT_Get_Name_Index(ftFace, const_cast<char*>(glyphName));
}

const char*
XeTeXFontInst::getGlyphName(GlyphID gid, int& nameLen)
{
	if (FT_HAS_GLYPH_NAMES(ftFace)) {
		static char buffer[256];
		FT_Get_Glyph_Name(ftFace, gid, buffer, 256);
		nameLen = strlen(buffer);
		return &buffer[0];
	}
	else {
		nameLen = 0;
		return NULL;
	}
}

UChar32
XeTeXFontInst::getFirstCharCode()
{
	FT_UInt  gindex;
	return FT_Get_First_Char(ftFace, &gindex);
}

UChar32
XeTeXFontInst::getLastCharCode()
{
	FT_UInt  gindex;
	UChar32	ch = FT_Get_First_Char(ftFace, &gindex);
	UChar32	prev = ch;
	while (gindex != 0) {
		prev = ch;
		ch = FT_Get_Next_Char(ftFace, ch, &gindex);
	}
	return prev;
}
