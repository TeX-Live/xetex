/* xetexfontdict.cpp
 * Font dictionaries for XeTeX
 */

#include <stdlib.h>

#include <string>
#include <map>
#include <vector>
#include <algorithm>

#include "xetexfontdict.h"
#include "XeTeXLayoutInterface.h"

#include "Features.h"
#include "GlyphPositioningTables.h"

extern char*	nameoffile;	/* the buffer used by TeX for packed names */
extern int		namelength;	/* length of the name */

extern "C" {
	extern void*	xmalloc(long size);	/* malloc a buffer, die on failure */
};

engineTech	preferredTech;


typedef struct {
	unsigned short	subFamilyID;
	unsigned short	minSize;
	unsigned short	maxSize;
	ATSUFontID		fontID;
} familyMember;

inline bool operator< (const familyMember& lhs, const familyMember& rhs)
{
	if (lhs.subFamilyID < rhs.subFamilyID)
		return true;
	if (lhs.subFamilyID > rhs.subFamilyID)
		return false;
	if (lhs.minSize < rhs.minSize)
		return true;
	if (lhs.minSize > rhs.minSize)
		return false;
	if (lhs.maxSize < rhs.maxSize)
		return true;
	if (lhs.maxSize > rhs.maxSize)
		return false;
	if (lhs.fontID < rhs.fontID)
		return true;
	if (lhs.fontID > rhs.fontID)
		return false;
	return false;
}

typedef std::vector<familyMember>		opticalFamily;					// allocate one such family record per family+subfamily with optical size info

std::map<std::string,opticalFamily*>	opticalFamilyFromFamilyName;	// maps family+subfamily name to optical family record
std::map<ATSUFontID,opticalFamily*>		opticalFamilyFromFontID;		// maps ATSUFontIDs to family records
std::map<std::string,ATSUFontID>		fontIDFromFullName;				// maps full name (as used in TeX source) to font ID

static bool	fontDictsInitialized = false;

static void die(char*s, int i)
{
	fprintf(stderr, s, i);
	fprintf(stderr, " - exiting\n");
	exit(3);
}

ATSUFontID
get_optically_sized_font(ATSUFontID fontID, double opticalSize)
{
	std::map<ATSUFontID,opticalFamily*>::const_iterator i = opticalFamilyFromFontID.find(fontID);
	if (i == opticalFamilyFromFontID.end())
		return fontID;
	const opticalFamily*	optFam = i->second;
	if (optFam->size() < 2)
		return fontID;

	unsigned short	d = (unsigned short)((opticalSize * 720.0 / 72.27) + 0.5);
	opticalFamily::const_iterator prev_j = optFam->begin();
	for (opticalFamily::const_iterator j = optFam->begin(); j != optFam->end(); ++j) {
		if ((d >= j->minSize) && (d < j->maxSize))
			return j->fontID;
		if (d < j->minSize) {
			if (d - prev_j->maxSize < j->minSize - d)
				return prev_j->fontID;
			else
				return j->fontID;
		}
		prev_j = j;
	}
	return prev_j->fontID;
}

typedef struct {
	unsigned short	designSize;
	unsigned short	subFamilyID;
	unsigned short	nameCode;
	unsigned short	minSize;
	unsigned short	maxSize;
} opticalSizeRec;

void
get_optical_size_rec(ATSUFontID id, opticalSizeRec& sizeRec)
{
	sizeRec.subFamilyID = 0;
	ATSFontInstance	inst = createFontInstance(FMGetATSFontRefFromFont(id), 655360);
	if (inst != 0) {
		const GlyphPositioningTableHeader* gposTable = (const GlyphPositioningTableHeader*)getFontTablePtr(inst, 'GPOS');
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
		deleteFontInstance(inst);
	}
}

