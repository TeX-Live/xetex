/*  $Header: /home/cvsroot/dvipdfmx/src/type1.h,v 1.4 2002/10/30 02:27:21 chofchof Exp $

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

#ifndef _TYPE1_H_
#define _TYPE1_H_

#include "pdfobj.h"

extern void type1_set_verbose(void);
extern pdf_obj *type1_font_resource (int type1_id);
extern char *type1_font_used (int type1_id);
extern void type1_disable_partial (void);
extern int type1_font (const char *tex_name, int tfm_font_id, char
		       *resource_name, int encoding_id, int remap);
extern void type1_set_mapfile (const char *name);
extern void type1_close_all (void);

#endif /* _TYPE1_H_ */
