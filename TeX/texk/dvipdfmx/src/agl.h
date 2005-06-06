/*  $Header: /home/cvsroot/dvipdfmx/src/agl.h,v 1.5 2004/02/15 12:59:42 hirata Exp $

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

#ifndef _AGL_H_
#define _AGL_H_

#include <stdio.h>
#include <sys/types.h>

#define AGLMAP_DEFAULT_GLYPHLIST "glyphlist.txt"

typedef struct AGList AGList;
typedef struct AGLmap AGLmap;

extern void  AGL_set_verbose (void);

extern long  AGList_sputx_UTF16BE (AGList *glyph,
				   unsigned char **dst, unsigned char *dstend);

extern char *AGList_get_name (AGList *agl);
extern long  AGList_get_code (AGList *agl);

extern int   AGList_is_composite (AGList *agl);

extern AGList *AGList_next_alternative (AGList *agl);

extern char *AGName_strip_suffix (char *glyphname);
extern long  AGName_convert_uni  (const char *glyphname);
extern int   AGName_is_unicode   (const char *glyphname);

extern AGLmap *AGLmap_new     (void);
extern void    AGLmap_release (AGLmap *aglm);
extern AGList *AGLmap_lookup  (AGLmap *aglm, const char *glyphname, const char *suffix);
extern long    AGLmap_decode_UTF16BE (AGLmap *aglm, const char *glyphstr,
				      unsigned char **dst, unsigned char *dstend,
				      int *fail_count);

extern int     AGLmap_read    (AGLmap *aglm, FILE *fp, int format);

extern void    AGLmap_cache_init  (void);
extern int     AGLmap_cache_find  (const char *name);
extern AGLmap *AGLmap_cache_get   (int id);
extern void    AGLmap_cache_close (void);

#endif /* _AGL_H_ */
