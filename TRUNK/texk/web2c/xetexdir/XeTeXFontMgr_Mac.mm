#include "XeTeXFontMgr_Mac.h"

#ifdef XETEX_MAC // this is currently an ATSUI-based implementation

#define FONTMGR_ATS		1
#define FONTMGR_FM		2
#define FONTMGR_COCOA	3

#define FONTMGR		FONTMGR_COCOA

XeTeXFontMgr::NameCollection*
XeTeXFontMgr_Mac::readNames(ATSFontRef fontRef)
{
	const int	BUFUNIT = 256;
	static int		bufSize = BUFUNIT;
	static char*	buffer = new char[bufSize];

	NameCollection*	names = new NameCollection;

	CFStringRef	psName;
	OSStatus	status = ATSFontGetPostScriptName(fontRef, kATSOptionFlagsDefault, &psName);
	if (status != noErr)
		return names;
	
	// extract UTF-8 form of name
	CFIndex length = CFStringGetLength(psName);	// in 16-bit character units
	if (length > 0) {
		length = length * 6 + 1;
		if (length >= bufSize) {
			delete[] buffer;
			bufSize = ((length / BUFUNIT) + 1) * BUFUNIT;
			buffer = new char[bufSize];
		}
		if (CFStringGetCString(psName, buffer, bufSize, kCFStringEncodingUTF8))
			names->psName = buffer;
	}
	CFRelease(psName); 

	ATSUFontID	fontID = FMGetFontFromATSFontRef(fontRef);

	ItemCount	nameCount;
	status = ATSUCountFontNames(fontID, &nameCount);
	if (status != noErr)
		die("ATSUCountFontNames failed, status=%d", status);

	std::list<std::string>	familyNames;
	std::list<std::string>	subFamilyNames;

	for (int n = 0; n < nameCount; ++n) {
		ByteCount			nameLength;
		FontNameCode		nameCode;
		FontPlatformCode	namePlatform;
		FontScriptCode		nameScript;
		FontLanguageCode	nameLang;
		status = ATSUGetIndFontName(fontID, n, 0, 0, &nameLength,
									&nameCode, &namePlatform, &nameScript, &nameLang);
		if (status != noErr && status != kATSUNoFontNameErr)
			die("ATSUGetIndFontName failed, status=%d", status);
		if (status == noErr && nameLength > 0) {
			switch (nameCode) {
				case kFontFullName:
				case kFontFamilyName:
				case kFontStyleName:
				case 16:	// preferred family -- use instead of family, if present
				case 17:	// preferred subfamily -- use instead of style, if present
					{
						bool	preferredName = false;
						TextEncoding	encoding;
						CFStringRef		nameStr = 0;
						if (nameLength >= bufSize) {
							delete[] buffer;
							bufSize = ((nameLength / BUFUNIT) + 1) * BUFUNIT;
							buffer = new char[bufSize];
						}
						status = ATSUGetIndFontName(fontID, n, bufSize, buffer, &nameLength,
													&nameCode, &namePlatform, &nameScript, &nameLang);
						if (namePlatform == kFontMacintoshPlatform) {
							GetTextEncodingFromScriptInfo(nameScript, nameLang, 0, &encoding);
							nameStr = CFStringCreateWithBytes(0, (UInt8*)buffer, nameLength, encoding, false);
							if (nameScript == kFontRomanScript && nameLang == kFontEnglishLanguage)
								preferredName = true;
						}
						else if ((namePlatform == kFontUnicodePlatform) || (namePlatform == kFontMicrosoftPlatform))
							nameStr = CFStringCreateWithCharacters(0, (UniChar*)buffer, nameLength / 2);
						if (nameStr != 0) {
							std::list<std::string>*	nameList = NULL;
							switch (nameCode) {
								case kFontFullName:
									nameList = &names->fullNames;
									break;
								case kFontFamilyName:
									nameList = &names->familyNames;
									break;
								case kFontStyleName:
									nameList = &names->styleNames;
									break;
								case 16:
									nameList = &familyNames;
									break;
								case 17:
									nameList = &subFamilyNames;
									break;
							}
							
							// extract UTF-8 form of name
							length = CFStringGetLength(nameStr);	// in 16-bit character units
							if (length > 0) {
								length = length * 6 + 1;
								if (length >= bufSize) {
									delete[] buffer;
									bufSize = ((length / BUFUNIT) + 1) * BUFUNIT;
									buffer = new char[bufSize];
								}
								if (CFStringGetCString(nameStr, buffer, bufSize, kCFStringEncodingUTF8)) {
									if (buffer[0] != 0) {
										if (nameList != NULL) {
											if (preferredName)
												prependToList(nameList, buffer);
											else
												appendToList(nameList, buffer);
										}
									}
								}
							}
		
							CFRelease(nameStr); 
						}
					}
					break;
			}
		}
	}

	if (familyNames.size() > 0)
		names->familyNames = familyNames;
	if (subFamilyNames.size() > 0)
		names->styleNames = subFamilyNames;

	return names;
}

