/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/


#include "XeTeXFontManager_FC.h"

#include <fontconfig/fontconfig.h>

#include "XeTeXLayoutInterface.h"

#include "Features.h"
#include "GlyphPositioningTables.h"

extern char*	nameoffile;	/* the buffer used by TeX for packed names */
extern int		namelength;	/* length of the name */

extern "C" {
	extern void*	xmalloc(long size);	/* malloc a buffer, die on failure */
};

XeTeXFontManager_FC::XeTeXFontManager_FC()
{
	if (FcInit() == FcFalse) {
		fprintf(stderr, "fontconfig initialization failed!\n");
		exit(1);
	}
}

XeTeXFontManager_FC::~XeTeXFontManager_FC()
{
	FcFini();
}

void*
XeTeXFontManager_FC::findFont(const char* name, double pointSize)
	// return value is actually an FcPattern*
{
	FcConfig*	config = FcConfigGetCurrent();

	fontSpec	spec;
	bool	updateName = parseFontName(name, spec);

	FcPattern*	pat = FcPatternCreate();
	FcValue	value;
	value.type = FcTypeString;
	value.u.s = spec.name;
	FcPatternAdd(pat, FC_FULLNAME, value, FcFalse);
	FcConfigSubstitute(config, pat, FcMatchFont);
	FcDefaultSubstitute(pat);
	
	FcResult	result = FcResultMatch;
	FcPattern*	match = FcFontMatch(config, pat, &result);

	return match;
}
