/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2008 by SIL International
 copyright (c) 2009, 2011 by Jonathan Kew

 Written by Jonathan Kew

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

#include "layout/LETypes.h"
#include "layout/LEFontInstance.h"

#include "FontTableCache.h"

#include "sfnt.h"

#include "XeTeXFontMgr.h"
#include "XeTeX_ext.h"

extern "C" {
	void *xmalloc(size_t);	// from kpathsea
};

// create specific subclasses for each supported platform

class XeTeXFontInst : protected FontTableCache
{
friend class XeTeXGrFont;

protected:
    float    fPointSize;

    int32_t fUnitsPerEM;
    float fAscent;
    float fDescent;
    float fLeading;

    float fDeviceScaleX;
    float fDeviceScaleY;

	float fXHeight;
	float fItalicAngle;

    const HMTXTable *fMetricsTable;
    uint16_t fNumLongMetrics;
    uint16_t fNumGlyphs;
	bool fNumGlyphsInited;
	
	bool fVertical; // false = horizontal, true = vertical

	char *fFilename; // actually holds [filename:index], as used in xetex

	int fFirstCharCode;
	int fLastCharCode;

    virtual const void *readTable(LETag tag, uint32_t *length) const = 0;
    void deleteTable(const void *table) const;
    void getMetrics();

    const void *readFontTable(LETag tableTag) const;
    const void *readFontTable(LETag tableTag, uint32_t& len) const;

public:
    XeTeXFontInst(float pointSize, LEErrorCode &status);

    virtual ~XeTeXFontInst();

	virtual void initialize(LEErrorCode &status);

    virtual const void *getFontTable(LETag tableTag) const;
    virtual const void *getFontTable(LETag tableTag, uint32_t* length) const;

	virtual const char *getFilename() const
	{
		return fFilename;
	}

	virtual void setLayoutDirVertical(bool vertical);

	virtual bool getLayoutDirVertical() const
	{
		return fVertical;
	};

    virtual int32_t getUnitsPerEM() const
    {
        return fUnitsPerEM;
    };

    virtual int32_t getAscent() const
    {
        return (int32_t)fAscent;
    }

    virtual int32_t getDescent() const
    {
        return (int32_t)fDescent;
    }

    virtual int32_t getLeading() const
    {
        return (int32_t)fLeading;
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

    virtual LEGlyphID mapCharToGlyph(LEUnicode32 ch) const = 0; /* must be implemented by subclass */
    virtual LEGlyphID mapGlyphToIndex(const char* glyphName) const;

	virtual uint16_t getNumGlyphs() const;

    virtual void getGlyphAdvance(LEGlyphID glyph, LEPoint &advance) const;

    virtual le_bool getGlyphPoint(LEGlyphID glyph, int32_t pointNumber, LEPoint &point) const;

	virtual void getGlyphBounds(LEGlyphID glyph, GlyphBBox *bbox) = 0; /* must be implemented by subclass */

	float getGlyphWidth(LEGlyphID glyph);	
	void getGlyphHeightDepth(LEGlyphID glyph, float *ht, float* dp);	
	void getGlyphSidebearings(LEGlyphID glyph, float* lsb, float* rsb);
	float getGlyphItalCorr(LEGlyphID glyph);

	virtual const char* getGlyphName(LEGlyphID gid, int& nameLen);
	
	virtual LEUnicode32 getFirstCharCode() = 0; /* must be implemented by subclass */
	virtual LEUnicode32 getLastCharCode() = 0; /* must be implemented by subclass */

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

	int32_t unitsToPoints(float units) const
	{
		return (units * fPointSize) / (float) fUnitsPerEM;
	}
    float getXHeight() const
    {
        return fXHeight;
    }

    float getItalicAngle() const
    {
        return fItalicAngle;
    }

	hb_font_t* hbFont;
};

#endif
