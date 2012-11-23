/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2008 by SIL International
 copyright (c) 2009-2012 by Jonathan Kew
 copyright (c) 2012 by Khaled Hosny

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

#include "unicode/platform.h"	// We need this first

#include "XeTeXLayoutInterface.h"

#include "XeTeXFontInst.h"
#ifdef XETEX_MAC
#include "XeTeXFontInst_Mac.h"
#endif
#include "XeTeXFontInst_FT2.h"

#include "XeTeXFontMgr.h"

#include "XeTeXswap.h"

#ifdef XETEX_GRAPHITE
#include "XeTeXGrLayout.h"
#endif

#include "unicode/ubidi.h"
#include "unicode/utext.h"

struct XeTeXLayoutEngine_rec
	/* this is used for both ICU and Graphite, because so much of the font stuff is common;
	   however, it is not possible to call ICU-specific things like layoutChars for a
	   Graphite-based engine, or Graphite functions for an ICU one! */
{
	XeTeXFontInst*	font;
	PlatformFontRef	fontRef;
	hb_script_t		scriptTag;
	hb_language_t	languageTag;
	hb_feature_t*	features;
	int				nFeatures;
	UInt32			rgbValue;
	float			extend;
	float			slant;
	float			embolden;
	hb_buffer_t*	hbBuffer;
#ifdef XETEX_GRAPHITE
	gr::Segment*		grSegment;
	XeTeXGrFont*		grFont;
	XeTeXGrTextSource*	grSource;
#endif
};

/*******************************************************************/
/* Glyph bounding box cache to speed up \XeTeXuseglyphmetrics mode */
/*******************************************************************/
#include <map>

// key is combined value representing (font_id << 16) + glyph
// value is glyph bounding box in TeX points
static std::map<UInt32,GlyphBBox>	sGlyphBoxes;

int
getCachedGlyphBBox(UInt16 fontID, UInt16 glyphID, GlyphBBox* bbox)
{
	UInt32	key = ((UInt32)fontID << 16) + glyphID;
	std::map<UInt32,GlyphBBox>::const_iterator i = sGlyphBoxes.find(key);
	if (i == sGlyphBoxes.end()) {
		return 0;
	}
	*bbox = i->second;
	return 1;
}

void
cacheGlyphBBox(UInt16 fontID, UInt16 glyphID, const GlyphBBox* bbox)
{
	UInt32	key = ((UInt32)fontID << 16) + glyphID;
	sGlyphBoxes[key] = *bbox;
}
/*******************************************************************/

void terminatefontmanager()
{
	XeTeXFontMgr::Terminate();
}

XeTeXFont createFont(PlatformFontRef fontRef, Fixed pointSize)
{
	LEErrorCode status = LE_NO_ERROR;
#ifdef XETEX_MAC
	XeTeXFontInst* font = new XeTeXFontInst_Mac((ATSFontRef)fontRef, Fix2X(pointSize), status);
#else
	FcChar8*	pathname = 0;
	FcPatternGetString(fontRef, FC_FILE, 0, &pathname);
	int			index;
	FcPatternGetInteger(fontRef, FC_INDEX, 0, &index);
	XeTeXFontInst* font = new XeTeXFontInst_FT2((const char*)pathname, index, Fix2X(pointSize), status);
#endif
	if (LE_FAILURE(status)) {
		delete font;
		return NULL;
	}
	return (XeTeXFont)font;
}

XeTeXFont createFontFromFile(const char* filename, int index, Fixed pointSize)
{
	LEErrorCode status = LE_NO_ERROR;
	XeTeXFontInst* font = new XeTeXFontInst_FT2(filename, index, Fix2X(pointSize), status);
	if (LE_FAILURE(status)) {
		delete font;
		return NULL;
	}
	return (XeTeXFont)font;
}

void setFontLayoutDir(XeTeXFont font, int vertical)
{
	((XeTeXFontInst*)font)->setLayoutDirVertical(vertical != 0);
}

PlatformFontRef findFontByName(const char* name, char* var, double size)
{
	return (XeTeXFontMgr::GetFontManager()->findFont(name, var, size));
}

char getReqEngine()
{
	return XeTeXFontMgr::GetFontManager()->getReqEngine();
}

const char* getFullName(PlatformFontRef fontRef)
{
	return XeTeXFontMgr::GetFontManager()->getFullName(fontRef);
}

