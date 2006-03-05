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


#ifdef XETEX_MAC
/* functions in XeTeX_mac.c */
#ifdef __cplusplus
extern "C" {
#endif
	void DoAtsuiLayout(void* node, int justify);
	void GetGlyphHeightDepth_AAT(ATSUStyle style, UInt16 gid, float* ht, float* dp);
	void GetGlyphSidebearings_AAT(ATSUStyle style, UInt16 gid, float* lsb, float* rsb);
	int MapCharToGlyph_AAT(ATSUStyle style, UInt32 ch);
	float GetGlyphItalCorr_AAT(ATSUStyle style, UInt16 gid);
#ifdef __cplusplus
};
#endif
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
