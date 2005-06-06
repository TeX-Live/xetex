#include "XeTeXLayoutInterface.h"

#include "XeTeXOTLayoutEngine.h"
#include "ATSFontInst.h"

#include "Features.h"
#include "ScriptAndLanguage.h"

#include "xetexfontdict.h"

#include "unicode/ubidi.h"

struct XeTeXLayoutEngine_rec
{
	LayoutEngine*	layoutEngine;
	ATSFontInst*	fontInstance;
	UInt32			scriptTag;
	UInt32			languageTag;
	UInt32*			addedFeatures;
	UInt32*			removedFeatures;
	UInt32			rgbValue;
};

ATSFontInstance createFontInstance(ATSFontRef atsFont, Fixed pointSize)
{
	LEErrorCode status = LE_NO_ERROR;
	ATSFontInst* fontInstance = new ATSFontInst(atsFont, Fix2X(pointSize), status);
	if (LE_FAILURE(status)) {
		delete fontInstance;
		return NULL;
	}
	return (ATSFontInstance)fontInstance;
}

void deleteFontInstance(ATSFontInstance fontInstance)
{
	delete (ATSFontInst*)fontInstance;
}

void* getFontTablePtr(ATSFontInstance fontInstance, UInt32 tableTag)
{
	return const_cast<void*>(((ATSFontInst*)fontInstance)->getFontTable(tableTag));
}
/*
void printFeatures(ATSFontInstance fontInstance)
{
    const GlyphSubstitutionTableHeader* gsubTable = (const GlyphSubstitutionTableHeader*)((ATSFontInst*)fontInstance)->getFontTable('GSUB');
    ScriptListTable* scriptListTable = (ScriptListTable*)((char*)gsubTable + gsubTable->scriptListOffset);
	FeatureListTable*	featureListTable = (FeatureListTable*)((char*)gsubTable + gsubTable->featureListOffset);
	for (int i = 0; i < featureListTable->featureCount; ++i) {
		UInt32  x = *(UInt32*)&featureListTable->featureRecordArray[i].featureTag;
		fprintf(stderr, "%08X = %.4s\n", x, featureListTable->featureRecordArray[i].featureTag);
	}
}
*/

void applyOpticalSize(ATSFontInstance fontInstance, Fixed opticalSize)
{
	ATSUFontID	oldFontID = FMGetFontFromATSFontRef(((ATSFontInst*)fontInstance)->getATSFont());
	ATSUFontID	newFontID = get_optically_sized_font(oldFontID, opticalSize);
	if (newFontID != oldFontID)
		((ATSFontInst*)fontInstance)->setATSFont(FMGetATSFontRefFromFont(newFontID));
}

UInt32 countScripts(ATSFontInstance fontInstance)
{
    const GlyphSubstitutionTableHeader* gsubTable = (const GlyphSubstitutionTableHeader*)((ATSFontInst*)fontInstance)->getFontTable('GSUB');
	if (gsubTable == NULL)
		return 0;

	const ScriptListTable* scriptList = (const ScriptListTable*)((const char*)gsubTable + gsubTable->scriptListOffset);
	UInt32  scriptCount = scriptList->scriptCount;

	return scriptCount;
}

UInt32 getIndScript(ATSFontInstance fontInstance, UInt32 index)
{
    const GlyphSubstitutionTableHeader* gsubTable = (const GlyphSubstitutionTableHeader*)((ATSFontInst*)fontInstance)->getFontTable('GSUB');
	if (gsubTable == NULL)
		return 0;

	const ScriptListTable* scriptList = (const ScriptListTable*)((const char*)gsubTable + gsubTable->scriptListOffset);

	if (index < scriptList->scriptCount)
		return *(UInt32*)(scriptList->scriptRecordArray[index].tag);

	return 0;
}

