/*
 *******************************************************************************
 *
 *   Copyright (C) 1999-2004, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  LEFontInstance.cpp
 *
 *   created on: 02/06/2003
 *   created by: Eric R. Mader
 */

#include "LETypes.h"
#include "LEScripts.h"
#include "LEFontInstance.h"
#include "LEGlyphStorage.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(LEFontInstance)

const LEFontInstance *LEFontInstance::getSubFont(const LEUnicode chars[], le_int32 *offset, le_int32 limit,
                                                       le_int32 script, LEErrorCode &success) const
{
    if (LE_FAILURE(success)) {
        return NULL;
    }

    if (chars == NULL || *offset < 0 || limit < 0 || *offset >= limit || script < 0 || script >= scriptCodeCount) {
        success = LE_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    *offset = limit;
    return this;
}

void LEFontInstance::mapCharsToGlyphs(const LEUnicode chars[], le_int32 offset, le_int32 count,
                                      le_bool reverse, const LECharMapper *mapper, LEGlyphStorage &glyphStorage) const
{
    le_int32 i, out = 0, dir = 1;

    if (reverse) {
        out = count - 1;
        dir = -1;
    }

    for (i = offset; i < offset + count; i += 1, out += dir) {
        LEUnicode16 high = chars[i];
        LEUnicode32 code = high;

        if (i < offset + count - 1 && high >= 0xD800 && high <= 0xDBFF) {
            LEUnicode16 low = chars[i + 1];

            if (low >= 0xDC00 && low <= 0xDFFF) {
                code = (high - 0xD800) * 0x400 + low - 0xDC00 + 0x10000;
            }
        }

        glyphStorage[out] = mapCharToGlyph(code, mapper);

        if (code >= 0x10000) {
            i += 1;
            glyphStorage[out += dir] = 0xFFFF;
        }
    }
}

LEGlyphID LEFontInstance::mapCharToGlyph(LEUnicode32 ch, const LECharMapper *mapper) const
{
    LEUnicode32 mappedChar = mapper->mapChar(ch);

    if (mappedChar == 0xFFFE || mappedChar == 0xFFFF) {
        return 0xFFFF;
    }

    if (mappedChar == 0x200C || mappedChar == 0x200D) {
        return 1;
    }

    return mapCharToGlyph(mappedChar);
}

void LEFontInstance::getKernPair(LEGlyphID leftGlyph, LEGlyphID rightGlyph, LEPoint &kern) const
{
    kern.fX = kern.fY = 0;
}

U_NAMESPACE_END

