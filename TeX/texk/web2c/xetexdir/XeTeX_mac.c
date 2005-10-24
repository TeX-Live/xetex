/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

/* XeTeX_mac.c
 * additional plain C extensions for XeTeX - MacOS-specific routines
 */

#define EXTERN extern
#include "xetexd.h"

#undef input /* this is defined in texmfmp.h, but we don't need it and it confuses the carbon headers */
#include <Carbon/Carbon.h>
#include <Quicktime/Quicktime.h>

#include "XeTeX_ext.h"


static ATSUFontVariationAxis
find_axis_by_name(ATSUFontID fontID, const char* name, int nameLength)
{
	ATSUFontVariationAxis	result = 0;

	Str255	inName;
	inName[0] = nameLength;
	int i;
	for (i = 0; i < inName[0]; ++i)
		inName[i + 1] = name[i];

	ItemCount	varCount;
	ATSUCountFontVariations(fontID, &varCount);
	if (varCount > 0) {
		for (i = 0; i < varCount; ++i) {
			ATSUFontVariationAxis	axis;
			ATSUGetIndFontVariation(fontID, i, &axis, 0, 0, 0);
			FontNameCode	nameCode;
			ATSUGetFontVariationNameCode(fontID, axis, &nameCode);
			Str255		name;
			ByteCount	nameLen;
			ATSUFindFontName(fontID, nameCode, kFontMacintoshPlatform, kFontNoScript, kFontNoLanguage,
                                 255, (Ptr)&name[1], &nameLen, 0);
			name[0] = nameLen;
			if (EqualString(inName, name, false, true)) {
				result = axis;
				break;
			}
		}
	}
	
	return result;
}

static ATSUFontFeatureType
find_feature_by_name(ATSUFontID fontID, const char* name, int nameLength)
{
	ATSUFontFeatureType	result = 0x0000FFFF;

	Str255	inName;
	inName[0] = nameLength;
	int i;
	for (i = 0; i < inName[0]; ++i)
		inName[i + 1] = name[i];

	ItemCount	typeCount;
	ATSUCountFontFeatureTypes(fontID, &typeCount);
	if (typeCount > 0) {
		ATSUFontFeatureType*	types = (ATSUFontFeatureType*)xmalloc(typeCount * sizeof(ATSUFontFeatureType));
		ATSUGetFontFeatureTypes(fontID, typeCount, types, 0);
		for (i = 0; i < typeCount; ++i) {
			FontNameCode	nameCode;
			ATSUGetFontFeatureNameCode(fontID, types[i], 0x0000FFFF, &nameCode);
			Str255		name;
			ByteCount	nameLen;
			ATSUFindFontName(fontID, nameCode, kFontMacintoshPlatform, kFontNoScript, kFontNoLanguage,
                                 255, (Ptr)&name[1], &nameLen, 0);
			name[0] = nameLen;
			if (EqualString(inName, name, false, true)) {
				result = types[i];
				break;
			}
		}
		free((char*)types);
	}
	
	return result;
}

static ATSUFontFeatureSelector
find_selector_by_name(ATSUFontID fontID, ATSUFontFeatureType featureType, const char* name, int nameLength)
{
	ATSUFontFeatureSelector	result = 0x0000FFFF;

	Str255	inName;
	inName[0] = nameLength;
	int i;
	for (i = 0; i < inName[0]; ++i)
		inName[i + 1] = name[i];

	ItemCount	selectorCount;
	ATSUCountFontFeatureSelectors(fontID, featureType, &selectorCount);
	if (selectorCount > 0) {
		ATSUFontFeatureSelector*	selectors = (ATSUFontFeatureSelector*)xmalloc(selectorCount * sizeof(ATSUFontFeatureSelector));
		ATSUGetFontFeatureSelectors(fontID, featureType, selectorCount, selectors, 0, 0, 0);
		for (i = 0; i < selectorCount; ++i) {
			FontNameCode	nameCode;
			ATSUGetFontFeatureNameCode(fontID, featureType, selectors[i], &nameCode);
			Str255		name;
			ByteCount	nameLen;
			ATSUFindFontName(fontID, nameCode, kFontMacintoshPlatform, kFontNoScript, kFontNoLanguage,
                                 255, (Ptr)&name[1], &nameLen, 0);
			name[0] = nameLen;
			if (EqualString(inName, name, false, true)) {
				result = selectors[i];
				break;
			}
		}
		free((char*)selectors);
	}
	
	return result;
}