#if FONTMGR == FONTMGR_COCOA
#include <Cocoa/Cocoa.h>
#endif

void
XeTeXFontMgr_Mac::addFamilyToCaches(ATSFontFamilyRef familyRef)
{
#if FONTMGR == FONTMGR_ATS
/* This doesn't work.... the documentation is confused and the API is broken.
   So we end up caching all the fonts of all families on first access. Yuck.

	ATSFontFilter	filter;
	filter.version = kATSFontFilterCurrentVersion;
	filter.filterSelector = kATSFontFilterSelectorFontFamily;
	filter.filter.fontFamilyFilter = familyRef;
*/
	ATSFontIterator	iterator;
	OSStatus	status = ATSFontIteratorCreate(kATSFontContextGlobal, /*&filter*/ NULL, NULL, kATSOptionFlagsUnRestrictedScope, &iterator);
	if (status == noErr) {
		while (status == noErr) {
			ATSFontRef	fontRef;
			status = ATSFontIteratorNext(iterator, &fontRef);
			if (status == kATSIterationScopeModified) {
				status = ATSFontIteratorReset(kATSFontContextGlobal, /*&filter*/ NULL, NULL, kATSOptionFlagsUnRestrictedScope, &iterator);
				if (status == noErr)
					continue;
			}
			if (status != noErr)
				break;
			NameCollection*	names = readNames(fontRef);
			addToMaps(fontRef, names);
			delete names;
		}
		status = ATSFontIteratorRelease(&iterator);
	}
#endif

#if FONTMGR == FONTMGR_COCOA
	CFStringRef	nameStr;
	OSStatus	status = ATSFontFamilyGetName(familyRef, kATSOptionFlagsDefault, &nameStr);
	if (status == noErr) {
		NSArray*	members = [[NSFontManager sharedFontManager]
								availableMembersOfFontFamily: (NSString*)nameStr];
		CFRelease(nameStr);
		NSEnumerator*	enumerator = [members objectEnumerator];
		while (id aMember = [enumerator nextObject]) {
			ATSFontRef	fontRef = ATSFontFindFromPostScriptName((CFStringRef)[aMember objectAtIndex: 0], kATSOptionFlagsDefault);
			NameCollection*	names = readNames(fontRef);
			addToMaps(fontRef, names);
			delete names;
		}
	}
#endif

#if FONTMGR == FONTMGR_FM
/* unfortunately, this much faster version only works for FOND-based families */
	FMFontFamily	fam = FMGetFontFamilyFromATSFontFamilyRef(familyRef);
	FMFontFamilyInstanceIterator	iter;
	OSStatus status = FMCreateFontFamilyInstanceIterator(fam, &iter); 
	if (status == noErr) {
		while (1) {
			FMFont		font;
			FMFontStyle	style;
			FMFontSize	size;
			status = FMGetNextFontFamilyInstance(&iter, &font, &style, &size);
/*
			if (status == kFMIteratorScopeModified) {
				status = FMResetFontFamilyInstanceIterator(fam, &iter);
				if (status == noErr)
					continue;
			}
*/
			if (status != noErr)
				break;
			ATSFontRef	fontRef = FMGetATSFontRefFromFont(font);
			NameCollection*	names = readNames(fontRef);
			addToMaps(fontRef, names);
			delete names;
		}
		status = FMDisposeFontFamilyInstanceIterator(&iter);
	}
#endif
}

