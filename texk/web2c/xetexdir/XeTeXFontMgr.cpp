/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2006 by SIL International
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

#ifdef XETEX_MAC
#include "XeTeXFontMgr_Mac.h"
#else
#include "XeTeXFontMgr_FC.h"
#endif

#include "XeTeXLayoutInterface.h"
#include "XeTeXswap.h"

#include "Features.h"
#include "GlyphPositioningTables.h"

#include "sfnt.h"

#include <math.h>

XeTeXFontMgr*	XeTeXFontMgr::sFontManager = NULL;
char XeTeXFontMgr::sReqEngine = 0;

/* use our own fmax function because it seems to be missing on certain platforms
   (solaris2.9, at least) */
static inline double
my_fmax(double x, double y)
{
	return (x > y) ? x : y;
}

XeTeXFontMgr*
XeTeXFontMgr::GetFontManager()
{
	if (sFontManager == NULL) {
#ifdef XETEX_MAC
		sFontManager = new XeTeXFontMgr_Mac;
#else
		sFontManager = new XeTeXFontMgr_FC;
#endif
		sFontManager->initialize();
	}
	
	return sFontManager;
}

void
XeTeXFontMgr::Terminate()
{
	if (sFontManager != NULL) {
		sFontManager->terminate();
		// we don't actually deallocate the manager, just ask it to clean up
		// any auxiliary data such as the cocoa pool or freetype/fontconfig stuff
		// as we still need to access font names after this is called
	}
}

