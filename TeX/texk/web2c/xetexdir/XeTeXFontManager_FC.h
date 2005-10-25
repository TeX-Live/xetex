/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

#ifndef __XeTeXFontManager_FC_H
#define __XeTeXFontManager_FC_H

#include "XeTeXFontManager.h"

class XeTeXFontManager_FC
	: public XeTeXFontManager
{
public:
								XeTeXFontManager_FC();

	virtual 					~XeTeXFontManager_FC();

	virtual void*				findFont(const char* fontName, double pointSize);
//	virtual char*				getFontPSName(void* fontRef);
	
protected:
//	void						getOpticalSizeRec(ATSUFontID id, opticalSizeRec& sizeRec);
	
//	void						initFontDicts();
};

#endif
