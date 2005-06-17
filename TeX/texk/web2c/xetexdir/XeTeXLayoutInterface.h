/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

#include <Carbon/Carbon.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ATSFontInstance_rec* ATSFontInstance;
typedef struct XeTeXLayoutEngine_rec* XeTeXLayoutEngine;

ATSFontInstance createFontInstance(ATSFontRef atsFont, Fixed pointSize);

void deleteFontInstance(ATSFontInstance fontInstance);

void applyOpticalSize(ATSFontInstance fontInstance, Fixed opticalSize);

void* getFontTablePtr(ATSFontInstance fontInstance, UInt32 tableTag);

UInt32 countScripts(ATSFontInstance fontInstance);
UInt32 getIndScript(ATSFontInstance fontInstance, UInt32 index);
UInt32 countScriptLanguages(ATSFontInstance fontInstance, UInt32 script);
UInt32 getIndScriptLanguage(ATSFontInstance fontInstance, UInt32 script, UInt32 index);
UInt32 countFeatures(ATSFontInstance fontInstance, UInt32 script, UInt32 language);
UInt32 getIndFeature(ATSFontInstance fontInstance, UInt32 script, UInt32 language, UInt32 index);
float getGlyphWidth(ATSFontInstance fontInstance, UInt32 gid);
UInt32 countGlyphs(ATSFontInstance fontInstance);

XeTeXLayoutEngine createLayoutEngine(ATSFontInstance fontInstance, UInt32 scriptTag, UInt32 languageTag,
						UInt32* addFeatures, UInt32* removeFeatures, UInt32 rgbValue);

void deleteLayoutEngine(XeTeXLayoutEngine engine);

ATSFontInstance getFontInstance(XeTeXLayoutEngine engine);

SInt32 layoutChars(XeTeXLayoutEngine engine, UInt16 chars[], SInt32 offset, SInt32 count, SInt32 max,
						Boolean rightToLeft, float x, float y, OSStatus* status);

void getGlyphs(XeTeXLayoutEngine engine, UInt32 glyphs[], OSStatus* status);

void getGlyphPositions(XeTeXLayoutEngine engine, float positions[], OSStatus* status);

void getGlyphPosition(XeTeXLayoutEngine engine, SInt32 index, float* x, float* y, OSStatus* status);

UInt32 getScriptTag(XeTeXLayoutEngine engine);

UInt32 getLanguageTag(XeTeXLayoutEngine engine);

ATSFontRef getFontRef(XeTeXLayoutEngine engine);

float getPointSize(XeTeXLayoutEngine engine);

void getAscentAndDescent(XeTeXLayoutEngine engine, float* ascent, float* descent);

UInt32* getAddedFeatures(XeTeXLayoutEngine engine);

UInt32* getRemovedFeatures(XeTeXLayoutEngine engine);

int getDefaultDirection(XeTeXLayoutEngine engine);

UInt32 getRgbValue(XeTeXLayoutEngine engine);

#ifdef __cplusplus
};
#endif
