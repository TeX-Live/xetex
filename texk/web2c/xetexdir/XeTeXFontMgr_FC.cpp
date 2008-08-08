/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2008 by SIL International
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

#include "XeTeXFontMgr_FC.h"

/* allow compilation with old Fontconfig header */
#ifndef FC_FULLNAME
#define FC_FULLNAME "fullname"
#endif

#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H

#include "unicode/ucnv.h"

#define kFontFamilyName	1
#define kFontStyleName	2
#define kFontFullName	4
#define kPreferredFamilyName	16
#define kPreferredSubfamilyName	17

static UConverter*	macRomanConv = NULL;
static UConverter*	utf16beConv = NULL;
static UConverter*	utf8Conv = NULL;

extern FT_Library	gFreeTypeLibrary; /* shared with XeTeXFontInst_FT2 */

static char*
convertToUtf8(UConverter* conv, const unsigned char* name, int len)
{
static char*	buffer1 = NULL;
static char*	buffer2 = NULL;
static int		bufSize = -1;

	if (2 * (len + 1) > bufSize) {
		if (buffer1 != NULL) {
			delete[] buffer1;
			delete[] buffer2;
		}
		bufSize = 2 * len + 100;
		buffer1 = new char[bufSize];
		buffer2 = new char[bufSize];
	}

	UErrorCode	status = U_ZERO_ERROR;
	len = ucnv_toUChars(conv, (UChar*)buffer1, bufSize, (const char*)name, len, &status);
	len = ucnv_fromUChars(utf8Conv, buffer2, bufSize, (UChar*)buffer1, len, &status);
	buffer2[len] = 0;

	return buffer2;
}

XeTeXFontMgr::NameCollection*
XeTeXFontMgr_FC::readNames(FcPattern* pat)
{
	NameCollection*	names = new NameCollection;

	char*	pathname;
	if (FcPatternGetString(pat, FC_FILE, 0, (FcChar8**)&pathname) != FcResultMatch)
		return names;
	int index;
	if (FcPatternGetInteger(pat, FC_INDEX, 0, &index) != FcResultMatch)
		return names;

	FT_Face face;
	if (FT_New_Face(gFreeTypeLibrary, pathname, index, &face) != 0)
		return names;

	const char* name = FT_Get_Postscript_Name(face);
	if (name == NULL)
		return names;
	names->psName = name;

	// for sfnt containers, we'll read the name table ourselves, not rely on Fontconfig
	if (FT_IS_SFNT(face)) {
		std::list<std::string>	familyNames;
		std::list<std::string>	subFamilyNames;
		FT_SfntName	nameRec;
		for (index = 0; index < FT_Get_Sfnt_Name_Count(face); ++index) {
			char*	utf8name = NULL;
			if (FT_Get_Sfnt_Name(face, index, &nameRec) != 0)
				continue;
			switch (nameRec.name_id) {
				case kFontFullName:
				case kFontFamilyName:
				case kFontStyleName:
				case kPreferredFamilyName:
				case kPreferredSubfamilyName:
					{
						bool	preferredName = false;
						if (nameRec.platform_id == TT_PLATFORM_MACINTOSH
								&& nameRec.encoding_id == TT_MAC_ID_ROMAN && nameRec.language_id == 0) {
							utf8name = convertToUtf8(macRomanConv, nameRec.string, nameRec.string_len);
							preferredName = true;
						}
						else if ((nameRec.platform_id == TT_PLATFORM_APPLE_UNICODE)
								|| (nameRec.platform_id == TT_PLATFORM_MICROSOFT))
							utf8name = convertToUtf8(utf16beConv, nameRec.string, nameRec.string_len);

						if (utf8name != NULL) {
							std::list<std::string>*	nameList = NULL;
							switch (nameRec.name_id) {
								case kFontFullName:
									nameList = &names->fullNames;
									break;
								case kFontFamilyName:
									nameList = &names->familyNames;
									break;
								case kFontStyleName:
									nameList = &names->styleNames;
									break;
								case kPreferredFamilyName:
									nameList = &familyNames;
									break;
								case kPreferredSubfamilyName:
									nameList = &subFamilyNames;
									break;
							}
							if (preferredName)
								prependToList(nameList, utf8name);
							else
								appendToList(nameList, utf8name);
						}
					}
					break;
			}
		}
		if (familyNames.size() > 0)
			names->familyNames = familyNames;
		if (subFamilyNames.size() > 0)
			names->styleNames = subFamilyNames;
	}
	else {
		index = 0;
		while (FcPatternGetString(pat, FC_FULLNAME, index++, (FcChar8**)&name) == FcResultMatch)
			appendToList(&names->fullNames, name);
		index = 0;
		while (FcPatternGetString(pat, FC_FAMILY, index++, (FcChar8**)&name) == FcResultMatch)
			appendToList(&names->familyNames, name);
		index = 0;
		while (FcPatternGetString(pat, FC_STYLE, index++, (FcChar8**)&name) == FcResultMatch)
			appendToList(&names->styleNames, name);

		if (names->fullNames.size() == 0) {
			std::string fullName(names->familyNames.front());
			if (names->styleNames.size() > 0) {
				fullName += " ";
				fullName += names->styleNames.front();
			}
			names->fullNames.push_back(fullName);
		}
	}

	FT_Done_Face(face);

	return names;
}

