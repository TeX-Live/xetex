/*  
    
    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2002-2012 by Jin-Hwan Cho and Shunsaku Hirata,
    the dvipdfmx project team.
    
    Copyright (C) 1998, 1999 by Mark A. Wicks <mwicks@kettering.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/

#ifndef _PDF_COLOR_H_
#define _PDF_COLOR_H_

#include "pdfobj.h"

#define PDF_COLORSPACE_TYPE_DEVICECMYK -4
#define PDF_COLORSPACE_TYPE_DEVICERGB  -3
#define PDF_COLORSPACE_TYPE_DEVICEGRAY -1
#define PDF_COLORSPACE_TYPE_INVALID     0
#define PDF_COLORSPACE_TYPE_CALGRAY     1
#define PDF_COLORSPACE_TYPE_CIELAB      2
#define PDF_COLORSPACE_TYPE_CALRGB      3
#define PDF_COLORSPACE_TYPE_ICCBASED    4

#define PDF_COLORSPACE_TYPE_CMYK  PDF_COLORSPACE_TYPE_DEVICECMYK
#define PDF_COLORSPACE_TYPE_RGB   PDF_COLORSPACE_TYPE_DEVICERGB
#define PDF_COLORSPACE_TYPE_GRAY  PDF_COLORSPACE_TYPE_DEVICEGRAY


#define PDF_COLOR_COMPONENT_MAX 4

typedef struct
{
  int    num_components;
  double values[PDF_COLOR_COMPONENT_MAX];
} pdf_color;

extern void       pdf_color_set_verbose   (void);

extern int        pdf_color_rgbcolor      (pdf_color *color,
                                           double r, double g, double b);
extern int        pdf_color_cmykcolor     (pdf_color *color,
                                           double c, double m, double y, double k);
extern int        pdf_color_graycolor     (pdf_color *color, double g);
extern void       pdf_color_copycolor     (pdf_color *color1, const pdf_color *color2);

extern int        pdf_color_is_white      (pdf_color *color);
extern int        pdf_color_is_valid      (pdf_color *color);

/* Not check size */
extern pdf_obj *iccp_get_rendering_intent (const void *profile, long proflen);
extern int      iccp_check_colorspace     (int colortype,
					   const void *profile, long proflen);

/* returns colorspace ID */
extern int      iccp_load_profile (const char *ident,
				   const void *profile, long proflen);

extern void     pdf_init_colors  (void);
extern void     pdf_close_colors (void);

extern pdf_obj *pdf_get_colorspace_reference      (int cspc_id);

#if 0
extern int      pdf_get_colorspace_num_components (int cspc_id);
extern int      pdf_get_colorspace_subtype        (int cspc_id);
#endif

/* Not working */
extern int      pdf_colorspace_load_ICCBased      (const char *ident,
						   const char *profile_filename);

/* Color special
 * See remark in spc_color.c.
 */
extern void     pdf_color_set_default (const pdf_color *color);
extern void     pdf_color_push  (pdf_color *sc, pdf_color *fc);
extern void     pdf_color_pop   (void);

/* Color stack
 */
extern void     pdf_color_clear_stack (void);
extern void     pdf_color_get_current (pdf_color **sc, pdf_color **fc);

/* Reinstall color */
extern void     pdf_dev_preserve_color(void);

#endif /* _PDF_COLOR_H_ */
