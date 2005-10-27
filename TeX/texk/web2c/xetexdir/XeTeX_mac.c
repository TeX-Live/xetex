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

#include "TECkit_Engine.h"
#include "XeTeX_ext.h"

TECkit_Converter	load_mapping_file(const char* s, const char* e);


ATSUFontVariationAxis
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

ATSUFontFeatureType
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

ATSUFontFeatureSelector
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

OSErr
find_pic_file(char** path, realrect* bounds, int isPDF, int page)
{
	*path = NULL;

	OSStatus	result = fnfErr;
    UInt8*		pic_path = kpse_find_file(nameoffile + 1, kpse_pict_format, 1);
	CFURLRef	picFileURL = NULL;
	if (pic_path) {
		picFileURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, pic_path, strlen(pic_path), false);

		if (picFileURL != NULL) {
			/* get an FSRef for the URL we constructed */
			FSRef	picFileRef;
			CFURLGetFSRef(picFileURL, &picFileRef);
			
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
					CGRect r = CGPDFDocumentGetMediaBox(document, page);
					bounds->x = r.origin.x;
					bounds->y = r.origin.y;
					bounds->wd = r.size.width;
					bounds->ht = r.size.height;
					CGPDFDocumentRelease(document);
				}
			}
			else {
				/* make an FSSpec for the file, and try to import it as a pic */
				FSSpec	picFileSpec;
				result = FSGetCatalogInfo(&picFileRef, kFSCatInfoNone, NULL, NULL, &picFileSpec, NULL);
				if (result == noErr) {
					ComponentInstance	ci;
					result = GetGraphicsImporterForFile(&picFileSpec, &ci);
					if (result == noErr) {
						ImageDescriptionHandle	desc = NULL;
						result = GraphicsImportGetImageDescription(ci, &desc);
						bounds->x = 0;
						bounds->y = 0;
						bounds->wd = (*desc)->width * 72.0 / Fix2X((*desc)->hRes);
						bounds->ht = (*desc)->height * 72.0 / Fix2X((*desc)->vRes);
						DisposeHandle((Handle)desc);
						(void)CloseComponent(ci);
					}
				}
			}
			
			CFRelease(picFileURL);
		}

		/* if we couldn't import it, toss the pathname as we'll be returning an error */
		if (result != noErr)
			free(pic_path);
		else
			*path = (char*)pic_path;
	}
	
	return result;
}

/*
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
*/
