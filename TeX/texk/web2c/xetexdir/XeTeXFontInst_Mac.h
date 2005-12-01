/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

/*
 *   file name:  XeTeXFontInst_Mac.h
 *
 *   created on: 2005-10-22
 *   created by: Jonathan Kew
 */


#ifndef __XeTeXFontInst_Mac_H
#define __XeTeXFontInst_Mac_H

#include "XeTeXFontInst.h"

#include <ApplicationServices/ApplicationServices.h>

class XeTeXFontInst_Mac : public XeTeXFontInst
{
protected:
    ATSFontRef	fATSFont;

    const void *readTable(LETag tag, le_uint32 *length) const;

public:
    			XeTeXFontInst_Mac(ATSFontRef atsFont, float pointSize, LEErrorCode &status);

    virtual 	~XeTeXFontInst_Mac();

	virtual void initialize(LEErrorCode &status);

	ATSFontRef	getATSFont() const
					{ return fATSFont; }
	
	void		setATSFont(ATSFontRef fontRef);
};

#endif
