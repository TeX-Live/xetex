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
 *   file name:  XeTeXFontInst_FC.cpp
 *
 *   created on: 2005-10-25
 *   created by: Jonathan Kew
 */


#include "XeTeXFontInst_FC.h"

#include <freetype2/freetype/tttables.h>

static FT_Library	gLibrary = 0;


XeTeXFontInst_FC::XeTeXFontInst_FC(FcPattern* pattern, float pointSize, LEErrorCode &status)
    : XeTeXFontInst(pointSize, status)
    , face(0)
{
    if (LE_FAILURE(status)) {
        return;
    }

	FT_Error	err;
	if (!gLibrary) {
		err = FT_Init_Freetype(&gLibrary);
		if (err != 0) {
			fprintf(stderr, "FreeType initialization failed! (%d)\n", err);
			exit(1);
		}
	}
	
	FcChar8*	pathname = 0;
	FcPatternGetString(pattern, FC_FILE, 0, &pathname);

	err = FT_New_Face(library, pathname, 0, &face);

	if (err != 0) {
        status = LE_FONT_FILE_NOT_FOUND_ERROR;
        return;
    }

	initialize(status);
}

XeTeXFontInst_FC::~XeTeXFontInst_FC()
{
	if (face != 0) {
		FT_Done_Face(face);
		face = 0;
	}
}

void XeTeXFontInst_FC::initialize(LEErrorCode &status)
{
    if (face == 0) {
        status = LE_FONT_FILE_NOT_FOUND_ERROR;
        return;
    }

	XeTeXFontInst::initialize(status);

	if (status != LE_NO_ERROR) {
		FT_Done_Face(face);
		face = 0;
	}
	
    return;
}

const void *XeTeXFontInst_FC::readTable(LETag tag, le_uint32 *length) const
{
	FT_Error err = FT_Load_Sfnt_Table(face, tag, 0, NULL, length);
	if (err != 0) {
		*length = 0;
		return NULL;
	}
	
	void*	table = LE_NEW_ARRAY(char, *length);
	if (table != NULL) {
		err = FT_Load_Sfnt_Table(face, tag, 0, table, length);
		if (err != 0) {
			*length = 0;
			LE_DELETE_ARRAY(table);
			return NULL;
		}
	}

    return table;
}