long
loadAATfont(ATSUFontID fontID, long scaled_size, const char* name, int nameLen, const char* cp1)
{
	ATSUStyle	style = 0;
	OSStatus	status = ATSUCreateStyle(&style);
	if (status == noErr) {
		bool			colorSpecified = false;
		unsigned long	rgbValue;
		
		ATSUAttributeTag		tag[2] = { kATSUFontTag, kATSUSizeTag };
		ByteCount				valueSize[2] = { sizeof(ATSUFontID), sizeof(Fixed) };
		ATSUAttributeValuePtr	value[2];
		value[0] = &fontID;
		value[1] = &scaled_size;
		ATSUSetAttributes(style, 2, &tag[0], &valueSize[0], &value[0]);
		
		// FIXME: need to dynamically grow these arrays
		UInt16*	featureTypes = (UInt16*)xmalloc(200);
		UInt16*	selectorValues = (UInt16*)xmalloc(200);
		int	numFeatures = 0;
		
		UInt32*	axes = (UInt32*)xmalloc(200);
		SInt32*	values = (SInt32*)xmalloc(200);
		int	numVariations = 0;
		
		// interpret features & variations following ":"
		while (*cp1) {

			// locate beginning of name=value pair
			if (*cp1 == ':' || *cp1 == ';')	// skip over separator
				++cp1;
			while (*cp1 == ' ' || *cp1 == '\t')	// skip leading whitespace
				++cp1;
			if (*cp1 == 0)	// break if end of string
				break;

			// scan to end of pair
			const char*	cp2 = cp1;
			while (*cp2 && *cp2 != ';' && *cp2 != ':')
				++cp2;

			// look for the '=' separator
			const char*	cp3 = cp1;
			while (cp3 < cp2 && *cp3 != '=')
				++cp3;
			if (cp3 == cp2)
				goto bad_option;

			// now cp1 points to option name, cp3 to '=', cp2 to ';' or null
			
			// first try for a feature by this name
			ATSUFontFeatureType	featureType;
			featureType = find_feature_by_name(fontID, cp1, cp3 - cp1);
			if (featureType != 0x0000FFFF) {
				// look past the '=' separator for setting names
				int	featLen = cp3 - cp1;
				++cp3;
				while (cp3 < cp2) {
					// skip leading whitespace
					while (*cp3 == ' ' || *cp3 == '\t')
						++cp3;
				
					// possibly multiple settings...
					int	disable = 0;
					if (*cp3 == '!') {	// check for negation
						disable = 1;
						++cp3;
					}
					
					// scan for end of setting name
					const char*	cp4 = cp3;
					while (cp4 < cp2 && *cp4 != ',')
						++cp4;
					
					// now cp3 points to name, cp4 to ',' or ';' or null
					ATSUFontFeatureSelector	selectorValue = find_selector_by_name(fontID, featureType, cp3, cp4 - cp3);
					if (selectorValue != 0x0000FFFF) {
						featureTypes[numFeatures] = featureType;
						selectorValues[numFeatures] = selectorValue + disable;
						++numFeatures;
					}
					else {
						fontfeaturewarning(name, nameLen, cp1, featLen, cp3, cp4 - cp3);
					}
					
					// point beyond setting name terminator
					cp3 = cp4 + 1;
				}
				
				goto next_option;
			}
			
			// try to find a variation by this name
			ATSUFontVariationAxis	axis;
			axis = find_axis_by_name(fontID, cp1, cp3 - cp1);
			if (axis != 0) {
				// look past the '=' separator for the value
				++cp3;
				double	value = 0.0, decimal = 1.0;
				bool		negate = false;
				if (*cp3 == '-') {
					++cp3;
					negate = true;
				}
				while (cp3 < cp2) {
					int	v = *cp3 - '0';
					if (v >= 0 && v <= 9) {
						if (decimal != 1.0) {
							value += v / decimal;
							decimal *= 10.0;
						}
						else
							value = value * 10.0 + v;
					}
					else if (*cp3 == '.') {
						if (decimal != 1.0)
							break;
						decimal = 10.0;
					}
					else
						break;
					++cp3;
				}
				if (negate)
					value = -value;
				axes[numVariations] = axis;
				values[numVariations] = value * 65536.0;	//	X2Fix(value);
				++numVariations;
				
				goto next_option;
			}
			
			// didn't find feature or variation, try other options....

			if (strncmp(cp1, "mapping", 7) == 0) {
				cp3 = cp1 + 7;
				if (*cp3 != '=')
					goto bad_option;
				loadedfontmapping = (long)load_mapping_file(cp3 + 1, cp2);
				goto next_option;
			}
			
			if (strncmp(cp1, "color", 5) == 0) {
				cp3 = cp1 + 5;
				if (*cp3 != '=')
					goto bad_option;
				++cp3;
				rgbValue = 0;
				unsigned	alpha = 0;
				int i;
				for (i = 0; i < 6; ++i) {
					if (*cp3 >= '0' && *cp3 <= '9')
						rgbValue = (rgbValue << 4) + *cp3 - '0';
					else if (*cp3 >= 'A' && *cp3 <= 'F')
						rgbValue = (rgbValue << 4) + *cp3 - 'A' + 10;
					else if (*cp3 >= 'a' && *cp3 <= 'f')
						rgbValue = (rgbValue << 4) + *cp3 - 'a' + 10;
					else
						goto bad_option;
					++cp3;
				}
				rgbValue <<= 8;
				for (i = 0; i < 2; ++i) {
					if (*cp3 >= '0' && *cp3 <= '9')
						alpha = (alpha << 4) + *cp3 - '0';
					else if (*cp3 >= 'A' && *cp3 <= 'F')
						alpha = (alpha << 4) + *cp3 - 'A' + 10;
					else if (*cp3 >= 'a' && *cp3 <= 'f')
						alpha = (alpha << 4) + *cp3 - 'a' + 10;
					else
						break;
					++cp3;
				}
				if (i == 2)
					rgbValue += alpha;
				else
					rgbValue += 0xFF;
				colorSpecified = true;
				
				goto next_option;
			}
			
		bad_option:
			// not a name=value pair, or not recognized.... 
			// check for plain "vertical" before complaining
			if (strncmp(cp1, "vertical", 8) == 0) {
				cp3 = cp2;
				if (*cp3 == ';' || *cp3 == ':')
					--cp3;
				while (*cp3 == ' ' || *cp3 == '\t')
					--cp3;
				if (*cp3)
					++cp3;
				if (cp3 == cp1 + 8) {
					ATSUVerticalCharacterType	vert = kATSUStronglyVertical;
					tag[0] = kATSUVerticalCharacterTag;
					valueSize[0] = sizeof(ATSUVerticalCharacterType);
					value[0] = &vert;
					ATSUSetAttributes(style, 1, &tag[0], &valueSize[0], &value[0]);
					goto next_option;
				}
			}
		
			fontfeaturewarning(name, nameLen, cp1, cp2 - cp1, 0, 0);
			
		next_option:
			// go to next name=value pair
			cp1 = cp2;
		}
		
		if (numFeatures > 0)
			ATSUSetFontFeatures(style, numFeatures, featureTypes, selectorValues);

		if (numVariations > 0)
			ATSUSetVariations(style, numVariations, axes, values);

		if (colorSpecified) {
			ATSURGBAlphaColor	rgba;
			rgba.red	= ((rgbValue & 0xFF000000) >> 24) / 255.0;
			rgba.green	= ((rgbValue & 0x00FF0000) >> 16) / 255.0;
			rgba.blue	= ((rgbValue & 0x0000FF00) >> 8 ) / 255.0;
			rgba.alpha	= ((rgbValue & 0x000000FF)      ) / 255.0;
			tag[0] = kATSURGBAlphaColorTag;
			valueSize[0] = sizeof(ATSURGBAlphaColor);
			value[0] = &rgba;
			ATSUSetAttributes(style, 1, &tag[0], &valueSize[0], &value[0]);
		}
		
		free((char*)featureTypes);
		free((char*)selectorValues);
		free((char*)axes);
		free((char*)values);
	}

	nativefonttypeflag = AAT_FONT_FLAG;
	return (long)style;
}