void
XeTeXFontMgr_FC::getOpSizeRecAndStyleFlags(Font* theFont)
{
	XeTeXFontMgr::getOpSizeRecAndStyleFlags(theFont);
	
	if (theFont->weight == 0 && theFont->width == 0) {
		// try to get values from FontConfig, as it apparently wasn't an sfnt
		FcPattern*	pat = theFont->fontRef;
		int			value;
		if (FcPatternGetInteger(pat, FC_WEIGHT, 0, &value) == FcResultMatch)
			theFont->weight = value;
		if (FcPatternGetInteger(pat, FC_WIDTH, 0, &value) == FcResultMatch)
			theFont->width = value;
		if (FcPatternGetInteger(pat, FC_SLANT, 0, &value) == FcResultMatch)
			theFont->slant = value;
	}
}

void
XeTeXFontMgr_FC::cacheFamilyMembers(const std::list<std::string>& familyNames)
{
	if (familyNames.size() == 0)
		return;
	for (int f = 0; f < allFonts->nfont; ++f) {
		FcPattern*	pat = allFonts->fonts[f];
		if (platformRefToFont.find(pat) != platformRefToFont.end())
			continue;
		char*	s;
		for (int i = 0; FcPatternGetString(pat, FC_FAMILY, i, (FcChar8**)&s) == FcResultMatch; ++i) {
			for (std::list<std::string>::const_iterator j = familyNames.begin(); j != familyNames.end(); ++j) {
				if (*j == s) {
					NameCollection*	names = readNames(pat);
					addToMaps(pat, names);
					delete names;
					goto cached;
				}
			}
		}
	cached:
		;
	}
}

void
XeTeXFontMgr_FC::searchForHostPlatformFonts(const std::string& name)
{
	if (cachedAll) // we've already loaded everything on an earlier search
		return;

	std::string	famName;
	int	hyph = name.find('-');
	if (hyph > 0 && hyph < name.length() - 1)
		famName.assign(name.begin(), name.begin() + hyph);
	else
		hyph = 0;

	bool	found = false;
	while (1) {
		for (int f = 0; f < allFonts->nfont; ++f) {
		restart:
			FcPattern*	pat = allFonts->fonts[f];
			if (platformRefToFont.find(pat) != platformRefToFont.end())
				continue;

			if (cachedAll) {
				// failed to find it via FC; add everything to our maps (potentially slow) as a last resort
				NameCollection*	names = readNames(pat);
				addToMaps(pat, names);
				delete names;
				continue;
			}

			char*	s;
			int	i;
			for (i = 0; FcPatternGetString(pat, FC_FULLNAME, i, (FcChar8**)&s) == FcResultMatch; ++i) {
				if (name == s) {
					NameCollection*	names = readNames(pat);
					addToMaps(pat, names);
					cacheFamilyMembers(names->familyNames);
					delete names;
					found = true;
					goto next_font;
				}
			}
		
			for (i = 0; FcPatternGetString(pat, FC_FAMILY, i, (FcChar8**)&s) == FcResultMatch; ++i) {
				if (name == s || (hyph && famName == s)) {
					NameCollection*	names = readNames(pat);
					addToMaps(pat, names);
					cacheFamilyMembers(names->familyNames);
					delete names;
					found = true;
					goto next_font;
				}
				char*	t;
				for (int j = 0; FcPatternGetString(pat, FC_STYLE, j, (FcChar8**)&t) == FcResultMatch; ++j) {
					std::string full(s);
					full += " ";
					full += t;
					if (name == full) {
						NameCollection*	names = readNames(pat);
						addToMaps(pat, names);
						cacheFamilyMembers(names->familyNames);
						delete names;
						found = true;
						goto next_font;
					}
				}
			}

		next_font:
			;
		}

		if (found || cachedAll)
			break;
		cachedAll = true;
	}
}

void
XeTeXFontMgr_FC::initialize()
{
	if (FcInit() == FcFalse) {
		fprintf(stderr, "fontconfig initialization failed!\n");
		exit(9);
	}

	if (gFreeTypeLibrary == 0 && FT_Init_FreeType(&gFreeTypeLibrary) != 0) {
		fprintf(stderr, "FreeType initialization failed!\n");
		exit(9);
	}

	UErrorCode	err = U_ZERO_ERROR;
	macRomanConv = ucnv_open("macintosh", &err);
	utf16beConv = ucnv_open("UTF16BE", &err);
	utf8Conv = ucnv_open("UTF8", &err);
	if (err != 0) {
		fprintf(stderr, "internal error; cannot read font names\n");
		exit(3);
	}

	FcPattern*		pat = FcNameParse((const FcChar8*)":outline=true");
	FcObjectSet*	os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, FC_INDEX,
											FC_FULLNAME, FC_WEIGHT, FC_WIDTH, FC_SLANT, NULL);
	allFonts = FcFontList(FcConfigGetCurrent(), pat, os);
	FcObjectSetDestroy(os);
	FcPatternDestroy(pat);
	
	cachedAll = false;
}

void
XeTeXFontMgr_FC::terminate()
{
	if (macRomanConv != NULL)
		ucnv_close(macRomanConv);
	if (utf16beConv != NULL)
		ucnv_close(utf16beConv);
	if (utf8Conv != NULL)
		ucnv_close(utf8Conv);
}

std::string
XeTeXFontMgr_FC::getPlatformFontDesc(PlatformFontRef font) const
{
	std::string path;
	FcChar8* s;
	if (FcPatternGetString(font, FC_FILE, 0, (FcChar8**)&s) == FcResultMatch)
		path = (char*)s;
	else
		path = "[unknown]";
	return path;
}

