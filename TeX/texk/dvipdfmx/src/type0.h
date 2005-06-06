/*  $Header: /home/cvsroot/dvipdfmx/src/type0.h,v 1.6 2003/12/07 09:01:34 hirata Exp $
    
    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2002 by Jin-Hwan Cho and Shunsaku Hirata,
    the dvipdfmx project team <dvipdfmx@project.ktug.or.kr>
    
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

#ifndef _TYPE0_H_
#define _TYPE0_H_

#include "pdfobj.h"

#define add_to_used_chars2(b,c) {b[(c)/8] |= (1 << (7-((c)%8)));}
#define is_used_char2(b,c) ((b[(c)/8]) & (1 << (7-((c)%8))))

typedef struct Type0Font Type0Font;

extern void       Type0Font_set_verbose (void);

extern Type0Font *Type0Font_new     (void);
extern void       Type0Font_release (Type0Font *font);
extern void       Type0Font_dofont  (Type0Font *font);
extern void       Type0Font_flush   (Type0Font *font);

extern int        Type0Font_get_wmode     (Type0Font *font);
extern char      *Type0Font_get_encoding  (Type0Font *font);
extern char      *Type0Font_get_usedchars (Type0Font *font);

extern pdf_obj   *Type0Font_get_resource   (Type0Font *font);

/******************************** CACHE ********************************/

extern void       Type0Font_cache_init  (void);
extern Type0Font *Type0Font_cache_get   (int id);
extern int        Type0Font_cache_find  (const char *map_name,
					 const char *res_name, int cmap_id);
extern void       Type0Font_cache_close (void);

/******************************** COMAPT ********************************/

extern void     type0_set_verbose   (void);
extern pdf_obj *type0_font_resource (int type0_id);
extern int      type0_font_wmode    (int type0_id);
extern char    *type0_font_encoding (int type0_id);
extern char    *type0_font_used     (int type0_id);
extern int      type0_font          (const char *tex_name, int tfm_id,
				     char *res_name, int enc_id, int remap);
extern void     type0_close_all     (void);

extern pdf_obj *get_tounicode_cmap  (const char *cmap_name);

#endif /* _TYPE0_H_ */