UInt32 countScriptLanguages(ATSFontInstance fontInstance, UInt32 script)
{
    const GlyphSubstitutionTableHeader* gsubTable = (const GlyphSubstitutionTableHeader*)((ATSFontInst*)fontInstance)->getFontTable('GSUB');
	if (gsubTable == NULL)
		return 0;
	
	const ScriptListTable* scriptList = (const ScriptListTable*)((const char*)gsubTable + gsubTable->scriptListOffset);
	const ScriptTable*  scriptTable = scriptList->findScript(script);
	if (scriptTable == NULL)
		return 0;
	
	UInt32  langCount = scriptTable->langSysCount;
	
	return langCount;
}

UInt32 getIndScriptLanguage(ATSFontInstance fontInstance, UInt32 script, UInt32 index)
{
    const GlyphSubstitutionTableHeader* gsubTable = (const GlyphSubstitutionTableHeader*)((ATSFontInst*)fontInstance)->getFontTable('GSUB');
	if (gsubTable == NULL)
		return 0;

	const ScriptListTable* scriptList = (const ScriptListTable*)((const char*)gsubTable + gsubTable->scriptListOffset);
	const ScriptTable*  scriptTable = scriptList->findScript(script);
	if (scriptTable == NULL)
		return 0;

	if (index < scriptTable->langSysCount)
		return *(UInt32*)(scriptTable->langSysRecordArray[index].tag);

	return 0;
}

UInt32 countFeatures(ATSFontInstance fontInstance, UInt32 script, UInt32 language)
{
	UInt32  total = 0;
    const GlyphLookupTableHeader* table;
	for (int i = 0; i < 2; ++i) {
		table = (const GlyphLookupTableHeader*)((ATSFontInst*)fontInstance)->getFontTable(i == 0 ? 'GSUB' : 'GPOS');
		if (table != NULL) {
			const ScriptListTable* scriptList = (const ScriptListTable*)((const char*)table + table->scriptListOffset);
			const ScriptTable*  scriptTable = scriptList->findScript(script);
			if (scriptTable != NULL) {
				const LangSysTable* langTable = scriptTable->findLanguage(language, (language != 0));
				if (langTable != NULL) {
					total += langTable->featureCount;
					if (langTable->reqFeatureIndex != 0xffff)
						total += 1;
				}
			}
		}
	}
	
	return total;
}

UInt32 getIndFeature(ATSFontInstance fontInstance, UInt32 script, UInt32 language, UInt32 index)
{
    const GlyphLookupTableHeader* table;
	UInt16  featureIndex = 0xffff;
	for (int i = 0; i < 2; ++i) {
		table = (const GlyphLookupTableHeader*)((ATSFontInst*)fontInstance)->getFontTable(i == 0 ? 'GSUB' : 'GPOS');
		if (table != NULL) {
			const ScriptListTable* scriptList = (const ScriptListTable*)((const char*)table + table->scriptListOffset);
			const ScriptTable*  scriptTable = scriptList->findScript(script);
			if (scriptTable != NULL) {
				const LangSysTable* langTable = scriptTable->findLanguage(language, (language != 0));
				if (langTable != NULL) {
					if (langTable->reqFeatureIndex != 0xffff)
						if (index == 0)
							featureIndex = langTable->reqFeatureIndex;
						else
							index -= 1;
					if (index < langTable->featureCount)
						featureIndex = langTable->featureIndexArray[index];
					index -= langTable->featureCount;
				}
			}
			if (featureIndex != 0xffff) {
				LETag   featureTag;
				const FeatureListTable* featureListTable = (const FeatureListTable*)((const char*)table + table->featureListOffset);
				(void)featureListTable->getFeatureTable(featureIndex, &featureTag);
				return featureTag;
			}
		}
	}
	
	return 0;
}

float getGlyphWidth(ATSFontInstance fontInstance, UInt32 gid)
{
	LEPoint	adv;
	((ATSFontInst*)fontInstance)->getGlyphAdvance(gid, adv);
	return adv.fX;
}

UInt32
countGlyphs(ATSFontInstance fontInstance)
{
	return ((ATSFontInst*)fontInstance)->getNumGlyphs();
}

