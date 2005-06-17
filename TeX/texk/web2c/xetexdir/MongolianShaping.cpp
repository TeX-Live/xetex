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
 * @(#)MongolianShaping.cpp    1.00 04/10/07
 *
 * Only a beginning, for the Code2000 font; not a complete implementation
 */

#include "LETypes.h"
#include "OpenTypeTables.h"
#include "MongolianShaping.h"
#include "LEGlyphStorage.h"

U_NAMESPACE_BEGIN

enum {
    _c_ = MongolianShaping::ST_NOSHAPE_DUAL,
    _d_ = MongolianShaping::ST_DUAL,
    _n_ = MongolianShaping::ST_NONE,
    _r_ = MongolianShaping::ST_RIGHT,
    _t_ = MongolianShaping::ST_TRANSPARENT,
    _x_ = MongolianShaping::ST_NOSHAPE_NONE
};

const MongolianShaping::ShapeType MongolianShaping::shapeTypes[] =
{
   _n_, _n_, _n_, _n_, _n_, _n_, _n_, _n_, _n_, _n_, _n_, _t_, _t_, _t_, _n_, _n_,   // 0x1800 - 0x180f
   _n_, _n_, _n_, _n_, _n_, _n_, _n_, _n_, _n_, _n_, _n_, _n_, _n_, _n_, _n_, _n_,   // 0x1810 - 0x181f
   _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_,   // 0x1820 - 0x182f
   _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_,   // 0x1830 - 0x183f
   _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_,   // 0x1840 - 0x184f
   _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_,   // 0x1850 - 0x185f
   _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_,   // 0x1860 - 0x186f
   _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _n_, _n_, _n_, _n_, _n_, _n_, _n_, _n_,   // 0x1870 - 0x187f
   _n_, _n_, _n_, _n_, _n_, _n_, _n_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_,   // 0x1880 - 0x188f
   _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_,   // 0x1890 - 0x189f
   _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _d_, _t_, _n_, _n_, _n_, _n_, _n_, _n_,   // 0x18a0 - 0x18af
};

/*
    shaping array holds types for Mongolian chars between 1800 and 18af
    other values are either unshaped, or transparent if a mark or format
    code, except for format codes 200c (zero-width non-joiner) and 200d 
    (dual-width joiner) which are both unshaped and non_joining or
    dual-joining, respectively.
*/
MongolianShaping::ShapeType MongolianShaping::getShapeType(LEUnicode c)
{
    if (c >= 0x1800 && c <= 0x206f) {
        if (c < 0x18b0) {
            return shapeTypes[c - 0x1800];
        } else if (c == 0x200c) {   // ZWNJ
            return ST_NOSHAPE_NONE;
        } else if (c == 0x200d) {   // ZWJ
            return ST_NOSHAPE_DUAL;
        } else if (c >= 0x202a && c <= 0x202e) { // LRE - RLO
            return ST_TRANSPARENT;
        } else if (c >= 0x206a && c <= 0x206f) { // Inhibit Symmetric Swapping - Nominal Digit Shapes
            return ST_TRANSPARENT;
        }
    }

    return ST_NOSHAPE_NONE;
}

const LETag isolFeatureTag = LE_ISOL_FEATURE_TAG;
const LETag initFeatureTag = LE_INIT_FEATURE_TAG;
const LETag mediFeatureTag = LE_MEDI_FEATURE_TAG;
const LETag finaFeatureTag = LE_FINA_FEATURE_TAG;
const LETag markFeatureTag = LE_MARK_FEATURE_TAG;
const LETag ccmpFeatureTag = LE_CCMP_FEATURE_TAG;
const LETag rligFeatureTag = LE_RLIG_FEATURE_TAG;
const LETag cursFeatureTag = LE_CURS_FEATURE_TAG;
const LETag kernFeatureTag = LE_KERN_FEATURE_TAG;
const LETag mkmkFeatureTag = LE_MKMK_FEATURE_TAG;

const LETag emptyTag       = 0x00000000; // ''

const LETag featureOrder[] = 
{
    ccmpFeatureTag, isolFeatureTag, finaFeatureTag, mediFeatureTag, initFeatureTag, rligFeatureTag,
    cursFeatureTag, kernFeatureTag, markFeatureTag, mkmkFeatureTag, emptyTag
};

const LETag MongolianShaping::tagArray[] =
{
    isolFeatureTag, markFeatureTag, ccmpFeatureTag, rligFeatureTag,
        cursFeatureTag, kernFeatureTag, mkmkFeatureTag, emptyTag,

    finaFeatureTag, markFeatureTag, ccmpFeatureTag, rligFeatureTag,
        cursFeatureTag, kernFeatureTag, mkmkFeatureTag, emptyTag,

    initFeatureTag, markFeatureTag, ccmpFeatureTag, rligFeatureTag,
        cursFeatureTag, kernFeatureTag, mkmkFeatureTag, emptyTag,

    mediFeatureTag, markFeatureTag, ccmpFeatureTag, rligFeatureTag,
        cursFeatureTag, kernFeatureTag, mkmkFeatureTag, emptyTag
};

#define TAGS_PER_GLYPH ((sizeof MongolianShaping::tagArray / sizeof MongolianShaping::tagArray[0]) / 4)

const LETag *MongolianShaping::getFeatureOrder()
{
    return featureOrder;
}

void MongolianShaping::adjustTags(le_int32 outIndex, le_int32 shapeOffset, LEGlyphStorage &glyphStorage)
{
    LEErrorCode success = LE_NO_ERROR;
    const LETag *glyphTags = (const LETag *) glyphStorage.getAuxData(outIndex, success);

    glyphStorage.setAuxData(outIndex, (void *) &glyphTags[TAGS_PER_GLYPH * shapeOffset], success);
}

void MongolianShaping::shape(const LEUnicode *chars, le_int32 offset, le_int32 charCount, le_int32 charMax,
                          le_bool rightToLeft, LEGlyphStorage &glyphStorage)
{
    // iterate in logical order, store tags in visible order
    // 
    // the effective right char is the most recently encountered 
    // non-transparent char
    //
    // four boolean states:
    //   the effective right char shapes
    //   the effective right char causes left shaping
    //   the current char shapes
    //   the current char causes right shaping
    // 
    // if both cause shaping, then
    //   shaper.shape(errout, 2) (isolate to initial, or final to medial)
    //   shaper.shape(out, 1) (isolate to final)

    ShapeType rightType = ST_NOSHAPE_NONE, leftType = ST_NOSHAPE_NONE;
    LEErrorCode success = LE_NO_ERROR;
    le_int32 i;

    for (i = offset - 1; i >= 0; i -= 1) {
        rightType = getShapeType(chars[i]);
        
        if (rightType != ST_TRANSPARENT) {
            break;
        }
    }

    for (i = offset + charCount; i < charMax; i += 1) {
        leftType = getShapeType(chars[i]);

        if (leftType != ST_TRANSPARENT) {
            break;
        }
    }

    // erout is effective right logical index
    le_int32 erout = -1;
    le_bool rightShapes = FALSE;
    le_bool rightCauses = (rightType & MASK_SHAPE_LEFT) != 0;
    le_int32 in, e, out = 0, dir = 1;

    if (rightToLeft) {
        out = charCount - 1;
        erout = charCount;
        dir = -1;
    }

    for (in = offset, e = offset + charCount; in < e; in += 1, out += dir) {
        LEUnicode c = chars[in];
        ShapeType t = getShapeType(c);

        glyphStorage.setAuxData(out, (void *) tagArray, success);

        if ((t & MASK_TRANSPARENT) != 0) {
            continue;
        }

        le_bool curShapes = (t & MASK_NOSHAPE) == 0;
        le_bool curCauses = (t & MASK_SHAPE_RIGHT) != 0;

        if (rightCauses && curCauses) {
            if (rightShapes) {
                adjustTags(erout, 2, glyphStorage);
            }

            if (curShapes) {
                adjustTags(out, 1, glyphStorage);
            }
        }

        rightShapes = curShapes;
        rightCauses = (t & MASK_SHAPE_LEFT) != 0;
        erout = out;
    }

    if (rightShapes && rightCauses && (leftType & MASK_SHAPE_RIGHT) != 0) {
        adjustTags(erout, 2, glyphStorage);
    }
}

U_NAMESPACE_END
