/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2006 by SIL International
 written by Jonathan Kew

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
NONINFRINGEMENT. IN NO EVENT SHALL SIL INTERNATIONAL BE LIABLE FOR  
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION  
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of SIL International  
shall not be used in advertising or otherwise to promote the sale,  
use or other dealings in this Software without prior written  
authorization from SIL International.
\****************************************************************************/

#ifndef __XeTeXOTLayoutEngine_h
#define __XeTeXOTLayoutEngine_h

#include "OpenTypeLayoutEngine.h"
#include "ArabicLayoutEngine.h"
#include "IndicLayoutEngine.h"
#include "HanLayoutEngine.h"

#include "XeTeXFontInst.h"

class XeTeXOTLayoutEngine : public OpenTypeLayoutEngine
{
public:
    XeTeXOTLayoutEngine(const LEFontInstance* fontInstance, LETag scriptTag, LETag languageTag,
                            const GlyphSubstitutionTableHeader* gsubTable,
							const LETag* addFeatures, const LETag* removeFeatures);

    virtual ~XeTeXOTLayoutEngine();

	virtual void adjustFeatures(const LETag* addTags, const LETag* removeTags);

    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }
    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

    static LayoutEngine* LayoutEngineFactory
				(const XeTeXFontInst* fontInstance,
					LETag scriptTag, LETag languageTag,
					const LETag* addFeatures, const LETag* removeFeatures,
					LEErrorCode &success);

protected:
	const LETag*	fDefaultFeatures;
	
private:
    static const char fgClassID;
	
};

class XeTeXArabicLayoutEngine : public ArabicOpenTypeLayoutEngine
{
public:
    XeTeXArabicLayoutEngine(const LEFontInstance *fontInstance, LETag scriptTag, LETag languageTag,
                            const GlyphSubstitutionTableHeader *gsubTable,
							const LETag *addFeatures, const LETag *removeFeatures);

    virtual ~XeTeXArabicLayoutEngine();

	virtual void adjustFeatures(const LETag* addTags, const LETag* removeTags);

    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }
    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

private:
    static const char fgClassID;

	const LETag*	fDefaultFeatures;
};

class XeTeXIndicLayoutEngine : public IndicOpenTypeLayoutEngine
{
public:
    XeTeXIndicLayoutEngine(const LEFontInstance *fontInstance, LETag scriptTag, LETag languageTag,
                            const GlyphSubstitutionTableHeader *gsubTable,
							const LETag *addFeatures, const LETag *removeFeatures);

    virtual ~XeTeXIndicLayoutEngine();

	virtual void adjustFeatures(const LETag* addTags, const LETag* removeTags);

    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }
    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

private:
    static const char fgClassID;

	const LETag*	fDefaultFeatures;
};

class XeTeXHanLayoutEngine : public XeTeXOTLayoutEngine
{
public:
    XeTeXHanLayoutEngine(const XeTeXFontInst *fontInstance, LETag scriptTag, LETag languageTag,
                            const GlyphSubstitutionTableHeader *gsubTable,
							const LETag *addFeatures, const LETag *removeFeatures);

    virtual ~XeTeXHanLayoutEngine();

//	virtual void adjustFeatures(const LETag* addTags, const LETag* removeTags);

    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }
    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

private:
    static const char fgClassID;
};

#endif

