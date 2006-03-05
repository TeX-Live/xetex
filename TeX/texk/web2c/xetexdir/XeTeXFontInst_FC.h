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
 *   file name:  XeTeXFontInst_FC.h
 *
 *   created on: 2005-10-25
 *   created by: Jonathan Kew
 */


#ifndef __XeTeXFontInst_FC_H
#define __XeTeXFontInst_FC_H

#include "XeTeXFontInst.h"

#include <fontconfig/fontconfig.h>

#include <ft2build.h>
#include FT_FREETYPE_H

class XeTeXFontInst_FC : public XeTeXFontInst
{
protected:

    const void *	readTable(LETag tag, le_uint32 *length) const;

	FT_Face			face;
	bool			fFreeTypeOnly;
	
public:
    				XeTeXFontInst_FC(FcPattern* pattern, float pointSize, LEErrorCode &status);

    virtual 		~XeTeXFontInst_FC();

	virtual void	initialize(LEErrorCode &status);

	virtual char*	getPSName() const;	// returns a malloced string

	virtual void	getGlyphBounds(LEGlyphID gid, GlyphBBox* bbox);

	// overrides of XeTeXFontInst methods, in case it's not an sfnt
	virtual le_uint16 getNumGlyphs() const;
    virtual void getGlyphAdvance(LEGlyphID glyph, LEPoint &advance) const;
    virtual LEGlyphID mapCharToGlyph(LEUnicode32 ch) const;
};

#endif
