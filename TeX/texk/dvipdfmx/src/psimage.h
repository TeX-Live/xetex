/*  $Header: /home/cvsroot/dvipdfmx/src/psimage.h,v 1.3 2002/10/30 02:27:15 chofchof Exp $

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

#ifndef _PSIMAGE_H_
#define _PSIMAGE_H_

#include "pdfobj.h"
#include "pdfspecial.h"

extern void set_distiller_template (char *s);
extern pdf_obj *ps_include (char *file_name, 
			   struct xform_info *p,
			   char *res_name,
			   double x_user, double y_user);

extern int check_for_ps (FILE *image_file);
extern void psimage_close (void);

#endif /* _PSIMAGE_H_ */
