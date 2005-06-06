/*
 *   file name:  ATSFontInst.h
 *
 *   created on: 2004-05-21
 *   created by: Jonathan Kew
 *	
 *	based on PortableFontInstance.h, hacked to call ATS instead of reading a .ttf
 */


#ifndef __ATSFontInst_H
#define __ATSFontInst_H

#include <stdio.h>

#include "LETypes.h"
#include "LEFontInstance.h"

#include "FontTableCache.h"

#include "sfnt.h"
#include "cmaps.h"

#include <ApplicationServices/ApplicationServices.h>

class ATSFontInst : public LEFontInstance, protected FontTableCache
{
private:
    ATSFontRef	fATSFont;

    float    fPointSize;

    le_int32 fUnitsPerEM;
    float fAscent;
    float fDescent;
    float fLeading;

    float fDeviceScaleX;
    float fDeviceScaleY;

    CMAPMapper *fCMAPMapper;

    const HMTXTable *fHMTXTable;
    le_uint16 fNumLongHorMetrics;
    le_uint16 fNumGlyphs;
	bool fNumGlyphsInited;

    const void *readTable(LETag tag, le_uint32 *length) const;
    void deleteTable(const void *table) const;
    void getMetrics();

    CMAPMapper *findUnicodeMapper();

protected:
    const void *readFontTable(LETag tableTag) const;

public:
    ATSFontInst(char *fontName, float pointSize, LEErrorCode &status);
    ATSFontInst(ATSFontRef atsFont, float pointSize, LEErrorCode &status);

    virtual ~ATSFontInst();

	virtual void initialize(LEErrorCode &status);

    virtual const void *getFontTable(LETag tableTag) const;

    virtual le_int32 getUnitsPerEM() const
    {
        return fUnitsPerEM;
    };

    virtual le_int32 getAscent() const
    {
        return (le_int32)fAscent;
    }

    virtual le_int32 getDescent() const
    {
        return (le_int32)fDescent;
    }

    virtual le_int32 getLeading() const
    {
        return (le_int32)fLeading;
    }

    virtual float getExactAscent() const
    {
        return fAscent;
    }

    virtual float getExactDescent() const
    {
        return fDescent;
    }

    virtual float getExactLeading() const
    {
        return fLeading;
    }

    virtual LEGlyphID mapCharToGlyph(LEUnicode32 ch) const
    {
        return fCMAPMapper->unicodeToGlyph(ch);
    }

	virtual le_uint16 getNumGlyphs() const;

    virtual void getGlyphAdvance(LEGlyphID glyph, LEPoint &advance) const;

    virtual le_bool getGlyphPoint(LEGlyphID glyph, le_int32 pointNumber, LEPoint &point) const;

    float getXPixelsPerEm() const
    {
        return fPointSize;
    };

    float getYPixelsPerEm() const
    {
        return fPointSize;
    };

    float getScaleFactorX() const
    {
        return 1.0;
    }

    float getScaleFactorY() const
    {
        return 1.0;
    }

	ATSFontRef	getATSFont() const
	{
		return fATSFont;
	}
	
	void		setATSFont(ATSFontRef fontRef);
};

#endif
