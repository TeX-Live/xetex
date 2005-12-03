#include "XeTeXFontMgr.h"

#ifdef XETEX_MAC // this is currently an ATSUI-based implementation

XeTeXFontMgr::NameCollection*
XeTeXFontMgr::readNames(ATSUFontID fontID)
{
	NameCollection*	names = new NameCollection;

	ItemCount	nameCount;
	OSStatus	status = ATSUCountFontNames(fontID, &nameCount);
	if (status != noErr)
		die("ATSUCountFontNames failed, status=%d", status);

	const int	BUFUNIT = 256;
	int			bufSize = BUFUNIT;
	char*		buffer = new char[bufSize];

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
				case kFontPostscriptName:
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
							CFIndex length = CFStringGetLength(nameStr);	// in 16-bit character units
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
										else if (names->psName.length() == 0)
											names->psName = buffer;
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

	delete[] buffer;

	if (familyNames.size() > 0)
		names->familyNames = familyNames;
	if (subFamilyNames.size() > 0)
		names->styleNames = subFamilyNames;

	return names;
}

void
XeTeXFontMgr::buildFontMaps()
{
	// initialize the nameToFont, nameToFamily, and platformRefToFont maps
	// with all fonts available on the platform

	OSStatus	status;
	ItemCount	fontCount;
	status = ATSUFontCount(&fontCount);
	if (status != noErr)
		die("ATSUFontCount failed, status=%d", status);

	ATSUFontID*	fontIDs = new ATSUFontID[fontCount];
	status = ATSUGetFontIDs(fontIDs, fontCount, &fontCount);
	if (status != noErr)
		die("ATSUGetFontIDs failed, status=%d", status);

	for (int f = 0; f < fontCount; ++f) {
		PlatformFontRef	platformFont = FMGetATSFontRefFromFont(fontIDs[f]);
		NameCollection*	names = readNames(fontIDs[f]);

		addToMaps(platformFont, names);

		delete names;
	}

	delete[] fontIDs;
}

#endif // XETEX_MAC
