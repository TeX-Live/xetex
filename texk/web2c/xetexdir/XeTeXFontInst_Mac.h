/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2008 by SIL International
 copyright (c) 2009 by Jonathan Kew

 Written by Jonathan Kew

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the copyright holders
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written
authorization from the copyright holders.
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
    const void *readTable(LETag tag, le_uint32 *length) const;

	ATSFontRef	fFontRef;
	ATSUStyle	fStyle;

public:
    			XeTeXFontInst_Mac(ATSFontRef atsFont, float pointSize, LEErrorCode &status);

    virtual 	~XeTeXFontInst_Mac();

	virtual void initialize(LEErrorCode &status);
	
	virtual void	getGlyphBounds(LEGlyphID gid, GlyphBBox* bbox);
};

#endif
