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
