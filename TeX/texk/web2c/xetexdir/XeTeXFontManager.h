/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

#ifndef __XeTeXFontManager_H
#define __XeTeXFontManager_H

#include "XeTeXLayoutInterface.h"

#include <string>

class XeTeXFontManager
{
public:
	static XeTeXFontManager*	gFontManager;	// the global font manager object
	static XeTeXFontManager*	GetFontManager();
	
	virtual void*				findFont(const char* fontName, double pointSize) = 0;
//	virtual char*				getFontPSName(void* fontRef) = 0;

	typedef struct {
		std::string	name;
		float		reqSize;
		bool		reqBold;
		bool		reqItal;
	} fontSpec;

	bool						parseFontName(const char* str, fontSpec& spec);
	
protected:
	typedef struct {
		unsigned short	designSize;
		unsigned short	subFamilyID;
		unsigned short	nameCode;
		unsigned short	minSize;
		unsigned short	maxSize;
	} opticalSizeRec;
	

								XeTeXFontManager()	// must create a subclass!
									{ }

	virtual 					~XeTeXFontManager()
									{ }

	void						getOpticalSizeRec(XeTeXFont font, opticalSizeRec& sizeRec);
};

#endif
