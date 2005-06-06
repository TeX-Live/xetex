/*  $Header: /home/cvsroot/dvipdfmx/src/pkfont.h,v 1.3 2002/10/30 02:27:15 chofchof Exp $

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

#ifndef _PK_FONT_H_
#define _PK_FONT_H_

#include "pdfobj.h"

extern void pk_set_verbose(void);
extern int pk_font (const char *tex_name, double ptsize, int tfm_id, char
	     *res_name);
extern pdf_obj *pk_font_resource (int pk_id);
extern char *pk_font_used (int pk_id);
extern void pk_close_all (void);
extern void pk_set_dpi (int dpi);

#endif /* _PK_FONT_H_ */
