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

#include "XeTeXOTMath.h"

#include "XeTeX_ext.h"
#include "XeTeXLayoutInterface.h"
#include "XeTeXFontInst.h"
#include "XeTeXswap.h"

typedef void*	voidptr;

extern "C" {
	extern voidptr*	fontlayoutengine;
	extern integer*	fontarea;
	extern integer*	fontsize;
}

static int32_t getCoverage(const Coverage* coverage, GlyphID g)
{
	if (SWAP(coverage->format) == 1) {
		const CoverageFormat1 *table = (const CoverageFormat1 *) coverage;
		for (int i = 0; i < SWAP(table->glyphCount); i++) {
			if (SWAP(table->glyphArray[i]) == g)
				return i;
		}
	} else if (SWAP(coverage->format) == 2) {
		const CoverageFormat2 *table = (const CoverageFormat2 *) coverage;
		for (int i = 0; i < SWAP(table->rangeCount); i++) {
			if (SWAP(table->rangeArray[i].start) <= g && SWAP(table->rangeArray[i].start) >= g)
				return SWAP(table->rangeArray[i].startCoverageIndex) + (g - SWAP(table->rangeArray[i].start));
		}
	}

	return -1;
}

static SInt16 getMathConstant(XeTeXFontInst* fontInst, mathConstantIndex whichConstant)
{
	const char* table = (const char*)fontInst->getFontTable(kMATH);
	if (table == NULL)
		return 0;

	const UInt16* constants = (const UInt16*)(table + SWAP(((const MathTableHeader*)table)->mathConstants));

	if (whichConstant < firstMathValueRecord) {
		/* it's a simple 16-bit value */
		return SWAP(constants[whichConstant]);
	}
	else if (whichConstant <= lastMathValueRecord) {
		const MathValueRecord* valueRecords = (const MathValueRecord*)
			((char*)constants + firstMathValueRecord * sizeof(UInt16) - firstMathValueRecord * sizeof(MathValueRecord));
		return SWAP(valueRecords[whichConstant].value);
	}
	else if (whichConstant <= lastMathConstant) {
		return SWAP(constants[whichConstant + (lastMathValueRecord - firstMathValueRecord + 1)]);
	}
	else
		return 0; /* or abort, with "internal error" or something */
}

int
get_ot_math_constant(int f, int n)
{
	int	rval = 0;

	if (fontarea[f] == OTGR_FONT_FLAG) {
		XeTeXFontInst*	font = (XeTeXFontInst*)getFont((XeTeXLayoutEngine)fontlayoutengine[f]);
		rval = getMathConstant(font, (mathConstantIndex)n);
		/* scale according to font size, except the ones that are percentages */
		if (n > scriptScriptPercentScaleDown && n < radicalDegreeBottomRaisePercent)
			rval = X2Fix(rval * Fix2X(fontsize[f]) / font->getUnitsPerEM());
	}
	return rval;
}

/* fontdimen IDs for math symbols font (family 2) */
#define math_x_height	5
#define math_quad		6
#define num1			8	/* numerator shift-up in display styles */
#define num2			9	/* numerator shift-up in non-display, non-\.{\\atop} */
#define num3			10	/* numerator shift-up in non-display \.{\\atop} */
#define denom1			11	/* denominator shift-down in display styles */
#define denom2			12	/* denominator shift-down in non-display styles */
#define sup1			13	/* superscript shift-up in uncramped display style */
#define sup2			14	/* superscript shift-up in uncramped non-display */
#define sup3			15	/* superscript shift-up in cramped styles */
#define sub1			16	/* subscript shift-down if superscript is absent */
#define sub2			17	/* subscript shift-down if superscript is present */
#define sup_drop		18	/* superscript baseline below top of large box */
#define sub_drop		19	/* subscript baseline below bottom of large box */
#define delim1			20	/* size of \.{\\atopwithdelims} delimiters */
#define delim2			21	/* size of \.{\\atopwithdelims} delimiters in non-displays */
#define axis_height		22	/* height of fraction lines above the baseline */

