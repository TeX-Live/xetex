#include "XeTeXFontMgr.h"

#include "XeTeXLayoutInterface.h"
#include "XeTeXswap.h"

#include "Features.h"
#include "GlyphPositioningTables.h"

#include <math.h>

XeTeXFontMgr::XeTeXFontMgr*	XeTeXFontMgr::sFontManager = NULL;
char XeTeXFontMgr::sReqEngine = 0;

XeTeXFontMgr::XeTeXFontMgr*
XeTeXFontMgr::GetFontManager()
{
	if (sFontManager == NULL) {
		sFontManager = new XeTeXFontMgr;
		sFontManager->buildFontMaps();
	}
	
	return sFontManager;
}

PlatformFontRef
XeTeXFontMgr::findFont(const char* name, const char* variant, double ptSize)
{
	std::string	nameStr(name);
	Font*	font = NULL;
	
	// try full name as given
	std::map<std::string,Font*>::iterator i = nameToFont.find(name);
	if (i != nameToFont.end())
		font = i->second;
	
	if (font == NULL) {
		// if there's a hyphen, split there and try Family-Style
		int	hyph = nameStr.find('-');
		if (hyph > 0 && hyph < nameStr.length() - 1) {
			std::string	family(nameStr.begin(), nameStr.begin() + hyph);
			std::map<std::string,Family*>::iterator	f = nameToFamily.find(family);
			if (f != nameToFamily.end()) {
				std::string	style(nameStr.begin() + hyph + 1, nameStr.end());
				i = f->second->styles->find(style);
				if (i != f->second->styles->end())
					font = i->second;
			}
		}
	}
	
	if (font == NULL) {
		// try as PostScript name
		i = psNameToFont.find(name);
		if (i != psNameToFont.end())
			font = i->second;
	}
	
	if (font == NULL) {
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
		}
	}

	if (font == NULL)
		return 0;

	Family*	parent = font->parent;
	
	// if there are variant requests, try to apply them
	sReqEngine = 0;
	bool	reqBold = false;
	bool	reqItal = false;
	if (font != NULL && variant != NULL) {
		const char* cp = variant;
		while (*cp) {
			if (strncmp(cp, "AAT", 3) == 0) {
				sReqEngine = 'A';
				cp += 3;
				continue;
			}
			if (strncmp(cp, "ICU", 3) == 0) {
				sReqEngine = 'I';
				cp += 3;
				continue;
			}
/*
			if (strncmp(cp, "USP", 3) == 0) {
				sReqEngine = 'U';
				cp += 3;
				continue;
			}
			if (strncmp(cp, "Pango", 5) == 0) {
				sReqEngine = 'P';
				cp += 5;
				continue;
			}
			if (strncmp(cp, "Graphite", 8) == 0) {
				sReqEngine = 'G';
				cp += 8;
				continue;
			}
*/
			if (*cp == 'S') {
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
				continue;
			}
			if (*cp == 'B')
				reqBold = true;
			else if (*cp == 'I')
				reqItal = true;
			++cp;
		}

		if (reqItal && !font->isItalic) {
			// look for an italic version with same boldness
			Font*	bestMatch = NULL;
			for (std::map<std::string,Font*>::iterator i = parent->styles->begin(); i != parent->styles->end(); ++i)
				if (i->second->isItalic)
					if (bestMatch == NULL || weightAndWidthDiff(i->second, font) < weightAndWidthDiff(bestMatch, font))
						bestMatch = i->second;
			if (bestMatch != NULL)
				font = bestMatch;
		}
		if (reqBold && !font->isBold)
			// look for a version with same italicness but more boldness
			for (std::map<std::string,Font*>::iterator i = parent->styles->begin(); i != parent->styles->end(); ++i)
				if (i->second->isItalic == font->isItalic && i->second->isBold) {
					font = i->second;
					break;
				}
	}

	// if there's optical size info, try to apply it
	if (font != NULL && font->opSizeInfo.subFamilyID != 0 && ptSize != 0.0) {
		ptSize = ptSize * 720 / 72.27;	// for comparison with decipoint values in opSizeInfo
		double	bestMismatch = fmax(font->opSizeInfo.minSize - ptSize, ptSize - font->opSizeInfo.maxSize);
		if (bestMismatch > 0.0) {
			Font*	bestMatch = font;
			for (std::map<std::string,Font*>::iterator i = parent->styles->begin(); i != parent->styles->end(); ++i) {
				if (i->second->opSizeInfo.subFamilyID != font->opSizeInfo.subFamilyID)
					continue;
				double	mismatch = fmax(i->second->opSizeInfo.minSize - ptSize, ptSize - i->second->opSizeInfo.maxSize);
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
XeTeXFontMgr::getPSName(PlatformFontRef font) const
{
	std::map<PlatformFontRef,Font*>::const_iterator	i = platformRefToFont.find(font);
	if (i == platformRefToFont.end())
		die("internal error %d in XeTeXFontMgr", 1);
	return i->second->psName->c_str();
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
		die("internal error %d in XeTeXFontMgr", 1);
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

void
XeTeXFontMgr::getOpSizeRecAndStyleFlags(Font* theFont)
{
	XeTeXFont	font = createFont(theFont->fontRef, 655360);
	if (font != 0) {
		const GlyphPositioningTableHeader* gposTable = (const GlyphPositioningTableHeader*)getFontTablePtr(font, LE_GPOS_TABLE_TAG);
		if (gposTable != NULL) {
			FeatureListTable*	featureListTable = (FeatureListTable*)((char*)gposTable + SWAP(gposTable->featureListOffset));
			for (int i = 0; i < SWAP(featureListTable->featureCount); ++i) {
				UInt32  tag = SWAP(*(UInt32*)&featureListTable->featureRecordArray[i].featureTag);
				if (tag == LE_SIZE_FEATURE_TAG) {
					FeatureTable*	feature = (FeatureTable*)((char*)featureListTable + SWAP(featureListTable->featureRecordArray[i].featureTableOffset));
					OpSizeRec*	pSizeRec = (OpSizeRec*)((char*)featureListTable + feature->featureParamsOffset);
					theFont->opSizeInfo.designSize = SWAP(pSizeRec->designSize);
					theFont->opSizeInfo.subFamilyID = SWAP(pSizeRec->subFamilyID);
					theFont->opSizeInfo.nameCode = SWAP(pSizeRec->nameCode);
					theFont->opSizeInfo.minSize = SWAP(pSizeRec->minSize);
					theFont->opSizeInfo.maxSize = SWAP(pSizeRec->maxSize);
					break;
				}
			}
		}

		struct OS2TableHeader {
			UInt16	version;
			SInt16	xAvgCharWidth;
			UInt16	usWeightClass;
			UInt16	usWidthClass;
			SInt16	fsType;
			SInt16	ySubscriptXSize;
			SInt16	ySubscriptYSize;
			SInt16	ySubscriptXOffset;
			SInt16	ySubscriptYOffset;
			SInt16	ySuperscriptXSize;
			SInt16	ySuperscriptYSize;
			SInt16	ySuperscriptXOffset;
			SInt16	ySuperscriptYOffset;
			SInt16	yStrikeoutSize;
			SInt16	yStrikeoutPosition;
			SInt16	sFamilyClass;
			UInt8	panose[10];
			UInt8	ulCharRange[16];	// spec'd as 4 longs, but do this to keep structure packed
			SInt8	achVendID[4];
			UInt16	fsSelection;
			UInt16	fsFirstCharIndex;
			UInt16	fsLastCharIndex;
		};

		const OS2TableHeader* os2Table = (const OS2TableHeader*)getFontTablePtr(font, LE_OS_2_TABLE_TAG);
		if (os2Table != NULL) {
			theFont->weight = SWAP(os2Table->usWeightClass);
			theFont->width = SWAP(os2Table->usWidthClass);
			UInt16 sel = SWAP(os2Table->fsSelection);
			theFont->isReg = (sel & (1 << 6)) != 0;
			theFont->isBold = (sel & (1 << 5)) != 0;
			theFont->isItalic = (sel & (1 << 0)) != 0;
		}

		struct HeadTable {
			Fixed	version;
			Fixed	fontRevision;
			UInt32	checkSumAdjustment;
			UInt32	magicNumber;
			UInt16	flags;
			UInt16	unitsPerEm;
			UInt32	created[2];		// actually LONGDATETIME values
			UInt32	modified[2];
			SInt16	xMin;
			SInt16	yMin;
			SInt16	xMax;
			SInt16	yMax;
			UInt16	macStyle;
			UInt16	lowestRecPPEM;
			SInt16	fontDirectionHint;
			SInt16	indexToLocFormat;
			SInt16	glyphDataFormat;
		};
		
		const HeadTable* headTable = (const HeadTable*)getFontTablePtr(font, LE_HEAD_TABLE_TAG);
		if (headTable != NULL) {
			UInt16	ms = SWAP(headTable->macStyle);
			if ((ms & (1 << 0)) != 0)
				theFont->isBold = true;
			if ((ms & (1 << 1)) != 0)
				theFont->isItalic = true;
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

	for (std::list<std::string>::const_iterator i = names->familyNames.begin(); i != names->familyNames.end(); ++i) {
		std::map<std::string,Family*>::iterator	iFam = nameToFamily.find(*i);
		Family*	family;
		if (iFam == nameToFamily.end()) {
			family = new Family;
			nameToFamily[*i] = family;
		}
		else
			family = iFam->second;
		
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
	
	for (std::list<std::string>::const_iterator i = names->fullNames.begin(); i != names->fullNames.end(); ++i) {
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

