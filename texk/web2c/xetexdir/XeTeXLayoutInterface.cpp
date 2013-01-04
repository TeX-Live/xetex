/****************************************************************************\
 Part of the XeTeX typesetting system
 Copyright (c) 1994-2008 by SIL International
 Copyright (c) 2009-2012 by Jonathan Kew
 Copyright (c) 2012, 2013 by Khaled Hosny

 SIL Author(s): Jonathan Kew

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

#include <unicode/platform.h>	// We need this first
#include <unicode/ubidi.h>
#include <unicode/utext.h>

#include "XeTeXLayoutInterface.h"
#include "XeTeXFontInst.h"
#ifdef XETEX_MAC
#include "XeTeXFontInst_Mac.h"
#endif
#include "XeTeXFontInst_FT2.h"
#include "XeTeXFontMgr.h"
#include "XeTeXswap.h"

#ifdef _MSC_VER
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#endif

struct XeTeXLayoutEngine_rec
{
	XeTeXFontInst*	font;
	PlatformFontRef	fontRef;
	char*			script;
	char*			language;
	hb_feature_t*	features;
	char**			ShaperList;	// the requested shapers
	char*			shaper;		// the actually used shaper
	int				nFeatures;
	UInt32			rgbValue;
	float			extend;
	float			slant;
	float			embolden;
	hb_buffer_t*	hbBuffer;
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
	int status = 0;
#ifdef XETEX_MAC
	XeTeXFontInst* font = new XeTeXFontInst_Mac((ATSFontRef)fontRef, Fix2D(pointSize), status);
#else
	FcChar8*	pathname = 0;
	FcPatternGetString(fontRef, FC_FILE, 0, &pathname);
	int			index;
	FcPatternGetInteger(fontRef, FC_INDEX, 0, &index);
	XeTeXFontInst* font = new XeTeXFontInst_FT2((const char*)pathname, index, Fix2D(pointSize), status);
#endif
	if (status != 0) {
		delete font;
		return NULL;
	}
	return (XeTeXFont)font;
}

XeTeXFont createFontFromFile(const char* filename, int index, Fixed pointSize)
{
	int status = 0;
	XeTeXFontInst* font = new XeTeXFontInst_FT2(filename, index, Fix2D(pointSize), status);
	if (status != 0) {
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

void setReqEngine(char reqEngine)
{
	XeTeXFontMgr::GetFontManager()->setReqEngine(reqEngine);
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
	return D2Fix(tan(-italAngle * M_PI / 180.0));
}

static UInt32
getLargerScriptListTable(XeTeXFont font, hb_tag_t** scriptList, hb_tag_t* tableTag)
{
	hb_face_t* face = hb_font_get_face(((XeTeXFontInst*)font)->hbFont);

	hb_tag_t* scriptListSub = NULL;
	hb_tag_t* scriptListPos = NULL;

	uint32_t scriptCountSub = hb_ot_layout_table_get_script_tags(face, HB_OT_TAG_GSUB, 0, NULL, NULL);
	scriptListSub = (hb_tag_t*) xmalloc(scriptCountSub * sizeof(hb_tag_t*));
	hb_ot_layout_table_get_script_tags(face, HB_OT_TAG_GSUB, 0, &scriptCountSub, scriptListSub);

	uint32_t scriptCountPos = hb_ot_layout_table_get_script_tags(face, HB_OT_TAG_GPOS, 0, NULL, NULL);
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
	return 0;
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
			uint32_t langCount = hb_ot_layout_script_get_language_tags(face, tableTag, i, 0, NULL, NULL);
			hb_tag_t* langList = (hb_tag_t*) xmalloc(langCount * sizeof(hb_tag_t*));
			hb_ot_layout_script_get_language_tags(face, tableTag, i, 0, &langCount, langList);

			if (index < langCount)
				return langList[index];

			return 0;
		}
	}
	return 0;
}

UInt32 countFeatures(XeTeXFont font, UInt32 script, UInt32 language)
{
	hb_face_t* face = hb_font_get_face(((XeTeXFontInst*)font)->hbFont);
	UInt32 total = 0;

	for (int i = 0; i < 2; ++i) {
		uint32_t scriptIndex, langIndex = 0;
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
		uint32_t scriptIndex, langIndex = 0;
		hb_tag_t tableTag = i == 0 ? HB_OT_TAG_GSUB : HB_OT_TAG_GPOS;
		if (hb_ot_layout_table_find_script(face, tableTag, script, &scriptIndex)) {
			if (hb_ot_layout_script_find_language(face, tableTag, scriptIndex, language, &langIndex) || language == 0) {
				uint32_t featCount = hb_ot_layout_language_get_feature_tags(face, tableTag, scriptIndex, langIndex, 0, NULL, NULL);
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

UInt32 countGraphiteFeatures(XeTeXLayoutEngine engine)
{
	uint32_t rval = 0;

	hb_face_t* hbFace = hb_font_get_face(engine->font->hbFont);
	gr_face* grFace = hb_graphite2_face_get_gr_face(hbFace);

	if (grFace != NULL)
		rval = gr_face_n_fref(grFace);

	return rval;
}

UInt32 getGraphiteFeatureCode(XeTeXLayoutEngine engine, UInt32 index)
{
	uint32_t rval = 0;

	hb_face_t* hbFace = hb_font_get_face(engine->font->hbFont);
	gr_face* grFace = hb_graphite2_face_get_gr_face(hbFace);

	if (grFace != NULL) {
		const gr_feature_ref* feature = gr_face_fref(grFace, index);
		rval = gr_fref_id(feature);
	}

	return rval;
}

UInt32 countGraphiteFeatureSettings(XeTeXLayoutEngine engine, UInt32 featureID)
{
	uint32_t rval = 0;

	hb_face_t* hbFace = hb_font_get_face(engine->font->hbFont);
	gr_face* grFace = hb_graphite2_face_get_gr_face(hbFace);

	if (grFace != NULL) {
		const gr_feature_ref* feature = gr_face_find_fref(grFace, featureID);
		rval = gr_fref_n_values(feature);
	}

	return rval;
}

UInt32 getGraphiteFeatureSettingCode(XeTeXLayoutEngine engine, UInt32 featureID, UInt32 index)
{
	uint32_t rval = 0;

	hb_face_t* hbFace = hb_font_get_face(engine->font->hbFont);
	gr_face* grFace = hb_graphite2_face_get_gr_face(hbFace);

	if (grFace != NULL) {
		const gr_feature_ref* feature = gr_face_find_fref(grFace, featureID);
		rval = gr_fref_value(feature, index);
	}

	return rval;
}

void getGraphiteFeatureLabel(XeTeXLayoutEngine engine, UInt32 featureID, char* buf)
{
	buf[0] = '\0';

	hb_face_t* hbFace = hb_font_get_face(engine->font->hbFont);
	gr_face* grFace = hb_graphite2_face_get_gr_face(hbFace);

	if (grFace != NULL) {
		const gr_feature_ref* feature = gr_face_find_fref(grFace, featureID);
		uint32_t len = 0;
		uint16_t langID = 0x409;

		buf = (char*) gr_fref_label(feature, &langID, gr_utf8, &len);

		if (len > 128 + 1) { // 128 is set in XeTeX_ext.c
			buf = (char*) realloc(buf, len + 1);
			buf = (char*) gr_fref_label(feature, &langID, gr_utf8, &len);
		}

		buf[len + 1] = '\0';
	}
}

void getGraphiteFeatureSettingLabel(XeTeXLayoutEngine engine, UInt32 featureID, UInt32 settingID, char* buf)
{
	buf[0] = '\0';

	hb_face_t* hbFace = hb_font_get_face(engine->font->hbFont);
	gr_face* grFace = hb_graphite2_face_get_gr_face(hbFace);

	if (grFace != NULL) {
		const gr_feature_ref* feature = gr_face_find_fref(grFace, featureID);
		for (int i = 0; i < gr_fref_n_values(feature); i++) {
			if (settingID == gr_fref_value(feature, i)) {
				uint32_t len = 0;
				uint16_t langID = 0x409;

				buf = (char*) gr_fref_value_label(feature, i, &langID, gr_utf8, &len);

				if (len > 128 + 1) { // 128 is set in XeTeX_ext.c
					buf = (char*) realloc(buf, len + 1);
					buf = (char*) gr_fref_value_label(feature, i, &langID, gr_utf8, &len);
				}

				buf[len + 1] = '\0';
				break;
			}
		}
	}
}

bool
findGraphiteFeature(XeTeXLayoutEngine engine, const char* s, const char* e, int* f, int* v)
	/* s...e is a "feature[=setting]" string; look for this in the font */
{
	*f = 0;
	*v = 0;
	while (*s == ' ' || *s == '\t')
		++s;
	const char* cp = s;
	while (cp < e && *cp != '=')
		++cp;

	*f = findGraphiteFeatureNamed(engine, s, cp - s);
	if (*f == -1)
		return false;

	++cp;
	while (cp < e && (*cp == ' ' || *cp == '\t'))
		++cp;
	if (cp >= e)
		// no setting was specified, so we just use the first
		// XXX the default is not always the first?
		return true;

	*v = findGraphiteFeatureSettingNamed(engine, *f, cp, e - cp);
	if (*v == -1)
		return false;

	return true;
}