double getDesignSize(XeTeXFont font)
{
	return XeTeXFontMgr::GetFontManager()->getDesignSize(font);
}

const char* getFontFilename(XeTeXLayoutEngine engine)
{
	return engine->font->getFilename();
}

void getNames(PlatformFontRef fontRef, const char** psName, const char** famName, const char** styName)
{
	XeTeXFontMgr::GetFontManager()->getNames(fontRef, psName, famName, styName);
}

PlatformFontRef getFontRef(XeTeXLayoutEngine engine)
{
	return engine->fontRef;
}

void deleteFont(XeTeXFont font)
{
	delete (XeTeXFontInst*)font;
}

void* getFontTablePtr(XeTeXFont font, UInt32 tableTag)
{
	return const_cast<void*>(((XeTeXFontInst*)font)->getFontTable(tableTag));
}

Fixed getSlant(XeTeXFont font)
{
	float italAngle = ((XeTeXFontInst*)font)->getItalicAngle();
	return X2Fix(tan(-italAngle * M_PI / 180.0));
}

static UInt32
getLargerScriptListTable(XeTeXFont font, hb_tag_t** scriptList, hb_tag_t* tableTag)
{
	hb_face_t* face = hb_font_get_face(((XeTeXFontInst*)font)->hbFont);

	hb_tag_t* scriptListSub = NULL;
	hb_tag_t* scriptListPos = NULL;

	UInt32 scriptCountSub = hb_ot_layout_table_get_script_tags(face, HB_OT_TAG_GSUB, 0, NULL, NULL);
	scriptListSub = (hb_tag_t*) xmalloc(scriptCountSub * sizeof(hb_tag_t*));
	hb_ot_layout_table_get_script_tags(face, HB_OT_TAG_GSUB, 0, &scriptCountSub, scriptListSub);

	UInt32 scriptCountPos = hb_ot_layout_table_get_script_tags(face, HB_OT_TAG_GPOS, 0, NULL, NULL);
	scriptListPos = (hb_tag_t*) xmalloc(scriptCountPos * sizeof(hb_tag_t*));
	hb_ot_layout_table_get_script_tags(face, HB_OT_TAG_GSUB, 0, &scriptCountPos, scriptListPos);

	if (scriptCountSub > scriptCountPos) {
		if (scriptList != NULL)
			*scriptList = scriptListSub;
		if (tableTag != NULL)
			*tableTag = HB_OT_TAG_GSUB;
		return scriptCountSub;
	} else {
		if (scriptList != NULL)
			*scriptList = scriptListPos;
		if (tableTag != NULL)
			*tableTag = HB_OT_TAG_GPOS;
		return scriptCountPos;
	}
}

UInt32 countScripts(XeTeXFont font)
{
	return getLargerScriptListTable(font, NULL, NULL);
}

UInt32 getIndScript(XeTeXFont font, UInt32 index)
{
	hb_tag_t* scriptList;

	UInt32 scriptCount = getLargerScriptListTable(font, &scriptList, NULL);
	if (scriptList == NULL)
		return 0;

	if (index < scriptCount)
		return scriptList[index];

	return 0;
}

UInt32 countScriptLanguages(XeTeXFont font, UInt32 script)
{
	hb_face_t* face = hb_font_get_face(((XeTeXFontInst*)font)->hbFont);
	hb_tag_t* scriptList;
	hb_tag_t tableTag;

	UInt32 scriptCount = getLargerScriptListTable(font, &scriptList, &tableTag);
	if (scriptList == NULL)
		return 0;

	for (int i = 0; i < scriptCount; i++) {
		if (scriptList[i] == script) {
			return hb_ot_layout_script_get_language_tags (face, tableTag, i, 0, NULL, NULL);
		}
	}
}

UInt32 getIndScriptLanguage(XeTeXFont font, UInt32 script, UInt32 index)
{
	hb_face_t* face = hb_font_get_face(((XeTeXFontInst*)font)->hbFont);
	hb_tag_t* scriptList;
	hb_tag_t tableTag;

	UInt32 scriptCount = getLargerScriptListTable(font, &scriptList, &tableTag);
	if (scriptList == NULL)
		return 0;

	for (int i = 0; i < scriptCount; i++) {
		if (scriptList[i] == script) {
			UInt32 langCount = hb_ot_layout_script_get_language_tags(face, tableTag, i, 0, NULL, NULL);
			hb_tag_t* langList = (hb_tag_t*) xmalloc(langCount * sizeof(hb_tag_t*));
			hb_ot_layout_script_get_language_tags(face, tableTag, i, 0, &langCount, langList);

			if (index < langCount)
				return langList[index];

			return 0;
		}
	}
}

