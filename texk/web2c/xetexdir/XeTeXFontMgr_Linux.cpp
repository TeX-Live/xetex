#include "XeTeXFontMgr_Linux.h"

#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H

#include "ucnv.h"

#define kFontFamilyName	1
#define kFontStyleName	2
#define kFontFullName	4
#define kPreferredFamilyName	16
#define kPreferredSubfamilyName	17

static UConverter*	macRomanConv = NULL;
static UConverter*	utf16beConv = NULL;
static UConverter*	utf8Conv = NULL;

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
XeTeXFontMgr_Linux::readNames(FcPattern* pat)
{
	NameCollection*	names = new NameCollection;

	char*	pathname;
	if (FcPatternGetString(pat, FC_FILE, 0, (FcChar8**)&pathname) == FcTrue)
		return names;
	int index;
	if (FcPatternGetInteger(pat, FC_INDEX, 0, &index) == FcTrue)
		return names;

	FT_Face face;
	if (FT_New_Face(ftLib, pathname, index, &face) != 0)
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
		while (FcPatternGetString(pat, FC_FULLNAME, index++, (FcChar8**)&name) == FcFalse)
			appendToList(&names->fullNames, name);
		index = 0;
		while (FcPatternGetString(pat, FC_FAMILY, index++, (FcChar8**)&name) == FcFalse)
			appendToList(&names->familyNames, name);
		index = 0;
		while (FcPatternGetString(pat, FC_STYLE, index++, (FcChar8**)&name) == FcFalse)
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
XeTeXFontMgr_Linux::getOpSizeRecAndStyleFlags(Font* theFont)
{
	XeTeXFontMgr::getOpSizeRecAndStyleFlags(theFont);
	
	if (theFont->weight == 0 && theFont->width == 0) {
		// try to get values from FontConfig, as it apparently wasn't an sfnt
		FcPattern*	pat = theFont->fontRef;
		int			value;
		if (FcPatternGetInteger(pat, FC_WEIGHT, 0, &value) == FcFalse)
			theFont->weight = value;
		if (FcPatternGetInteger(pat, FC_WIDTH, 0, &value) == FcFalse)
			theFont->width = value;
		if (FcPatternGetInteger(pat, FC_SLANT, 0, &value) == FcFalse)
			theFont->slant = value;
	}
}

void
XeTeXFontMgr_Linux::searchForHostPlatformFonts(const std::string& name)
{
	static	FcFontSet*	allFonts = 0;
	
	if (allFonts == 0) {
		FcPattern*		pat = FcNameParse((const FcChar8*)":outline=true");
		FcObjectSet*	os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, FC_INDEX,
												FC_FULLNAME, FC_WEIGHT, FC_WIDTH, FC_SLANT, NULL);
	
		allFonts = FcFontList(FcConfigGetCurrent(), pat, os);
	
		FcObjectSetDestroy(os);
		FcPatternDestroy(pat);
	}
	
	std::string	famName;
	int	hyph = name.find('-');
	if (hyph > 0 && hyph < name.length() - 1)
		famName.assign(name.begin(), name.begin() + hyph);
	else
		hyph = 0;

	for (int f = 0; f < allFonts->nfont; ++f) {
	restart:
		FcPattern*	pat = allFonts->fonts[f];
		if (platformRefToFont.find(pat) != platformRefToFont.end())
			continue;
		char*	s;
		int	i;
		for (i = 0; FcPatternGetString(pat, FC_FULLNAME, i, (FcChar8**)&s) == FcFalse; ++i) {
			if (name == s) {
				NameCollection*	names = readNames(pat);
				addToMaps(pat, names);
				delete names;
				goto next_font;
			}
		}
		
		for (i = 0; FcPatternGetString(pat, FC_FAMILY, i, (FcChar8**)&s) == FcFalse; ++i) {
			if (name == s || (hyph && famName == s)) {
				NameCollection*	names = readNames(pat);
				addToMaps(pat, names);
				delete names;
				goto next_font;
			}
			char*	t;
			for (int j = 0; FcPatternGetString(pat, FC_STYLE, j, (FcChar8**)&t) == FcFalse; ++j) {
				std::string full(s);
				full += " ";
				full += t;
				if (name == full) {
					// need to ensure we'll pick up the whole family
					famName = s;
					hyph = 1;
					f = 0;
					goto restart;
				}
			}
		}

	next_font:
		;
	}
}

void
XeTeXFontMgr_Linux::initialize()
{
	if (FcInit() == FcFalse) {
		fprintf(stderr, "fontconfig initialization failed!\n");
		exit(9);
	}

	if (FT_Init_FreeType(&ftLib) != 0) {
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
}

void
XeTeXFontMgr_Linux::terminate()
{
	if (macRomanConv != NULL)
		ucnv_close(macRomanConv);
	if (utf16beConv != NULL)
		ucnv_close(utf16beConv);
	if (utf8Conv != NULL)
		ucnv_close(utf8Conv);
}