long
atsufontget(int what, ATSUStyle style)
{
	long	rval = -1;

	ATSUFontID	fontID;
	ATSUGetAttribute(style, kATSUFontTag, sizeof(ATSUFontID), &fontID, 0);
	ItemCount	count;

	switch (what) {
		case XeTeX_count_glyphs:
			{
				ByteCount	tableSize;
				if (ATSFontGetTable(fontID, 'maxp', 0, 0, 0, &tableSize) == noErr) {
					Byte*	table = xmalloc(tableSize);
					ATSFontGetTable(fontID, 'maxp', 0, tableSize, table, 0);
					rval = *(unsigned short*)(table + 4);
					free(table);
				}
			}
			break;

		case XeTeX_count_variations:
			if (ATSUCountFontVariations(fontID, &count) == noErr)
				rval = count;
			break;

		case XeTeX_count_features:
			if (ATSUCountFontFeatureTypes(fontID, &count) == noErr)
				rval = count;
			break;
	}
	return rval;
}

long
atsufontget1(int what, ATSUStyle style, int param)
{
	long	rval = -1;

	ATSUFontID	fontID;
	ATSUGetAttribute(style, kATSUFontTag, sizeof(ATSUFontID), &fontID, 0);
	
	ATSUFontVariationAxis	axis;
	ATSUFontVariationValue	value;
	ItemCount	count;
	Boolean		exclusive;
	switch (what) {
		case XeTeX_variation:
			if (ATSUGetIndFontVariation(fontID, param, &axis, 0, 0, 0) == noErr)
				rval = axis;
			break;

		case XeTeX_variation_min:
		case XeTeX_variation_max:
		case XeTeX_variation_default:
			if (ATSUCountFontVariations(fontID, &count) == noErr)
				while (count-- > 0)
					if (ATSUGetIndFontVariation(fontID, count, &axis,
							(what == XeTeX_variation_min) ? &value : 0,
							(what == XeTeX_variation_max) ? &value : 0,
							(what == XeTeX_variation_default) ? &value : 0) == noErr)
						if (axis == param) {
							rval = value;
							break;
						}
			break;

		case XeTeX_feature_code:
			if (ATSUCountFontFeatureTypes(fontID, &count) == noErr) {
				if (param < count) {
					ATSUFontFeatureType*	types = xmalloc(count * sizeof(ATSUFontFeatureType));
					if (ATSUGetFontFeatureTypes(fontID, count, types, 0) == noErr)
						rval = types[param];
					free(types);
				}
			}
			break;

		case XeTeX_is_exclusive_feature:
			if (ATSUGetFontFeatureSelectors(fontID, param, 0, 0, 0, 0, &exclusive) == noErr)
				rval = exclusive ? 1 : 0;
			break;

		case XeTeX_count_selectors:
			if (ATSUCountFontFeatureSelectors(fontID, param, &count) == noErr)
				rval = count;
			break;
	}
	
	return rval;
}