UInt32 countFeatures(XeTeXFont font, UInt32 script, UInt32 language)
{
	hb_face_t* face = hb_font_get_face(((XeTeXFontInst*)font)->hbFont);
	UInt32 total = 0;

	for (int i = 0; i < 2; ++i) {
		UInt32 scriptIndex, langIndex = 0;
		hb_tag_t tableTag = i == 0 ? HB_OT_TAG_GSUB : HB_OT_TAG_GPOS;
		if (hb_ot_layout_table_find_script(face, tableTag, script, &scriptIndex)) {
			if (hb_ot_layout_script_find_language(face, tableTag, scriptIndex, language, &langIndex) || language == 0) {
				total += hb_ot_layout_language_get_feature_tags(face, tableTag, scriptIndex, langIndex, 0, NULL, NULL);
			}
		}
	}

	return total;
}

UInt32 getIndFeature(XeTeXFont font, UInt32 script, UInt32 language, UInt32 index)
{
	hb_face_t* face = hb_font_get_face(((XeTeXFontInst*)font)->hbFont);

	for (int i = 0; i < 2; ++i) {
		UInt32 scriptIndex, langIndex = 0;
		hb_tag_t tableTag = i == 0 ? HB_OT_TAG_GSUB : HB_OT_TAG_GPOS;
		if (hb_ot_layout_table_find_script(face, tableTag, script, &scriptIndex)) {
			if (hb_ot_layout_script_find_language(face, tableTag, scriptIndex, language, &langIndex) || language == 0) {
				UInt32 featCount = hb_ot_layout_language_get_feature_tags(face, tableTag, scriptIndex, langIndex, 0, NULL, NULL);
				hb_tag_t* featList = (hb_tag_t*) xmalloc(featCount * sizeof(hb_tag_t*));
				hb_ot_layout_language_get_feature_tags(face, tableTag, scriptIndex, langIndex, 0, &featCount, featList);

				if (index < featCount)
					return featList[index];

				index -= featCount;
			}
		}
	}

	return 0;
}

#ifdef XETEX_GRAPHITE
UInt32 countGraphiteFeatures(XeTeXLayoutEngine engine)
{
	std::pair<gr::FeatureIterator, gr::FeatureIterator>	fi = engine->grFont->getFeatures();
	UInt32	rval = 0;
	while (fi.first != fi.second) {
		++rval;
		++fi.first;
	}
	return rval;
}

UInt32 getGraphiteFeatureCode(XeTeXLayoutEngine engine, UInt32 index)
{
	std::pair<gr::FeatureIterator, gr::FeatureIterator>	fi = engine->grFont->getFeatures();
	while (index > 0 && fi.first != fi.second) {
		--index;
		++fi.first;
	}
	if (fi.first != fi.second)
		return *fi.first;
	return 0;
}

UInt32 countGraphiteFeatureSettings(XeTeXLayoutEngine engine, UInt32 feature)
{
	std::pair<gr::FeatureIterator, gr::FeatureIterator>	fi = engine->grFont->getFeatures();
	while (fi.first != fi.second && *fi.first != feature)
		++fi.first;
	if (fi.first != fi.second) {
		std::pair<gr::FeatureSettingIterator, gr::FeatureSettingIterator>
				si = engine->grFont->getFeatureSettings(fi.first);
		UInt32	rval = 0;
		while (si.first != si.second) {
			++rval;
			++si.first;
		}
		return rval;
	}
	return 0;
}

UInt32 getGraphiteFeatureSettingCode(XeTeXLayoutEngine engine, UInt32 feature, UInt32 index)
{
	std::pair<gr::FeatureIterator, gr::FeatureIterator>	fi = engine->grFont->getFeatures();
	while (fi.first != fi.second && *fi.first != feature)
		++fi.first;
	if (fi.first != fi.second) {
		std::pair<gr::FeatureSettingIterator, gr::FeatureSettingIterator>
				si = engine->grFont->getFeatureSettings(fi.first);
		while (index > 0 && si.first != si.second) {
			--index;
			++si.first;
		}
		if (si.first != si.second)
			return *si.first;
	}
	return 0;
}

