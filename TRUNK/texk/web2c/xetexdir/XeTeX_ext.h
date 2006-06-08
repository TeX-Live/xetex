/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

#ifndef __XETEXEXT_H
#define __XETEXEXT_H

/***** copied from TeX/texk/web2c/config.h -- difficult to include in C++ source files ******/
#ifndef INTEGER_TYPE

#if SIZEOF_LONG > 4 && !defined (NO_DUMP_SHARE)
/* If we have 64-bit longs and want to share format files (with 32-bit
   machines), use `int'.  */
#define INTEGER_IS_INT
#endif

#ifdef INTEGER_IS_INT
#define INTEGER_TYPE int
#define INTEGER_MAX INT_MAX
#define INTEGER_MIN INT_MIN
#else
#define INTEGER_TYPE long
#define INTEGER_MAX LONG_MAX
#define INTEGER_MIN LONG_MIN
#endif /* not INTEGER_IS_INT */

typedef INTEGER_TYPE integer;
#endif /* not INTEGER_TYPE */
/***** end of config.h stuff *****/

#ifndef XETEX_UNICODE_FILE_DEFINED
typedef struct UFILE* unicodefile;
#endif

#define AAT_FONT_FLAG	65535
#define	OT_FONT_FLAG	65534

#define FONT_FLAGS_COLORED	0x01
#define FONT_FLAGS_VERTICAL	0x02

/* some typedefs that XeTeX uses - on Mac OS, we get these from Apple headers,
   but otherwise we'll need these substitute definitions */

#ifdef XETEX_MAC
#include <Carbon/Carbon.h>
#else
#ifndef __TECkit_Common_H__
typedef unsigned char	UInt8;
typedef unsigned short	UInt16;
typedef unsigned int	UInt32;
typedef UInt16			UniChar;
#endif

typedef signed char		SInt8;
typedef short			SInt16;
typedef int				SInt32;

typedef SInt32			OSStatus;
typedef SInt32			Fixed;
typedef struct {
	Fixed	x;
	Fixed	y;
} FixedPoint;
#endif


/* these are also in xetex-new.ch and must correspond! */
#define XeTeX_count_glyphs	1

#define XeTeX_count_variations	2
#define XeTeX_variation	3
#define XeTeX_find_variation_by_name	4
#define XeTeX_variation_min	5
#define XeTeX_variation_max	6
#define XeTeX_variation_default	7

#define XeTeX_count_features	8
#define XeTeX_feature_code	9
#define XeTeX_find_feature_by_name	10
#define XeTeX_is_exclusive_feature	11
#define XeTeX_count_selectors	12
#define XeTeX_selector_code	13
#define XeTeX_find_selector_by_name	14
#define XeTeX_is_default_selector	15

#define XeTeX_OT_count_scripts	16
#define XeTeX_OT_count_languages	17
#define XeTeX_OT_count_features	18
#define XeTeX_OT_script_code	19
#define XeTeX_OT_language_code	20
#define XeTeX_OT_feature_code	21

#define XeTeX_map_char_to_glyph_code	22

#define XeTeX_variation_name	7	// must match xetex.web
#define XeTeX_feature_name	8
#define XeTeX_selector_name	9


/* definitions used to access info in a native_word_node; must correspond with defines in xetex-new.ch */
#define width_offset		1
#define depth_offset		2
#define height_offset		3
#define native_info_offset	4
#define native_glyph_info_offset	5

#define node_width(node)			node[width_offset].cint
#define node_depth(node)			node[depth_offset].cint
#define node_height(node)			node[height_offset].cint
#define native_length(node)			node[native_info_offset].hh.v.RH
#define native_font(node)			node[native_info_offset].hh.b1
#define native_glyph_count(node)	node[native_glyph_info_offset].hh.v.LH
#define native_glyph_info_ptr(node)	node[native_glyph_info_offset].hh.v.RH
#define native_glyph_info_size		10	/* info for each glyph is location (FixedPoint) + glyph ID (UInt16) */

