/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

#ifdef XETEX_MAC
#include <Carbon/Carbon.h>
#endif

#include "XeTeX_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct XeTeXFont_rec* XeTeXFont;
typedef struct XeTeXLayoutEngine_rec* XeTeXLayoutEngine;

extern char	gPrefEngine;

#ifdef XETEX_MAC
ATSFontRef getFontRef(XeTeXLayoutEngine engine);
XeTeXFont createFont(ATSFontRef atsFont, Fixed pointSize);

ATSFontRef findFontByName(const char* name, double size);
#else
// appropriate functions for other platforms
XeTeXFont createFont(void* fontRef, Fixed pointSize);
void* findFontByName(const char* name, double size);
#endif

void deleteFont(XeTeXFont font);

void* getFontTablePtr(XeTeXFont font, UInt32 tableTag);

UInt32 countScripts(XeTeXFont font);
UInt32 getIndScript(XeTeXFont font, UInt32 index);
UInt32 countScriptLanguages(XeTeXFont font, UInt32 script);
UInt32 getIndScriptLanguage(XeTeXFont font, UInt32 script, UInt32 index);
UInt32 countFeatures(XeTeXFont font, UInt32 script, UInt32 language);
UInt32 getIndFeature(XeTeXFont font, UInt32 script, UInt32 language, UInt32 index);
float getGlyphWidth(XeTeXFont font, UInt32 gid);
UInt32 countGlyphs(XeTeXFont font);

XeTeXLayoutEngine createLayoutEngine(XeTeXFont font, UInt32 scriptTag, UInt32 languageTag,
						UInt32* addFeatures, UInt32* removeFeatures, UInt32 rgbValue);

void deleteLayoutEngine(XeTeXLayoutEngine engine);

XeTeXFont getFont(XeTeXLayoutEngine engine);
char* getFontPSName(XeTeXLayoutEngine engine);

SInt32 layoutChars(XeTeXLayoutEngine engine, UInt16* chars, SInt32 offset, SInt32 count, SInt32 max,
						char rightToLeft, float x, float y, SInt32* status);

void getGlyphs(XeTeXLayoutEngine engine, UInt32* glyphs, SInt32* status);

void getGlyphPositions(XeTeXLayoutEngine engine, float* positions, SInt32* status);

void getGlyphPosition(XeTeXLayoutEngine engine, SInt32 index, float* x, float* y, SInt32* status);

UInt32 getScriptTag(XeTeXLayoutEngine engine);

UInt32 getLanguageTag(XeTeXLayoutEngine engine);

float getPointSize(XeTeXLayoutEngine engine);

void getAscentAndDescent(XeTeXLayoutEngine engine, float* ascent, float* descent);

UInt32* getAddedFeatures(XeTeXLayoutEngine engine);

UInt32* getRemovedFeatures(XeTeXLayoutEngine engine);

int getDefaultDirection(XeTeXLayoutEngine engine);

UInt32 getRgbValue(XeTeXLayoutEngine engine);

#ifdef __cplusplus
};
#endif
