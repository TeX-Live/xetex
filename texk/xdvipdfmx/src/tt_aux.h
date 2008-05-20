/*  $Header: /home/cvsroot/dvipdfmx/src/tt_aux.h,v 1.6 2008/05/17 04:18:47 chofchof Exp $
    
    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2002 by Jin-Hwan Cho and Shunsaku Hirata,
    the dvipdfmx project team <dvipdfmx@project.ktug.or.kr>
    
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

#ifndef _TT_AUX_H_
#define _TT_AUX_H_

#include "pdfobj.h"
#include "sfnt.h"

extern void tt_aux_set_verbose();

/* TTC (TrueType Collection) */
extern ULONG    ttc_read_offset (sfnt *sfont, int ttc_idx);

/* FontDescriptor */
extern pdf_obj *tt_get_fontdesc (sfnt *sfont, int *embed, int stemv, int type, const char* fontname);

#endif /* _TT_AUX_H_ */
