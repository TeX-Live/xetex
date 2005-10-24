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
 *   file name:  XeTeXFontInst_Mac.cpp
 *
 *   created on: 2005-10-22
 *   created by: Jonathan Kew
 */


#include "XeTeXFontInst_Mac.h"


XeTeXFontInst_Mac::XeTeXFontInst_Mac(ATSFontRef atsFont, float pointSize, LEErrorCode &status)
    : XeTeXFontInst(pointSize, status)
    , fATSFont(atsFont)
{
    if (LE_FAILURE(status)) {
        return;
    }

    if (fATSFont == 0) {
        status = LE_FONT_FILE_NOT_FOUND_ERROR;
        return;
    }

	initialize(status);
}

XeTeXFontInst_Mac::~XeTeXFontInst_Mac()
{
}

void XeTeXFontInst_Mac::initialize(LEErrorCode &status)
{
    if (fATSFont == 0) {
        status = LE_FONT_FILE_NOT_FOUND_ERROR;
        return;
    }

	XeTeXFontInst::initialize(status);

	if (status != LE_NO_ERROR)
		fATSFont = 0;

    return;
}

void XeTeXFontInst_Mac::setATSFont(ATSFontRef fontRef)
{
	fATSFont = fontRef;

	flush();	// clear the font table cache

	LEErrorCode	status = (LEErrorCode)0;
	initialize(status);
}

const void *XeTeXFontInst_Mac::readTable(LETag tag, le_uint32 *length) const
{
	OSStatus status = ATSFontGetTable(fATSFont, tag, 0, 0, 0, (ByteCount*)length);
	if (status != noErr) {
		*length = 0;
		return NULL;
	}
	void*	table = LE_NEW_ARRAY(char, *length);
	if (table != NULL) {
		status = ATSFontGetTable(fATSFont, tag, 0, *length, table, (ByteCount*)length);
		if (status != noErr) {
			*length = 0;
			LE_DELETE_ARRAY(table);
			return NULL;
		}
	}

    return table;
}