long
atsufontget2(int what, ATSUStyle style, int param1, int param2)
{
	long	rval = -1;

	ATSUFontID	fontID;
	ATSUGetAttribute(style, kATSUFontTag, sizeof(ATSUFontID), &fontID, 0);

	ItemCount	count;
	if (ATSUCountFontFeatureSelectors(fontID, param1, &count) == noErr) {
		ATSUFontFeatureSelector*	selectors = xmalloc(count * sizeof(ATSUFontFeatureSelector));
		Boolean*					isDefault = xmalloc(count * sizeof(Boolean));
		if (ATSUGetFontFeatureSelectors(fontID, param1, count, selectors, isDefault, 0, 0) == noErr) {
			switch (what) {
				case XeTeX_selector_code:
					if (param2 < count)
							rval = selectors[param2];
					break;
					
				case XeTeX_is_default_selector:
					while (count-- > 0)
						if (selectors[count] == param2) {
							rval = isDefault[count] ? 1 : 0;
							break;
						}
					break;
			}
		}
		free(isDefault);
		free(selectors);
	}
	
	return rval;
}

long
atsufontgetnamed(int what, ATSUStyle style)
{
	long	rval = -1;
	ATSUFontID	fontID;
	ATSUGetAttribute(style, kATSUFontTag, sizeof(ATSUFontID), &fontID, 0);
	
	switch (what) {
		case XeTeX_find_variation_by_name:
			rval = find_axis_by_name(fontID, nameoffile + 1, namelength);
			if (rval == 0)
				rval = -1;
			break;
		
		case XeTeX_find_feature_by_name:
			rval = find_feature_by_name(fontID, nameoffile + 1, namelength);
			if (rval == 0x0000FFFF)
				rval = -1;
			break;
	}
	
	return rval;
}

