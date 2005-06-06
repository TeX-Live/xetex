/*  $Header: /home/cvsroot/dvipdfmx/src/mpost.h,v 1.4 2002/10/30 02:27:12 chofchof Exp $
    
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

#include "pdfobj.h"
#include "pdfspecial.h"

#ifndef _MPOST_H_
#define _MPOST_H_

extern int check_for_mp (FILE *image_file);
extern pdf_obj *mp_include (FILE *image_file,  struct xform_info *p,
		     char *res_name, double x_user, double y_user);
extern int do_raw_ps_special (char **start, char *end, int cleanup,
			      double x_user, double y_user);
extern void mp_cleanup(int sloppy_is_okay);
extern struct xform_info *texfig_info (void);
extern void mp_eop_cleanup(void);

#endif /* _MPOST_H_ */
