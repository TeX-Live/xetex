/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

#include "XeTeXFontManager.h"

#ifdef XETEX_MAC
#include "XeTeXFontManager_Mac.h"
#else
#include "XeTeXFontManager_FC.h"
#endif

#include "Features.h"
#include "GlyphPositioningTables.h"

XeTeXFontManager*	XeTeXFontManager::gFontManager;

XeTeXFontManager::XeTeXFontManager*
XeTeXFontManager::GetFontManager()
{
	if (0 == gFontManager) {
#ifdef XETEX_MAC
		gFontManager = new XeTeXFontManager_Mac();	// create Mac OS version
#else
		gFontManager = new XeTeXFontManager_FC();	// FontConfig version
#endif
	}

	return gFontManager;
}

void
XeTeXFontManager::getOpticalSizeRec(XeTeXFont font, opticalSizeRec& sizeRec)
{
	const GlyphPositioningTableHeader* gposTable = (const GlyphPositioningTableHeader*)getFontTablePtr(font, 'GPOS');
	if (gposTable != NULL) {
		FeatureListTable*	featureListTable = (FeatureListTable*)((char*)gposTable + gposTable->featureListOffset);
		for (int i = 0; i < featureListTable->featureCount; ++i) {
			UInt32  tag = *(UInt32*)&featureListTable->featureRecordArray[i].featureTag;
			if (tag == 'size') {
				FeatureTable*	feature = (FeatureTable*)((char*)featureListTable + featureListTable->featureRecordArray[i].featureTableOffset);
				sizeRec = *(opticalSizeRec*)((char*)featureListTable + feature->featureParamsOffset);
				break;
			}
		}
	}
}


char gPrefEngine;

bool
XeTeXFontManager::parseFontName(const char* str, fontSpec& spec)
	// returns true if name is not identical to that passed in
{
	spec.reqSize = -1.0;
	spec.reqBold = false;
	spec.reqItal = false;
	gPrefEngine = 0;

	const char*	cp = str;
	while (*cp && *cp != '/')
		++cp;
	if (*cp == '/') {
		spec.name = std::string(str, cp - str);
		++cp;
		while (*cp) {
			if (strncmp(cp, "ICU", 3) == 0) {
				gPrefEngine = 'I';
				cp += 3;
				continue;
			}
			if (strncmp(cp, "AAT", 3) == 0) {
				gPrefEngine = 'A';
				cp += 3;
				continue;
			}
			if (*cp == 'S') {
				++cp;
				if (*cp == '=')
					++cp;
				spec.reqSize = 0.0;
				while (*cp >= '0' && *cp <= '9') {
					spec.reqSize = spec.reqSize * 10 + *cp - '0';
					++cp;
				}
				if (*cp == '.') {
					double	dec = 1.0;
					++cp;
					while (*cp >= '0' && *cp <= '9') {
						dec = dec * 10.0;
						spec.reqSize = spec.reqSize + (*cp - '0') / dec;
						++cp;
					}
				}
				continue;
			}
			if (*cp == 'B')
				spec.reqBold = true;
			else if (*cp == 'I')
				spec.reqItal = true;
			++cp;
		}
		
		return true;
	}

	spec.name = std::string(str);
	return false;
}

