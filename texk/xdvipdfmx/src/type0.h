/*  $Header: /home/cvsroot/dvipdfmx/src/type0.h,v 1.12 2004/09/05 13:30:06 hirata Exp $
    
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

#define add_to_used_chars2(b,c) {(b)[(c)/8] |= (1 << (7-((c)%8)));}
#define is_used_char2(b,c) (((b)[(c)/8]) & (1 << (7-((c)%8))))

typedef struct Type0Font Type0Font;

extern void       Type0Font_set_verbose (void);

extern int        Type0Font_get_wmode     (Type0Font *font);
extern char      *Type0Font_get_encoding  (Type0Font *font);
extern char      *Type0Font_get_usedchars (Type0Font *font);

extern pdf_obj   *Type0Font_get_resource  (Type0Font *font);

extern void       Type0Font_set_ToUnicode (Type0Font *font, pdf_obj *cmap_ref);

#include "fontmap.h"
extern int        pdf_font_findfont0      (const char *font_name,
					   int cmap_id, fontmap_opt *fmap_opt);

extern unsigned short *Type0Font_get_ft_to_gid(int id);

/******************************** CACHE ********************************/

extern void       Type0Font_cache_init  (void);
extern Type0Font *Type0Font_cache_get   (int id);
extern int        Type0Font_cache_find  (const char *map_name, int cmap_id, fontmap_opt *fmap_opt);
extern void       Type0Font_cache_close (void);

#endif /* _TYPE0_H_ */
