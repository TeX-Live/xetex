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
 *   file name:  XeTeXFontInst.h
 *
 *   created on: 2005-10-22
 *   created by: Jonathan Kew
 *	
 *	originally based on PortableFontInstance.h from ICU
 */


#ifndef __XeTeXFontInst_H
#define __XeTeXFontInst_H

#include <stdio.h>

#include "LETypes.h"
#include "LEFontInstance.h"

#include "FontTableCache.h"

#include "sfnt.h"
#include "cmaps.h"

#include "XeTeXFontMgr.h"

extern "C" {
	void *xmalloc(unsigned);	// from kpathsea
};

// Abstract superclass that XeTeXOTLayoutEngine uses;
// create specific subclasses for each supported platform

class XeTeXFontInst : public LEFontInstance, protected FontTableCache
{
protected:
	PlatformFontRef fFontRef;

    float    fPointSize;

    le_int32 fUnitsPerEM;
    float fAscent;
    float fDescent;
    float fLeading;

    float fDeviceScaleX;
    float fDeviceScaleY;

	float fXHeight;
	float fItalicAngle;

    CMAPMapper *fCMAPMapper;

    const HMTXTable *fHMTXTable;
    le_uint16 fNumLongHorMetrics;
    le_uint16 fNumGlyphs;
	bool fNumGlyphsInited;

    virtual const void *readTable(LETag tag, le_uint32 *length) const = 0;
    void deleteTable(const void *table) const;
    void getMetrics();

    CMAPMapper *findUnicodeMapper();

    const void *readFontTable(LETag tableTag) const;

public:
	typedef struct {
		float	xMin;
		float	yMin;
		float	xMax;
		float	yMax;
	} GlyphBBox;

    XeTeXFontInst(PlatformFontRef fontRef, float pointSize, LEErrorCode &status);

    virtual ~XeTeXFontInst();

	virtual PlatformFontRef getFontRef() const
		{ return fFontRef; }

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

	virtual void getGlyphBounds(LEGlyphID glyph, GlyphBBox *bbox) = 0; /* must be implemented by subclass */

	void getGlyphHeightDepth(LEGlyphID glyph, float *ht, float* dp);	
	void getGlyphSidebearings(LEGlyphID glyph, float* lsb, float* rsb);
	float getGlyphItalCorr(LEGlyphID glyph);

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

    float getXHeight() const
    {
        return fXHeight;
    }

    float getItalicAngle() const
    {
        return fItalicAngle;
    }
};

#endif