#define native_glyph(p)		native_length(p)	/* glyph ID field in a glyph_node */

#define XDV_GLYPH_STRING	254
#define	XDV_GLYPH_ARRAY		253

/* OT-related constants we need */
#define kGSUB	0x47535542
#define kGPOS	0x47504f53

#define kLatin	0x6c61746e
#define kSyriac	0x73797263
#define kArabic	0x61726162
#define kThaana	0x74686161
#define kHebrew	0x68656272


struct postTable {
	Fixed	format;
	Fixed	italicAngle;
	SInt16	underlinePosition;
	SInt16	underlineThickness;
	UInt16	isFixedPitch;
	UInt16	reserved;
	UInt32	minMemType42;
	UInt32	maxMemType42;
	UInt32	minMemType1;
	UInt32	maxMemType1;
};

#define kPost	0x706f7374


typedef struct
{
	float	xMin;
	float	yMin;
	float	xMax;
	float	yMax;
} GlyphBBox;

#ifdef __cplusplus
extern "C" {
#endif
	void setinputfileencoding(unicodefile f, int mode, int encodingData);
	void uclose(unicodefile f);
	int input_line_icu(unicodefile f);
	void linebreakstart(int localeStrNum, const UniChar* text, int textLength);
	int linebreaknext();
	long getencodingmodeandinfo(long* info);
	void printutf8str(const unsigned char* str, int len);
	void printchars(const unsigned short* str, int len);
	void* load_mapping_file(const char* s, const char* e);
	void* findnativefont(unsigned char* name, long scaled_size);
	void releasefontengine(void* engine, int type_flag);

	/* 'integer' params here are really TeX 'scaled' values, but that typedef isn't available every place this is included */
	void otgetfontmetrics(void* engine, integer* ascent, integer* descent, integer* xheight, integer* capheight, integer* slant);
	void getnativecharheightdepth(int font, int ch, integer* height, integer* depth);
	void getnativecharsidebearings(int font, int ch, integer* lsb, integer* rsb);

	long otfontget(int what, void* engine);
	long otfontget1(int what, void* engine, long param);
	long otfontget2(int what, void* engine, long param1, long param2);
	long otfontget3(int what, void* engine, long param1, long param2, long param3);
	int makeXDVGlyphArrayData(void* p);
	long makefontdef(long f);
	int applymapping(void* cnv, const UniChar* txtPtr, int txtLen);
	void store_justified_native_glyphs(void* node);
	void measure_native_node(void* node, int use_glyph_metrics);
	Fixed get_native_ital_corr(void* node);
	Fixed get_native_glyph_ital_corr(void* node);
	void measure_native_glyph(void* node, int use_glyph_metrics);
	int mapchartoglyph(int font, unsigned int ch);
	int mapglyphtoindex(int font);

#ifdef XETEX_MAC
/* functions in XeTeX_mac.c */
	void* loadAATfont(ATSFontRef fontRef, long scaled_size, const char* cp1);
	void DoAtsuiLayout(void* node, int justify);
	void GetGlyphBBox_AAT(ATSUStyle style, UInt16 gid, GlyphBBox* bbox);
	void GetGlyphHeightDepth_AAT(ATSUStyle style, UInt16 gid, float* ht, float* dp);
	void GetGlyphSidebearings_AAT(ATSUStyle style, UInt16 gid, float* lsb, float* rsb);
	int MapCharToGlyph_AAT(ATSUStyle style, UInt32 ch);
	int MapGlyphToIndex_AAT(ATSUStyle style, const char* glyphName);
	float GetGlyphItalCorr_AAT(ATSUStyle style, UInt16 gid);
#endif
#ifdef __cplusplus
};
#endif


/* some Mac OS X functions that we provide ourselves for other platforms */
#ifndef XETEX_MAC
#ifdef __cplusplus
extern "C" {
#endif
	double	Fix2X(Fixed f);
	Fixed	X2Fix(double d);
#ifdef __cplusplus
};
#endif
#endif

#endif
