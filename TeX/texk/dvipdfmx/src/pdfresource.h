/*  $Header: /home/cvsroot/dvipdfmx/src/pdfresource.h,v 1.3 2003/12/07 09:01:33 hirata Exp $

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

#ifndef _PDF_RESOURCE_H_
#define _PDF_RESOURCE_H_

#include "pdfobj.h"

#define PDF_RES_TYPE_FONT     0
#define PDF_RES_TYPE_ENCODING 1
#define PDF_RES_TYPE_CIDFONT  2
#define PDF_RES_TYPE_CMAP     3

#define PDF_RES_TYPE_LAST     4

#define PDF_RES_SUBTYPE_TYPE1    0
#define PDF_RES_SUBTYPE_TRUETYPE 1
#define PDF_RES_SUBTYPE_TYPE0    2
#define PDF_RES_SUBTYPE_CIDTYPE0 3
#define PDF_RES_SUBTYPE_CIDTYPE2 4
#define PDF_RES_SUBTYPE_IMAGE    5

#define PDF_RES_SUBTYPE_LAST     5


extern void PDF_resource_init  (void);
extern void PDF_resource_close (void);

extern pdf_obj *PDF_defineresource (const char *res_name, pdf_obj *res_obj, int res_type);
extern pdf_obj *PDF_findresource   (const char *res_name, int res_type);

#endif /* _PDF_RESOURCE_H_ */