void
init_font_dicts()
{
	fontDictsInitialized = true;

	OSStatus	status;
	ItemCount	fontCount;
	status = ATSUFontCount(&fontCount);
	if (status != noErr)
		die("ATSUFontCount failed, status=%d", status);

	ATSUFontID*	fontIDs = (ATSUFontID*)xmalloc(fontCount * sizeof(ATSUFontID));
	status = ATSUGetFontIDs(fontIDs, fontCount, &fontCount);
	if (status != noErr)
		die("ATSUGetFontIDs failed, status=%d", status);

	int	f;
	for (f = 0; f < fontCount; ++f) {
		char*	familyName = NULL;
		char*	subFamilyName = NULL;
		opticalSizeRec	sizeRec;
		get_optical_size_rec(fontIDs[f], sizeRec);
	
		ItemCount	nameCount;
		status = ATSUCountFontNames(fontIDs[f], &nameCount);
		if (status != noErr)
			die("ATSUCountFontNames failed, index=%d", f);
		int n;
		for (n = 0; n < nameCount; ++n) {
			ByteCount			nameLength;
			FontNameCode		nameCode;
			FontPlatformCode	namePlatform;
			FontScriptCode		nameScript;
			FontLanguageCode	nameLang;
			status = ATSUGetIndFontName(fontIDs[f], n, 0, 0, &nameLength,
										&nameCode, &namePlatform, &nameScript, &nameLang);
			if ((status != noErr) && (status != kATSUNoFontNameErr))
				die("ATSUGetIndFontName failed, status=%d", status);
			if ((status == noErr) && (nameLength > 0)) {
				if (nameCode == kFontFullName) {
					TextEncoding	encoding;
					CFStringRef		nameStr = 0;
					char*			nameBuf = (char*)xmalloc(nameLength + 1);
					status = ATSUGetIndFontName(fontIDs[f], n, nameLength, nameBuf, &nameLength,
												&nameCode, &namePlatform, &nameScript, &nameLang);
					nameBuf[nameLength] = 0;
					if (namePlatform == kFontMacintoshPlatform) {
						GetTextEncodingFromScriptInfo(nameScript, nameLang, 0, &encoding);
						nameStr = CFStringCreateWithBytes(0, (UInt8*)nameBuf, nameLength, encoding, false);
					}
					else if ((namePlatform == kFontUnicodePlatform) || (namePlatform == kFontMicrosoftPlatform))
						nameStr = CFStringCreateWithCharacters(0, (UniChar*)nameBuf, nameLength / 2);
					if (nameStr != 0) {
						// quick path first
						if (const char *s = CFStringGetCStringPtr(nameStr, kCFStringEncodingUTF8))
							fontIDFromFullName[s] = fontIDs[f];
						else {
							// need to extract into buffer
							CFIndex length = CFStringGetLength(nameStr);	// in 16-bit character units
							char *buffer = new char[6 * length + 1];	// pessimistic
							if (CFStringGetCString(nameStr, buffer, 6 * length + 1, kCFStringEncodingUTF8))
								fontIDFromFullName[buffer] = fontIDs[f];
							delete[] buffer;
						}
	
						CFRelease(nameStr); 
					}
					free(nameBuf);
					continue;
				}
				if (sizeRec.subFamilyID != 0) {
					if ((nameCode == kFontFamilyName) && (namePlatform == kFontMacintoshPlatform)
							&& (nameScript == kFontRomanScript) && (nameLang == kFontEnglishLanguage)) {
						familyName = (char*)xmalloc(nameLength + 1);
						status = ATSUGetIndFontName(fontIDs[f], n, nameLength, familyName, &nameLength,
													&nameCode, &namePlatform, &nameScript, &nameLang);
						familyName[nameLength] = 0;
						continue;
					}
					if ((nameCode == sizeRec.nameCode) && (namePlatform == kFontMacintoshPlatform)
							&& (nameScript == kFontRomanScript) && (nameLang == kFontEnglishLanguage)) {
						subFamilyName = (char*)xmalloc(nameLength + 1);
						status = ATSUGetIndFontName(fontIDs[f], n, nameLength, subFamilyName, &nameLength,
													&nameCode, &namePlatform, &nameScript, &nameLang);
						subFamilyName[nameLength] = 0;
						continue;
					}
				}
			}
		}

		if ((familyName != NULL) && (subFamilyName != NULL)) {
			std::string	familyStr(familyName);
			familyStr += " ";
			familyStr += subFamilyName;
			opticalFamily*	optFam = NULL;
			std::map<std::string,opticalFamily*>::iterator	fi = opticalFamilyFromFamilyName.find(familyStr);
			if (fi == opticalFamilyFromFamilyName.end()) {
				optFam = new opticalFamily;
				opticalFamilyFromFamilyName[familyStr] = optFam;
			}
			else
				optFam = fi->second;
			opticalFamilyFromFontID[fontIDs[f]] = optFam;
			familyMember	member = { sizeRec.subFamilyID, sizeRec.minSize, sizeRec.maxSize, fontIDs[f] };
			optFam->push_back(member);
			free(familyName);
			free(subFamilyName);
		}
	}
	free(fontIDs);
	
	// now we sort the family records, so that we can terminate searches early
	for (std::map<std::string,opticalFamily*>::iterator	fi = opticalFamilyFromFamilyName.begin(); fi != opticalFamilyFromFamilyName.end(); ++fi) {
		opticalFamily*	optFam = fi->second;
		std::sort(optFam->begin(), optFam->end());
	}
	
	// and now we can discard the opticalFamilyFromFamilyName map, we'll access the records by opticalFamilyFromFontID
}

