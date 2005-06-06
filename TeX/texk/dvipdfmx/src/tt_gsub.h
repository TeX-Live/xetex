/*  $Header: /home/cvsroot/dvipdfmx/src/tt_gsub.h,v 1.4 2004/01/30 18:34:24 hirata Exp $
    
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

#ifndef _TT_GSUB_H_
#define _TT_GSUB_H_

#include "sfnt.h"

typedef void *tt_gsub_t;

extern tt_gsub_t tt_gsub_require_feature (sfnt *sfont, const char *tag);
extern void tt_gsub_release (tt_gsub_t gsubs);

extern void tt_gsub_substitute(tt_gsub_t gsub, USHORT *gid);

#endif /* _TT_GSUB_H_ */