long
atsufontgetnamed1(int what, ATSUStyle style, int param)
{
	long	rval = -1;
	ATSUFontID	fontID;
	ATSUGetAttribute(style, kATSUFontTag, sizeof(ATSUFontID), &fontID, 0);
	
	switch (what) {
		case XeTeX_find_selector_by_name:
			rval = find_selector_by_name(fontID, param, nameoffile + 1, namelength);
			if (rval == 0x0000FFFF)
				rval = -1;
			break;
	}
	
	return rval;
}

void
atsuprintfontname(int what, ATSUStyle style, int param1, int param2)
{
	ATSUFontID	fontID;
	ATSUGetAttribute(style, kATSUFontTag, sizeof(ATSUFontID), &fontID, 0);

	FontNameCode	code;
	OSStatus	status = 1;
	switch (what) {
		case XeTeX_variation_name:
			status = ATSUGetFontVariationNameCode(fontID, param1, &code);
			break;
			
		case XeTeX_feature_name:
			status = ATSUGetFontFeatureNameCode(fontID, param1, kATSUNoSelector, &code);
			break;
			
		case XeTeX_selector_name:
			status = ATSUGetFontFeatureNameCode(fontID, param1, param2, &code);
			break;
	}

	if (status == noErr) {
		ByteCount	len = 0;
		char		name[1024];
		do {
			if (ATSUFindFontName(fontID, code, kFontMacintoshPlatform, kFontRomanScript, kFontEnglishLanguage, 1024, name, &len, 0) == noErr) break;
			if (ATSUFindFontName(fontID, code, kFontMacintoshPlatform, kFontRomanScript, kFontNoLanguageCode, 1024, name, &len, 0) == noErr) break;
		} while (0);
		if (len > 0) {
			char*	cp = &name[0];
			while (len-- > 0)
				printchar(*(cp++));
		}
		else {
			do {
				if (ATSUFindFontName(fontID, code, kFontUnicodePlatform, kFontNoScriptCode, kFontEnglishLanguage, 1024, name, &len, 0) == noErr) break;
				if (ATSUFindFontName(fontID, code, kFontMicrosoftPlatform, kFontNoScriptCode, kFontEnglishLanguage, 1024, name, &len, 0) == noErr) break;
				if (ATSUFindFontName(fontID, code, kFontUnicodePlatform, kFontNoScriptCode, kFontNoLanguageCode, 1024, name, &len, 0) == noErr) break;
				if (ATSUFindFontName(fontID, code, kFontMicrosoftPlatform, kFontNoScriptCode, kFontNoLanguageCode, 1024, name, &len, 0) == noErr) break;
			} while (0);
			if (len > 0) {
				printchars((unsigned short*)(&name[0]), len / 2);
			}
		}
	}
}