PlatformFontRef
XeTeXFontMgr::findFont(const char* name, char* variant, double ptSize)
	// ptSize is in TeX points
	// "variant" string will be shortened (in-place) by removal of /B and /I if present
{
	std::string	nameStr(name);
	Font*	font = NULL;
	
	for (int pass = 0; pass < 2; ++pass) {
		// try full name as given
		std::map<std::string,Font*>::iterator i = nameToFont.find(nameStr);
		if (i != nameToFont.end()) {
			font = i->second;
			break;
		}

		// if there's a hyphen, split there and try Family-Style
		int	hyph = nameStr.find('-');
		if (hyph > 0 && hyph < nameStr.length() - 1) {
			std::string	family(nameStr.begin(), nameStr.begin() + hyph);
			std::map<std::string,Family*>::iterator	f = nameToFamily.find(family);
			if (f != nameToFamily.end()) {
				std::string	style(nameStr.begin() + hyph + 1, nameStr.end());
				i = f->second->styles->find(style);
				if (i != f->second->styles->end()) {
					font = i->second;
					break;
				}
			}
		}
		
		// try as PostScript name
		i = psNameToFont.find(nameStr);
		if (i != psNameToFont.end()) {
			font = i->second;
			break;
		}
		
		// try for the name as a family name
		std::map<std::string,Family*>::iterator	f = nameToFamily.find(nameStr);
		
		if (f != nameToFamily.end()) {
			// look for a family member with the "regular" bit set in OS/2
			for (i = f->second->styles->begin(); i != f->second->styles->end(); ++i)
				if (i->second->isReg) {
					font = i->second;
					break;
				}
			
			if (font == NULL) {
				// try for style "Regular", "Plain", or "Normal"
				i = f->second->styles->find("Regular");
				if (i != f->second->styles->end())
					font = i->second;
				else {
					i = f->second->styles->find("Plain");
					if (i != f->second->styles->end())
						font = i->second;
					else {
						i = f->second->styles->find("Normal");
						if (i != f->second->styles->end())
							font = i->second;
					}
				}
			}
			
			if (font == NULL) {
				// look through the family for the (weight, width, slant) nearest to (80, 100, 0)
				font = bestMatchFromFamily(f->second, 80, 100, 0);
			}
			
			if (font != NULL)
				break;
		}
	
		if (pass == 0) {
			// didn't find it in our caches, so do a platform search (may be relatively expensive);
			// this will update the caches with any fonts that seem to match the name given,
			// so that the second pass might find it
			searchForHostPlatformFonts(nameStr);
		}
	}
	
	if (font == NULL)
		return 0;
	
	Family*	parent = font->parent;
	
	// if there are variant requests, try to apply them
	// and delete B or I codes from the string
	sReqEngine = 0;
	bool	reqBold = false;
	bool	reqItal = false;
	if (font != NULL && variant != NULL) {
		std::string	varString;
		char* cp = variant;
		while (*cp) {
			if (strncmp(cp, "AAT", 3) == 0) {
				sReqEngine = 'A';
				cp += 3;
				if (varString.length() > 0 && *(varString.end() - 1) != '/')
					varString.append("/");
				varString.append("AAT");
				goto skip_to_slash;
			}
			if (strncmp(cp, "ICU", 3) == 0) {
				sReqEngine = 'I';
				cp += 3;
				if (varString.length() > 0 && *(varString.end() - 1) != '/')
					varString.append("/");
				varString.append("ICU");
				goto skip_to_slash;
			}
/*
			if (strncmp(cp, "USP", 3) == 0) {
				sReqEngine = 'U';
				cp += 3;
				if (varString.length() > 0 && *(varString.end() - 1) != '/')
					varString.append("/");
				varString.append("USP");
				goto skip_to_slash;
			}
			if (strncmp(cp, "PAN", 5) == 0) {
				sReqEngine = 'P';
				cp += 3;
				if (varString.length() > 0 && *(varString.end() - 1) != '/')
					varString.append("/");
				varString.append("PAN");
				goto skip_to_slash;
			}
			if (strncmp(cp, "GRA", 8) == 0) {
				sReqEngine = 'G';
				cp += 3;
				if (varString.length() > 0 && *(varString.end() - 1) != '/')
					varString.append("/");
				varString.append("GRA");
				goto skip_to_slash;
			}
*/
			if (*cp == 'S') {
				char*	start = cp;
				++cp;
				if (*cp == '=')
					++cp;
				ptSize = 0.0;
				while (*cp >= '0' && *cp <= '9') {
					ptSize = ptSize * 10 + *cp - '0';
					++cp;
				}
				if (*cp == '.') {
					double	dec = 1.0;
					++cp;
					while (*cp >= '0' && *cp <= '9') {
						dec = dec * 10.0;
						ptSize = ptSize + (*cp - '0') / dec;
						++cp;
					}
				}
				if (varString.length() > 0 && *(varString.end() - 1) != '/')
					varString.append("/");
				varString.append(start, cp);
				goto skip_to_slash;
			}
			
			/* if the code is "B" or "I", we skip putting it in varString */
			while (1) {
				if (*cp == 'B') {
					reqBold = true;
					++cp;
					continue;
				}
				if (*cp == 'I') {
					reqItal = true;
					++cp;
					continue;
				}
				break;
			}
			
		skip_to_slash:
			while (*cp && *cp != '/')
				++cp;
			if (*cp == '/')
				++cp;
		}
		strcpy(variant, varString.c_str());
		
		std::map<std::string,Font*>::iterator i;
		if (reqItal) {
			Font*	bestMatch = font;
			if (font->slant < parent->maxSlant)
				// try for a face with more slant
				bestMatch = bestMatchFromFamily(parent, font->weight, font->width, parent->maxSlant);

			if (bestMatch == font && font->slant > parent->minSlant)
				// maybe the slant is negated, or maybe this was something like "Times-Italic/I"
				bestMatch = bestMatchFromFamily(parent, font->weight, font->width, parent->minSlant);

			if (parent->minWeight == parent->maxWeight && bestMatch->isBold != font->isBold) {
				// try again using the bold flag, as we can't trust weight values
				Font*	newBest = NULL;
				for (i = parent->styles->begin(); i != parent->styles->end(); ++i) {
					if (i->second->isBold == font->isBold) {
						if (newBest == NULL && i->second->isItalic != font->isItalic) {
							newBest = i->second;
							break;
						}
					}
				}
				if (newBest != NULL)
					bestMatch = newBest;
			}

			if (bestMatch == font) {
				// maybe slant values weren't present; try the style bits as a fallback
				bestMatch = NULL;
				for (i = parent->styles->begin(); i != parent->styles->end(); ++i)
					if (i->second->isItalic == !font->isItalic)
						if (parent->minWeight != parent->maxWeight) {
							// weight info was available, so try to match that
							if (bestMatch == NULL || weightAndWidthDiff(i->second, font) < weightAndWidthDiff(bestMatch, font))
								bestMatch = i->second;
						}
						else {
							// no weight info, so try matching style bits
							if (bestMatch == NULL && i->second->isBold == font->isBold) {
								bestMatch = i->second;
								break;	// found a match, no need to look further as we can't distinguish!
							}
						}
			}
			if (bestMatch != NULL)
				font = bestMatch;
		}

		if (reqBold) {
			// try for more boldness, with the same width and slant
			Font*	bestMatch = font;
			if (font->weight < parent->maxWeight) {
				// try to increase weight by 1/2 x (max - min), rounding up
				bestMatch = bestMatchFromFamily(parent,
					font->weight + (parent->maxWeight - parent->minWeight) / 2 + 1,
					font->width, font->slant);
				if (parent->minSlant == parent->maxSlant) {
					// double-check the italic flag, as we can't trust slant values
					Font*	newBest = NULL;
					for (i = parent->styles->begin(); i != parent->styles->end(); ++i) {
						if (i->second->isItalic == font->isItalic) {
							if (newBest == NULL || weightAndWidthDiff(i->second, bestMatch) < weightAndWidthDiff(newBest, bestMatch))
								newBest = i->second;
						}
					}
					if (newBest != NULL)
						bestMatch = newBest;
				}
			}
			if (bestMatch == font && !font->isBold) {
				for (i = parent->styles->begin(); i != parent->styles->end(); ++i) {
					if (i->second->isItalic == font->isItalic && i->second->isBold) {
						bestMatch = i->second;
						break;
					}
				}
			}
			font = bestMatch;
		}
	}

	// if there's optical size info, try to apply it
	if (font != NULL && font->opSizeInfo.subFamilyID != 0 && ptSize != 0.0) {
		ptSize = ptSize * 720 / 72.27;	// convert TeX points to PS decipoints for comparison with the opSize values
		double	bestMismatch = my_fmax(font->opSizeInfo.minSize - ptSize, ptSize - font->opSizeInfo.maxSize);
		if (bestMismatch > 0.0) {
			Font*	bestMatch = font;
			for (std::map<std::string,Font*>::iterator i = parent->styles->begin(); i != parent->styles->end(); ++i) {
				if (i->second->opSizeInfo.subFamilyID != font->opSizeInfo.subFamilyID)
					continue;
				double	mismatch = my_fmax(i->second->opSizeInfo.minSize - ptSize, ptSize - i->second->opSizeInfo.maxSize);
				if (mismatch < bestMismatch) {
					bestMatch = i->second;
					bestMismatch = mismatch;
				}
				if (bestMismatch <= 0.0)
					break;
			}
			font = bestMatch;
		}
	}

	return font->fontRef;
}