UInt32 getGraphiteFeatureDefaultSetting(XeTeXLayoutEngine engine, UInt32 feature)
{
	std::pair<gr::FeatureIterator, gr::FeatureIterator>	fi = engine->grFont->getFeatures();
	while (fi.first != fi.second && *fi.first != feature)
		++fi.first;
	if (fi.first != fi.second) {
		gr::FeatureSettingIterator	si = engine->grFont->getDefaultFeatureValue(fi.first);
		return *si;
	}
	return 0;
}

void getGraphiteFeatureLabel(XeTeXLayoutEngine engine, UInt32 feature, unsigned short* buf)
{
	buf[0] = 0;
	std::pair<gr::FeatureIterator, gr::FeatureIterator>	fi = engine->grFont->getFeatures();
	while (fi.first != fi.second && *fi.first != feature)
		++fi.first;
	if (fi.first != fi.second) {
		engine->grFont->getFeatureLabel(fi.first, 0x0409, buf);
	}
}

void getGraphiteFeatureSettingLabel(XeTeXLayoutEngine engine, UInt32 feature, UInt32 setting, unsigned short* buf)
{
	buf[0] = 0;
	std::pair<gr::FeatureIterator, gr::FeatureIterator>	fi = engine->grFont->getFeatures();
	while (fi.first != fi.second && *fi.first != feature)
		++fi.first;
	if (fi.first != fi.second) {
		std::pair<gr::FeatureSettingIterator, gr::FeatureSettingIterator>
				si = engine->grFont->getFeatureSettings(fi.first);
		while (si.first != si.second && *si.first != setting)
			++si.first;
		if (si.first != si.second) {
			engine->grFont->getFeatureSettingLabel(si.first, 0x0409, buf);
		}
	}
}
#endif /* XETEX_GRAPHITE */

long findGraphiteFeatureNamed(XeTeXLayoutEngine engine, const char* name, int namelength)
{
	long		rval = -1;

#ifdef XETEX_GRAPHITE
	UErrorCode	status = (UErrorCode)0;

	std::pair<gr::FeatureIterator,gr::FeatureIterator>	features = engine->grFont->getFeatures();
	while (features.first != features.second) {
		gr::utf16	label[128];
		if (engine->grFont->getFeatureLabel(features.first, 0x409, &label[0])) {
			UText* ut1 = utext_openUTF8(NULL, name, namelength, &status);
			UText* ut2 = utext_openUChars(NULL, (const UChar*)&label[0], -1, &status);
			UChar32	ch1 = 0, ch2 = 0;
			while (ch1 != U_SENTINEL) {
				ch1 = utext_next32(ut1);
				ch2 = utext_next32(ut2);
				if (ch1 != ch2)
					break;
			}
			ut1 = utext_close(ut1);
			ut2 = utext_close(ut2);
			if (ch1 == ch2) {
				rval = *features.first;
				break;
			}
		}
		++features.first;
	}
#endif

	return rval;
}

long findGraphiteFeatureSettingNamed(XeTeXLayoutEngine engine, UInt32 feature, const char* name, int namelength)
{
	long		rval = -1;

#ifdef XETEX_GRAPHITE
	UErrorCode	status = (UErrorCode)0;

	std::pair<gr::FeatureIterator,gr::FeatureIterator>	features = engine->grFont->getFeatures();
	while (features.first != features.second) {
		if (*features.first == feature) {
			std::pair<gr::FeatureSettingIterator,gr::FeatureSettingIterator>
					settings = engine->grFont->getFeatureSettings(features.first);
			while (settings.first != settings.second) {
				gr::utf16	label[128];
				if (engine->grFont->getFeatureSettingLabel(settings.first, 0x409, &label[0])) {
					UText* ut1 = utext_openUTF8(NULL, name, namelength, &status);
					UText* ut2 = utext_openUChars(NULL, (const UChar*)&label[0], -1, &status);
					UChar32	ch1 = 0, ch2 = 0;
					while (ch1 != U_SENTINEL) {
						ch1 = utext_next32(ut1);
						ch2 = utext_next32(ut2);
						if (ch1 != ch2)
							break;
					}
					ut1 = utext_close(ut1);
					ut2 = utext_close(ut2);
					if (ch1 == ch2) {
						rval = *settings.first;
						break;
					}
				}
				++settings.first;
			}
			break;
		}
		++features.first;
	}
#endif

	return rval;
}

