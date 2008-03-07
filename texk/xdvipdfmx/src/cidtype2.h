/*  $Header: /home/cvsroot/dvipdfmx/src/cidtype2.h,v 1.6 2004/08/07 09:52:34 hirata Exp $
    
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

#ifndef _CIDTYPE2_H_
#define _CIDTYPE2_H_

#include "cid.h"
#include "cid_p.h"

extern void CIDFont_type2_set_verbose (void);
extern void CIDFont_type2_set_flags   (long flags);

extern int  CIDFont_type2_open    (CIDFont *font, const char *name,
				   CIDSysInfo *cmap_csi, cid_opt *opt);
extern void CIDFont_type2_dofont  (CIDFont *font);
extern void CIDFont_type2_release (CIDFont *font);

#endif /* _CIDTYPE2_H_ */