const char*
XeTeXFontMgr::getFullName(PlatformFontRef font) const
{
	std::map<PlatformFontRef,Font*>::const_iterator	i = platformRefToFont.find(font);
	if (i == platformRefToFont.end())
		die("internal error %d in XeTeXFontMgr", 2);
	if (i->second->fullName != NULL)
		return i->second->fullName->c_str();
	else
		return i->second->psName->c_str();
}

void
XeTeXFontMgr::getNames(PlatformFontRef font, const char** psName,
	const char** famName, const char** styName) const
{
	std::map<PlatformFontRef,Font*>::const_iterator	i = platformRefToFont.find(font);
	if (i == platformRefToFont.end())
		die("internal error %d in XeTeXFontMgr", 3);
	*psName = i->second->psName->c_str();
	*famName = i->second->familyName->c_str();
	*styName = i->second->styleName->c_str();
}

char
XeTeXFontMgr::getReqEngine() const
{
	return sReqEngine;
}

int
XeTeXFontMgr::weightAndWidthDiff(const Font* a, const Font* b) const
{
	if (a->weight == 0 && a->width == 0) {
		// assume there was no OS/2 info
		if (a->isBold == b->isBold)
			return 0;
		else
			return 10000;
	}
	
	int	widDiff = labs(a->width - b->width);
	if (widDiff < 10)
		widDiff *= 50;
	
	return labs(a->weight - b->weight) + widDiff;
}

