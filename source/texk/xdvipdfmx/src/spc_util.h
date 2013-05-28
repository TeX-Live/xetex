/*  
    
    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2002-2012 by Jin-Hwan Cho and Shunsaku Hirata,
    the dvipdfmx project team.
    
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

#ifndef _SPC_UTIL_H_
#define _SPC_UTIL_H_

#include "pdfcolor.h"
#include "pdfdev.h"

#include "specials.h"

/* syntax 1: ((rgb|cmyk|hsb|gray) colorvalues)|colorname
 * syntax 0: pdf_number|pdf_array
 *
 * This is for reading *single* color specification.
 */
extern int  spc_util_read_colorspec (struct spc_env *spe, pdf_color *colorspec, struct spc_arg *args, int syntax);
extern int  spc_util_read_dimtrns   (struct spc_env *spe, transform_info *dimtrns, struct spc_arg *args, long *page, int syntax);
extern int  spc_util_read_length    (struct spc_env *spe, double *length, struct spc_arg *ap);

extern int  spc_util_read_numbers   (double *values, int num_values, struct spc_env *spe, struct spc_arg *args);

#endif /* _SPC_UTIL_H_ */
