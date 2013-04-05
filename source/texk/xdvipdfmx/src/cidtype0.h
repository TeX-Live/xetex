/*  
    
    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2002-2012 by Jin-Hwan Cho and Shunsaku Hirata,
    the dvipdfmx project team.
    
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

#ifndef _CIDTYPE0_H_
#define _CIDTYPE0_H_

#include "cid.h"
#include "cid_p.h"
#include "fontmap.h"

extern void CIDFont_type0_set_verbose (void);
extern void CIDFont_type0_set_flags   (long flags);

extern int  CIDFont_type0_open    (CIDFont *font, const char *name,
				   CIDSysInfo *cmap_csi, cid_opt *opt);
extern void CIDFont_type0_dofont  (CIDFont *font);
extern void CIDFont_type0_release (CIDFont *font);

/* Type1 --> CFF CIDFont */
extern int  t1_load_UnicodeCMap  (const char *font_name, const char *otl_tags, int wmode);
extern int  CIDFont_type0_t1open (CIDFont *font, const char *name,
				  CIDSysInfo *cmap_csi, cid_opt *opt);
extern void CIDFont_type0_t1dofont (CIDFont *font);
extern int  CIDFont_type0_t1copen (CIDFont *font, const char *name,
				   CIDSysInfo *cmap_csi, cid_opt *opt);
extern void CIDFont_type0_t1cdofont (CIDFont *font);

#endif /* _CIDTYPE0_H_ */