int
XeTeXFontMgr::styleDiff(const Font* a, int wt, int wd, int slant) const
{
	int	widDiff = labs(a->width - wd);
	if (widDiff < 10)
		widDiff *= 200;
	
	return labs(labs(a->slant) - labs(slant)) * 2 + labs(a->weight - wt) + widDiff;
}

XeTeXFontMgr::Font*
XeTeXFontMgr::bestMatchFromFamily(const Family* fam, int wt, int wd, int slant) const
{
	Font*	bestMatch = NULL;
	for (std::map<std::string,Font*>::iterator s = fam->styles->begin(); s != fam->styles->end(); ++s)
		if (bestMatch == NULL || styleDiff(s->second, wt, wd, slant) < styleDiff(bestMatch, wt, wd, slant))
			bestMatch = s->second;
	return bestMatch;
}

void
XeTeXFontMgr::getOpSizeRecAndStyleFlags(Font* theFont)
{
	XeTeXFont	font = createFont(theFont->fontRef, 655360);
	if (font != 0) {
		const GlyphPositioningTableHeader* gposTable = (const GlyphPositioningTableHeader*)getFontTablePtr(font, LE_GPOS_TABLE_TAG);
		if (gposTable != NULL) {
			const FeatureListTable*	featureListTable = (const FeatureListTable*)((const char*)gposTable + SWAP(gposTable->featureListOffset));
			for (int i = 0; i < SWAP(featureListTable->featureCount); ++i) {
				UInt32  tag = SWAPT(featureListTable->featureRecordArray[i].featureTag);
				if (tag == LE_SIZE_FEATURE_TAG) {
					const FeatureTable*	feature = (const FeatureTable*)((const char*)featureListTable
													+ SWAP(featureListTable->featureRecordArray[i].featureTableOffset));
					UInt16	offset = SWAP(feature->featureParamsOffset);
					const OpSizeRec*	pSizeRec;
					/* if featureParamsOffset < (offset of feature from featureListTable),
					   then we have a correct size table;
					   otherwise we (presumably) have a "broken" one from the old FDK */
					if (offset < (const char*)feature - (const char*)featureListTable)
						pSizeRec = (const OpSizeRec*)((char*)feature + offset);
					else
						pSizeRec = (const OpSizeRec*)((char*)featureListTable + offset);
					theFont->opSizeInfo.designSize = SWAP(pSizeRec->designSize);
					theFont->opSizeInfo.subFamilyID = SWAP(pSizeRec->subFamilyID);
					theFont->opSizeInfo.nameCode = SWAP(pSizeRec->nameCode);
					theFont->opSizeInfo.minSize = SWAP(pSizeRec->minSize);
					theFont->opSizeInfo.maxSize = SWAP(pSizeRec->maxSize);
					break;
				}
			}
		}

		const OS2TableHeader* os2Table = (const OS2TableHeader*)getFontTablePtr(font, LE_OS_2_TABLE_TAG);
		if (os2Table != NULL) {
			theFont->weight = SWAP(os2Table->usWeightClass);
			theFont->width = SWAP(os2Table->usWidthClass);
			UInt16 sel = SWAP(os2Table->fsSelection);
			theFont->isReg = (sel & (1 << 6)) != 0;
			theFont->isBold = (sel & (1 << 5)) != 0;
			theFont->isItalic = (sel & (1 << 0)) != 0;
		}

		const HEADTable* headTable = (const HEADTable*)getFontTablePtr(font, LE_HEAD_TABLE_TAG);
		if (headTable != NULL) {
			UInt16	ms = SWAP(headTable->macStyle);
			if ((ms & (1 << 0)) != 0)
				theFont->isBold = true;
			if ((ms & (1 << 1)) != 0)
				theFont->isItalic = true;
		}

		const POSTTable* postTable = (const POSTTable*)getFontTablePtr(font, LE_POST_TABLE_TAG);
		if (postTable != NULL) {
			theFont->slant = (int)(1000 * (tan(Fix2X(-SWAP(postTable->italicAngle)) * M_PI / 180.0)));
		}
		deleteFont(font);
	}
}

// append a name but only if it's not already in the list
void
XeTeXFontMgr::appendToList(std::list<std::string>* list, const char* str)
{
	for (std::list<std::string>::const_iterator i = list->begin(); i != list->end(); ++i)
		if (*i == str)
			return;
	list->push_back(str);
}

