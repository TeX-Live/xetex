/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

#ifndef __XETEXFONTDICT_H__
#define __XETEXFONTDICT_H__

#include <Carbon/Carbon.h>

typedef enum engineTechE {
   kEngineAuto = 0,
   kEngineAAT = 1,
   kEngineICU = 2
} engineTech;

extern engineTech	preferredTech;

#ifdef __cplusplus
extern "C" {
#endif

	void		init_font_dict(void);

	ATSUFontID	find_font_by_name(CFStringRef fontNameStr, double pointSize);

	ATSUFontID	get_optically_sized_font(ATSUFontID fontID, double opticalSize);

#ifdef __cplusplus
};
#endif

#endif /* __XETEXFONTDICT_H__ */
