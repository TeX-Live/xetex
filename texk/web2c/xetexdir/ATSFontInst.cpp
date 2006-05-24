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
 *   file name:  ATSFontInst.cpp
 *
 *   created on: 2004-05-21
 *   created by: Jonathan Kew
 *	
 *	based on PortableFontInstance.cpp, hacked to call ATS instead of reading a .ttf
 */


#include <stdio.h>

#include "layout/LETypes.h"
#include "layout/LEFontInstance.h"
#include "layout/LESwaps.h"

#include "ATSFontInst.h"

#include "sfnt.h"

#include <string.h>


ATSFontInst::ATSFontInst(char *fontName, float pointSize, LEErrorCode &status)
    : fATSFont(0), fPointSize(pointSize), fUnitsPerEM(0), fAscent(0), fDescent(0), fLeading(0),
      fCMAPMapper(NULL), fHMTXTable(NULL), fNumLongHorMetrics(0), fNumGlyphs(0), fNumGlyphsInited(false)
{
    if (LE_FAILURE(status)) {
        return;
    }

	CFStringRef	nameString = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,
									fontName, kCFStringEncodingASCII, kCFAllocatorNull);
	fATSFont = ATSFontFindFromPostScriptName(nameString, kATSOptionFlagsDefault);
	CFRelease(nameString);

    if (fATSFont == 0) {
        status = LE_FONT_FILE_NOT_FOUND_ERROR;
        return;
    }

	initialize(status);
}

ATSFontInst::ATSFontInst(ATSFontRef atsFont, float pointSize, LEErrorCode &status)
    : fATSFont(atsFont), fPointSize(pointSize), fUnitsPerEM(0), fAscent(0), fDescent(0), fLeading(0),
      fCMAPMapper(NULL), fHMTXTable(NULL), fNumLongHorMetrics(0), fNumGlyphs(0), fNumGlyphsInited(false)
{
    if (LE_FAILURE(status)) {
        return;
    }

    if (fATSFont == 0) {
        status = LE_FONT_FILE_NOT_FOUND_ERROR;
        return;
    }

	initialize(status);
}

ATSFontInst::~ATSFontInst()
{
	if (fATSFont != 0) {
		deleteTable(fHMTXTable);
		delete fCMAPMapper;
	}
}

void ATSFontInst::initialize(LEErrorCode &status)
{
    const LETag headTag = LE_HEAD_TABLE_TAG;
    const LETag hheaTag = LE_HHEA_TABLE_TAG;
    const HEADTable *headTable = NULL;
    const HHEATable *hheaTable = NULL;

    // read unitsPerEm from 'head' table
    headTable = (const HEADTable *) readFontTable(headTag);

    if (headTable == NULL) {
        status = LE_MISSING_FONT_TABLE_ERROR;
        goto error_exit;
    }

    fUnitsPerEM = SWAPW(headTable->unitsPerEm);
    deleteTable(headTable);

    hheaTable = (HHEATable *) readFontTable(hheaTag);

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

    return;

error_exit:
	fATSFont = 0;
	
    return;
}

void ATSFontInst::setATSFont(ATSFontRef fontRef)
{
	fATSFont = fontRef;

	flush();	// clear the font table cache

	LEErrorCode	status = (LEErrorCode)0;
	initialize(status);
}

void ATSFontInst::deleteTable(const void *table) const
{
    LE_DELETE_ARRAY(table);
}

const void *ATSFontInst::readTable(LETag tag, le_uint32 *length) const
{
	OSStatus status = ATSFontGetTable(fATSFont, tag, 0, 0, 0, (ByteCount*)length);
	if (status != noErr) {
		*length = 0;
		return NULL;
	}
	void*	table = LE_NEW_ARRAY(char, *length);
	if (table != NULL) {
		status = ATSFontGetTable(fATSFont, tag, 0, *length, table, (ByteCount*)length);
		if (status != noErr) {
			*length = 0;
			LE_DELETE_ARRAY(table);
			return NULL;
		}
	}

    return table;
}

const void *ATSFontInst::getFontTable(LETag tableTag) const
{
    return FontTableCache::find(tableTag);
}

const void *ATSFontInst::readFontTable(LETag tableTag) const
{
    le_uint32 len;

    return readTable(tableTag, &len);
}

CMAPMapper *ATSFontInst::findUnicodeMapper()
{
    LETag cmapTag = LE_CMAP_TABLE_TAG;
    const CMAPTable *cmap = (CMAPTable *) readFontTable(cmapTag);

    if (cmap == NULL) {
        return NULL;
    }

    return CMAPMapper::createUnicodeMapper(cmap);
}


le_uint16 ATSFontInst::getNumGlyphs() const
{
    if (!fNumGlyphsInited) {
        LETag maxpTag = LE_MAXP_TABLE_TAG;
        const MAXPTable *maxpTable = (MAXPTable *) readFontTable(maxpTag);

        if (maxpTable != NULL) {
			ATSFontInst *realThis = (ATSFontInst *) this;
            realThis->fNumGlyphs = SWAPW(maxpTable->numGlyphs);
            deleteTable(maxpTable);
			realThis->fNumGlyphsInited = true;
		}
    }

	return fNumGlyphs;
}

void ATSFontInst::getGlyphAdvance(LEGlyphID glyph, LEPoint &advance) const
{
    TTGlyphID ttGlyph = (TTGlyphID) LE_GET_GLYPH(glyph);

    if (fHMTXTable == NULL) {
        LETag hmtxTag = LE_HMTX_TABLE_TAG;
        ATSFontInst *realThis = (ATSFontInst *) this;
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

le_bool ATSFontInst::getGlyphPoint(LEGlyphID glyph, le_int32 pointNumber, LEPoint &point) const
{
    return FALSE;
}