// prepend a name, removing it from later in the list if present
void
XeTeXFontMgr::prependToList(std::list<std::string>* list, const char* str)
{
	for (std::list<std::string>::iterator i = list->begin(); i != list->end(); ++i)
		if (*i == str) {
			list->erase(i);
			break;
		}
	list->push_front(str);
}

void
XeTeXFontMgr::addToMaps(PlatformFontRef platformFont, const NameCollection* names)
{
	if (names->psName.length() == 0)
		return;	// can't use a font that lacks a PostScript name

	if (psNameToFont.find(names->psName) != psNameToFont.end())
		return;	// duplicates an earlier PS name, so skip

	Font*	thisFont = new Font(platformFont);
	thisFont->psName = new std::string(names->psName);
	getOpSizeRecAndStyleFlags(thisFont);

	psNameToFont[names->psName] = thisFont;
	platformRefToFont[platformFont] = thisFont;

	if (names->fullNames.size() > 0)
		thisFont->fullName = new std::string(*(names->fullNames.begin()));

	if (names->familyNames.size() > 0)
		thisFont->familyName = new std::string(*(names->familyNames.begin()));
	else
		thisFont->familyName = new std::string(names->psName);

	if (names->styleNames.size() > 0)
		thisFont->styleName = new std::string(*(names->styleNames.begin()));
	else
		thisFont->styleName = new std::string;

	std::list<std::string>::const_iterator i;
	for (i = names->familyNames.begin(); i != names->familyNames.end(); ++i) {
		std::map<std::string,Family*>::iterator	iFam = nameToFamily.find(*i);
		Family*	family;
		if (iFam == nameToFamily.end()) {
			family = new Family;
			nameToFamily[*i] = family;
			family->minWeight = thisFont->weight;
			family->maxWeight = thisFont->weight;
			family->minWidth = thisFont->width;
			family->maxWidth = thisFont->width;
			family->minSlant = thisFont->slant;
			family->maxSlant = thisFont->slant;
		}
		else {
			family = iFam->second;
			if (thisFont->weight < family->minWeight)
				family->minWeight = thisFont->weight;
			if (thisFont->weight > family->maxWeight)
				family->maxWeight = thisFont->weight;
			if (thisFont->width < family->minWidth)
				family->minWidth = thisFont->width;
			if (thisFont->width > family->maxWidth)
				family->maxWidth = thisFont->width;
			if (thisFont->slant < family->minSlant)
				family->minSlant = thisFont->slant;
			if (thisFont->slant > family->maxSlant)
				family->maxSlant = thisFont->slant;
		}

		if (thisFont->parent == NULL)
			thisFont->parent = family;
				
		// ensure all style names in the family point to thisFont
		for (std::list<std::string>::const_iterator j = names->styleNames.begin(); j != names->styleNames.end(); ++j) {
			std::map<std::string,Font*>::iterator iFont = family->styles->find(*j);
			if (iFont == family->styles->end())
				(*family->styles)[*j] = thisFont;
/*
			else if (iFont->second != thisFont)
				fprintf(stderr, "# Font name warning: ambiguous Style \"%s\" in Family \"%s\" (PSNames \"%s\" and \"%s\")\n",
							j->c_str(), i->c_str(), iFont->second->psName->c_str(), thisFont->psName->c_str());
*/
		}
	}
	
	for (i = names->fullNames.begin(); i != names->fullNames.end(); ++i) {
		std::map<std::string,Font*>::iterator iFont = nameToFont.find(*i);
		if (iFont == nameToFont.end())
			nameToFont[*i] = thisFont;
/*
		else if (iFont->second != thisFont)
			fprintf(stderr, "# Font name warning: ambiguous FullName \"%s\" (PSNames \"%s\" and \"%s\")\n",
						i->c_str(), iFont->second->psName->c_str(), thisFont->psName->c_str());
*/
	}
}

void
XeTeXFontMgr::die(const char*s, int i) const
{
	fprintf(stderr, s, i);
	fprintf(stderr, " - exiting\n");
	exit(3);
}

void
XeTeXFontMgr::terminate()
{
}

