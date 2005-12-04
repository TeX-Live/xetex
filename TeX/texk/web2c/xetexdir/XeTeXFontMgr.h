/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

#ifndef __XETEX_FONT_MANAGER_H
#define __XETEX_FONT_MANAGER_H

#include "XeTeX_ext.h"

#ifdef XETEX_MAC
#include <Carbon/Carbon.h>
typedef ATSFontRef	PlatformFontRef;
#else
#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H
typedef FcPattern*	PlatformFontRef;
#endif

#ifdef __cplusplus	/* allow inclusion in plain C files just to get the typedefs above */

#include <string>
#include <map>
#include <list>
#include <vector>

class XeTeXFontMgr
{
public:
	static XeTeXFontMgr*			GetFontManager();
		// returns the global fontmanager (creating it if necessary)

	PlatformFontRef					findFont(const char* name, const char* variant, double ptSize);
		// 1st arg is name as specified by user (C string, UTF-8)
		// 2nd is /B/I/AAT/ICU[/USP]/S=## qualifiers
		// 1. try name given as "full name"
		// 2. if there's a hyphen, split and try "family-style"
		// 3. try as PostScript name
		// 4. try name as family with "Regular/Plain/Normal" style
		// apply style qualifiers and optical sizing if present

		// SIDE EFFECT: sets sReqEngine to 'A' or 'I' [or 'U'] if appropriate,
		//   else clears it to 0

		// ???
		// SIDE EFFECT: may update TeX variables /nameoffile/ and /namelength/,
		//   to match the actual font found
		// ???
		
	const char*						getPSName(PlatformFontRef font) const;
		// return PostScript name of a font, suitable for use in the .xdv file

	const char*						getFullName(PlatformFontRef font) const;
		// return the full name of the font, suitable for use in XeTeX source
		// without requiring style qualifiers

	char							getReqEngine() const;
		// return the requested rendering technology for the most recent findFont
		// or 0 if no specific technology was requested

protected:
	static XeTeXFontMgr*			sFontManager;
	static char						sReqEngine;
	
									XeTeXFontMgr()
										{ }
	virtual							~XeTeXFontMgr()
										{ }
										
	void							buildFontMaps();

	class Font;
	class Family;

	struct OpSizeRec {
		unsigned short	designSize;
		unsigned short	subFamilyID;
		unsigned short	nameCode;
		unsigned short	minSize;
		unsigned short	maxSize;
	};

	class Font {
		public:
							Font(PlatformFontRef ref)
								: fullName(NULL), psName(NULL), parent(NULL)
								, fontRef(ref), weight(0), width(0), isBold(false), isItalic(false)
								{ opSizeInfo.subFamilyID = 0; }
							~Font()
								{ delete fullName; delete psName; }

			std::string*	fullName;
			std::string*	psName;
			Family*			parent;
			PlatformFontRef	fontRef;
			OpSizeRec		opSizeInfo;
			UInt16			weight;
			UInt16			width;
			bool			isReg;
			bool			isBold;
			bool			isItalic;
	};
	
	class Family {
		public:
											Family()
												{
													styles = new std::map<std::string,Font*>;
												}
											~Family()
												{
													delete styles;
												}

			std::map<std::string,Font*>*	styles;
	};

	class NameCollection {
	public:
		std::list<std::string>	familyNames;
		std::list<std::string>	styleNames;
		std::list<std::string>	fullNames;
		std::string				psName;
		std::string				subFamily;
	};	

	std::map<std::string,Font*>					nameToFont;						// maps full name (as used in TeX source) to font record
	std::map<std::string,Family*>				nameToFamily;
	std::map<PlatformFontRef,Font*>				platformRefToFont;
	std::map<std::string,Font*>					psNameToFont;					// maps PS name (as used in .xdv) to font record

	int				weightAndWidthDiff(const Font* a, const Font* b) const;
	void			getOpSizeRecAndStyleFlags(Font* theFont);
	void			appendToList(std::list<std::string>* list, const char* str);
	void			prependToList(std::list<std::string>* list, const char* str);
	void			addToMaps(PlatformFontRef platformFont, const NameCollection* names);
	
#ifdef XETEX_MAC
	NameCollection*	readNames(ATSUFontID fontID);
#else
	NameCollection*	readNames(PlatformFontRef fontRef);
#endif

	void	die(const char*s, int i) const;	/* for fatal internal errors! */
};

#endif	/* __cplusplus */


#endif	/* __XETEX_FONT_MANAGER_H */