OSErr
find_pic_file(Handle* picFileAlias, CGRect* bounds, int isPDF, int page)
{
	*picFileAlias = NULL;
	OSStatus	result = fnfErr;
    UInt8*		pic_path = kpse_find_file(nameoffile + 1, kpse_pict_format, 1);
	CFURLRef	picFileURL = NULL;
	if (pic_path) {
		picFileURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, pic_path, strlen(pic_path), false);
		free(pic_path);
	}

	if (picFileURL != NULL) {
		/* get an FSRef for the URL we constructed */
		FSRef	picFileRef;
		CFURLGetFSRef(picFileURL, &picFileRef);
		
		/* make an alias record, which we'll need to return assuming we find a usable picture */
		/* we do this up front as it is also a means of converting an FSRef to an FSSpec */
		result = FSNewAlias(0, &picFileRef, (AliasHandle*)picFileAlias);
		if (result == noErr) {
			FSSpec	picFileSpec;
			Boolean	wasChanged;
			ResolveAlias(0, (AliasHandle)*picFileAlias, &picFileSpec, &wasChanged);
		
			if (isPDF) {
				CGPDFDocumentRef	document = CGPDFDocumentCreateWithURL(picFileURL);
				if (document != NULL) {
					CGRect	mediaBox;
					int	nPages = CGPDFDocumentGetNumberOfPages(document);
					if (page < 0)
						page = nPages + 1 + page;
					if (page < 1)
						page = 1;
					if (page > nPages)
						page = nPages;
					*bounds = CGPDFDocumentGetMediaBox(document, page);
					CGPDFDocumentRelease(document);
				}
			}
			else {
				/* now that we have an FSSpec for the file (if it exists), we can try to import it as a pic */
				ComponentInstance	ci;
				result = GetGraphicsImporterForFile(&picFileSpec, &ci);
				if (result == noErr) {
					ImageDescriptionHandle	desc = NULL;
					result = GraphicsImportGetImageDescription(ci, &desc);
					bounds->origin.x = 0;
					bounds->origin.y = 0;
					bounds->size.width = (*desc)->width * 72.0 / Fix2X((*desc)->hRes);
					bounds->size.height = (*desc)->height * 72.0 / Fix2X((*desc)->vRes);
					DisposeHandle((Handle)desc);
					(void)CloseComponent(ci);
				}
			}
		}
		
		CFRelease(picFileURL);
	}

	/* if file not found or couldn't import it as a picture, toss the alias as we'll be returning an error */
	if (result != noErr) {
		if (*picFileAlias != 0) {
			DisposeHandle(*picFileAlias);
			*picFileAlias = 0;
		}
	}
	
	return result;
}


void
applyrotation(int rotation, Fixed* x_size, Fixed* y_size, Fixed* x_shift, Fixed* y_shift)
{
	CGAffineTransform	t = CGAffineTransformMakeRotation(rotation * M_PI / 180.0);
	CGPoint	ll = CGPointMake(65536, 65536);
	CGPoint	ur = CGPointMake(-65536, -65536);
	int	i;
	CGPoint	p[4];
	p[0] = CGPointMake(0, 0);
	p[1] = CGPointMake(Fix2X(*x_size), 0);
	p[2] = CGPointMake(p[1].x, Fix2X(*y_size));
	p[3] = CGPointMake(0, p[2].y);
	for (i = 0; i < 4; ++i)
		p[i] = CGPointApplyAffineTransform(p[i], t);
	for (i = 0; i < 4; ++i) {
		if (p[i].x < ll.x)	ll.x = p[i].x;
		if (p[i].x > ur.x)	ur.x = p[i].x;
		if (p[i].y < ll.y)	ll.y = p[i].y;
		if (p[i].y > ur.y)	ur.y = p[i].y;
	}
	*x_size = X2Fix(ur.x - ll.x);
	*y_size = X2Fix(ur.y - ll.y);
	*x_shift -= X2Fix(ll.x);
	*y_shift -= X2Fix(ll.y);
}