const mathConstantIndex TeX_sym_to_OT_map[] = {
	unknown,
	unknown,
	unknown,
	unknown,
	unknown,
	accentBaseHeight, // x-height
	unknown, // quad
	unknown,
	fractionNumeratorDisplayStyleShiftUp,
	fractionNumeratorShiftUp,
	stackTopShiftUp,
	fractionDenominatorDisplayStyleShiftDown,
	fractionDenominatorShiftDown,
	superscriptShiftUp, // ??
	superscriptShiftUp, // ??
	superscriptShiftUpCramped,
	subscriptShiftDown, // ??
	subscriptShiftDown, // ??
	superscriptBaselineDropMax, // ??
	subscriptBaselineDropMin, // ??
	delimitedSubFormulaMinHeight,
	unknown, // using quad instead for now
	axisHeight
};

int
get_native_mathsy_param(int f, int n)
{
	int	rval = 0;

	if (n == math_quad || n == delim2)
		rval = fontsize[f];
	else {
		if (n < sizeof(TeX_sym_to_OT_map) / sizeof(mathConstantIndex)) {
			mathConstantIndex ot_index = TeX_sym_to_OT_map[n];
			if (ot_index != unknown)
				rval = get_ot_math_constant(f, (int)ot_index);
		}
	}
//	fprintf(stderr, " math_sy(%d, %d) returns %.3f\n", f, n, Fix2X(rval));
	
	return rval;
}

/* fontdimen IDs for math extension font (family 3) */
#define default_rule_thickness	8	/* thickness of \.{\\over} bars */
#define big_op_spacing1			9	/* minimum clearance above a displayed op */
#define big_op_spacing2			10	/* minimum clearance below a displayed op */
#define big_op_spacing3			11	/* minimum baselineskip above displayed op */
#define big_op_spacing4			12	/* minimum baselineskip below displayed op */
#define big_op_spacing5			13	/* padding above and below displayed limits */

const mathConstantIndex TeX_ext_to_OT_map[] = {
	unknown,
	unknown,
	unknown,
	unknown,
	unknown,
	accentBaseHeight, // x-height
	unknown, // quad
	unknown,
	fractionRuleThickness, // default_rule_thickness
	upperLimitGapMin, // big_op_spacing1
	lowerLimitGapMin, // big_op_spacing2
	upperLimitBaselineRiseMin, // big_op_spacing3
	lowerLimitBaselineDropMin, // big_op_spacing4
	stackGapMin // big_op_spacing5
};

int
get_native_mathex_param(int f, int n)
{
	int	rval = 0;

	if (n == math_quad)
		rval = fontsize[f];
	else {
		if (n < sizeof(TeX_ext_to_OT_map) / sizeof(mathConstantIndex)) {
			mathConstantIndex ot_index = TeX_ext_to_OT_map[n];
			if (ot_index != unknown)
				rval = get_ot_math_constant(f, (int)ot_index);
		}
	}
//	fprintf(stderr, " math_ex(%d, %d) returns %.3f\n", f, n, Fix2X(rval));
	
	return rval;
}