float getGlyphWidth(XeTeXFont font, UInt32 gid)
{
	LEPoint	adv;
	((XeTeXFontInst*)font)->getGlyphAdvance(gid, adv);
	return adv.fX;
}

UInt32
countGlyphs(XeTeXFont font)
{
	return ((XeTeXFontInst*)font)->getNumGlyphs();
}

XeTeXFont getFont(XeTeXLayoutEngine engine)
{
	return (XeTeXFont)(engine->font);
}

float getExtendFactor(XeTeXLayoutEngine engine)
{
	return engine->extend;
}

float getSlantFactor(XeTeXLayoutEngine engine)
{
	return engine->slant;
}

float getEmboldenFactor(XeTeXLayoutEngine engine)
{
	return engine->embolden;
}

XeTeXLayoutEngine createLayoutEngine(PlatformFontRef fontRef, XeTeXFont font, hb_script_t scriptTag, hb_language_t languageTag,
										hb_feature_t* features, int nFeatures, UInt32 rgbValue,
										float extend, float slant, float embolden)
{
	XeTeXLayoutEngine result = new XeTeXLayoutEngine_rec;
	result->fontRef = fontRef;
	result->font = (XeTeXFontInst*)font;
	result->scriptTag = scriptTag;
	result->languageTag = languageTag;
	result->features = features;
	result->nFeatures = nFeatures;
	result->rgbValue = rgbValue;
	result->extend = extend;
	result->slant = slant;
	result->embolden = embolden;

#ifdef XETEX_GRAPHITE
	result->grSegment = NULL;
	result->grSource = NULL;
	result->grFont = NULL;
#endif

	result->hbBuffer = hb_buffer_create();

	return result;
}

void deleteLayoutEngine(XeTeXLayoutEngine engine)
{
	hb_buffer_destroy(engine->hbBuffer);
	delete engine->font;
#ifdef XETEX_GRAPHITE
	delete engine->grSegment;
	delete engine->grSource;
	delete engine->grFont;
#endif
}

