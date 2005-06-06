/*  $Header: /home/cvsroot/dvipdfmx/src/pdfspecial.h,v 1.6 2003/08/12 04:50:47 chofchof Exp $

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
	
#ifndef _PDFSPECIAL_H_
#define _PDFSPECIAL_H_

#include "numbers.h"
#include "pdfobj.h"

struct xform_info 
{
  double width;
  double height;
  double depth;
  double scale;
  double xscale;
  double yscale;
  double rotate;
  /* If the user enters a "bounding box", user_bbox is nonzero */
  unsigned char user_bbox;
  /* If the user enters a "bounding box", the use specified
     coordinates follow */
  double u_llx, u_lly, u_urx, u_ury;
  /* The following lines are for compatibility with dvips specials.
     The method is a bit clumsy, and requires a lengthy explanation,
     because dvipdfm uses form xobjects to embed all image types.
     
     Because the image is a form xobject, the image must be clipped
     somewhere.  Form xobjects always crop to their bounding box.
     However, this is not the dvips behavior.  If the user specifies
     "clip", PS and PDF images will be clipped to the user specified
     bounding box; in other words, the user specified bounding box and
     the clipping bounding box are one and the same.  Otherwise,
     dvipdfm will clip the image at the bounding box contained in the
     image file.  (The eps header in the case of Metapost images or
     the media box or crop box in the case of PDF images.)  In the
     case that the user bounding box and clip bounding box are
     different, the image is scaled to match any user specified
     dimensions.

     Processing of all image inclusion specials are handled by common
     code which fills in the user bounding box, if any, as well as the
     width, height, depth, xscale, yscale, scale, and rotate fields.
     That common routine calls a routine specific to each image type.
     The routines specific to each file type MUST fill in the c_llx
     through c_ury fields, because the source of this information is
     image type dependent.  They are also responsible for calling
     pdf_scale_image(), which checks the information for consistency
     and recalculates ` the scaling required to achieve the desired
     effect. */
  unsigned char clip;
  double c_llx, c_lly, c_urx, c_ury;
};

extern void pdf_special_set_verbose(void);

extern int pdf_parse_special(char *buffer, UNSIGNED_QUAD size, double
		       x_user, double y_user);
extern void pdf_finish_specials(void);
extern pdf_obj *lookup_reference (char *name);
extern struct xform_info *new_xform_info (void);
extern void release_xform_info (struct xform_info *p);
extern int validate_image_xform_info (struct xform_info *p);
extern void add_xform_matrix (double xoff, double yoff, double xscale, double
		       yscale, double rotate);

extern void pdf_special_ignore_colors(void);

extern double parse_one_unit (char **start, char *end);
extern void pdf_scale_image (struct xform_info *p);
extern pdf_obj *embed_image (char *filename, struct xform_info *p, 
			     double x_user, double y_user, char
			     *objname);
extern void pdf_finish_specials(void);
extern void pdf_special_set_grow (double g);
extern double pdf_special_tell_grow (void);

extern void release_dpx_images (void);

#ifndef M_PI
#define M_PI (4.0*atan(1.0))
#endif /* M_PI */

#endif /* _PDFSPECIAL_H_ */