int
get_ot_math_variant(int f, int g, int v, integer* adv, int horiz)
{
	int	rval = g;
	*adv = -1;
	
	if (fontarea[f] == OTGR_FONT_FLAG) {
		XeTeXFontInst*	font = (XeTeXFontInst*)getFont((XeTeXLayoutEngine)fontlayoutengine[f]);

		const char* table = (const char*)font->getFontTable(kMATH);
		if (table == NULL)
			return rval;

		uint16_t	offset = SWAP(((const MathTableHeader*)table)->mathVariants);
		if (offset == 0)
			return rval;
		const MathVariants* variants = (const MathVariants*)(table + offset);

		offset = horiz ? SWAP(variants->horizGlyphCoverage) : SWAP(variants->vertGlyphCoverage);
		if (offset == 0)
			return rval;
		const Coverage* coverage = (const Coverage*)(((const char*)variants) + offset);

		int32_t	index = getCoverage(coverage, g);
		if (index >= 0) {
			if (horiz)
				index += SWAP(variants->vertGlyphCount);
			const MathGlyphConstruction*	construction = (const MathGlyphConstruction*)(((const char*)variants)
															+ SWAP(variants->vertGlyphConstruction[index]));
			if (v < SWAP(construction->variantCount)) {
				rval = SWAP(construction->mathGlyphVariantRecord[v].variantGlyph);
				*adv = X2Fix(SWAP(construction->mathGlyphVariantRecord[v].advanceMeasurement)
								* Fix2X(fontsize[f]) / font->getUnitsPerEM());
			}
		}
	}
	
	return rval;
}

void*
get_ot_assembly_ptr(int f, int g, int horiz)
{
	void*	rval = NULL;
	
	if (fontarea[f] == OTGR_FONT_FLAG) {
		XeTeXFontInst*	font = (XeTeXFontInst*)getFont((XeTeXLayoutEngine)fontlayoutengine[f]);

		const char* table = (const char*)font->getFontTable(kMATH);
		if (table == NULL)
			return rval;

		uint16_t	offset = SWAP(((const MathTableHeader*)table)->mathVariants);
		if (offset == 0)
			return rval;
		const MathVariants* variants = (const MathVariants*)(table + offset);

		offset = horiz ? SWAP(variants->horizGlyphCoverage) : SWAP(variants->vertGlyphCoverage);
		if (offset == 0)
			return rval;
		const Coverage* coverage = (const Coverage*)(((const char*)variants) + offset);

		int32_t	index = getCoverage(coverage, g);
		if (index >= 0) {
			if (horiz)
				index += SWAP(variants->vertGlyphCount);
			const MathGlyphConstruction*	construction = (const MathGlyphConstruction*)(((const char*)variants)
															+ SWAP(variants->vertGlyphConstruction[index]));
			offset = SWAP(construction->glyphAssembly);
			if (offset != 0)
				rval = (void*)(((const char*)construction) + offset);
		}
	}
	
	return rval;
}

int
get_ot_math_ital_corr(int f, int g)
{
	int	rval = 0;
	
	if (fontarea[f] == OTGR_FONT_FLAG) {
		XeTeXFontInst*	font = (XeTeXFontInst*)getFont((XeTeXLayoutEngine)fontlayoutengine[f]);

		const char* table = (const char*)font->getFontTable(kMATH);
		if (table == NULL)
			return rval;

		uint16_t	offset = SWAP(((const MathTableHeader*)table)->mathGlyphInfo);
		if (offset == 0)
			return rval;
		const MathGlyphInfo* glyphInfo = (const MathGlyphInfo*)(table + offset);

		offset = SWAP(glyphInfo->mathItalicsCorrectionInfo);
		if (offset == 0)
			return rval;
		const MathItalicsCorrectionInfo* italCorrInfo = (const MathItalicsCorrectionInfo*)(((const char*)glyphInfo) + offset);

		offset = SWAP(italCorrInfo->coverage);
		if (offset == 0)
			return rval;
		const Coverage* coverage = (const Coverage*)(((const char*)italCorrInfo) + offset);

		int32_t	index = getCoverage(coverage, g);
		if (index >= 0 && index < SWAP(italCorrInfo->italicsCorrectionCount))
			rval = X2Fix(SWAP(italCorrInfo->italicsCorrection[index].value) * Fix2X(fontsize[f]) / font->getUnitsPerEM());
	}
	
	return rval;
}