int layoutChars(XeTeXLayoutEngine engine, UInt16 chars[], SInt32 offset, SInt32 count, SInt32 max,
						bool rightToLeft)
{
	hb_buffer_reset(engine->hbBuffer);
	hb_buffer_add_utf16(engine->hbBuffer, chars, max, offset, count);
	hb_buffer_set_direction(engine->hbBuffer, rightToLeft ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
	hb_buffer_set_script(engine->hbBuffer, engine->scriptTag);
	hb_buffer_set_language(engine->hbBuffer, engine->languageTag);

	hb_shape(engine->font->hbFont, engine->hbBuffer, engine->features, engine->nFeatures);
	int glyphCount = hb_buffer_get_length(engine->hbBuffer);

	return glyphCount;
}

void getGlyphs(XeTeXLayoutEngine engine, UInt32 glyphs[])
{
	int glyphCount = hb_buffer_get_length(engine->hbBuffer);
	hb_glyph_info_t *hbGlyphs = hb_buffer_get_glyph_infos(engine->hbBuffer, NULL);

	for (int i = 0; i < glyphCount; i++)
		glyphs[i] = hbGlyphs[i].codepoint;
}

void getGlyphPositions(XeTeXLayoutEngine engine, float positions[])
{
	int glyphCount = hb_buffer_get_length(engine->hbBuffer);
	hb_glyph_position_t *hbPositions = hb_buffer_get_glyph_positions(engine->hbBuffer, NULL);

	int i = 0;
   	float x = 0, y = 0;

	for (i = 0; i < glyphCount; i++) {
		positions[2*i]   =   x + hbPositions[i].x_offset / 64.0;
		positions[2*i+1] = -(y + hbPositions[i].y_offset / 64.0); /* negative is upwards */
		x += hbPositions[i].x_advance / 64.0;
		y += hbPositions[i].y_advance / 64.0;
	}
	positions[2*i]   = x;
	positions[2*i+1] = y;

	if (engine->extend != 1.0 || engine->slant != 0.0)
		for (int i = 0; i <= glyphCount; ++i)
			positions[2*i] = positions[2*i] * engine->extend - positions[2*i+1] * engine->slant;
}

float getPointSize(XeTeXLayoutEngine engine)
{
	return engine->font->getXPixelsPerEm();
}

void getAscentAndDescent(XeTeXLayoutEngine engine, float* ascent, float* descent)
{
	*ascent = engine->font->getExactAscent();
	*descent = engine->font->getExactDescent();
}

int getDefaultDirection(XeTeXLayoutEngine engine)
{
	if (hb_script_get_horizontal_direction (engine->scriptTag) == HB_DIRECTION_RTL)
		return UBIDI_DEFAULT_RTL;
	else
		return UBIDI_DEFAULT_LTR;
}

UInt32 getRgbValue(XeTeXLayoutEngine engine)
{
	return engine->rgbValue;
}

void getGlyphBounds(XeTeXLayoutEngine engine, UInt32 glyphID, GlyphBBox* bbox)
{
	engine->font->getGlyphBounds(glyphID, bbox);
	if (engine->extend != 0.0) {
	    bbox->xMin *= engine->extend;
	    bbox->xMax *= engine->extend;
    }
}

float getGlyphWidthFromEngine(XeTeXLayoutEngine engine, UInt32 glyphID)
{
	return engine->extend * engine->font->getGlyphWidth(glyphID);
}

void getGlyphHeightDepth(XeTeXLayoutEngine engine, UInt32 glyphID, float* height, float* depth)
{
	engine->font->getGlyphHeightDepth(glyphID, height, depth);
}

void getGlyphSidebearings(XeTeXLayoutEngine engine, UInt32 glyphID, float* lsb, float* rsb)
{
	engine->font->getGlyphSidebearings(glyphID, lsb, rsb);
	if (engine->extend != 0.0) {
	    *lsb *= engine->extend;
	    *rsb *= engine->extend;
	}
}

float getGlyphItalCorr(XeTeXLayoutEngine engine, UInt32 glyphID)
{
	return engine->extend * engine->font->getGlyphItalCorr(glyphID);
}

UInt32 mapCharToGlyph(XeTeXLayoutEngine engine, UInt32 charCode)
{
	return engine->font->mapCharToGlyph(charCode);
}

#include "appleGlyphNames.c"

int
findGlyphInPostTable(const char* buffer, int tableSize, const char* glyphName)
{
	const postTable* p = (const postTable*)buffer;
	UInt16	g = 0;
	switch (SWAP(p->format)) {
		case 0x00010000:
			{
				const char*	cp;
				while ((cp = appleGlyphNames[g]) != 0) {
					if (strcmp(glyphName, cp) == 0)
						return g;
					++g;
				}
			}
			break;
		
		case 0x00020000:
			{
				const UInt16*	n = (UInt16*)(p + 1);
				UInt16	numGlyphs = SWAP(*n++);
				const UInt8*	ps = (const UInt8*)(n + numGlyphs);
				std::vector<std::string>	newNames;
				while (ps < (const UInt8*)buffer + tableSize) {
					newNames.push_back(std::string((char*)ps + 1, *ps));
					ps += *ps + 1;
				}
				for (g = 0; g < numGlyphs; ++g) {
					if (SWAP(*n) < 258) {
						if (strcmp(appleGlyphNames[SWAP(*n)], glyphName) == 0)
							return g;
					}
					else {
						if (strcmp(newNames[SWAP(*n) - 258].c_str(), glyphName) == 0)
							return g;
					}
					++n;
				}
			}
			break;
		
		case 0x00028000:
			break;
		
		case 0x00030000:
			// TODO: see if it's a CFF OpenType font, and if so, get the glyph names from the CFF data
			break;
		
		case 0x00040000:
			break;
		
		default:
			break;
	}

	return 0;
}

const char*
getGlyphNamePtr(const char* buffer, int tableSize, UInt16 gid, int* len)
{
	const postTable* p = (const postTable*)buffer;
	switch (SWAP(p->format)) {
		case 0x00010000:
			{
				if (gid < 258) {
					*len = strlen(appleGlyphNames[gid]);
					return appleGlyphNames[gid];
				}
			}
			break;
		
		case 0x00020000:
			{
				const UInt16*	n = (UInt16*)(p + 1);
				UInt16	numGlyphs = SWAP(*n++);
				const UInt8*	ps = (const UInt8*)(n + numGlyphs);
				std::vector<const UInt8*>	namePtrs;
				while (ps < (const UInt8*)buffer + tableSize) {
					namePtrs.push_back(ps);
					ps += *ps + 1;
				}
				if (gid < numGlyphs) {
					gid = SWAP(n[gid]);
					if (gid < 258) {
						*len = strlen(appleGlyphNames[gid]);
						return appleGlyphNames[gid];
					}
					else {
						ps = namePtrs[gid - 258];
						*len = *ps;
						return (char*)(ps + 1);
					}
				}
			}
			break;
		
		case 0x00028000:
			break;
		
		case 0x00030000:
			// TODO: see if it's a CFF OpenType font, and if so, get the glyph names from the CFF data
			break;
		
		case 0x00040000:
			break;
		
		default:
			break;
	}

	/* no name found */
	*len = 0;
	return NULL;
}

int
getFontCharRange(XeTeXLayoutEngine engine, int reqFirst)
{
	if (reqFirst)
		return engine->font->getFirstCharCode();
	else
		return engine->font->getLastCharCode();
}

const char*
getGlyphName(XeTeXFont font, UInt16 gid, int* len)
{
	return ((XeTeXFontInst*)font)->getGlyphName(gid, *len);
}

int
mapGlyphToIndex(XeTeXLayoutEngine engine, const char* glyphName)
{
	return engine->font->mapGlyphToIndex(glyphName);
}

#ifdef XETEX_GRAPHITE

/* Graphite interface */

gr::LayoutEnvironment	layoutEnv;

XeTeXLayoutEngine createGraphiteEngine(PlatformFontRef fontRef, XeTeXFont font,
										const char* name,
										UInt32 rgbValue, int rtl, UInt32 languageTag,
										float extend, float slant, float embolden,
										int nFeatures, const int* featureIDs, const int* featureValues)
{
	// check if the font supports graphite, and return NULL if not
	const UInt32	kttiSilf = 0x53696C66;	// from GrConstants.h
	if (getFontTablePtr(font, kttiSilf) == NULL)
		return NULL;

	XeTeXLayoutEngine result = new XeTeXLayoutEngine_rec;
	result->fontRef = fontRef;
	result->font = (XeTeXFontInst*)font;
	result->scriptTag = HB_SCRIPT_INVALID;
	result->languageTag = HB_LANGUAGE_INVALID;
	result->features = NULL;
	result->nFeatures = 0;
	result->rgbValue = rgbValue;
	result->extend = extend;
	result->slant = slant;
	result->embolden = embolden;
	result->hbBuffer = NULL;

	result->grFont = new XeTeXGrFont(result->font, name);

	result->grSource = new XeTeXGrTextSource(rtl);
	result->grSource->setFeatures(nFeatures, featureIDs, featureValues);
	if (languageTag != 0)
		result->grSource->setLanguage(languageTag);

	result->grSegment = NULL;
	
	layoutEnv.setDumbFallback(true);

	return result;
}

int
makeGraphiteSegment(XeTeXLayoutEngine engine, const UniChar* txtPtr, int txtLen)
{
	if (engine->grSegment) {
		delete engine->grSegment;
		engine->grSegment = NULL;
	}

	engine->grSource->setText(txtPtr, txtLen);
	try {
		engine->grSegment = new gr::RangeSegment(engine->grFont, engine->grSource, &layoutEnv);
	}
	catch (gr::FontException f) {
		fprintf(stderr, "*** makeGraphiteSegment: font error %d, returning 0\n", (int)f.errorCode);
		return 0;
	}
	catch (...) {
		fprintf(stderr, "*** makeGraphiteSegment: segment creation failed, returning 0\n");
		return 0;
	}

	return engine->grSegment->glyphs().second - engine->grSegment->glyphs().first;
}

void
getGraphiteGlyphInfo(XeTeXLayoutEngine engine, int index, UInt16* glyphID, float* x, float* y)
{
	if (engine->grSegment == NULL) {
		*glyphID = 0;
		*x = *y = 0.0;
		return;
	}
	gr::GlyphIterator	i = engine->grSegment->glyphs().first + index;
	*glyphID = i->glyphID();
	if (engine->extend != 1.0 || engine->slant != 0.0)
		*x = i->origin() * engine->extend + i->yOffset() * engine->slant;
	else
		*x = i->origin();
	*y = - i->yOffset();
}

float
graphiteSegmentWidth(XeTeXLayoutEngine engine)
{
	if (engine->grSegment == NULL)
		return 0.0;
	//return engine->grSegment->advanceWidth(); // can't use this because it ignores trailing WS
	try {
		return engine->extend * engine->grSegment->getRangeWidth(0, engine->grSource->getLength(), false, false, false);
	}
	catch (gr::FontException f) {
		fprintf(stderr, "*** graphiteSegmentWidth: font error %d, returning 0.0\n", (int)f.errorCode);
		return 0.0;
	}
	catch (...) {
		fprintf(stderr, "*** graphiteSegmentWidth: getRangeWidth failed, returning 0.0\n");
		return 0.0;
	}
}

/* line-breaking uses its own private textsource and segment, not the engine's ones */
static XeTeXGrTextSource*	lbSource = NULL;
static gr::Segment*			lbSegment = NULL;
static XeTeXLayoutEngine	lbEngine = NULL;

#endif /* XETEX_GRAPHITE */

void
initGraphiteBreaking(XeTeXLayoutEngine engine, const UniChar* txtPtr, int txtLen)
{
#ifdef XETEX_GRAPHITE
	if (lbSource == NULL)
		lbSource = new XeTeXGrTextSource(0);

	if (lbSegment != NULL) {
		delete lbSegment;
		lbSegment = NULL;
	}

	lbSource->setText(txtPtr, txtLen);
	
	if (lbEngine != engine) {
		gr::FeatureSetting	features[64];
		size_t	nFeatures = engine->grSource->getFontFeatures(0, features);
		lbSource->setFeatures(nFeatures, &features[0]);
		lbEngine = engine;
	}
	
	try {
		lbSegment = new gr::RangeSegment(engine->grFont, lbSource, &layoutEnv);
	}
	catch (gr::FontException f) {
		fprintf(stderr, "*** initGraphiteBreaking: font error %d\n", (int)f.errorCode);
	}
	catch (...) {
		fprintf(stderr, "*** initGraphiteBreaking: segment creation failed\n");
	}
#endif
}

int
findNextGraphiteBreak(int iOffset, int iBrkVal)
{
#ifdef XETEX_GRAPHITE
	if (lbSegment == NULL)
		return -1;
	if (iOffset < lbSource->getLength()) {
		while (++iOffset < lbSource->getLength()) {
			const std::pair<gr::GlyphSetIterator,gr::GlyphSetIterator> gsiPair = lbSegment->charToGlyphs(iOffset);
			const gr::GlyphSetIterator&	gsi = gsiPair.first;
			if (gsi == gsiPair.second)
				continue;
			if (gsi->breakweight() < gr::klbNoBreak && gsi->breakweight() >= -(gr::LineBrk)iBrkVal)
				return iOffset;
			if (gsi->breakweight() > gr::klbNoBreak && gsi->breakweight() <= (gr::LineBrk)iBrkVal)
				return iOffset + 1;
		}
		return lbSource->getLength();
	}
	else
#endif
		return -1;
}

int usingGraphite(XeTeXLayoutEngine engine)
{
#ifdef XETEX_GRAPHITE
	return engine->grFont != NULL;
#else
	return 0;
#endif
}

int usingOpenType(XeTeXLayoutEngine engine)
{
	return engine->font->hbFont != NULL;
}

#define kMATHTableTag	0x4D415448 /* 'MATH' */

int isOpenTypeMathFont(XeTeXLayoutEngine engine)
{
	if (engine->font->hbFont != NULL) {
		if (engine->font->getFontTable(kMATHTableTag) != NULL)
			return 1;
	}
	return 0;
}

int
findGraphiteFeature(XeTeXLayoutEngine engine, const char* s, const char* e, int* f, int* v)
	/* s...e is a "feature=setting" string; look for this in the font */
{
	while (*s == ' ' || *s == '\t')
		++s;
	const char* cp = s;
	while (cp < e && *cp != '=')
		++cp;
	if (cp == e)
		return 0;

	*f = findGraphiteFeatureNamed(engine, s, cp - s);
	if (*f == -1)
		return 0;

	++cp;
	while (cp < e && (*cp == ' ' || *cp == '\t'))
		++cp;
	if (cp == e)
		return 0;

	*v = findGraphiteFeatureSettingNamed(engine, *f, cp, e - cp);
	if (*v == -1)
		return 0;
	
	return 1;
}