ATSFontInstance getFontInstance(XeTeXLayoutEngine engine)
{
	return (ATSFontInstance)(engine->fontInstance);
}

XeTeXLayoutEngine createLayoutEngine(ATSFontInstance fontInstance, UInt32 scriptTag, UInt32 languageTag,
										UInt32* addFeatures, UInt32* removeFeatures, UInt32 rgbValue)
{
	LEErrorCode status = LE_NO_ERROR;
	XeTeXLayoutEngine result = new XeTeXLayoutEngine_rec;
	result->fontInstance = (ATSFontInst*)fontInstance;
	result->scriptTag = scriptTag;
	result->languageTag = languageTag;
	result->addedFeatures = addFeatures;
	result->removedFeatures = removeFeatures;
	result->rgbValue = rgbValue;
	result->layoutEngine = XeTeXOTLayoutEngine::LayoutEngineFactory((ATSFontInst*)fontInstance, scriptTag, languageTag, (LETag*)addFeatures, (LETag*)removeFeatures, status);
	if (LE_FAILURE(status) || result->layoutEngine == NULL) {
		delete result;
		return NULL;
	}
	return result;
}

void deleteLayoutEngine(XeTeXLayoutEngine engine)
{
	delete engine->layoutEngine;
	delete engine->fontInstance;
}

SInt32 layoutChars(XeTeXLayoutEngine engine, UInt16 chars[], SInt32 offset, SInt32 count, SInt32 max,
						Boolean rightToLeft, float x, float y, OSStatus* status)
{
	LEErrorCode success = (LEErrorCode)*status;
	le_int32	glyphCount = engine->layoutEngine->layoutChars(chars, offset, count, max, rightToLeft, x, y, success);
	*status = success;
	return glyphCount;
}

void getGlyphs(XeTeXLayoutEngine engine, UInt32 glyphs[], OSStatus* status)
{
	LEErrorCode success = (LEErrorCode)*status;
	engine->layoutEngine->getGlyphs((LEGlyphID*)glyphs, success);
	*status = success;
}

void getGlyphPositions(XeTeXLayoutEngine engine, float positions[], OSStatus* status)
{
	LEErrorCode success = (LEErrorCode)*status;
	engine->layoutEngine->getGlyphPositions(positions, success);
	*status = success;
}

void getGlyphPosition(XeTeXLayoutEngine engine, SInt32 index, float* x, float* y, OSStatus* status)
{
	LEErrorCode success = (LEErrorCode)*status;
	engine->layoutEngine->getGlyphPosition(index, *x, *y, success);
	*status = success;
}

UInt32 getScriptTag(XeTeXLayoutEngine engine)
{
	return engine->scriptTag;
}

UInt32 getLanguageTag(XeTeXLayoutEngine engine)
{
	return engine->languageTag;
}

ATSFontRef getFontRef(XeTeXLayoutEngine engine)
{
	return engine->fontInstance->getATSFont();
}

float getPointSize(XeTeXLayoutEngine engine)
{
	return engine->fontInstance->getXPixelsPerEm();
}

void getAscentAndDescent(XeTeXLayoutEngine engine, float* ascent, float* descent)
{
	*ascent = engine->fontInstance->getExactAscent();
	*descent = engine->fontInstance->getExactDescent();
}

UInt32* getAddedFeatures(XeTeXLayoutEngine engine)
{
	return engine->addedFeatures;
}

UInt32* getRemovedFeatures(XeTeXLayoutEngine engine)
{
	return engine->removedFeatures;
}

int getDefaultDirection(XeTeXLayoutEngine engine)
{
	switch (engine->scriptTag) {
		case 'syrc':
		case 'thaa':
		case 'hebr':
		case 'arab':
			return UBIDI_DEFAULT_RTL;
	}
	return UBIDI_DEFAULT_LTR;
}

UInt32 getRgbValue(XeTeXLayoutEngine engine)
{
	return engine->rgbValue;
}
