/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

#include "XeTeXOTLayoutEngine.h"

#include "ThaiLayoutEngine.h"
#include "KhmerLayoutEngine.h"

#include "LEScripts.h"
#include "LELanguages.h"

const LETag emptyTag = 0x00000000;

static le_int32 getScriptCode(LETag scriptTag)
{
	for (le_int32 i = 0; i < scriptCodeCount; ++i)
		if (OpenTypeLayoutEngine::getScriptTag(i) == scriptTag)
			return i;
	return -1;
}

static le_int32 getLanguageCode(LETag languageTag)
{
	for (le_int32 i = 0; i < languageCodeCount; ++i)
		if (OpenTypeLayoutEngine::getLangSysTag(i) == languageTag)
			return i;
	return -1;
}

LayoutEngine* XeTeXOTLayoutEngine::LayoutEngineFactory
				(const LEFontInstance* fontInstance,
					LETag scriptTag, LETag languageTag,
					const LETag* addFeatures, const LETag* removeFeatures,
					LEErrorCode &success)
{
    static le_uint32 gsubTableTag = LE_GSUB_TABLE_TAG;

    if (LE_FAILURE(success))
        return NULL;

    const GlyphSubstitutionTableHeader* gsubTable = (const GlyphSubstitutionTableHeader*)fontInstance->getFontTable(gsubTableTag);
    LayoutEngine *result = NULL;
	
	le_uint32   scriptCode = getScriptCode(scriptTag);
	le_uint32   languageCode = getLanguageCode(languageTag);

	le_int32	typoFlags = 3;

    if (gsubTable != NULL && gsubTable->coversScript(scriptTag)) {
        switch (scriptCode) {
        case bengScriptCode:
        case devaScriptCode:
        case gujrScriptCode:
        case kndaScriptCode:
        case mlymScriptCode:
        case oryaScriptCode:
        case guruScriptCode:
        case tamlScriptCode:
        case teluScriptCode:
//            result = new XeTeXIndicLayoutEngine(fontInstance, scriptTag, languageTag, gsubTable, addFeatures, removeFeatures);
            result = new IndicOpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, gsubTable);
            break;

        case arabScriptCode:
        case syrcScriptCode:
        case mongScriptCode:
//            result = new XeTeXArabicLayoutEngine(fontInstance, scriptTag, languageTag, gsubTable, addFeatures, removeFeatures);
            result = new ArabicOpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, gsubTable);
            break;

        case haniScriptCode:
            switch (languageCode) {
            case korLanguageCode:
            case janLanguageCode:
            case zhtLanguageCode:
            case zhsLanguageCode:
                if (gsubTable->coversScriptAndLanguage(scriptTag, languageTag, TRUE)) {
//                    result = new XeTeXHanLayoutEngine(fontInstance, scriptTag, languageTag, gsubTable, addFeatures, removeFeatures);
                    result = new HanOpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, gsubTable);
                    break;
                }

                // note: falling through to default case.
            default:
//                result = new XeTeXOTLayoutEngine(fontInstance, scriptTag, languageTag, gsubTable, addFeatures, removeFeatures);
                result = new OpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, gsubTable);
                break;
            }

            break;

        case khmrScriptCode:
            result = new KhmerOpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, gsubTable);
            break;

        default:
            result = new XeTeXOTLayoutEngine(fontInstance, scriptTag, languageTag, gsubTable, addFeatures, removeFeatures);
            break;
        }
    }
	else {
		switch (scriptCode) {
		case bengScriptCode:
		case devaScriptCode:
		case gujrScriptCode:
		case kndaScriptCode:
		case mlymScriptCode:
		case oryaScriptCode:
		case guruScriptCode:
		case tamlScriptCode:
		case teluScriptCode:
			result = new IndicOpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags);
			break;

		case arabScriptCode:
		case hebrScriptCode:
			result = new UnicodeArabicOpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags);
			break;

		case thaiScriptCode:
			result = new ThaiLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags);
			break;

		default:
			result = new OpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags);
			break;
		}
    }

    if (result == NULL)
        success = LE_MEMORY_ALLOCATION_ERROR;

    return result;
}

/*
 * XeTeXOTLayoutEngine
 */

const char XeTeXOTLayoutEngine::fgClassID=0;

XeTeXOTLayoutEngine::XeTeXOTLayoutEngine(
	const LEFontInstance* fontInstance, LETag scriptTag, LETag languageTag,
	const GlyphSubstitutionTableHeader* gsubTable,
	const LETag* addFeatures, const LETag* removeFeatures)
		: OpenTypeLayoutEngine(fontInstance, getScriptCode(scriptTag), getLanguageCode(languageTag), 3, gsubTable)
{
	fDefaultFeatures = fFeatureList;
	fScriptTag = scriptTag;
	fLangSysTag = languageTag;
	adjustFeatures(addFeatures, removeFeatures);
}

XeTeXOTLayoutEngine::~XeTeXOTLayoutEngine()
{
	if (fFeatureList != NULL && fFeatureList != fDefaultFeatures)
		LE_DELETE_ARRAY(fFeatureList);
}