int
get_ot_math_accent_pos(int f, int g)
{
	int	rval = 0x7fffffffUL;
	
	if (fontarea[f] == OTGR_FONT_FLAG) {
		XeTeXFontInst*	font = (XeTeXFontInst*)getFont((XeTeXLayoutEngine)fontlayoutengine[f]);

		const char* table = (const char*)font->getFontTable(kMATH);
		if (table == NULL)
			return rval;

		uint16_t	offset = SWAP(((const MathTableHeader*)table)->mathGlyphInfo);
		if (offset == 0)
			return rval;
		const MathGlyphInfo* glyphInfo = (const MathGlyphInfo*)(table + offset);

		offset = SWAP(glyphInfo->mathTopAccentAttachment);
		if (offset == 0)
			return rval;
		const MathTopAccentAttachment* accentAttachment = (const MathTopAccentAttachment*)(((const char*)glyphInfo) + offset);

		offset = SWAP(accentAttachment->coverage);
		if (offset == 0)
			return rval;
		const Coverage* coverage = (const Coverage*)(((const char*)accentAttachment) + offset);

		int32_t	index = getCoverage(coverage, g);
		if (index >= 0 && index < SWAP(accentAttachment->topAccentAttachmentCount)) {
			rval = (int16_t)SWAP(accentAttachment->topAccentAttachment[index].value);
			rval = X2Fix(rval * Fix2X(fontsize[f]) / font->getUnitsPerEM());
		}
	}
	
	return rval;
}

int
ot_min_connector_overlap(int f)
{
	int	rval = 0;
	
	if (fontarea[f] == OTGR_FONT_FLAG) {
		XeTeXFontInst*	font = (XeTeXFontInst*)getFont((XeTeXLayoutEngine)fontlayoutengine[f]);

		const char* table = (const char*)font->getFontTable(kMATH);
		if (table == NULL)
			return rval;

		uint16_t	offset = SWAP(((const MathTableHeader*)table)->mathVariants);
		if (offset == 0)
			return rval;
		const MathVariants* variants = (const MathVariants*)(table + offset);

		rval = X2Fix(SWAP(variants->minConnectorOverlap) * Fix2X(fontsize[f]) / font->getUnitsPerEM());
	}

	return rval;
}

int
ot_part_count(const GlyphAssembly* a)
{
	return SWAP(a->partCount);
}

int
ot_part_glyph(const GlyphAssembly* a, int i)
{
	return SWAP(a->partRecords[i].glyph);
}

int
ot_part_is_extender(const GlyphAssembly* a, int i)
{
	return (SWAP(a->partRecords[i].partFlags) & fExtender) != 0;
}

int
ot_part_start_connector(int f, const GlyphAssembly* a, int i)
{
	int	rval = 0;
	
	if (fontarea[f] == OTGR_FONT_FLAG) {
		XeTeXFontInst*	font = (XeTeXFontInst*)getFont((XeTeXLayoutEngine)fontlayoutengine[f]);
		rval = X2Fix(SWAP(a->partRecords[i].startConnectorLength) * Fix2X(fontsize[f]) / font->getUnitsPerEM());
	}
	
	return rval;
}

int
ot_part_end_connector(int f, const GlyphAssembly* a, int i)
{
	int	rval = 0;
	
	if (fontarea[f] == OTGR_FONT_FLAG) {
		XeTeXFontInst*	font = (XeTeXFontInst*)getFont((XeTeXLayoutEngine)fontlayoutengine[f]);
		rval = X2Fix(SWAP(a->partRecords[i].endConnectorLength) * Fix2X(fontsize[f]) / font->getUnitsPerEM());
	}
	
	return rval;
}

int
ot_part_full_advance(int f, const GlyphAssembly* a, int i)
{
	int	rval = 0;
	
	if (fontarea[f] == OTGR_FONT_FLAG) {
		XeTeXFontInst*	font = (XeTeXFontInst*)getFont((XeTeXLayoutEngine)fontlayoutengine[f]);
		rval = X2Fix(SWAP(a->partRecords[i].fullAdvance) * Fix2X(fontsize[f]) / font->getUnitsPerEM());
	}
	
	return rval;
}