void
XeTeXFontMgr_Mac::addFontAndSiblingsToCaches(ATSFontRef fontRef)
{
#if (FONTMGR == FONTMGR_ATS) || (FONTMGR == FONTMGR_FM)
//	ATSFontFamilyRef	familyRef = ATSFontFamilyFromFont(fontRef);
		// GRRR... this function (or equiv) doesn't exist!!!

	FMFontFamily	fontFamily;
	FMFontStyle		style;
	OSStatus	status = FMGetFontFamilyInstanceFromFont(FMGetFontFromATSFontRef(fontRef), &fontFamily, &style);
	if (status == noErr)
		addFamilyToCaches(FMGetATSFontFamilyRefFromFontFamily(fontFamily));
#endif
#if FONTMGR == FONTMGR_COCOA
	CFStringRef	name;
	OSStatus	status = ATSFontGetPostScriptName(fontRef, kATSOptionFlagsDefault, &name);
	if (status == noErr) {
		NSFont*	font = [NSFont fontWithName:(NSString*)name size:10.0];
		CFRelease(name);
		NSArray*	members = [[NSFontManager sharedFontManager]
								availableMembersOfFontFamily: [font familyName]];
		NSEnumerator*	enumerator = [members objectEnumerator];
		while (id aMember = [enumerator nextObject]) {
			ATSFontRef	fontRef = ATSFontFindFromPostScriptName((CFStringRef)[aMember objectAtIndex: 0], kATSOptionFlagsDefault);
			NameCollection*	names = readNames(fontRef);
			addToMaps(fontRef, names);
			delete names;
		}
	}
#endif
}

void
XeTeXFontMgr_Mac::searchForHostPlatformFonts(const std::string& name)
{
	// the name might be:
	//	FullName
	//	Family-Style (if there's a hyphen)
	//	PSName
	//	Family
	// ...so we need to try it as each of these

	CFStringRef	nameStr = CFStringCreateWithCString(kCFAllocatorDefault, name.c_str(), kCFStringEncodingUTF8);
	ATSFontRef	fontRef = ATSFontFindFromName(nameStr, kATSOptionFlagsDefault);
	if (fontRef != kATSFontRefUnspecified) {
		// found it, so locate the family, and add all members to the caches
		addFontAndSiblingsToCaches(fontRef);
		return;
	}

	int	hyph = name.find('-');
	if (hyph > 0 && hyph < name.length() - 1) {
		std::string			family(name.begin(), name.begin() + hyph);
		CFStringRef			familyStr = CFStringCreateWithCString(kCFAllocatorDefault, family.c_str(), kCFStringEncodingUTF8);
		ATSFontFamilyRef	familyRef = ATSFontFamilyFindFromName(familyStr, kATSOptionFlagsDefault);
		if (familyRef != 0xffffffff) {
			addFamilyToCaches(familyRef);
			return;
		}
	}
	
	fontRef = ATSFontFindFromPostScriptName(nameStr, kATSOptionFlagsDefault);
	if (fontRef != kATSFontRefUnspecified) {
		addFontAndSiblingsToCaches(fontRef);
		return;
	}

	ATSFontFamilyRef	familyRef = ATSFontFamilyFindFromName(nameStr, kATSOptionFlagsDefault);
	if (familyRef != 0xffffffff) {
		addFamilyToCaches(familyRef);
		return;
	}
}

#if FONTMGR == FONTMGR_COCOA
NSAutoreleasePool* pool = NULL;
#endif

void
XeTeXFontMgr_Mac::initialize()
{
#if FONTMGR == FONTMGR_COCOA
	pool = [[NSAutoreleasePool alloc] init];
//	[NSApplication sharedApplication];
#endif
}

void
XeTeXFontMgr_Mac::terminate()
{
#if FONTMGR == FONTMGR_COCOA
	if (pool != NULL) {
		[pool release];
	}
#endif
}

#endif // XETEX_MAC