void XeTeXOTLayoutEngine::adjustFeatures(const LETag* addTags, const LETag* removeTags)
{
	// bail out if nothing was requested!
	if ((addTags == NULL || *addTags == emptyTag) && (removeTags == NULL || *removeTags == emptyTag))
		return;
	
	// count the max possible number of tags we might end up with
	le_uint32   count = 0;
	const LETag*	pTag = fFeatureList;
	while (*pTag++ != emptyTag)
		count++;
	if (addTags != NULL && *addTags != emptyTag) {
		pTag = addTags;
		while (*pTag++ != emptyTag)
			count++;
	}
	
	// allocate new array for the tag list, big enough for max count
	LETag* newList = LE_NEW_ARRAY(LETag, count + 1);
	
	// copy the existing tags, skipping any in the remove list
	LETag*  dest = newList;
	pTag = fFeatureList;
	while (*pTag != emptyTag) {
		const LETag*	t = removeTags;
		if (t != NULL)
			while (*t != emptyTag)
				if (*t == *pTag)
					break;
				else
					t++;
		if (t == NULL || *t == emptyTag)
			*dest++ = *pTag;
		pTag++;
	}
	
	// copy the added tags, skipping any already present
	pTag = addTags;
	if (pTag != NULL)
		while (*pTag != emptyTag) {
			const LETag*	t = newList;
			while (t < dest)
				if (*t == *pTag)
					break;
				else
					t++;
			if (t == dest)
				*dest++ = *pTag;
			pTag++;
		}
	
	// terminate the new list
	*dest = emptyTag;
	
	// delete the previous list, unless it was the static default array
	if (fFeatureList != fDefaultFeatures)
		LE_DELETE_ARRAY(fFeatureList);
	
	fFeatureList = newList;
}

#if 0
/*
 * XeTeXArabicLayoutEngine
 */

const char XeTeXArabicLayoutEngine::fgClassID=0;

XeTeXArabicLayoutEngine::XeTeXArabicLayoutEngine(
	const LEFontInstance* fontInstance, LETag scriptTag, LETag languageTag,
	const GlyphSubstitutionTableHeader* gsubTable,
	const LETag* addFeatures, const LETag* removeFeatures)
		: ArabicOpenTypeLayoutEngine(fontInstance, getScriptCode(scriptTag), getLanguageCode(languageTag), gsubTable)
{
//	fDefaultFeatures = fFeatureList;
	fScriptTag = scriptTag;
	fLangSysTag = languageTag;
	adjustFeatures(addFeatures, removeFeatures);
}

XeTeXArabicLayoutEngine::~XeTeXArabicLayoutEngine()
{
//	if (fFeatureList != NULL && fFeatureList != fDefaultFeatures)
//		LE_DELETE_ARRAY(fFeatureList);
}

void XeTeXArabicLayoutEngine::adjustFeatures(const LETag* addTags, const LETag* removeTags)
{
	// FIXME: not yet supported
}

/*
 * XeTeXIndicLayoutEngine
 */

const char XeTeXIndicLayoutEngine::fgClassID=0;

XeTeXIndicLayoutEngine::XeTeXIndicLayoutEngine(
	const LEFontInstance* fontInstance, LETag scriptTag, LETag languageTag,
	const GlyphSubstitutionTableHeader* gsubTable,
	const LETag* addFeatures, const LETag* removeFeatures)
		: IndicOpenTypeLayoutEngine(fontInstance, getScriptCode(scriptTag), getLanguageCode(languageTag), gsubTable)
{
//	fDefaultFeatures = fFeatureList;
	fScriptTag = scriptTag;
	fLangSysTag = languageTag;
	adjustFeatures(addFeatures, removeFeatures);
}

XeTeXIndicLayoutEngine::~XeTeXIndicLayoutEngine()
{
//	if (fFeatureList != NULL && fFeatureList != fDefaultFeatures)
//		LE_DELETE_ARRAY(fFeatureList);
}

void XeTeXIndicLayoutEngine::adjustFeatures(const LETag* addTags, const LETag* removeTags)
{
	// FIXME!
}

/*
 * XeTeXHanLayoutEngine
 */

const char XeTeXHanLayoutEngine::fgClassID=0;

XeTeXHanLayoutEngine::XeTeXHanLayoutEngine(
	const LEFontInstance* fontInstance, LETag scriptTag, LETag languageTag,
	const GlyphSubstitutionTableHeader* gsubTable,
	const LETag* addFeatures, const LETag* removeFeatures)
		: HanOpenTypeLayoutEngine(fontInstance, getScriptCode(scriptTag), getLanguageCode(languageTag), gsubTable)
{
//	fDefaultFeatures = fFeatureList;
	fScriptTag = scriptTag;
	fLangSysTag = languageTag;
	adjustFeatures(addFeatures, removeFeatures);
}

XeTeXHanLayoutEngine::~XeTeXHanLayoutEngine()
{
//	if (fFeatureList != NULL && fFeatureList != fDefaultFeatures)
//		LE_DELETE_ARRAY(fFeatureList);
}

void XeTeXHanLayoutEngine::adjustFeatures(const LETag* addTags, const LETag* removeTags)
{
	// FIXME!
}

#endif
