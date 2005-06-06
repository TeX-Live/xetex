/*  $Header: /home/cvsroot/dvipdfmx/src/pngimage.h,v 1.3 2002/10/30 02:27:15 chofchof Exp $

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

	
#ifndef _PNGIMAGE_H_
#define _PNGIMAGE_H_

#include <stdio.h>
#include "pdfobj.h"

#include "config.h"
#ifdef HAVE_LIBPNG
extern pdf_obj *start_png_image (FILE *file, char *res_name);
extern int check_for_png (FILE *file);
#endif

#endif /* _PNGIMAGE_H_ */
