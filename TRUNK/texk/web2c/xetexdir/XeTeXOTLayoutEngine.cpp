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
    static le_uint32 gposTableTag = LE_GPOS_TABLE_TAG;

	fDefaultFeatures = fFeatureList;
	
	// check the result of setScriptAndLanguageTags(), in case they were unknown to ICU
	if (fScriptTag != scriptTag || fLangSysTag != languageTag) {
		fScriptTag = scriptTag;
		fLangSysTag = languageTag;
	
		// reset the GPOS if the tags changed
		fGPOSTable = NULL;
		const GlyphPositioningTableHeader *gposTable = (const GlyphPositioningTableHeader *) getFontTable(gposTableTag);
		if (gposTable != NULL && gposTable->coversScriptAndLanguage(fScriptTag, fLangSysTag)) {
			fGPOSTable = gposTable;
		}
	}

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

