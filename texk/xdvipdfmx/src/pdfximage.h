/*  $Header: /home/cvsroot/dvipdfmx/src/pdfximage.h,v 1.11 2008/02/13 20:22:21 matthias Exp $
    
    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2007 by Jin-Hwan Cho and Shunsaku Hirata,
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

#ifndef _PDFXIMAGE_H_
#define _PDFXIMAGE_H_

#include "pdfdev.h"

#define PDF_XOBJECT_TYPE_FORM  0
#define PDF_XOBJECT_TYPE_IMAGE 1

typedef struct {
  int  flags;

  long width;
  long height;

  int  bits_per_component;
  int  num_components;

  long min_dpi; /* NOT USED YET */

  double xdensity, ydensity; /* scale factor for bp */
} ximage_info;

typedef struct {
  int         flags;
  
  pdf_rect    bbox;
  pdf_tmatrix matrix;
} xform_info;

typedef struct pdf_ximage_ pdf_ximage;

extern void     pdf_ximage_set_verbose    (void);

extern void     pdf_init_images           (void);
extern void     pdf_close_images          (void);

extern char    *pdf_ximage_get_resname    (int xobj_id);
extern pdf_obj *pdf_ximage_get_reference  (int xobj_id);


extern int      pdf_ximage_findresource   (const char *ident, long page_no/*, int pdf_box*/);
extern int      pdf_ximage_defineresource (const char *ident,
					   int subtype, void *cdata, pdf_obj *resource);

/* Called by pngimage, jpegimage, epdf, mpost, etc. */
extern void pdf_ximage_init_image_info (ximage_info *info);
extern void pdf_ximage_init_form_info  (xform_info  *info);
extern void pdf_ximage_set_image (pdf_ximage *ximage, void *info, pdf_obj *resource);
extern void pdf_ximage_set_form  (pdf_ximage *ximage, void *info, pdf_obj *resource);
extern void pdf_ximage_set_page  (pdf_ximage *ximage, long page_no, long page_count);
extern long pdf_ximage_get_page  (pdf_ximage *I);

/* from psimage.h */
extern void set_distiller_template (char *s);
extern char *get_distiller_template (void);

extern int
pdf_ximage_scale_image (int            id,
                        pdf_tmatrix    *M, /* ret */
                        pdf_rect       *r, /* ret */
                        transform_info *p  /* arg */
                       );

/* from dvipdfmx.c */
extern void pdf_ximage_disable_ebb (void);
#endif /* _PDFXIMAGE_H_ */
