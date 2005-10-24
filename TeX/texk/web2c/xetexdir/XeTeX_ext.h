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

#include "TECkit_Engine.h"

#define AAT_FONT_FLAG	65535
#define	OT_FONT_FLAG	65534


/* some typedefs that XeTeX uses - on Mac OS, we get these from Apple headers,
   but otherwise we'll need these substitute definitions */

#ifndef XETEX_MAC
typedef signed char		SInt8;
typedef unsigned char	UInt8;
typedef short			SInt16;
typedef unsigned short	UInt16;
typedef int				SInt32;
typedef unsigned int	UInt32;

typedef UInt16			UniChar;
typedef SInt32			OSStatus;
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

#define XeTeX_variation_name	7	// must match xetex.web
#define XeTeX_feature_name	8
#define XeTeX_selector_name	9


TECkit_Converter	load_mapping_file(const char* s, const char* e);

#endif
