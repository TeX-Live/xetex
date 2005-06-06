/*  $Header: /home/cvsroot/dvipdfmx/src/cid.h,v 1.7 2003/12/07 09:01:32 hirata Exp $
    
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

#ifndef _CID_H_
#define _CID_H_

/* CIDFont types */
#define CIDFONT_TYPE0 1
#define CIDFONT_TYPE2 2

typedef struct {
  char *registry;
  char *ordering;
  int   supplement;
} CIDSysInfo;

typedef struct CIDFont CIDFont;

extern void CIDFont_set_verbose     (void);
extern int  CIDFont_require_version (void);

#include "pdfobj.h"
#include "type0.h"

extern CIDFont *CIDFont_new     (void);
extern void     CIDFont_release (CIDFont *font);
extern void     CIDFont_flush   (CIDFont *font);

extern char       *CIDFont_get_fontname   (CIDFont *font);
extern int         CIDFont_get_subtype    (CIDFont *font);
extern int         CIDFont_get_embedding  (CIDFont *font);
extern pdf_obj    *CIDFont_get_resource   (CIDFont *font);
extern CIDSysInfo *CIDFont_get_CIDSysInfo (CIDFont *font);

extern void     CIDFont_attach_parent (CIDFont *font, int parent_id, int wmode);
extern int      CIDFont_get_parent_id (CIDFont *font, int wmode);

extern int      CIDFont_is_BaseFont (CIDFont *font);
extern int      CIDFont_is_ACCFont  (CIDFont *font);
extern int      CIDFont_is_UCSFont  (CIDFont *font);

extern void     CIDFont_cache_init  (void);
extern int      CIDFont_cache_find  (const char *map_name, CIDSysInfo *cmap_csi);
extern CIDFont *CIDFont_cache_get   (int fnt_id);
extern void     CIDFont_cache_close (void);

#endif /* _CID_H_ */
