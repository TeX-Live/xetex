/*  $Header: /home/cvsroot/dvipdfmx/src/type1c.h,v 1.3 2003/12/02 09:55:55 hirata Exp $

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

#ifndef _TYPE1C_H_
#define _TYPE1C_H_

#include "pdfobj.h"

typedef struct Type1CFont Type1CFont;

extern void     Type1CFont_set_verbose (void);

/******************************** MAIN ********************************/

extern Type1CFont *Type1CFont_new     (void);
extern void        Type1CFont_release (Type1CFont *font);
extern int         Type1CFont_open    (Type1CFont *font, const char *name,
				       int encoding_id, int embed);
extern void        Type1CFont_flush   (Type1CFont *font);
extern void        Type1CFont_dofont  (Type1CFont *font);

extern pdf_obj    *Type1CFont_get_resource  (Type1CFont *font);
extern char       *Type1CFont_get_usedchars (Type1CFont *font);

/******************************** CACHE ********************************/

extern void        Type1CFont_cache_init  (void);
extern Type1CFont *Type1CFont_cache_get   (int font_id);
extern int         Type1CFont_cache_find  (const char *map_name, char *res_name,
					   int encoding_id, int tfm_id, int remap);
extern void        Type1CFont_cache_close (void);


/******************************** COMPAT ********************************/

extern void     type1c_set_verbose    (void);
extern pdf_obj *type1c_font_resource  (int font_id);
extern char    *type1c_font_used      (int font_id);
extern void     type1c_disable_partial(void);
extern int      type1c_font (const char *tex_name, int tfm_font_id,
			     char *resource_name, int encoding_id, int remap);
extern void     type1c_close_all (void);

#endif /* _TYPE1C_H_ */
