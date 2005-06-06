/*  $Header: /home/cvsroot/dvipdfmx/src/encodings.h,v 1.7 2004/02/12 12:20:21 hirata Exp $
    
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

#ifndef _ENCODINGS_H_
#define _ENCODINGS_H_

typedef struct Encoding Encoding;

extern void      Encoding_set_verbose (void);

extern Encoding *Encoding_new     (void);
extern void      Encoding_flush   (Encoding *encoding);
extern void      Encoding_release (Encoding *encoding);

extern pdf_obj  *Encoding_get_resource (Encoding *encoding);

extern int       Encoding_is_ASL_charset (Encoding *encoding);
extern int       Encoding_is_predefined  (Encoding *encoding);

extern void      Encoding_set_name  (Encoding *encoding, const char *name);
extern char     *Encoding_get_name  (Encoding *encoding);

extern void      Encoding_set_encoding (Encoding *encoding,
					char **encoding_vec, const char *baseenc);
extern char    **Encoding_get_encoding (Encoding *encoding);

#if 0
extern pdf_obj  *Encoding_make_resource (char **encoding, char **baseenc);
#endif

extern void      Encoding_cache_init  (void);
extern void      Encoding_cache_close (void);
extern Encoding *Encoding_cache_get   (int encoding_id);
extern int       Encoding_cache_find  (const char *enc_name);

#endif /* _ENCODINGS_H_ */
