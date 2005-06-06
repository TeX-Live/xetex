/*  $Header: /home/cvsroot/dvipdfmx/src/fontmap.h,v 1.3 2003/11/25 11:29:02 hirata Exp $
    
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

#ifndef _FONTMAP_H_
#define _FONTMAP_H_

extern void fontmap_set_verbose (void);
extern void release_map_record (void);
extern void read_mapfile (char *filename);
extern int get_map_record (char *tex_name, int *subfont_id);
/* extern int subfont_locate_font (char *tex_name); */
extern char *fontmap_tex_name (int map_id);
extern char *fontmap_enc_name (int map_id);
extern char *fontmap_font_name (int map_id);
extern double fontmap_extend (int map_id);
extern double fontmap_slant (int map_id);
extern int fontmap_remap (int map_id);
extern int fontmap_mapc (int map_id);

#endif /* _FONTMAP_H_ */
