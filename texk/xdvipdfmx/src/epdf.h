/*  $Header: /home/cvsroot/dvipdfmx/src/epdf.h,v 1.8 2004/09/11 14:50:28 hirata Exp $
    
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

#ifndef _EPDF_H_
#define _EPDF_H_

#include "mfileio.h"
#include "pdfximage.h"

#define pdfbox_crop  1
#define pdfbox_media 2
#define pdfbox_bleed 3
#define pdfbox_trim  4
#define pdfbox_art   5

extern int pdf_copy_clip (FILE *image_file, int page_index, double x_user, double y_user);
//extern int pdf_include_page (pdf_ximage *ximage, FILE *file, int page_index, int pdf_box);
extern int pdf_include_page (pdf_ximage *ximage, FILE *file);

#endif /* _EPDF_H_ */
