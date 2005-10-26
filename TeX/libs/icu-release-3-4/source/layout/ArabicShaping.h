/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __ARABICSHAPING_H
#define __ARABICSHAPING_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "OpenTypeTables.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;

class ArabicShaping /* not : public UObject because all methods are static */ {
public:
    // shaping bit masks
    enum ShapingBitMasks
    {
        MASK_SHAPE_RIGHT    = 1, // if this bit set, shapes to right
        MASK_SHAPE_LEFT     = 2, // if this bit set, shapes to left
        MASK_TRANSPARENT    = 4, // if this bit set, is transparent (ignore other bits)
        MASK_NOSHAPE        = 8, // if this bit set, don't shape this char, i.e. tatweel
        
        MASK_ALAPH          = 16, // if this bit set, char is Syriac ALAPH
        MASK_DALATH_RISH    = 32  // if this bit set, char is Syriac DALATH/RISH
    };

    // shaping values
    enum ShapeTypeValues
    {
        ST_NONE         = 0,
        ST_RIGHT        = MASK_SHAPE_RIGHT,
        ST_LEFT         = MASK_SHAPE_LEFT,
        ST_DUAL         = MASK_SHAPE_RIGHT | MASK_SHAPE_LEFT,
        ST_TRANSPARENT  = MASK_TRANSPARENT,
        ST_NOSHAPE_DUAL = MASK_NOSHAPE | ST_DUAL,
        ST_NOSHAPE_NONE = MASK_NOSHAPE,
        ST_ALAPH        = MASK_ALAPH | MASK_SHAPE_RIGHT,
        ST_DALATH_RISH  = MASK_DALATH_RISH | MASK_SHAPE_RIGHT
    };

    typedef le_int32 ShapeType;

    static void shape(const LEUnicode *chars, le_int32 offset, le_int32 charCount, le_int32 charMax,
                      le_bool rightToLeft, LEGlyphStorage &glyphStorage);

    static const LETag *getFeatureOrder();

private:
    // forbid instantiation
    ArabicShaping();

    static const LETag tagArray[];

    static ShapeType getShapeType(LEUnicode c);

    static const ShapeType shapeTypes[];
    static const ShapeType mongolianTypes[];

    static void adjustTags(le_int32 outIndex, le_int32 shapeOffset, LEGlyphStorage &glyphStorage); 
};

U_NAMESPACE_END
#endif
