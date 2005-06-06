/*  $Header: /home/cvsroot/dvipdfmx/src/ttf.h,v 1.6 2003/11/28 23:57:52 hirata Exp $
    
    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2002 by Jin-Hwan Cho and Shunsaku Hirata,
    the dvipdfmx project team <dvipdfmx@project.ktug.or.kr>
    
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

#ifndef _TTF_H_
#define _TTF_H_

#include "pdfobj.h"

typedef struct TTFont TTFont;

extern void     TTFont_set_verbose (void);

/******************************** MAIN ********************************/

extern TTFont  *TTFont_new     (void);
extern void     TTFont_release (TTFont *font);
extern int      TTFont_open    (TTFont *font, const char *name, int encoding_id, int embed);
extern void     TTFont_flush   (TTFont *font);
extern void     TTFont_dofont  (TTFont *font);

extern pdf_obj *TTFont_get_resource  (TTFont *font);
extern char    *TTFont_get_usedchars (TTFont *font);

/******************************** CACHE ********************************/

extern void    TTFont_cache_init  (void);
extern TTFont *TTFont_cache_get   (int font_id);
extern int     TTFont_cache_find  (const char *map_name, char *res_name,
				   int encoding_id, int tfm_id, int remap);
extern void    TTFont_cache_close (void);


/******************************** COMPAT ********************************/

extern void     ttf_set_verbose    (void);
extern pdf_obj *ttf_font_resource  (int ttf_id);
extern char    *ttf_font_used      (int ttf_id);
extern void     ttf_disable_partial(void);
extern int      ttf_font (const char *tex_name, int tfm_font_id,
			  char *resource_name, int encoding_id, int remap);
extern void     ttf_set_mapfile (const char *name);
extern void     ttf_close_all   (void);

#endif /* _TTF_H_ */