ATSUFontID
find_font_by_name(CFStringRef fontNameStr, double pointSize)
{
	if (fontDictsInitialized == false)
		init_font_dicts();

	ATSUFontID	fontID = kATSUInvalidFontID;
	
	preferredTech = kEngineAuto;

	bool	updateName = false;

	FMFontStyle	reqStyle = 0;
	CFStringRef	baseNameStr;
	CFRange	slashRange = CFStringFind(fontNameStr, CFSTR("/"), 0);
	if (slashRange.location != kCFNotFound) {
		updateName = true;
		baseNameStr = CFStringCreateWithSubstring(kCFAllocatorDefault, fontNameStr, CFRangeMake(0, slashRange.location));
		CFIndex i;
		CFIndex	loc = slashRange.location + 1;
		for (i = loc; i < CFStringGetLength(fontNameStr); ++i) {
			if ((i == loc) && (i + 3 <= CFStringGetLength(fontNameStr))) {
				CFStringRef	str3 = CFStringCreateWithSubstring(kCFAllocatorDefault, fontNameStr, CFRangeMake(i, 3));
				if (CFStringCompare(str3, CFSTR("AAT"), kCFCompareCaseInsensitive) == 0) {
					preferredTech = kEngineAAT;
					i += 2;
					CFRelease(str3);
					continue;
				}
				else if (CFStringCompare(str3, CFSTR("ICU"), kCFCompareCaseInsensitive) == 0) {
					preferredTech = kEngineICU;
					i += 2;
					CFRelease(str3);
					continue;
				}
			}
			UniChar	c = CFStringGetCharacterAtIndex(fontNameStr, i);
			if (c == 'B')
				reqStyle |= bold;
			else if (c == 'I')
				reqStyle |= italic;
			else if (c == 'S') {
				double	reqSize = 0.0;
				c = CFStringGetCharacterAtIndex(fontNameStr, ++i);
				if (c == '=')
					c = CFStringGetCharacterAtIndex(fontNameStr, ++i);
				while ((c >= '0') && (c <= '9')) {
					reqSize = reqSize * 10 + c - '0';
					c = CFStringGetCharacterAtIndex(fontNameStr, ++i);
				}
				if (c == '.') {
					c = CFStringGetCharacterAtIndex(fontNameStr, ++i);
					double	dec = 1.0;
					while ((c >= '0') && (c <= '9')) {
						dec = dec * 10.0;
						reqSize = reqSize + (c - '0') / dec;
						c = CFStringGetCharacterAtIndex(fontNameStr, ++i);
					}
				}
				pointSize = reqSize;
			}
			else if (c == '/')
				loc = i + 1;
		}
	}
	else
		baseNameStr = CFStringCreateCopy(kCFAllocatorDefault, fontNameStr);

	std::map<std::string,ATSUFontID>::iterator	entry;
	if (const char *s = CFStringGetCStringPtr(baseNameStr, kCFStringEncodingUTF8))
		entry = fontIDFromFullName.find(s);
	else {
		CFIndex length = CFStringGetLength(baseNameStr);
		char *buffer = new char[6 * length + 1];
		if (CFStringGetCString(baseNameStr, buffer, 6 * length + 1, kCFStringEncodingUTF8))
			entry = fontIDFromFullName.find(buffer);
		delete[] buffer;
	}
	if (entry != fontIDFromFullName.end())
		fontID = entry->second;

	CFRelease(baseNameStr);
	
	if (reqStyle != 0) {
		FMFontFamily	family; 
		FMFontStyle		style;
		FMGetFontFamilyInstanceFromFont(fontID, &family, &style);

		FMFont	styledFontID;
		FMGetFontFromFontFamilyInstance(family, reqStyle, &fontID, &style);
		updateName = true;
	}
	
	if (pointSize != 0.0) {
		ATSUFontID	sizedFontID = get_optically_sized_font(fontID, pointSize);
		if (sizedFontID != fontID) {
			updateName = true;
			fontID = sizedFontID;
		}
	}
	
	if (updateName) {
		ItemCount	nameIndex;
		ByteCount	nameLen;
		OSStatus	status;
		do {
			status = ATSUFindFontName(fontID, kFontFullName, kFontUnicodePlatform, kFontRomanScript, kFontNoLanguageCode, 0, 0, &nameLen, &nameIndex);
			if (status == noErr) break;
			status = ATSUFindFontName(fontID, kFontFullName, kFontMicrosoftPlatform, kFontRomanScript, kFontNoLanguageCode, 0, 0, &nameLen, &nameIndex);
			if (status == noErr) break;
			status = ATSUFindFontName(fontID, kFontFullName, kFontUnicodePlatform, kFontNoScriptCode, kFontNoLanguageCode, 0, 0, &nameLen, &nameIndex);
			if (status == noErr) break;
			status = ATSUFindFontName(fontID, kFontFullName, kFontMicrosoftPlatform, kFontNoScriptCode, kFontNoLanguageCode, 0, 0, &nameLen, &nameIndex);
			if (status == noErr) break;
			status = ATSUFindFontName(fontID, kFontFullName, kFontMacintoshPlatform, kFontNoScriptCode, kFontNoLanguageCode, 0, 0, &nameLen, &nameIndex);
			if (status == noErr) break;
		} while (0);

		if (status == noErr) {
			char*	nameBuf = (char*)xmalloc(nameLen);
			FontPlatformCode	platform;
			FontScriptCode		script;
			FontLanguageCode	language;
			ATSUGetIndFontName(fontID, nameIndex, nameLen, nameBuf, 0, 0, &platform, &script, &language);

			TextEncoding		encoding;
			CFStringRef			styledNameStr = 0;
			if (platform == kFontMacintoshPlatform) {
				GetTextEncodingFromScriptInfo(script, language, 0, &encoding);
				styledNameStr = CFStringCreateWithBytes(0, (UInt8*)nameBuf, nameLen, encoding, false);
			}
			else if ((platform == kFontUnicodePlatform) || (platform == kFontMicrosoftPlatform))
				styledNameStr = CFStringCreateWithCharacters(0, (UniChar*)nameBuf, nameLen / 2);

			if (styledNameStr != 0) {
				// we found a different font, so change the UTF8 name in nameoffile....
				char*	cp = nameoffile + 1;
				while (cp <= nameoffile + namelength)
					if (*cp == ':')
						break;
					else
						++cp;

				int	nameBufSize = (nameLen * 3 / 2) + (namelength - (cp - nameoffile)) + 10;
				CFIndex	usedBufLen;
				char*	newName = (char*)xmalloc(nameBufSize);
				CFStringGetBytes(styledNameStr, CFRangeMake(0, CFStringGetLength(styledNameStr)),
									kCFStringEncodingUTF8, 0, false, (UInt8*)newName + 1, nameBufSize, &usedBufLen);

				char*	cp2 = newName + 1 + usedBufLen;
				while (cp <= nameoffile + namelength)
					*cp2++ = *cp++;
				namelength = cp2 - newName - 1;
				free(nameoffile);
				nameoffile = newName;
				CFRelease(styledNameStr);
			}
			free(nameBuf);
		}
		
	}

	
	return fontID;
}
