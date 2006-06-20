/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

#ifndef __XETEX_FONT_MGR_MAC_H
#define __XETEX_FONT_MGR_MAC_H

#include "XeTeXFontMgr.h"

#include <Cocoa/Cocoa.h>

class XeTeXFontMgr_Mac
	: public XeTeXFontMgr
{
public:
									XeTeXFontMgr_Mac()
										{ }
	virtual							~XeTeXFontMgr_Mac()
										{ }

protected:
										
	virtual void					initialize();
	virtual void					terminate();

	virtual void					searchForHostPlatformFonts(const std::string& name);
	
	virtual NameCollection*			readNames(ATSUFontID fontID);

private:
	void		addFontsToCaches(NSArray* fonts);

	void		addFamilyToCaches(ATSFontFamilyRef familyRef);

	void		addFontAndSiblingsToCaches(ATSFontRef fontRef);
};

#endif	/* __XETEX_FONT_MGR_MAC_H */