long findGraphiteFeatureNamed(XeTeXLayoutEngine engine, const char* name, int namelength)
{
	long		rval = -1;

	hb_face_t* hbFace = hb_font_get_face(engine->font->hbFont);
	gr_face* grFace = hb_graphite2_face_get_gr_face(hbFace);

	if (grFace != NULL) {
		for (int i = 0; i < gr_face_n_fref(grFace); i++) {
			const gr_feature_ref* feature = gr_face_fref(grFace, i);
			uint32_t len = 0;
			uint16_t langID = 0x409;

			// the first call is to get the length of the string
			gr_fref_label(feature, &langID, gr_utf8, &len);
			char* label = (char*) xmalloc(len);
			label = (char*) gr_fref_label(feature, &langID, gr_utf8, &len);

			if (strncmp(label, name, namelength) == 0) {
				rval = gr_fref_id(feature);
				gr_label_destroy(label);
				break;
			}

			gr_label_destroy(label);
		}
	}

	return rval;
}

long findGraphiteFeatureSettingNamed(XeTeXLayoutEngine engine, UInt32 id, const char* name, int namelength)
{
	long		rval = -1;

	hb_face_t* hbFace = hb_font_get_face(engine->font->hbFont);
	gr_face* grFace = hb_graphite2_face_get_gr_face(hbFace);

	if (grFace != NULL) {
		const gr_feature_ref* feature = gr_face_find_fref(grFace, id);
		for (int i = 0; i < gr_fref_n_values(feature); i++) {
			uint32_t len = 0;
			uint16_t langID = 0x409;

			// the first call is to get the length of the string
			gr_fref_value_label(feature, i, &langID, gr_utf8, &len);
			char* label = (char*) xmalloc(len);
			label = (char*) gr_fref_value_label(feature, i, &langID, gr_utf8, &len);

			if (strncmp(label, name, namelength) == 0) {
				rval = gr_fref_value(feature, i);
				gr_label_destroy(label);
				break;
			}

			gr_label_destroy(label);
		}
	}

	return rval;
}

