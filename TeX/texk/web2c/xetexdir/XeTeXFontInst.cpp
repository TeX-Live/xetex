/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

/*
 *   file name:  XeTeXFontInst.cpp
 *
 *   created on: 2005-10-22
 *   created by: Jonathan Kew
 *	
 *	originally based on PortableFontInstance.cpp from ICU
 */


#include "layout/LETypes.h"
#include "layout/LEFontInstance.h"
#include "layout/LESwaps.h"

#include "XeTeXFontInst.h"

#ifdef XETEX_MAC
#include <Carbon/Carbon.h>
#endif

#include "XeTeX_ext.h"

#include "sfnt.h"

#include <string.h>


XeTeXFontInst::XeTeXFontInst(float pointSize, LEErrorCode &status)
    : fPointSize(pointSize)
    , fUnitsPerEM(0)
    , fAscent(0)
    , fDescent(0)
    , fLeading(0)
    , fXHeight(0)
    , fItalicAngle(0)
    , fCMAPMapper(NULL)
    , fHMTXTable(NULL)
    , fNumLongHorMetrics(0)
    , fNumGlyphs(0)
    , fNumGlyphsInited(false)
{
	// the concrete subclass is responsible to call initialize()
}

XeTeXFontInst::~XeTeXFontInst()
{
	if (fHMTXTable != NULL)
		deleteTable(fHMTXTable);

	if (fCMAPMapper != NULL)
		delete fCMAPMapper;
}

void XeTeXFontInst::initialize(LEErrorCode &status)
{
    const LETag headTag = LE_HEAD_TABLE_TAG;
    const LETag hheaTag = LE_HHEA_TABLE_TAG;
    const LETag postTag = LE_POST_TABLE_TAG;
    const HEADTable *headTable = NULL;
    const HHEATable *hheaTable = NULL;
    const POSTTable *postTable = NULL;

    // read unitsPerEm from 'head' table
    headTable = (const HEADTable *) readFontTable(headTag);

    if (headTable == NULL) {
        status = LE_MISSING_FONT_TABLE_ERROR;
        goto error_exit;
    }

    fUnitsPerEM = SWAPW(headTable->unitsPerEm);
    deleteTable(headTable);

    hheaTable = (const HHEATable *) readFontTable(hheaTag);

    if (hheaTable == NULL) {
        status = LE_MISSING_FONT_TABLE_ERROR;
        goto error_exit;
    }

    fAscent  = yUnitsToPoints((float) SWAPW(hheaTable->ascent));
    fDescent = yUnitsToPoints((float) SWAPW(hheaTable->descent));
    fLeading = yUnitsToPoints((float) SWAPW(hheaTable->lineGap));

    fNumLongHorMetrics = SWAPW(hheaTable->numOfLongHorMetrics);

    deleteTable((void *) hheaTable);

    fCMAPMapper = findUnicodeMapper();

    if (fCMAPMapper == NULL) {
        status = LE_MISSING_FONT_TABLE_ERROR;
        goto error_exit;
    }

    postTable = (const POSTTable *) readFontTable(postTag);

    if (postTable != NULL) {
		fItalicAngle = Fix2X(SWAPL(postTable->italicAngle));
		deleteTable((void*)postTable);
    }

    return;

error_exit:
    return;
}

void XeTeXFontInst::deleteTable(const void *table) const
{
    LE_DELETE_ARRAY(table);
}

const void *XeTeXFontInst::getFontTable(LETag tableTag) const
{
    return FontTableCache::find(tableTag);
}

const void *XeTeXFontInst::readFontTable(LETag tableTag) const
{
    le_uint32 len;

    return readTable(tableTag, &len);
}

CMAPMapper *XeTeXFontInst::findUnicodeMapper()
{
    LETag cmapTag = LE_CMAP_TABLE_TAG;
    const CMAPTable *cmap = (CMAPTable *) readFontTable(cmapTag);

    if (cmap == NULL) {
        return NULL;
    }

    return CMAPMapper::createUnicodeMapper(cmap);
}


le_uint16 XeTeXFontInst::getNumGlyphs() const
{
    if (!fNumGlyphsInited) {
        LETag maxpTag = LE_MAXP_TABLE_TAG;
        const MAXPTable *maxpTable = (MAXPTable *) readFontTable(maxpTag);

        if (maxpTable != NULL) {
			XeTeXFontInst *realThis = (XeTeXFontInst *) this;
            realThis->fNumGlyphs = SWAPW(maxpTable->numGlyphs);
            deleteTable(maxpTable);
			realThis->fNumGlyphsInited = true;
		}
    }

	return fNumGlyphs;
}

void XeTeXFontInst::getGlyphAdvance(LEGlyphID glyph, LEPoint &advance) const
{
    TTGlyphID ttGlyph = (TTGlyphID) LE_GET_GLYPH(glyph);

    if (fHMTXTable == NULL) {
        LETag hmtxTag = LE_HMTX_TABLE_TAG;
        XeTeXFontInst *realThis = (XeTeXFontInst *) this;
        realThis->fHMTXTable = (const HMTXTable *) readFontTable(hmtxTag);
    }

    le_uint16 index = ttGlyph;

    if (ttGlyph >= getNumGlyphs() || fHMTXTable == NULL) {
        advance.fX = advance.fY = 0;
        return;
    }

    if (ttGlyph >= fNumLongHorMetrics) {
        index = fNumLongHorMetrics - 1;
    }

    advance.fX = xUnitsToPoints(SWAPW(fHMTXTable->hMetrics[index].advanceWidth));
    advance.fY = 0;
}

le_bool XeTeXFontInst::getGlyphPoint(LEGlyphID glyph, le_int32 pointNumber, LEPoint &point) const
{
    return FALSE;
}
