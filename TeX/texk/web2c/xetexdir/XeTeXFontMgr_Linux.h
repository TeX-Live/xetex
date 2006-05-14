/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

#ifndef __XETEX_FONT_MGR_LINUX_H
#define __XETEX_FONT_MGR_LINUX_H

#include "XeTeXFontMgr.h"

class XeTeXFontMgr_Linux
	: public XeTeXFontMgr
{
public:
									XeTeXFontMgr_Linux()
										{ }
	virtual							~XeTeXFontMgr_Linux()
										{ }

protected:
										
	virtual void					initialize();

	virtual void					getOpSizeRecAndStyleFlags(Font* theFont);
	virtual void					searchForHostPlatformFonts(const std::string& name);
	
	virtual NameCollection*			readNames(FcPattern* pat);

private:
	FT_Library	ftLib;
};

#endif	/* __XETEX_FONT_MGR_LINUX_H */