float getGlyphWidth(XeTeXFont font, UInt32 gid)
{
	return ((XeTeXFontInst*)font)->getGlyphWidth(gid);
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

XeTeXLayoutEngine createLayoutEngine(PlatformFontRef fontRef, XeTeXFont font, char* script, char* language,
										hb_feature_t* features, int nFeatures, char **shapers, UInt32 rgbValue,
										float extend, float slant, float embolden)
{
	XeTeXLayoutEngine result = new XeTeXLayoutEngine_rec;
	result->fontRef = fontRef;
	result->font = (XeTeXFontInst*)font;
	result->script = script;
	result->language = language;
	result->features = features;
	result->ShaperList = shapers;
	result->shaper = NULL;
	result->nFeatures = nFeatures;
	result->rgbValue = rgbValue;
	result->extend = extend;
	result->slant = slant;
	result->embolden = embolden;
	result->hbBuffer = hb_buffer_create();

	return result;
}

void deleteLayoutEngine(XeTeXLayoutEngine engine)
{
	hb_buffer_destroy(engine->hbBuffer);
	delete engine->font;
}

int layoutChars(XeTeXLayoutEngine engine, UInt16 chars[], SInt32 offset, SInt32 count, SInt32 max,
						bool rightToLeft)
{
	bool res;
	hb_script_t script = HB_SCRIPT_INVALID;
	hb_language_t language = HB_LANGUAGE_INVALID;
	hb_direction_t direction = HB_DIRECTION_LTR;
	hb_segment_properties_t segment_props;
	hb_shape_plan_t *shape_plan;
	hb_font_t* hbFont = engine->font->hbFont;
	hb_face_t* hbFace = hb_font_get_face(hbFont);

	if (engine->font->getLayoutDirVertical())
		direction = HB_DIRECTION_TTB;
	else if (rightToLeft)
		direction = HB_DIRECTION_RTL;

	if (engine->script != NULL)
		script = hb_script_from_string(engine->script, -1);

	if (engine->language != NULL)
		language = hb_language_from_string(engine->language, -1);

	hb_buffer_reset(engine->hbBuffer);
	hb_buffer_add_utf16(engine->hbBuffer, chars, max, offset, count);
	hb_buffer_set_direction(engine->hbBuffer, direction);
	hb_buffer_set_script(engine->hbBuffer, script);
	hb_buffer_set_language(engine->hbBuffer, language);

	hb_buffer_guess_segment_properties(engine->hbBuffer);
	hb_buffer_get_segment_properties(engine->hbBuffer, &segment_props);

	shape_plan = hb_shape_plan_create_cached(hbFace, &segment_props, engine->features, engine->nFeatures, engine->ShaperList);
	res = hb_shape_plan_execute(shape_plan, hbFont, engine->hbBuffer, engine->features, engine->nFeatures);

	if (res) {
		engine->shaper = strdup(hb_shape_plan_get_shaper(shape_plan));
		hb_buffer_set_content_type(engine->hbBuffer, HB_BUFFER_CONTENT_TYPE_GLYPHS);
	} else {
		// all selected shapers failed, retrying with default
		// we don't use _cached here as the cached plain will always fail.
		hb_shape_plan_destroy(shape_plan);
		shape_plan = hb_shape_plan_create(hbFace, &segment_props, engine->features, engine->nFeatures, NULL);
		res = hb_shape_plan_execute(shape_plan, hbFont, engine->hbBuffer, engine->features, engine->nFeatures);

		if (res) {
			engine->shaper = strdup(hb_shape_plan_get_shaper(shape_plan));
			hb_buffer_set_content_type(engine->hbBuffer, HB_BUFFER_CONTENT_TYPE_GLYPHS);
		} else {
			fprintf(stderr, "\nERROR: all shapers failed\n");
			exit(3);
		}
	}

	hb_shape_plan_destroy(shape_plan);

	int glyphCount = hb_buffer_get_length(engine->hbBuffer);

#ifdef DEBUG
	char buf[1024];
	unsigned int consumed;

	printf ("shaper: %s\n", engine->shaper);

	hb_buffer_serialize_flags_t flags = HB_BUFFER_SERIALIZE_FLAGS_DEFAULT;
	hb_buffer_serialize_format_t format = HB_BUFFER_SERIALIZE_FORMAT_TEXT;

	hb_buffer_serialize_glyphs (engine->hbBuffer, 0, glyphCount, buf, sizeof(buf), &consumed, hbFont, format, flags);
	if (consumed)
		printf ("buffer glyphs: %s\n", buf);
#endif

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

	if (engine->font->getLayoutDirVertical()) {
		x -= hbPositions[0].y_offset / 64.0; // hack to compensate offset of 1st glyph
		for (i = 0; i < glyphCount; i++) {
			positions[2*i]   = -(x + hbPositions[i].y_offset / 64.0); /* negative is forwards */
			positions[2*i+1] =  (y + hbPositions[i].x_offset / 64.0);
			x += hbPositions[i].y_advance / 64.0;
			y += hbPositions[i].x_advance / 64.0;
		}
		positions[2*i]   = -x;
		positions[2*i+1] =  y;
	} else {
		for (i = 0; i < glyphCount; i++) {
			positions[2*i]   =   x + hbPositions[i].x_offset / 64.0;
			positions[2*i+1] = -(y + hbPositions[i].y_offset / 64.0); /* negative is upwards */
			x += hbPositions[i].x_advance / 64.0;
			y += hbPositions[i].y_advance / 64.0;
		}
		positions[2*i]   =  x;
		positions[2*i+1] = -y;
	}

	if (engine->extend != 1.0 || engine->slant != 0.0)
		for (int i = 0; i <= glyphCount; ++i)
			positions[2*i] = positions[2*i] * engine->extend - positions[2*i+1] * engine->slant;
}

float getPointSize(XeTeXLayoutEngine engine)
{
	return engine->font->getPointSize();
}

void getAscentAndDescent(XeTeXLayoutEngine engine, float* ascent, float* descent)
{
	*ascent = engine->font->getAscent();
	*descent = engine->font->getDescent();
}

int getDefaultDirection(XeTeXLayoutEngine engine)
{
	hb_script_t script = hb_buffer_get_script(engine->hbBuffer);
	if (hb_script_get_horizontal_direction (script) == HB_DIRECTION_RTL)
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

static gr_segment* grSegment = NULL;
static const gr_slot* grPrevSlot = NULL;

bool
initGraphiteBreaking(XeTeXLayoutEngine engine, const UniChar* txtPtr, int txtLen)
{
	hb_face_t* hbFace = hb_font_get_face(engine->font->hbFont);
	gr_face* grFace = hb_graphite2_face_get_gr_face(hbFace);
	gr_font* grFont = hb_graphite2_font_get_gr_font(engine->font->hbFont);
	if (grFace != NULL && grFont != NULL) {
		if (grSegment != NULL) {
			gr_seg_destroy(grSegment);
			grSegment = NULL;
			grPrevSlot = NULL;
		}

		hb_tag_t script = HB_TAG_NONE, lang = HB_TAG_NONE;

		if (engine->script != NULL)
			script = hb_tag_from_string(engine->script, -1);

		if (engine->language != NULL)
			lang = hb_tag_from_string(engine->language, -1);

		gr_feature_val *grFeatures = gr_face_featureval_for_lang (grFace, lang);

		int nFeatures = engine->nFeatures;
		hb_feature_t *features =  engine->features;
		while (nFeatures--) {
			const gr_feature_ref *fref = gr_face_find_fref (grFace, features->tag);
			if (fref)
				gr_fref_set_feature_value (fref, features->value, grFeatures);
			features++;
		}

		grSegment = gr_make_seg(grFont, grFace, script, grFeatures, gr_utf16, txtPtr, txtLen, 0);
		grPrevSlot = gr_seg_first_slot(grSegment);

		return true;
	}

	return false;
}

int
findNextGraphiteBreak(void)
{
	if (grSegment == NULL)
		return -1;

	// XXX: gr_cinfo_base() returns "code unit" index not char index, so this
	// is broken outside BMP
	if (grPrevSlot && grPrevSlot != gr_seg_last_slot(grSegment)) {
		const gr_slot* s;
		const gr_char_info* ci = NULL;
		for (s = gr_slot_next_in_segment(grPrevSlot); s != NULL; s = gr_slot_next_in_segment(s)) {
			int bw;

			ci = gr_seg_cinfo(grSegment, gr_slot_index(s));
			bw = gr_cinfo_break_weight(ci);
			if (bw < gr_breakNone && bw >= gr_breakBeforeWord) {
				grPrevSlot = s;
				return gr_cinfo_base(ci);
			}

			if (bw > gr_breakNone && bw <= gr_breakWord) {
				grPrevSlot = gr_slot_next_in_segment(s);
				return gr_cinfo_base(ci) + 1;
			}
		}

		grPrevSlot = gr_seg_last_slot(grSegment);
		ci = gr_seg_cinfo(grSegment, gr_slot_after(grPrevSlot));
		return gr_cinfo_base(ci) + 1;
	} else {
		return -1;
	}
}

bool usingGraphite(XeTeXLayoutEngine engine)
{
	if (strcmp("graphite2", engine->shaper) == 0)
		return true;
	else
		return false;
}

bool usingOpenType(XeTeXLayoutEngine engine)
{
	if (strcmp("ot", engine->shaper) == 0)
		return true;
	else
		return false;
}

bool isOpenTypeMathFont(XeTeXLayoutEngine engine)
{
	if (engine->font->getFontTable(kMATH) != NULL)
		return true;
	else
		return false;
}

