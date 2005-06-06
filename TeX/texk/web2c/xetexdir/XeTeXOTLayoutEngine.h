#ifndef __XeTeXOTLayoutEngine_h
#define __XeTeXOTLayoutEngine_h

#include "OpenTypeLayoutEngine.h"
#include "ArabicLayoutEngine.h"
#include "IndicLayoutEngine.h"
#include "HanLayoutEngine.h"

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
				(const LEFontInstance* fontInstance,
					LETag scriptTag, LETag languageTag,
					const LETag* addFeatures, const LETag* removeFeatures,
					LEErrorCode &success);

private:
    static const char fgClassID;
	
	const LETag*	fDefaultFeatures;
	
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

class XeTeXHanLayoutEngine : public HanOpenTypeLayoutEngine
{
public:
    XeTeXHanLayoutEngine(const LEFontInstance *fontInstance, LETag scriptTag, LETag languageTag,
                            const GlyphSubstitutionTableHeader *gsubTable,
							const LETag *addFeatures, const LETag *removeFeatures);

    virtual ~XeTeXHanLayoutEngine();

	virtual void adjustFeatures(const LETag* addTags, const LETag* removeTags);

    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }
    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

private:
    static const char fgClassID;

	const LETag*	fDefaultFeatures;
};

#endif

