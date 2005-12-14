#include "XeTeXFontMgr_Linux.h"

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
	FT_Done_Face(face);

	names->psName = 0;

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

	return names;
}

void
XeTeXFontMgr_Linux::searchForHostPlatformFonts(const std::string& name)
{
	static	FcFontSet*	allFonts = 0;
	
	if (allFonts == 0) {
		FcPattern*		pat = FcNameParse((const FcChar8*)":outline=true");
		FcObjectSet*	os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, FC_INDEX, FC_FULLNAME);
	
		allFonts = FcFontList(FcConfigGetCurrent(), pat, os);
	
		FcObjectSetDestroy(os);
		FcPatternDestroy(pat);
	}
	
	for (int f = 0; f < allFonts->nfont; ++f) {
		pat = allFonts->fonts[f];
		char*	s;
		for (int i = 0; FcPatternGetString(pat, FC_FULLNAME, i, &s) == FcTrue; ++i) {
			if (name == s) {
				NameCollection*	names = readNames(pat);
				addToMaps(pat, names);
				delete names;
				break;
			}
		}
		for (int i = 0; FcPatternGetString(pat, FC_FAMILY, i, &s) == FcTrue; ++i) {
			if (name == s) {
				NameCollection*	names = readNames(pat);
				addToMaps(pat, names);
				delete names;
				break;
			}
		}
	}
}

void
XeTeXFontMgr_Linux::initialize()
{
	// initialize the nameToFont, nameToFamily, and platformRefToFont maps
	// with all fonts available on the platform

	if (FcInit() == FcFalse) {
		fprintf(stderr, "fontconfig initialization failed!\n");
		exit(9);
	}

	if (FT_Init_FreeType(&ftLib) != 0) {
		fprintf(stderr, "FreeType initialization failed!\n");
		exit(9);
	}
}
